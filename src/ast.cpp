#include "ast.h"

void *CompUnitAST::toRaw() const
{
    auto raw_program = new koopa_raw_program_t;

    raw_program->values.buffer = nullptr;
    raw_program->values.len = 0;
    raw_program->values.kind = KOOPA_RSIK_VALUE;

    raw_program->funcs.len = 1;
    raw_program->funcs.buffer = new const void*[raw_program->funcs.len];
    raw_program->funcs.kind = KOOPA_RSIK_FUNCTION;
    raw_program->funcs.buffer[0] = func_def->toRaw();

    return raw_program;
}

void *FuncDefAST::toRaw() const
{
    auto raw_function = new koopa_raw_function_data_t;
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_FUNCTION;
    ty->data.function.params.buffer = nullptr;
    ty->data.function.params.len = 0;
    ty->data.function.params.kind = KOOPA_RSIK_TYPE;
    ty->data.function.ret = (koopa_raw_type_t)func_type->toRaw();

    raw_function->ty = ty;

    auto name = new char[ident.size() + 2];
    name[0] = '@';
    std::copy(ident.begin(), ident.end(), name + 1);
    name[ident.size() + 1] = '\0';
    raw_function->name = name;

    raw_function->params.len = 0;
    //raw_function->params.buffer = new const void*[raw_function->params.len];
    raw_function->params.buffer = nullptr;
    raw_function->params.kind = KOOPA_RSIK_VALUE;

    raw_function->bbs.len = 1;
    raw_function->bbs.buffer = new const void*[raw_function->bbs.len];
    raw_function->bbs.kind = KOOPA_RSIK_BASIC_BLOCK;
    raw_function->bbs.buffer[0] = block->toRaw();

    return raw_function;
}

void *FuncTypeAST::toRaw() const
{
    auto ty = new koopa_raw_type_kind_t;
    if (type == "void") {
        ty->tag = KOOPA_RTT_UNIT;
    }
    else if (type == "int") {
        ty->tag = KOOPA_RTT_INT32;
    }
    else {
        assert(false);
    }
    
    return ty;
}

void *BlockAST::toRaw() const
{
    auto raw_block = new koopa_raw_basic_block_data_t;

    raw_block->name = "%entry";

    raw_block->params.buffer = nullptr;
    raw_block->params.len = 0;
    raw_block->params.kind = KOOPA_RSIK_VALUE;

    raw_block->used_by.buffer = nullptr;
    raw_block->used_by.len = 0;
    raw_block->used_by.kind = KOOPA_RSIK_VALUE;

    raw_block->insts.kind = KOOPA_RSIK_VALUE;
    auto insts_vec = (std::vector<koopa_raw_value_data*>*) stmt->toRaw();
    int index = 0;
    for (auto inst : *insts_vec) {
        if (inst->kind.tag != KOOPA_RVT_INTEGER)
            index++;
    }
    raw_block->insts.len = index;
    raw_block->insts.buffer = new const void*[raw_block->insts.len];
    index = 0;
    for (auto inst: *insts_vec) {
        if (inst->kind.tag != KOOPA_RVT_INTEGER)
            raw_block->insts.buffer[index++] = inst;
    }

    return raw_block;
}

void *StmtAST::toRaw() const
{
    auto raw_stmt = new koopa_raw_value_data_t;

    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_INT32;
    raw_stmt->ty = ty;

    raw_stmt->name = nullptr;

    raw_stmt->used_by.buffer = nullptr;
    raw_stmt->used_by.kind = KOOPA_RSIK_VALUE;
    raw_stmt->used_by.len = 0;

    raw_stmt->kind.tag = KOOPA_RVT_RETURN;

    auto insts = (std::vector<koopa_raw_value_data*>*)exp->toRaw();

    assert(insts->size() > 0);

    auto value = *insts->rbegin();

    value->used_by.len = 1;
    value->used_by.buffer = new const void*[1];
    value->used_by.buffer[0] = raw_stmt;
    value->used_by.kind = KOOPA_RSIK_VALUE;

    raw_stmt->kind.data.ret.value = value;

    insts->push_back(raw_stmt);

    return insts;
}

void *ExpAST::toRaw() const
{
    return unary_exp->toRaw();
}

void *PrimaryExpAST::toRaw() const
{
    if (is_number) {
        auto raw = new koopa_raw_value_data_t;

        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_INT32;
        raw->ty = ty;

        raw->name = nullptr;

        raw->kind.tag = KOOPA_RVT_INTEGER;
        raw->kind.data.integer.value = number;
        auto insts = new std::vector<koopa_raw_value_data*>;
        insts->push_back(raw);
        return insts;
    }
    else {
        return exp->toRaw();
    }
}

void *UnaryExpAST::toRaw() const
{
    if (is_primary_exp) {
        return primary_exp->toRaw();
    }

    if (unaryop == '+') {
        return unary_exp->toRaw();
    }

    auto raw = new koopa_raw_value_data_t;

    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_INT32;
    raw->ty = ty;

    raw->name = nullptr;

    raw->kind.tag = KOOPA_RVT_BINARY;
    switch (unaryop) {
    case '-':
        raw->kind.data.binary.op = KOOPA_RBO_SUB;
        break;
    case '!':
        raw->kind.data.binary.op = KOOPA_RBO_NOT_EQ;
        break;
    }

    auto zero = new koopa_raw_value_data_t;
    auto zero_ty = new koopa_raw_type_kind_t;
    zero_ty->tag = KOOPA_RTT_INT32;
    zero->ty = zero_ty;

    zero->name = nullptr;
    zero->kind.tag = KOOPA_RVT_INTEGER;
    zero->kind.data.integer.value = 0;

    zero->used_by.kind = KOOPA_RSIK_VALUE;
    zero->used_by.len = 1;
    zero->used_by.buffer = new const void*[zero->used_by.len];
    zero->used_by.buffer[0] = raw;

    raw->kind.data.binary.lhs = zero;

    auto insts = (std::vector<koopa_raw_value_data*>*)unary_exp->toRaw();
    assert(insts->size() > 0);

    auto value = *insts->rbegin();

    raw->kind.data.binary.rhs = value;

    value->used_by.len = 1;
    value->used_by.buffer = new const void*[1];
    value->used_by.buffer[0] = raw;
    value->used_by.kind = KOOPA_RSIK_VALUE;

    insts->push_back(raw);

    return insts;
}

