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
}

void *FuncDefAST::toRaw() const
{
    auto raw = new koopa_raw_function_data_t;
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_FUNCTION;
    ty->data.function.params.buffer = nullptr;
    ty->data.function.params.len = 0;
    ty->data.function.params.kind = KOOPA_RSIK_VALUE;
    ty->data.function.ret = (koopa_raw_type_t)func_type->toRaw();

    raw->ty = ty;

    auto name = new char[ident.size() + 2];
    name[0] = '@';
    std::copy(ident.begin(), ident.end(), name + 1);
    name[ident.size() + 1] = '\0';
    raw->name = name;

    raw->params.len = 0;
    raw->params.buffer = new void*[raw->params.len];
}

void *FuncTypeAST::toRaw() const
{
    auto ty = new koopa_raw_type_kind_t;
    if (type == "void")
    {
        ty->tag = KOOPA_RTT_UNIT;
    }
    else if (type == "int")
    {
        ty->tag = KOOPA_RTT_INT32;
    }
    else
    {
        assert(false);
    }
    
    return ty;
}