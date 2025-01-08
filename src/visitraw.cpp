#include "visitraw.h"
#include <iostream>
#include <cassert>
#include <string>
#include <cmath>
#include <cstring>
#include <map>

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
                break;
            case KOOPA_RVT_ALLOC:
            {
                int pos = Query(value);
                if (pos < 2048) {
                    std::cout << "  addi " << reg << ", sp, " << pos << std::endl;
                } else {
                    std::cout << "  li " << reg << ", " << pos << std::endl;
                    std::cout << "  add " << reg << ", " << reg << ", sp" << std::endl;
                }
                break;
            }
            default: // 剩下的都是在栈上的指令
                lw_safe(reg, Query(value));
        }
    }
}

const int n_regs = 6;

struct Reg_info {
    bool used = false;
    int life = 0;
    koopa_raw_value_t inst = nullptr;
    // bool operator<(const Reg_info &rhs) const {
    //     return life < rhs.life;
    // }
} reg_info[n_regs];

std::map<koopa_raw_value_t, int> reg_map;

std::string distribute_reg(koopa_raw_value_t inst, bool use=true) {
    for (int i = 0; i < n_regs; i++)
        if (reg_info[i].used)
            reg_info[i].life++;
    // 已经加载到寄存器中了
    if (reg_map.find(inst) != reg_map.end()) {
        int index = reg_map[inst];
        reg_info[index].life = 0;
        std::string reg = num2reg(index);
        // std::clog << "Used " << reg << std::endl;
        return reg;
    }
    // 寄存器还有空位
    for (int i = 0; i < n_regs; i++) {
        if (!reg_info[i].used) {
            reg_info[i].used = true;
            reg_info[i].life = 0;
            reg_info[i].inst = inst;
            reg_map[inst] = i;
            std::string reg = num2reg(i);
            if (use)
                Stack::Load2reg(inst, reg);
            // std::clog << "Allocated " << reg << " for inst " << ((inst->name == nullptr) ? std::to_string((long)(inst)) : inst->name) << std::endl;
            return reg;
        }
    }
    // 寄存器没有空位，需要选择一个寄存器替换，策略LRU
    int max_life = -1;
    int max_life_index = -1;
    for (int i = 0; i < n_regs; i++) {
        if (reg_info[i].life > max_life) {
            max_life = reg_info[i].life;
            max_life_index = i;
        }
    }

    auto spilt_inst = reg_info[max_life_index].inst;
    reg_map.erase(spilt_inst);

    reg_info[max_life_index].used = true;
    reg_info[max_life_index].life = 0;
    reg_info[max_life_index].inst = inst;
    reg_map[inst] = max_life_index;
    std::string reg = num2reg(max_life_index);
    if (use)
        Stack::Load2reg(inst, reg);
    // std::clog << "Allocated " << reg << " for inst " << ((inst->name == nullptr) ? std::to_string((long)(inst)) : inst->name) << std::endl;
    return reg;
}

inline std::string num2reg(int n) {
    if ((n >= 0) && (n < 7)) {
        return "t" + std::to_string(n);
    }
    assert(false);
}

void clear_reg_info() {
    for (int i = 0; i < n_regs; i++) {
        reg_info[i].used = false;
        reg_info[i].life = 0;
        reg_info[i].inst = nullptr;
    }
    reg_map.clear();
}

void lw_safe(std::string reg, int loc) { // 从栈上加载到寄存器
    if (loc < 2048) {
        std::cout << "  lw " << reg << ", " << loc << "(sp)" << std::endl;
    } else {
        std::cout << "  li t6, " << loc << std::endl;
        std::cout << "  add t6, t6, sp" << std::endl;
        std::cout << "  lw " << reg << ", 0(t6)" << std::endl;
    }
}

void sw_safe(std::string reg, int loc) {
    if (loc < 2048) {
        std::cout << "  sw " << reg << ", " << loc << "(sp)" << std::endl;
    } else {
        std::cout << "  li t6, " << loc << std::endl;
        std::cout << "  add t6, t6, sp" << std::endl;
        std::cout << "  sw " << reg << ", 0(t6)" << std::endl;
    }
}

