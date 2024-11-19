#pragma once

#include "koopa.h"

struct inst_reg_use {
    koopa_raw_value_t data;
    int n_used_by;
};

int alloc_reg(const koopa_raw_value_t &value);

int use_inst(koopa_raw_value_t inst);

void Visit(const koopa_raw_program_t &program);

void Visit(const koopa_raw_slice_t &slice);

void Visit(const koopa_raw_function_t &func);

void Visit(const koopa_raw_basic_block_t &bb);

void Visit(const koopa_raw_value_t &value);

void Visit(const koopa_raw_return_t &ret);

void Visit(const koopa_raw_integer_t &integer, int pos);

void Visit(const koopa_raw_binary_t &binary, int pos);