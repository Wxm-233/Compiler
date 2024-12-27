#include "visitraw.h"
#include <iostream>
#include <cassert>
#include <string>
#include <cstring>
#include <map>
// 函数声明略
// ...

//inst_reg_use regs[8];

void lw_safe(std::string reg, int loc) {
    if (loc < 2048) {
        std::cout << "  lw " << reg << ", " << loc << "(sp)" << std::endl;
    } else {
        std::cout << "  li a0, " << loc << std::endl;
        std::cout << "  add a0, sp, a0" << std::endl;
        std::cout << "  lw " << reg << ", 0(a0)" << std::endl;
    }
}

void sw_safe(std::string reg, int loc) {
    if (loc < 2048) {
        std::cout << "  sw " << reg << ", " << loc << "(sp)" << std::endl;
    } else {
        std::cout << "  li a0, " << loc << std::endl;
        std::cout << "  add a0, sp, a0" << std::endl;
        std::cout << "  sw " << reg << ", 0(a0)" << std::endl;
    }
}

namespace Stack {
    int R;
    static std::map<koopa_raw_value_t, int> loc_map;
    static int current_loc;
    static int stack_frame_length;
    int Query(koopa_raw_value_t inst) {
        return loc_map[inst];
    }
    void Insert(koopa_raw_value_t inst, int loc) {
        loc_map.insert({inst, loc});
    }
    void Load2reg(koopa_raw_value_t value, std::string reg) {
        switch (value->kind.tag) {
            case KOOPA_RVT_INTEGER:
                std::cout << "  li " << reg << ", " << value->kind.data.integer.value << std::endl;
                break;
            case KOOPA_RVT_BLOCK_ARG_REF:
                std::cout << "  mv " << reg << ", a" << value->kind.data.block_arg_ref.index << std::endl;
                break;
            case KOOPA_RVT_FUNC_ARG_REF:
                if (value->kind.data.func_arg_ref.index < 8) {
                    std::cout << "  mv " << reg << ", a" << value->kind.data.func_arg_ref.index << std::endl;
                } else {
                    lw_safe(reg, 4 * (value->kind.data.func_arg_ref.index - 8) + stack_frame_length);
                }
                break;
            case KOOPA_RVT_GLOBAL_ALLOC:
                std::cout << "  la " << reg << ", " << value->name + 1 << std::endl;
                std::cout << "  lw " << reg << ", 0(" << reg << ")" << std::endl;
                break;
            default:
                lw_safe(reg, Query(value));
        }
    }
    void StoreFromreg(koopa_raw_value_t value, std::string reg, std::string reg2) {
        if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            std::cout << "  la " << reg2 << ", " << value->name + 1 << std::endl;
            std::cout << "  sw " << reg << ", 0(" << reg2 << ")" << std::endl;
        }
        else sw_safe(reg, Query(value));
    }
}

// int use_inst(koopa_raw_value_t inst)
// {
//     // std::clog << "inst: " << inst->kind.tag << std::endl;
//     for (int i = 0; i < 8; i++) {
//         if (regs[i].data == inst) {
//             regs[i].n_used_by -= 1;
//             if (regs[i].n_used_by == 0) {
//                 regs[i].data = nullptr;
//             }
//             return i;
//         }
//     }
//     assert(false);
// }

// int alloc_reg(const koopa_raw_value_t &value)
// {
//     int pos = 0;
//     if (value->used_by.len > 0) {
//         for (pos = 0; pos < 8; pos++) {
//             if (regs[pos].n_used_by == 0) {
//                 regs[pos].data = value;
//                 regs[pos].n_used_by = value->used_by.len;
//                 break;
//             }
//         }
//     }
//     return pos;
// }


// 访问 raw program
void Visit(const koopa_raw_program_t &program)
{
    // std::clog << "visiting raw program..." << std::endl;
    // 执行一些其他的必要操作
    // for (auto i : regs) {
    //     i = {nullptr, 0};
    // }
    // ...
    // 访问所有全局变量
    std::cout << "  .data" << std::endl;
    Visit(program.values);
    // 访问所有函数
    std::cout << "  .text" << std::endl;
    Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice)
{
    // std::clog << "visiting slice buffer..." << std::endl;
    for (size_t i = 0; i < slice.len; ++i)
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            assert(false);
        }
    }
}