int array_len(const koopa_raw_type_kind* t) {
    switch (t->tag) {
        case KOOPA_RTT_INT32:
            return 1;
        case KOOPA_RTT_ARRAY:
            return t->data.array.len * array_len(t->data.array.base);
        case KOOPA_RTT_POINTER:
            return 1;
        default:
            assert(false);
    }
}


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
    int total_alloc_len = 0;
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
                insts_on_stack += 1;
                break;
            case KOOPA_RVT_ALLOC:
                total_alloc_len += array_len(inst->ty->data.pointer.base);
                break;
            case KOOPA_RVT_BINARY:
            case KOOPA_RVT_LOAD:
            case KOOPA_RVT_GET_PTR:
            case KOOPA_RVT_GET_ELEM_PTR:
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
    int L = total_alloc_len * 4;
    Stack::stack_frame_length = (S + Stack::R + A + L + 15) / 16 * 16;

    Stack::loc_map = std::map<koopa_raw_value_t, int>();

    if (Stack::stack_frame_length > 0) {
        if (Stack::stack_frame_length < 2048)
            std::cout << "  addi sp, sp, " << -Stack::stack_frame_length << std::endl;
        else {
            std::cout << "  li t6, " << -Stack::stack_frame_length << std::endl;
            std::cout << "  add sp, sp, t6" << std::endl;
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
    clear_reg_info();
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
    case KOOPA_RVT_ALLOC:
        // 访问 alloc 指令
        Stack::Insert(value, Stack::current_loc);
        Stack::current_loc += 4 * array_len(value->ty->data.pointer.base);
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        // 访问 global_alloc 指令
        std::cout << "  .globl " << value->name+1 << std::endl;
        std::cout << value->name+1 << ":" << std::endl;
        Visit(kind.data.global_alloc);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令
        Visit(kind.data.binary, value);
        Stack::Insert(value, Stack::current_loc);
        Stack::current_loc += 4;
        break;
    case KOOPA_RVT_STORE:
        // 访问 store 指令
        Visit(kind.data.store);
        break;
    case KOOPA_RVT_LOAD:
        // 访问 load 指令
        Visit(kind.data.load, value);
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
        Visit(kind.data.call, value);
        Stack::Insert(value, Stack::current_loc);
        Stack::current_loc += 4;
        clear_reg_info();
        break;
    case KOOPA_RVT_GET_ELEM_PTR:
        Visit(kind.data.get_elem_ptr, value);
        Stack::Insert(value, Stack::current_loc);
        Stack::current_loc += 4;
        break;
    case KOOPA_RVT_GET_PTR:
        Visit(kind.data.get_ptr, value);
        Stack::Insert(value, Stack::current_loc);
        Stack::current_loc += 4;
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
    std::cout << std::endl;
}

// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...

// 访问 return 指令
void Visit(const koopa_raw_return_t &ret)
{
    if (ret.value != nullptr) {
        std::string value = distribute_reg(ret.value);
        std::cout << "  mv a0," << value << std::endl;
    }
    if (Stack::R != 0) {
        lw_safe("ra", Stack::stack_frame_length - 4);
    }
    if (Stack::stack_frame_length > 0) {
        if (Stack::stack_frame_length < 2048)
            std::cout << "  addi sp, sp, " << Stack::stack_frame_length << std::endl;
        else {
            std::cout << "  li t6, " << Stack::stack_frame_length << std::endl;
            std::cout << "  add sp, sp, t6" << std::endl;
        }
    }
    std::cout << "  ret" << std::endl;
}

// 访问二元运算指令
void Visit(const koopa_raw_binary_t &binary, koopa_raw_value_t value)
{
    std::string reg_left = distribute_reg(binary.lhs);
    std::string reg_right = distribute_reg(binary.rhs);
    std::string reg_value = distribute_reg(value, false);

    switch (binary.op) {
        case KOOPA_RBO_NOT_EQ:
            std::cout << "  sub "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            std::cout << "  snez " << reg_value << ", " << reg_value                            << std::endl;
            break;
        case KOOPA_RBO_EQ:
            std::cout << "  sub "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            std::cout << "  seqz " << reg_value << ", " << reg_value                            << std::endl;
            break;
        case KOOPA_RBO_GT:
            std::cout << "  sgt "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        case KOOPA_RBO_LT:
            std::cout << "  slt "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        case KOOPA_RBO_GE:
            std::cout << "  slt "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            std::cout << "  seqz " << reg_value << ", " << reg_value                            << std::endl;
            break;
        case KOOPA_RBO_LE:
            std::cout << "  sgt "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            std::cout << "  seqz " << reg_value << ", " << reg_value                            << std::endl;
            break;
        case KOOPA_RBO_ADD:
            std::cout << "  add "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        case KOOPA_RBO_SUB:
            std::cout << "  sub "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        case KOOPA_RBO_MUL:
            std::cout << "  mul "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        case KOOPA_RBO_DIV:
            std::cout << "  div "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        case KOOPA_RBO_MOD:
            std::cout << "  rem "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        case KOOPA_RBO_AND:
            std::cout << "  and "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        case KOOPA_RBO_OR:
            std::cout << "  or "   << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        case KOOPA_RBO_XOR:
            std::cout << "  xor "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        case KOOPA_RBO_SHL:
            std::cout << "  sll "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        case KOOPA_RBO_SHR:
            std::cout << "  srl "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        case KOOPA_RBO_SAR:
            std::cout << "  sra "  << reg_value << ", " << reg_left     << ", " << reg_right    << std::endl;
            break;
        default:
            assert(false);
    }

    sw_safe(reg_value, Stack::current_loc);
}

// 访问 store 指令
void Visit(const koopa_raw_store_t &store)
{
    std::string reg_value = distribute_reg(store.value);
    std::string reg_dest  = distribute_reg(store.dest);

    std::cout << "  sw " << reg_value << ", 0(" << reg_dest << ")" << std::endl;
}

// 访问 load 指令
void Visit(const koopa_raw_load_t &load, koopa_raw_value_t value)
{
    std::string reg_src  = distribute_reg(load.src);
    std::string reg_dest = distribute_reg(value, false);

    std::cout << "  lw " << reg_dest << ", 0(" << reg_src << ")" << std::endl;
    sw_safe(reg_dest, Stack::current_loc);
}

// 访问global_alloc指令
void Visit(const koopa_raw_global_alloc_t &global_alloc)
{
    switch (global_alloc.init->kind.tag) {
        case KOOPA_RVT_INTEGER:
            std::cout << "  .word " << global_alloc.init->kind.data.integer.value << std::endl;
            break;
        case KOOPA_RVT_ZERO_INIT:
            std::cout << "  .zero " << 4*array_len(global_alloc.init->ty) << std::endl;
            break;
        case KOOPA_RVT_AGGREGATE:
            Visit(global_alloc.init->kind.data.aggregate);
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
        std::string reg_cond = distribute_reg(branch.cond);
        // 这里就比较dirty了，因为我只用了最简单的block_arg_ref，所以就写成这个样子了（甚至其实可以更简单）
        for (int i = 0; i < branch.true_args.len; i++) {
            assert(((koopa_raw_value_data_t*)branch.true_args.buffer[i])->kind.tag == KOOPA_RVT_INTEGER);
            std::cout << "  li a" << i << ", " << ((koopa_raw_value_data_t*)branch.true_args.buffer[i])->kind.data.integer.value << std::endl;
        }
        for (int i = 0; i < branch.false_args.len; i++) {
            assert(((koopa_raw_value_data_t*)branch.false_args.buffer[i])->kind.tag == KOOPA_RVT_INTEGER);
            std::cout << "  li a" << i << ", " << ((koopa_raw_value_data_t*)branch.false_args.buffer[i])->kind.data.integer.value << std::endl;
        }
        std::cout << "  bnez " << reg_cond << ", j2" << branch.true_bb->name + 1 << std::endl;
        std::cout << "  j " << branch.false_bb->name + 1 << std::endl;
        std::cout << "j2" << branch.true_bb->name + 1 << ":" << std::endl;
        std::cout << "  j " << branch.true_bb->name + 1 << std::endl;
    }
}

// 访问jump指令
void Visit(const koopa_raw_jump_t &jump)
{
    for (int i = 0; i < jump.args.len; i++) {
        std::string reg_arg = distribute_reg((koopa_raw_value_t)jump.args.buffer[i]);
        std::cout << "  mv a" << i << ", " << reg_arg << std::endl;
    }
    std::cout << "  j " << jump.target->name + 1 << std::endl;
}

// 访问call指令
void Visit(const koopa_raw_call_t &call, koopa_raw_value_t value)
{
    for (int i = 0; i < call.args.len; i++) {
        std::string reg_arg = distribute_reg((koopa_raw_value_t)call.args.buffer[i]);
        if (i < 8) {
            std::cout << "  mv a" << i << ", " << reg_arg << std::endl;
        } else {
            sw_safe(reg_arg, 4*(i-8));
        }
    }
    std::cout << "  call " << call.callee->name + 1 << std::endl;

    std::string reg_value = distribute_reg(value, false);
    std::cout << "  mv " << reg_value << ", a0" << std::endl;

    sw_safe(reg_value, Stack::current_loc);
}

// 访问get_elem_ptr指令
void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, koopa_raw_value_t value)
{
    std::string reg_src = distribute_reg(get_elem_ptr.src);
    std::string reg_index = distribute_reg(get_elem_ptr.index);
    std::string reg_value = distribute_reg(value, false);

    int multipler = 4 * array_len(get_elem_ptr.src->ty->data.pointer.base->data.array.base);

    if ((multipler & (multipler - 1)) == 0) { // 是2的整数次幂
        int digits = log2(multipler);
        std::cout << "  slli " << reg_index << ", " << reg_index << ", " << digits << std::endl;
    } else {
        std::cout << "  li " << reg_value << ", " << multipler << std::endl;
        std::cout << "  mul " << reg_index << ", " << reg_index << ", " << reg_value << std::endl;
    }
    std::cout << "  add " << reg_value << ", " << reg_src << ", " << reg_index << std::endl;
    sw_safe(reg_value, Stack::current_loc);
}

