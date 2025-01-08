#pragma once

#include "koopa.h"
#include <string>

namespace Stack {
    int Query(koopa_raw_value_t inst);
    void Insert(koopa_raw_value_t inst, int loc);
    void Load2reg(koopa_raw_value_t value, std::string reg);
}

void lw_safe(std::string reg, int loc);
void sw_safe(std::string reg, int loc);

std::string distribute_reg(koopa_raw_value_t inst, bool use);
inline std::string num2reg(int n);
void clear_reg_info();

void Visit(const koopa_raw_program_t &program);

void Visit(const koopa_raw_slice_t &slice);

void Visit(const koopa_raw_function_t &func);

void Visit(const koopa_raw_basic_block_t &bb);

void Visit(const koopa_raw_value_t &value);

void Visit(const koopa_raw_return_t &ret);

void Visit(const koopa_raw_binary_t &binary, koopa_raw_value_t value);

void Visit(const koopa_raw_global_alloc_t &global_alloc);

void Visit(const koopa_raw_store_t &store);

void Visit(const koopa_raw_load_t &load, koopa_raw_value_t value);

void Visit(const koopa_raw_branch_t &branch);

void Visit(const koopa_raw_jump_t &jump);

void Visit(const koopa_raw_call_t &call, koopa_raw_value_t value);

void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, koopa_raw_value_t value);

void Visit(const koopa_raw_get_ptr_t &get_ptr, koopa_raw_value_t value);

void Visit(const koopa_raw_aggregate_t &aggregate);