// 访问函数
void Visit(const koopa_raw_function_t &func)
{
    if (func->bbs.len == 0)
        return;
    // 执行一些其他的必要操作
    // ...
    std::cout << "  .globl " << func->name + 1 << std::endl;
    std::cout << func->name + 1 << ":" << std::endl;

    // 计算栈帧长度
    int insts_on_stack = 0;
    bool has_call = false;
    int extra_args_in_call = 0;
    // 遍历基本块
    for (size_t i = 0; i < func->bbs.len; ++i)
    {
        auto bb = (koopa_raw_basic_block_t)func->bbs.buffer[i];
        // 遍历指令
        for (size_t j = 0; j < bb->insts.len; ++j)
        {
            auto inst = (koopa_raw_value_t)bb->insts.buffer[j];
            switch (inst->kind.tag) {
            case KOOPA_RVT_CALL:
                has_call = true;
                extra_args_in_call = std::max(extra_args_in_call, (int)inst->kind.data.call.args.len - 8);
                // break;
            case KOOPA_RVT_ALLOC:
            case KOOPA_RVT_BINARY:
            case KOOPA_RVT_LOAD:
                insts_on_stack += 1;
                break;
            default:
                break;
            }
        }
    }
    int S = insts_on_stack * 4;
    Stack::R = has_call ? 4 : 0;
    int A = extra_args_in_call * 4;
    Stack::stack_frame_length = (S + Stack::R + A + 15) / 16 * 16;

    Stack::loc_map = std::map<koopa_raw_value_t, int>();

    if (Stack::stack_frame_length > 0) {
        if (Stack::stack_frame_length < 2048)
            std::cout << "  addi sp, sp, " << -Stack::stack_frame_length << std::endl;
        else {
            std::cout << "  li t0, " << -Stack::stack_frame_length << std::endl;
            std::cout << "  add sp, sp, t0" << std::endl;
        }
    }
    if (Stack::R != 0) {
        sw_safe("ra", Stack::stack_frame_length - 4);
    }

    Stack::current_loc = A;
    // 访问所有基本块
    Visit(func->bbs);

    std::cout << std::endl;
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb)
{
    // 执行一些其他的必要操作
    std::cout << bb->name + 1 << ":" << std::endl;
    assert(bb->params.len <= 8);
    // ...
    // 访问所有指令
    Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value)
{
    // 根据指令类型判断后续需要如何访问
    // std::clog << "visiting value: kind.tag=" << value->kind.tag << std::endl;
    const auto &kind = value->kind;
    switch (kind.tag)
    {
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        Visit(kind.data.ret);
        break;
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        // std::clog << "integer: " << kind.data.integer.value << std::endl;
        // Visit(kind.data.integer, value);
        break;
    case KOOPA_RVT_ALLOC:
        // 访问 alloc 指令
        Stack::Insert(value, Stack::current_loc);
        Stack::current_loc += 4;
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        // 访问 global_alloc 指令
        std::cout << "  .globl " << value->name+1 << std::endl;
        std::cout << value->name+1 << ":" << std::endl;
        Visit(kind.data.global_alloc);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令
        // std::clog << "binary: " << kind.data.binary.op << std::endl;
        Visit(kind.data.binary);
        Stack::Insert(value, Stack::current_loc);
        Stack::current_loc += 4;
        break;
    case KOOPA_RVT_STORE:
        // 访问 store 指令
        Visit(kind.data.store);
        break;
    case KOOPA_RVT_LOAD:
        // 访问 load 指令
        Visit(kind.data.load);
        Stack::Insert(value, Stack::current_loc);
        Stack::current_loc += 4;
        break;
    case KOOPA_RVT_BRANCH:
        // 访问 branch 指令
        Visit(kind.data.branch);
        break;
    case KOOPA_RVT_JUMP:
        // 访问 jump 指令
        Visit(kind.data.jump);
        break;
    case KOOPA_RVT_CALL:
        // 访问 call 指令
        Visit(kind.data.call);
        Stack::Insert(value, Stack::current_loc);
        Stack::current_loc += 4;
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
}

// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...

// 访问 return 指令
void Visit(const koopa_raw_return_t &ret)
{
    if (ret.value != nullptr) {
        Stack::Load2reg(ret.value, "a0");
    }
    if (Stack::R != 0) {
        lw_safe("ra", Stack::stack_frame_length - 4);
    }
    if (Stack::stack_frame_length > 0) {
        if (Stack::stack_frame_length < 2048)
            std::cout << "  addi sp, sp, " << Stack::stack_frame_length << std::endl;
        else {
            std::cout << "  li t0, " << Stack::stack_frame_length << std::endl;
            std::cout << "  add sp, sp, t0" << std::endl;
        }
    }
    std::cout << "  ret" << std::endl;
}

// 实际上用不到了
// void Visit(const koopa_raw_integer_t &integer, const koopa_raw_value_t value)
// {
//     // int pos = alloc_reg(value);
//     // std::cout << "  li a" << pos << ", " << integer.value << std::endl;
//     std::cout << "  li t0 " << integer.value << std::endl;
// }

// 访问二元运算指令
void Visit(const koopa_raw_binary_t &binary)
{
    Stack::Load2reg(binary.lhs, "t0");
    Stack::Load2reg(binary.rhs, "t1");
    // int left_pos = use_inst(binary.lhs);
    // int right_pos = use_inst(binary.rhs);
    // int pos = alloc_reg(value);

    switch (binary.op) {
        case KOOPA_RBO_NOT_EQ:
            std::cout << "  sub t0, t0, t1" << std::endl;
            std::cout << "  snez t0, t0" << std::endl;
            break;
        case KOOPA_RBO_EQ:
            std::cout << "  sub t0, t0, t1" << std::endl;
            std::cout << "  seqz t0, t0" << std::endl;
            break;
        case KOOPA_RBO_GT:
            std::cout << "  sgt t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_LT:
            std::cout << "  slt t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_GE:
            std::cout << "  slt t0, t0, t1" << std::endl;
            std::cout << "  seqz t0, t0" << std::endl;
            break;
        case KOOPA_RBO_LE:
            std::cout << "  sgt t0, t0, t1" << std::endl;
            std::cout << "  seqz t0, t0" << std::endl;
            break;
        case KOOPA_RBO_ADD:
            std::cout << "  add t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_SUB:
            std::cout << "  sub t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_MUL:
            std::cout << "  mul t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_DIV:
            std::cout << "  div t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_MOD:
            std::cout << "  rem t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_AND:
            std::cout << "  and t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_OR:
            std::cout << "  or t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_XOR:
            std::cout << "  xor t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_SHL:
            std::cout << "  sll t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_SHR:
            std::cout << "  srl t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_SAR:
            std::cout << "  sra t0, t0, t1" << std::endl;
            break;
        default:
            assert(false);
    }
    sw_safe("t0", Stack::current_loc);

    // 以下是弃用的代码
    // switch (binary.op) {
    //     case KOOPA_RBO_NOT_EQ:
    //         std::cout << "  sub a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         std::cout << "  snez a" << pos << ", a" << pos << std::endl;
    //         break;
    //     case KOOPA_RBO_EQ:
    //         std::cout << "  sub a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         std::cout << "  seqz a" << pos << ", a" << pos << std::endl;
    //         break;
    //     case KOOPA_RBO_GT:
    //         std::cout << "  sgt a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     case KOOPA_RBO_LT:
    //         std::cout << "  slt a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     case KOOPA_RBO_GE:
    //         std::cout << "  slt a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         std::cout << "  seqz a" << pos << ", a" << pos << std::endl;
    //         break;
    //     case KOOPA_RBO_LE:
    //         std::cout << "  sgt a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         std::cout << "  seqz a" << pos << ", a" << pos << std::endl;
    //         break;
    //     case KOOPA_RBO_ADD:
    //         std::cout << "  add a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     case KOOPA_RBO_SUB:
    //         std::cout << "  sub a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     case KOOPA_RBO_MUL:
    //         std::cout << "  mul a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     case KOOPA_RBO_DIV:
    //         std::cout << "  div a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     case KOOPA_RBO_MOD:
    //         std::cout << "  rem a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     case KOOPA_RBO_AND:
    //         std::cout << "  and a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     case KOOPA_RBO_OR:
    //         std::cout << "  or a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     case KOOPA_RBO_XOR:
    //         std::cout << "  xor a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     case KOOPA_RBO_SHL:
    //         std::cout << "  sll a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     case KOOPA_RBO_SHR:
    //         std::cout << "  srl a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     case KOOPA_RBO_SAR:
    //         std::cout << "  sra a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
    //         break;
    //     default:
    //         assert(false);
    // }
}

// 访问 store 指令
void Visit(const koopa_raw_store_t &store)
{
    Stack::Load2reg(store.value, "t0");
    Stack::StoreFromreg(store.dest, "t0", "t1");
}

// 访问 load 指令
void Visit(const koopa_raw_load_t &load)
{
    Stack::Load2reg(load.src, "t0");
    sw_safe("t0", Stack::current_loc);
}

// 访问alloc指令
// void Visit_alloc(koopa_raw_value_t value)
// {

// }

// 访问global_alloc指令
void Visit(const koopa_raw_global_alloc_t &global_alloc)
{
    switch (global_alloc.init->kind.tag) {
        case KOOPA_RVT_INTEGER:
            std::cout << "  .word " << global_alloc.init->kind.data.integer.value << std::endl;
            break;
        case KOOPA_RVT_ZERO_INIT:
            std::cout << "  .zero 4" << std::endl;
            break;
        default:
            assert(false);
    }
}

// 访问branch指令
void Visit(const koopa_raw_branch_t &branch)
{
    if (branch.cond->kind.tag == KOOPA_RVT_INTEGER) {
        if (branch.cond->kind.data.integer.value) {
            std::cout << "  j " << branch.true_bb->name + 1 << std::endl;
        } else {
            std::cout << "  j " << branch.false_bb->name + 1 << std::endl;
        }
    } else {
        Stack::Load2reg(branch.cond, "t0");
        for (int i = 0; i < branch.true_args.len; i++) {
            assert(((koopa_raw_value_data_t*)branch.true_args.buffer[i])->kind.tag == KOOPA_RVT_INTEGER);
            std::cout << "  li a" << i << ", " << ((koopa_raw_value_data_t*)branch.true_args.buffer[i])->kind.data.integer.value << std::endl;
        }
        for (int i = 0; i < branch.false_args.len; i++) {
            assert(((koopa_raw_value_data_t*)branch.false_args.buffer[i])->kind.tag == KOOPA_RVT_INTEGER);
            std::cout << "  li a" << i << ", " << ((koopa_raw_value_data_t*)branch.false_args.buffer[i])->kind.data.integer.value << std::endl;
        }
        std::cout << "  bnez t0, " << branch.true_bb->name + 1 << std::endl;
        std::cout << "  j " << branch.false_bb->name + 1 << std::endl;
    }
}

// 访问jump指令
void Visit(const koopa_raw_jump_t &jump)
{
    for (int i = 0; i < jump.args.len; i++) {
        lw_safe("a" + std::to_string(i), Stack::Query((koopa_raw_value_t)jump.args.buffer[i]));
    }
    std::cout << "  j " << jump.target->name + 1 << std::endl;
}

// 访问call指令
void Visit(const koopa_raw_call_t &call)
{
    for (int i = 0; i < call.args.len; i++) {
        auto arg = (koopa_raw_value_t)call.args.buffer[i];
        if (arg->kind.tag == KOOPA_RVT_INTEGER) {
            if (i < 8) {
                std::cout << "  li a" << i << ", " << arg->kind.data.integer.value << std::endl;
            } else {
                std::cout << "  li t0, " << arg->kind.data.integer.value << std::endl;
                std::cout << "  sw t0, " << 4 * (i - 8) << "(sp)" << std::endl;
            }
        } else {
            if (i < 8) {
                lw_safe("a" + std::to_string(i), Stack::Query(arg));
            } else {
                lw_safe("t0", Stack::Query(arg));
                std::cout << "  sw t0, " << 4 * (i - 8) << "(sp)" << std::endl;
            }
        }
    }
    std::cout << "  call " << call.callee->name + 1 << std::endl;

    sw_safe("a0", Stack::current_loc);
}