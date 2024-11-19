#include "visitraw.h"
#include <iostream>
#include <cassert>
#include <string>
#include <cstring>

// 函数声明略
// ...

inst_reg_use regs[8];

int use_inst(koopa_raw_value_t inst)
{
    // std::clog << "inst: " << inst->kind.tag << std::endl;
    for (int i = 0; i < 8; i++) {
        if (regs[i].data == inst) {
            regs[i].n_used_by -= 1;
            if (regs[i].n_used_by == 0) {
                regs[i].data = nullptr;
            }
            return i;
        }
    }
    assert(false);
}

int alloc_reg(const koopa_raw_value_t &value)
{
    int pos = 0;
    if (value->used_by.len > 0) {
        for (pos = 0; pos < 8; pos++) {
            if (regs[pos].n_used_by == 0) {
                regs[pos].data = value;
                regs[pos].n_used_by = value->used_by.len;
                break;
            }
        }
    }
    return pos;
}

// 访问 raw program
void Visit(const koopa_raw_program_t &program)
{
    // std::clog << "visiting raw program..." << std::endl;
    // 执行一些其他的必要操作
    for (auto i : regs) {
        i = {nullptr, 0};
    }
    // ...
    std::cout << "  .text" << std::endl;
    // 访问所有全局变量
    Visit(program.values);
    // 访问所有函数
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
    // 访问所有基本块
    Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb)
{
    // 执行一些其他的必要操作
    if (strncmp(bb->name+1, "entry", 5))
        std::cout << bb->name+1 << ":" << std::endl;
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
        Visit(kind.data.integer, value);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令
        // std::clog << "binary: " << kind.data.binary.op << std::endl;
        Visit(kind.data.binary, value);
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
        if (ret.value->kind.tag == KOOPA_RVT_INTEGER) {
            std::cout << "  li a0, " << ret.value->kind.data.integer.value << std::endl; 
        }
        else {
            std::cout << "  mv a0, " << "a" << use_inst(ret.value) << std::endl;
        }
    }
    std::cout << "  ret";
}

void Visit(const koopa_raw_integer_t &integer, koopa_raw_value_t value)
{
    int pos = alloc_reg(value);
    std::cout << "  li a" << pos << ", " << integer.value << std::endl;
}

// 访问二元运算指令
void Visit(const koopa_raw_binary_t &binary, koopa_raw_value_t value)
{
    if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER) {
        Visit(binary.lhs);
    }
    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER) {
        Visit(binary.rhs);
    }
    int left_pos = use_inst(binary.lhs);
    int right_pos = use_inst(binary.rhs);
    int pos = alloc_reg(value);
    switch (binary.op) {
        case KOOPA_RBO_NOT_EQ:
            std::cout << "  sub a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            std::cout << "  snez a" << pos << ", a" << pos << std::endl;
            break;
        case KOOPA_RBO_EQ:
            std::cout << "  sub a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            std::cout << "  seqz a" << pos << ", a" << pos << std::endl;
            break;
        case KOOPA_RBO_GT:
            std::cout << "  sgt a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        case KOOPA_RBO_LT:
            std::cout << "  slt a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        case KOOPA_RBO_GE:
            std::cout << "  slt a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            std::cout << "  seqz a" << pos << ", a" << pos << std::endl;
            break;
        case KOOPA_RBO_LE:
            std::cout << "  sgt a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            std::cout << "  seqz a" << pos << ", a" << pos << std::endl;
            break;
        case KOOPA_RBO_ADD:
            std::cout << "  add a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        case KOOPA_RBO_SUB:
            std::cout << "  sub a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        case KOOPA_RBO_MUL:
            std::cout << "  mul a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        case KOOPA_RBO_DIV:
            std::cout << "  div a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        case KOOPA_RBO_MOD:
            std::cout << "  rem a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        case KOOPA_RBO_AND:
            std::cout << "  and a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        case KOOPA_RBO_OR:
            std::cout << "  or a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        case KOOPA_RBO_XOR:
            std::cout << "  xor a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        case KOOPA_RBO_SHL:
            std::cout << "  sll a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        case KOOPA_RBO_SHR:
            std::cout << "  srl a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        case KOOPA_RBO_SAR:
            std::cout << "  sra a" << pos << ", a" << left_pos << ", a" << right_pos << std::endl;
            break;
        default:
            assert(false);
    }
}