// 访问get_ptr指令
void Visit(const koopa_raw_get_ptr_t &get_ptr, koopa_raw_value_t value)
{
   std::string reg_src = distribute_reg(get_ptr.src);
    std::string reg_index = distribute_reg(get_ptr.index);
    std::string reg_value = distribute_reg(value, false);

    int multipler = 4 * array_len(get_ptr.src->ty->data.pointer.base);

    if ((multipler & (multipler - 1)) == 0) { // 是2的整数次幂
        int digits = log2(multipler);
        std::cout << "  slli " << reg_index << ", " << reg_index << ", " << digits << std::endl;
    } else {
        std::cout << "  li " << reg_value << ", " << multipler << std::endl;
        std::cout << "  mul " << reg_index << ", " << reg_index << ", " << reg_value << std::endl;
    }
    std::cout << "  add " << reg_value << ", " << reg_src << ", " << reg_index << std::endl;
    sw_safe(reg_value, Stack::current_loc);
}

// 访问aggregate指令
void Visit(const koopa_raw_aggregate_t &aggregate)
{
    for (int i = 0; i < aggregate.elems.len; i++) {
        auto value = (koopa_raw_value_t)aggregate.elems.buffer[i];
        switch (value->kind.tag) {
            case KOOPA_RVT_INTEGER:
                std::cout << "  .word " << value->kind.data.integer.value << std::endl;
                break;
            case KOOPA_RVT_ZERO_INIT:
                std::cout << "  .zero " << 4*array_len(value->ty) << std::endl;
                break;
            case KOOPA_RVT_AGGREGATE:
                Visit(value->kind.data.aggregate);
                break;
            default:
                assert(false);
        }
    }
}