#include "ast.h"
#include "symtab.h"

std::vector<koopa_raw_basic_block_data_t*> current_bbs;
std::vector<koopa_raw_value_data_t*> current_values;
std::vector<koopa_raw_value_data_t*> global_values;

koopa_raw_value_data_t* BaseAST::build_number(int number, koopa_raw_value_data_t* user=nullptr)
{
    auto num = new koopa_raw_value_data_t({
        .ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_INT32
        }),
        .name = nullptr,
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .kind = {
            .tag = KOOPA_RVT_INTEGER,
            .data = {
                .integer = {
                    .value = number,
                },
            },
        },
    });

    return num;
}

char* BaseAST::build_ident(const std::string& ident, char c)
{
    auto name = new char[ident.size() + 2];
    name[0] = c;
    std::copy(ident.begin(), ident.end(), name + 1);
    name[ident.size() + 1] = '\0';
    return name;
}

koopa_raw_basic_block_data_t* BaseAST::build_block_from_insts(std::vector<koopa_raw_value_data_t*>* insts=nullptr, const char* block_name=nullptr)
{
    auto raw_block = new koopa_raw_basic_block_data_t({
        .name = block_name,
        .params = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .insts = {
            .kind = KOOPA_RSIK_VALUE,
        },
    });

    if (insts == nullptr || insts->size() == 0) {
        raw_block->insts = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        };
        return raw_block;
    }
    raw_block->insts.len = insts->size();
    raw_block->insts.buffer = new const void*[raw_block->insts.len];
    for (int i = 0; i < raw_block->insts.len; i++) {
        raw_block->insts.buffer[i] = insts->at(i);
    }
    return raw_block;
}

koopa_raw_value_data_t* BaseAST::build_jump(koopa_raw_basic_block_t target, koopa_raw_slice_t* args=nullptr)
{
    auto raw_jump = new koopa_raw_value_data_t({
        .ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_UNIT
        }),
        .name = nullptr,
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .kind = {
            .tag = KOOPA_RVT_JUMP,
            .data = {
                .jump = {
                    .target = target,
                },
            },
        },
    });

    if (args == nullptr) {
        raw_jump->kind.data.jump.args = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        };
    } else raw_jump->kind.data.jump.args = *args;

    return raw_jump;
}

koopa_raw_slice_t BaseAST::combine_slices(koopa_raw_slice_t& s1, koopa_raw_slice_t& s2)
{
    koopa_raw_slice_t result;
    result.len = s1.len + s2.len;
    result.buffer = new const void*[result.len];
    result.kind = s1.kind;
    for (int i = 0; i < s1.len; i++) {
        result.buffer[i] = s1.buffer[i];
    }
    for (int i = 0; i < s2.len; i++) {
        result.buffer[i+s1.len] = s2.buffer[i];
    }
    return result;
}

void BaseAST::filter_basic_block(koopa_raw_basic_block_data_t* bb)
{
    auto insts = new std::vector<koopa_raw_value_data*>();
    for (int i = 0; i < bb->insts.len; i++) {
        auto inst = (koopa_raw_value_data*)bb->insts.buffer[i];
        if (inst != nullptr && inst->kind.tag != KOOPA_RVT_INTEGER && inst->kind.tag != KOOPA_RVT_BLOCK_ARG_REF) {
            insts->push_back(inst);
            if (inst->kind.tag == KOOPA_RVT_RETURN || inst->kind.tag == KOOPA_RVT_BRANCH || inst->kind.tag == KOOPA_RVT_JUMP)
                break;
        }
    }
    bb->insts.len = insts->size();
    bb->insts.buffer = new const void*[bb->insts.len];
    for (int i = 0; i < bb->insts.len; i++) {
        bb->insts.buffer[i] = insts->at(i);
    }
}

koopa_raw_value_data_t* BaseAST::build_branch(
    koopa_raw_value_data* cond,
    koopa_raw_basic_block_data_t* true_bb,
    koopa_raw_basic_block_data_t* false_bb,
    koopa_raw_slice_t* true_args=nullptr,
    koopa_raw_slice_t* false_args=nullptr
) 
{
    auto raw_stmt = new koopa_raw_value_data_t({
        .ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_UNIT
        }),
        .name = nullptr,
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .kind = {
            .tag = KOOPA_RVT_BRANCH,
            .data = {
                .branch = {
                    .cond = cond,
                    .true_bb = true_bb,
                    .false_bb = false_bb,
                }
            },
        },
    });

    if (true_args == nullptr) {
        raw_stmt->kind.data.branch.true_args = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        };
    } else raw_stmt->kind.data.branch.true_args = *true_args;
    if (false_args == nullptr) {
        raw_stmt->kind.data.branch.false_args = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        };
    } else raw_stmt->kind.data.branch.false_args = *false_args;

    return raw_stmt;
}

koopa_raw_value_data_t* BaseAST::build_alloc(const char* name)
{
    auto raw_alloc = new koopa_raw_value_data_t({
        .ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_POINTER,
            .data = {
                .pointer = {
                    .base = new koopa_raw_type_kind_t({.tag = KOOPA_RTT_INT32}),
                }
            },
        }),
        .name = name,
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .kind = {
            .tag = KOOPA_RVT_ALLOC,
        },
    });

    return raw_alloc;
}

koopa_raw_value_data_t* BaseAST::build_store(koopa_raw_value_data_t* value, koopa_raw_value_data_t* dest)
{
    auto raw_store = new koopa_raw_value_data_t({
        .ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_INT32,
        }),
        .name = nullptr,
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .kind = {
            .tag = KOOPA_RVT_STORE,
            .data = {
                .store = {
                    .value = value,
                    .dest = dest,
                },
            },
        },
    });

    return raw_store;
}

koopa_raw_function_data_t* BaseAST::build_function(
    const char* name, 
    std::vector<std::string> params, 
    std::string ret
)
{
    auto ret_ty = new koopa_raw_type_kind_t;
    if (ret == "void") {
        ret_ty->tag = KOOPA_RTT_UNIT;
    }
    else if (ret == "int") {
        ret_ty->tag = KOOPA_RTT_INT32;
    }
    else {
        assert(false);
    }
    auto raw_function = new koopa_raw_function_data_t({
        .name = build_ident(name, '@'),
        .params = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .bbs = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_BASIC_BLOCK,
        },
    });

    auto ty = new koopa_raw_type_kind_t({
        .tag = KOOPA_RTT_FUNCTION,
        .data = {
            .function = {
                .params = {
                    .buffer = new const void*[params.size()],
                    .len = (unsigned int)params.size(),
                    .kind = KOOPA_RSIK_TYPE,
                },
                .ret = ret_ty,
            },
        },
    });

    Symbol::insert(name, Symbol::TYPE_FUNCTION, raw_function);

    for (int i = 0; i < params.size(); i++) {
        auto param = params[i];
        koopa_raw_type_kind_t* raw_param_ty = nullptr;
        if (param == "int")
            raw_param_ty = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_INT32
            });
        else if (param == "p2i") {
            raw_param_ty = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_POINTER,
                .data = {
                    .pointer = {
                        .base = new koopa_raw_type_kind_t({
                            .tag = KOOPA_RTT_INT32
                        }),
                    }
                },
            });
        }
        else assert(false);
        ty->data.function.params.buffer[i] = raw_param_ty;
    }

    raw_function->ty = ty;
    
    return raw_function;
}

koopa_raw_value_data_t* BaseAST::build_aggregate(std::vector<int>* dim_vec, std::vector<int>* result_vec)
{
    int dim = dim_vec->back();
    if (dim_vec->size() == 1) {
        auto raw_aggregate = new koopa_raw_value_data_t({
            .ty = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_ARRAY,
                .data = {
                    .array = {
                        .base = new koopa_raw_type_kind_t({
                            .tag = KOOPA_RTT_INT32,
                        }),
                        .len = (unsigned long)dim,
                    },
                },
            }),
            .name = nullptr,
            .used_by = {
                .buffer = nullptr,
                .len = 0,
                .kind = KOOPA_RSIK_VALUE,
            },
            .kind = {
                .tag = KOOPA_RVT_AGGREGATE,
                .data = {
                    .aggregate = {
                        .elems = {
                            .buffer = new const void*[dim],
                            .len = (unsigned int)dim,
                            .kind = KOOPA_RSIK_VALUE,
                        },
                    },
                },
            },
        });
        for (int i = 0; i < dim; i++) {
            raw_aggregate->kind.data.aggregate.elems.buffer[i] = build_number(result_vec->at(i));
        }
        return raw_aggregate;
    }
    auto raw_aggregate = new koopa_raw_value_data_t({
        .ty = nullptr,
        .name = nullptr,
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .kind = {
            .tag = KOOPA_RVT_AGGREGATE,
            .data = {
                .aggregate = {
                    .elems = {
                        .buffer = new const void*[dim],
                        .len = (unsigned int)dim,
                        .kind = KOOPA_RSIK_VALUE,
                    },
                },
            },
        },
    });
    auto ty_integer = new koopa_raw_type_kind_t({
        .tag = KOOPA_RTT_INT32,
    });
    koopa_raw_type_kind_t* temp_ty = ty_integer;
    for (auto i : *dim_vec) {
        auto ty_array = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_ARRAY,
            .data = {
                .array = {
                    .base = temp_ty,
                    .len = (unsigned long)i,
                },
            },
        });
        temp_ty = ty_array;
    }
    raw_aggregate->ty = temp_ty;
    auto sub_dim_vec = new std::vector<int>(dim_vec->begin(), dim_vec->end() - 1);
    int sub_len = 1;
    for (auto i : *sub_dim_vec) {
        sub_len *= i;
    }
    for (int i = 0; i < dim; i++) {
        auto sub_vector = new std::vector<int>(result_vec->begin() + i * sub_len, result_vec->begin() + (i + 1) * sub_len);
        raw_aggregate->kind.data.aggregate.elems.buffer[i] = build_aggregate(sub_dim_vec, sub_vector);
    }
        
    return raw_aggregate;
}

koopa_raw_value_data_t* BaseAST::build_aggregate(std::vector<int>* dim_vec, std::vector<koopa_raw_value_data_t*>* result_vec)
{
    auto int_vec = new std::vector<int>();
    for (auto i : *result_vec) {
        auto inst = (koopa_raw_value_data_t*)i;
        assert(inst->kind.tag == KOOPA_RVT_INTEGER);
        int_vec->push_back(inst->kind.data.integer.value);
    }
    return build_aggregate(dim_vec, int_vec);
}

koopa_raw_value_data_t* BaseAST::build_binary(koopa_raw_binary_op op, koopa_raw_value_data_t* lhs, koopa_raw_value_data_t* rhs)
{
    auto binary = new koopa_raw_value_data_t({
        .ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_INT32,
        }),
        .name = nullptr,
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .kind = {
            .tag = KOOPA_RVT_BINARY,
            .data = {
                .binary = {
                    .op = op,
                    .lhs = lhs,
                    .rhs = rhs,
                },
            },
        },
    });
    return binary;
}

static koopa_raw_value_data_t* build_get_ptr(koopa_raw_value_data_t* src, koopa_raw_value_data_t* index)
{
    auto get_ptr = new koopa_raw_value_data_t({
        .ty = src->ty,
        .name = nullptr,
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .kind = {
            .tag = KOOPA_RVT_GET_PTR,
            .data = {
                .get_ptr = {
                    .src = src,
                    .index = index,
                },
            },
        },
    });
    return get_ptr;
}

koopa_raw_value_data_t* BaseAST::build_get_elem_ptr(koopa_raw_value_data_t* src, koopa_raw_value_data_t* index)
{
    auto get_elem_ptr = new koopa_raw_value_data_t({
        .ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_POINTER,
            .data = {
                .pointer = {
                    .base = src->ty->data.pointer.base->data.array.base,
                },
            },
        }),
        .name = nullptr,
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .kind = {
            .tag = KOOPA_RVT_GET_ELEM_PTR,
            .data = {
                .get_elem_ptr = {
                    .src = src,
                    .index = index,
                },
            },
        },
    });
    return get_elem_ptr;
}

void BaseAST::store2array(koopa_raw_value_data_t* src, std::vector<koopa_raw_value_data*>* values, std::vector<int>* dims)
{
    if (dims->size() == 1) {
        assert(values->size() == dims->back());
        for (int i = 0; i < dims->back(); i++) {
            auto get_elem_ptr = build_get_elem_ptr(src, build_number(i));
            auto store = build_store(values->at(i), get_elem_ptr);
            append_value(get_elem_ptr);
            append_value(store);
        }
    }
    else {
        assert(values->size() % dims->back() == 0);
        int sub_len = values->size() / dims->back();
        for (int i = 0; i < dims->back(); i++) {
            auto sub_values = new std::vector<koopa_raw_value_data*>(values->begin() + i * sub_len, values->begin() + (i + 1) * sub_len);
            auto get_elem_ptr = build_get_elem_ptr(src, build_number(i));
            append_value(get_elem_ptr);
            store2array(get_elem_ptr, sub_values, new std::vector<int>(dims->begin(), dims->end() - 1));
        }
    }
}

void BaseAST::store2array(koopa_raw_value_data_t* src, std::vector<int>* values, std::vector<int>* dims)
{
    auto new_values = new std::vector<koopa_raw_value_data_t*>();
    for (auto i : *values) {
        new_values->push_back(build_number(i));
    }
    store2array(src, new_values, dims);
}

void append_value(koopa_raw_value_data_t* value)
{
    current_values.push_back(value);
}

void end_block()
{
    auto last_bb = BaseAST::build_block_from_insts(&current_values);
    current_values.clear();
    current_bbs.push_back(last_bb);
}

void append_bb(koopa_raw_basic_block_data_t* bb)
{
    end_block();
    current_bbs.push_back(bb);
}

void *CompUnitAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto raw_program = new koopa_raw_program_t;

    std::vector<koopa_raw_function_data_t*> funcs;

    Symbol::enter_scope();

    funcs.push_back(build_function("getint", {}, "int"));
    funcs.push_back(build_function("getch", {}, "int"));
    funcs.push_back(build_function("getarray", {"p2i"}, "int"));
    funcs.push_back(build_function("putint", {"int"}, "void"));
    funcs.push_back(build_function("putch", {"int"}, "void"));
    funcs.push_back(build_function("putarray", {"int", "p2i"}, "void"));
    funcs.push_back(build_function("starttime", {}, "void"));
    funcs.push_back(build_function("stoptime", {}, "void"));

    for (auto& global_def : *global_def_list) {
        auto func_def = (koopa_raw_function_data_t*)global_def->toRaw();
        if (func_def != nullptr)
            funcs.push_back(func_def);
    }

    raw_program->values.len = global_values.size();
    raw_program->values.buffer = new const void*[raw_program->values.len];
    raw_program->values.kind = KOOPA_RSIK_VALUE;
    for (int i = 0; i < global_values.size(); i++) {
        raw_program->values.buffer[i] = global_values[i];
    }

    raw_program->funcs.len = funcs.size();
    raw_program->funcs.buffer = new const void*[raw_program->funcs.len];
    raw_program->funcs.kind = KOOPA_RSIK_FUNCTION;
    for (int i = 0; i < funcs.size(); i++) {
        raw_program->funcs.buffer[i] = funcs[i];
    }

    Symbol::leave_scope();

    return raw_program;
}

void *GlobalDefAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    switch (type)
    {
        case FUNC_DEF:
            return func_def->toRaw();
        case DECL:
            // global decl
            decl->toRaw(1);
            return nullptr;
        default:
            assert(false);
    }
}

void *FuncDefAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto raw_function = new koopa_raw_function_data_t;
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_FUNCTION;
    ty->data.function.params.len = raw_function->params.len = func_f_param_list->size();
    ty->data.function.params.buffer = new const void*[func_f_param_list->size()];
    raw_function->params.buffer = new const void*[func_f_param_list->size()];
    ty->data.function.params.kind = KOOPA_RSIK_TYPE;
    raw_function->params.kind = KOOPA_RSIK_VALUE;

    auto ret_ty = new koopa_raw_type_kind_t;
    if (func_type == "void") {
        ret_ty->tag = KOOPA_RTT_UNIT;
    }
    else if (func_type == "int") {
        ret_ty->tag = KOOPA_RTT_INT32;
    }
    else {
        assert(false);
    }
    ty->data.function.ret = ret_ty;

    Symbol::insert(ident, Symbol::TYPE_FUNCTION, raw_function);
    Symbol::enter_scope();

    auto var_decl_insts = new std::vector<koopa_raw_value_data*>();

    for (int i = 0; i < func_f_param_list->size(); i++) {
        auto f_param = (koopa_raw_value_data_t*)func_f_param_list->at(i)->toRaw(i);
        raw_function->params.buffer[i] = f_param;
        ty->data.function.params.buffer[i] = f_param->ty;
        switch (f_param->ty->tag) {
            case KOOPA_RTT_INT32:
            {
                auto raw_alloc = build_alloc(build_ident(f_param->name + 1, '%'));
                var_decl_insts->push_back(raw_alloc);
                var_decl_insts->push_back(build_store(f_param, raw_alloc));
                Symbol::insert(f_param->name + 1, Symbol::TYPE_VAR, raw_alloc);
            }
            break;
            case KOOPA_RTT_POINTER:
            {
                auto raw_alloc = build_alloc(build_ident(f_param->name + 1, '%'));
                raw_alloc->ty = new koopa_raw_type_kind({
                    .tag = KOOPA_RTT_POINTER,
                    .data = {
                        .pointer = {
                            .base = f_param->ty,
                        },
                    },
                });
                var_decl_insts->push_back(raw_alloc);
                var_decl_insts->push_back(build_store(f_param, raw_alloc));
                Symbol::insert(f_param->name + 1, Symbol::TYPE_POINTER, raw_alloc);
            }
            break;
            default:
                assert(false);
        }
    }
    raw_function->ty = ty;
    raw_function->name = build_ident(ident, '@');
    current_bbs.clear();
    block->toRaw(-1);
    end_block();
    auto filtered_bbs = std::vector<koopa_raw_basic_block_data_t*>();
    for (auto block_bb : current_bbs) {
        if (block_bb->insts.len == 0 && block_bb->name == nullptr) {
            // delete block_bb;
            continue;
        }
        else if (block_bb->name == nullptr) {
            auto last = filtered_bbs.back();
            last->insts = combine_slices(last->insts, block_bb->insts);
        }
        else filtered_bbs.push_back(block_bb);
    }
    raw_function->bbs.len = filtered_bbs.size();
    raw_function->bbs.buffer = new const void*[raw_function->bbs.len];
    raw_function->bbs.kind = KOOPA_RSIK_BASIC_BLOCK;
    for (int i = 0; i < filtered_bbs.size(); i++) {
        filter_basic_block(filtered_bbs[i]);
        char *bb_name = new char[strlen(filtered_bbs[i]->name) + ident.length() + 2];
        memset(bb_name, 0, strlen(filtered_bbs[i]->name) + ident.length() + 2);
        bb_name[0] = '%';
        strcat(bb_name, ident.c_str());
        strcat(bb_name, "_");
        strcat(bb_name, filtered_bbs[i]->name + 1);
        filtered_bbs[i]->name = bb_name;
        raw_function->bbs.buffer[i] = filtered_bbs[i];
    }

    auto first_bb = (koopa_raw_basic_block_data_t*)raw_function->bbs.buffer[0];
    auto vec_def_bb = build_block_from_insts(var_decl_insts);
    first_bb->insts = combine_slices(vec_def_bb->insts, first_bb->insts);

    auto last_bb = (koopa_raw_basic_block_data_t*)raw_function->bbs.buffer[raw_function->bbs.len-1];
    if (last_bb->insts.len == 0 || ((koopa_raw_value_data_t*)(last_bb->insts.buffer[last_bb->insts.len-1]))->kind.tag != KOOPA_RVT_RETURN) {
        last_bb->insts.len += 1;
        auto buffer = new const void*[last_bb->insts.len];
        for (int i = 0; i < last_bb->insts.len - 1; i++) {
            buffer[i] = last_bb->insts.buffer[i];
        }
        last_bb->insts.buffer = buffer;
        auto raw_ret = new koopa_raw_value_data;
        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_UNIT;
        raw_ret->ty = ty;
        raw_ret->name = nullptr;
        raw_ret->kind.tag = KOOPA_RVT_RETURN;
        if (raw_function->ty->data.function.ret->tag == KOOPA_RTT_UNIT) {
            raw_ret->kind.data.ret.value = nullptr;
        }
        else if (raw_function->ty->data.function.ret->tag == KOOPA_RTT_INT32) {
            raw_ret->kind.data.ret.value = build_number(0);
        }
        else assert(false);
        buffer[last_bb->insts.len-1] = raw_ret;
    }

    Symbol::leave_scope();

    return raw_function;
}

void *FuncFParamAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto f_param = new koopa_raw_value_data_t({
        .name = build_ident(ident, '@'),
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .kind = {
            .tag = KOOPA_RVT_FUNC_ARG_REF,
            .data = {
                .func_arg_ref = {
                    .index = (unsigned long)n,
                },
            },
        },
    });

    if (!is_array) {
        f_param->ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_INT32,
        });
    }
    else {
        if (dim_list == nullptr) {
            f_param->ty = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_POINTER,
                .data = {
                    .pointer = {
                        .base = new koopa_raw_type_kind_t({
                            .tag = KOOPA_RTT_INT32,
                        }),
                    },
                },
            });
        }
        else {
            auto dim_vec = new std::vector<int>();
            for (auto i = dim_list->rbegin(); i != dim_list->rend(); i++) {
                auto dim = (long)(*i)->toRaw();
                dim_vec->push_back(dim);
            }
            auto temp_ty = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_INT32,
            });
            for (auto i : *dim_vec) {
                auto ty_array = new koopa_raw_type_kind_t({
                    .tag = KOOPA_RTT_ARRAY,
                    .data = {
                        .array = {
                            .base = temp_ty,
                            .len = (unsigned long)i,
                        },
                    },
                });
                temp_ty = ty_array;
            }
            f_param->ty = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_POINTER,
                .data = {
                    .pointer = {
                        .base = temp_ty,
                    },
                },
            });
        }
    }

    return f_param;
}

void *BlockAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto basic_block = new koopa_raw_basic_block_data_t;
    if (n == -1) {
        basic_block->name = "%entry";
        n = 0;
    }
    else {
        basic_block->name = nullptr;
    }
    basic_block->params = {
        .buffer = nullptr,
        .len = 0,
        .kind = KOOPA_RSIK_VALUE,
    };
    basic_block->used_by = {
        .buffer = nullptr,
        .len = 0,
        .kind = KOOPA_RSIK_VALUE,
    };
    basic_block->insts = {
        .buffer = nullptr,
        .len = 0,
        .kind = KOOPA_RSIK_VALUE,
    };

    append_bb(basic_block);
    for (auto& block_item : *block_item_list) {
        block_item->toRaw(n, args);
    }
    return nullptr;
    // return basic_block;
}

void *BlockItemAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    switch (type) {
        case DECL:
            return decl->toRaw(n, args);
        case STMT:
            return stmt->toRaw(n, args);
        default:
            assert(false);
    }
}

void *StmtAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    switch (type)
    {
        case OPEN_STMT:
            return open_stmt->toRaw(n, args);
        case CLOSED_STMT:
            return closed_stmt->toRaw(n, args);
        default:
            assert(false);
    }
}

void *OpenStmtAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    switch (type) {
        case IF:
        {
            auto true_entry_bb = build_block_from_insts(nullptr, "%if_true_entry");
            auto end_bb = build_block_from_insts(nullptr, "%if_end");
            auto cond = (koopa_raw_value_data_t*)exp->toRaw();
            auto branch = build_branch(cond, true_entry_bb, end_bb);
            append_value(branch);
            append_bb(true_entry_bb);
            stmt->toRaw(n, args);
            auto true_jmp = build_jump(end_bb);
            append_value(true_jmp);
            append_bb(end_bb);
            return nullptr;
        }
        case IF_ELSE:
        {
            auto true_entry_bb = build_block_from_insts(nullptr, "if_true_entry");
            auto false_entry_bb = build_block_from_insts(nullptr, "if_false_entry");
            auto end_bb = build_block_from_insts(nullptr, "if_end");
            auto cond = (koopa_raw_value_data_t*)exp->toRaw();
            auto branch = build_branch(cond, true_entry_bb, false_entry_bb);
            append_value(branch);
            append_bb(true_entry_bb);
            closed_stmt->toRaw(n, args);
            auto true_jmp = build_jump(end_bb);
            append_value(true_jmp);
            append_bb(false_entry_bb);
            open_stmt->toRaw(n, args);
            auto false_jmp = build_jump(end_bb);
            append_value(false_jmp);
            append_bb(end_bb);
            return nullptr;
        }
        case WHILE:
        {
            auto entry_bb = build_block_from_insts(nullptr, "%while_entry");
            auto body_bb = build_block_from_insts(nullptr, "%while_body");
            auto end_bb = build_block_from_insts(nullptr, "%while_end");
            auto init_jmp = build_jump(entry_bb);
            append_value(init_jmp);
            Symbol::enter_loop(entry_bb, end_bb);
            append_bb(entry_bb);
            auto cond = (koopa_raw_value_data_t*)exp->toRaw();
            auto branch = build_branch(cond, body_bb, end_bb);
            append_value(branch);
            append_bb(body_bb);
            stmt->toRaw(n, args);
            auto jmp = build_jump(entry_bb);
            append_value(jmp);
            append_bb(end_bb);
            Symbol::leave_loop();
            return nullptr;
        }
        default:
            assert(false);
    }
}

void *ClosedStmtAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    switch (type) {
        case SIMPLE_STMT:
        {
            return simple_stmt->toRaw(n, args);
        }
        case IF_ELSE:
        {
            auto true_entry_bb = build_block_from_insts(nullptr, "if_true_entry");
            auto false_entry_bb = build_block_from_insts(nullptr, "if_false_entry");
            auto end_bb = build_block_from_insts(nullptr, "if_end");
            auto cond = (koopa_raw_value_data_t*)exp->toRaw();
            auto branch = build_branch(cond, true_entry_bb, false_entry_bb);
            append_value(branch);
            append_bb(true_entry_bb);
            closed_stmt->toRaw(n, args);
            auto true_jmp = build_jump(end_bb);
            append_value(true_jmp);
            append_bb(false_entry_bb);
            closed_stmt2->toRaw(n, args);
            auto false_jmp = build_jump(end_bb);
            append_value(false_jmp);
            append_bb(end_bb);
            return nullptr;
        }
        case WHILE:
        {
            auto entry_bb = build_block_from_insts(nullptr, "%while_entry");
            auto body_bb = build_block_from_insts(nullptr, "%while_body");
            auto end_bb = build_block_from_insts(nullptr, "%while_end");
            auto init_jmp = build_jump(entry_bb);
            append_value(init_jmp);
            Symbol::enter_loop(entry_bb, end_bb);
            append_bb(entry_bb);
            auto cond = (koopa_raw_value_data_t*)exp->toRaw();
            auto branch = build_branch(cond, body_bb, end_bb);
            append_value(branch);
            append_bb(body_bb);
            closed_stmt->toRaw(n, args);
            auto jmp = build_jump(entry_bb);
            append_value(jmp);
            append_bb(end_bb);
            Symbol::leave_loop();
            return nullptr;
        }
        default:
            assert(false);
    }
}

void *SimpleStmtAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    switch (type)
    {
        case LVAL_ASSIGN:
        {
            auto value = (koopa_raw_value_data_t*)exp->toRaw();
            auto dest = (koopa_raw_value_data_t*)lval->toRaw();
            auto raw_store = build_store(value, dest);
            append_value(raw_store);
            return nullptr;
        }
        case RETURN:
        {
            auto raw_return = new koopa_raw_value_data_t({
                .ty = new koopa_raw_type_kind_t({
                    .tag = KOOPA_RTT_INT32
                }),
                .name = nullptr,
                .used_by = {
                    .buffer = nullptr,
                    .len = 0,
                    .kind = KOOPA_RSIK_VALUE,
                },
                .kind = {
                    .tag = KOOPA_RVT_RETURN,
                    .data = {
                        .ret = {
                            .value = (koopa_raw_value_data_t*)exp->toRaw(),
                        },
                    },
                },
            });
            append_value(raw_return);
            return nullptr;
        }
        case EMPTY_RETURN:
        {
            auto raw_return = new koopa_raw_value_data_t({
                .ty = new koopa_raw_type_kind_t({
                    .tag = KOOPA_RTT_UNIT
                }),
                .name = nullptr,
                .used_by = {
                    .buffer = nullptr,
                    .len = 0,
                    .kind = KOOPA_RSIK_VALUE,
                },
                .kind = {
                    .tag = KOOPA_RVT_RETURN,
                    .data = {
                        .ret = {
                            .value = nullptr,
                        },
                    },
                },
            });
            append_value(raw_return);
            return nullptr;
        }
        case EXP:
        {
            exp->toRaw();
            return nullptr;
        }
        case EMPTY:
        {
            return nullptr;
        }
        case BLOCK:
        {
            Symbol::enter_scope();
            block->toRaw(n, args);
            Symbol::leave_scope();
            return nullptr;
        }
        case BREAK:
        {
            koopa_raw_basic_block_data_t* target = Symbol::get_loop_exit();
            assert(target != nullptr);
            auto raw_jmp = build_jump(target);
            append_value(raw_jmp);
            return nullptr;
        }
        case CONTINUE:
        {
            koopa_raw_basic_block_data_t* target = Symbol::get_loop_header();
            assert(target != nullptr);
            auto raw_jmp = build_jump(target);
            append_value(raw_jmp);
            return nullptr;
        }
        default:
            assert(false);
    }
}

void *ConstExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto value = (koopa_raw_value_data_t*)exp->toRaw();
    assert(value->kind.tag == KOOPA_RVT_INTEGER);
    int val = value->kind.data.integer.value;
    return (void*)val;
}

void *ExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    return lor_exp->toRaw();
}

void *PrimaryExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    switch (type)
    {
    case EXP:
        return exp->toRaw();
    case LVAL:
    {
        auto value = (koopa_raw_value_data_t*)lval->toRaw();
        switch (value->kind.tag) {
            case KOOPA_RVT_INTEGER:
                return value;
            case KOOPA_RVT_ALLOC:
            case KOOPA_RVT_GLOBAL_ALLOC:
            case KOOPA_RVT_GET_PTR:
            case KOOPA_RVT_GET_ELEM_PTR:
            {
                switch (value->ty->data.pointer.base->tag) {
                    case KOOPA_RTT_POINTER:
                    case KOOPA_RTT_INT32:
                    {
                        auto load = new koopa_raw_value_data_t({
                            .ty = value->ty->data.pointer.base,
                            .name = nullptr,
                            .used_by = {
                                .buffer = nullptr,
                                .len = 0,
                                .kind = KOOPA_RSIK_VALUE,
                            },
                            .kind = {
                                .tag = KOOPA_RVT_LOAD,
                                .data = {
                                    .load = {
                                        .src = value,
                                    },
                                },
                            },
                        });
                        append_value(load);
                        return load;
                    }
                    case KOOPA_RTT_ARRAY:
                    {
                        auto get_elem_ptr = new koopa_raw_value_data_t({
                            .ty = new koopa_raw_type_kind_t({
                                .tag = KOOPA_RTT_POINTER,
                                .data = {
                                    .pointer = {
                                        .base = value->ty->data.pointer.base->data.array.base,
                                    }
                                }
                            }),
                            .name = nullptr,
                            .used_by = {
                                .buffer = nullptr,
                                .len = 0,
                                .kind = KOOPA_RSIK_VALUE,
                            },
                            .kind = {
                                .tag = KOOPA_RVT_GET_ELEM_PTR,
                                .data = {
                                    .get_elem_ptr = {
                                        .src = value,
                                        .index = build_number(0),
                                    }
                                },
                            },
                        });
                        append_value(get_elem_ptr);
                        return get_elem_ptr;
                    }
                    default:
                        assert(false);
                }
            }
            default:
                assert(false);
        }
    }
    case NUMBER:
    {
        return build_number(number);
    }
    default:
        assert(false);
    }
}

void *UnaryExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    switch (type)
    {
    case PRIMARY:
        return primary_exp->toRaw();
    case UNARY:
        if (unaryop == '+') {
            return unary_exp->toRaw();
        }
    {
        auto value = (koopa_raw_value_data_t*)unary_exp->toRaw();
        if (value->kind.tag == KOOPA_RVT_INTEGER) {
            int val = value->kind.data.integer.value;
            switch (unaryop) {
            case '-':
                val = -val;
                break;
            case '!':
                val = !val;
                break;
            default:
                assert(false);
            }
            return build_number(val);
        }

        koopa_raw_binary_op op;
        switch (unaryop) {
        case '-':
            op = KOOPA_RBO_SUB;
            break;
        case '!':
            op = KOOPA_RBO_EQ;
            break;
        default:
            assert(false);
        }
        auto unary = build_binary(op, build_number(0), value);
        append_value(unary);
        return unary;
    }
    case FUNC_CALL:
    {
        auto call = new koopa_raw_value_data_t({
            .ty = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_INT32,
            }),
            .name = nullptr,
            .used_by = {
                .buffer = nullptr,
                .len = 0,
                .kind = KOOPA_RSIK_VALUE,
            },
            .kind = {
                .tag = KOOPA_RVT_CALL,
                .data = {
                    .call = {
                        .callee = Symbol::query(ident).function,
                        .args = {
                            .kind = KOOPA_RSIK_VALUE,
                        },
                    },
                },
            },
        });

        auto params = new std::vector<koopa_raw_value_data_t*>();
        
        for (auto &exp : *func_r_param_list) {
            auto exp_value = (koopa_raw_value_data_t*)exp->toRaw();
            params->push_back(exp_value);
        }
        call->kind.data.call.args.len = params->size();
        call->kind.data.call.args.buffer = new const void* [params->size()];
        for (int i = 0; i < params->size(); i++) {
            call->kind.data.call.args.buffer[i] = params->at(i);
        }
        append_value(call);
        return call;
    }
    default:
        assert(false);
    }
}

void *MulExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (type == UNARY)
        return unary_exp->toRaw();
    assert(type == MUL);

    auto left_value = (koopa_raw_value_data_t*)mul_exp->toRaw();
    auto right_value = (koopa_raw_value_data_t*)unary_exp->toRaw();

    if (left_value->kind.tag == KOOPA_RVT_INTEGER && right_value->kind.tag == KOOPA_RVT_INTEGER) {
        int left = left_value->kind.data.integer.value;
        int right = right_value->kind.data.integer.value;
        int result;
        switch (op) {
        case '*':
            result = left * right;
            break;
        case '/':
            result = left / right;
            break;
        case '%':
            result = left % right;
            break;
        default:
            assert(false);
        }

        return build_number(result);
    }

    koopa_raw_binary_op bop;
    switch (op)
    {
    case '*':
        bop = KOOPA_RBO_MUL;
        break;
    case '/':
        bop = KOOPA_RBO_DIV;
        break;
    case '%':
        bop = KOOPA_RBO_MOD;
        break;
    default:
        assert(false);
    }
    auto mul = build_binary(bop, left_value, right_value);
    append_value(mul);
    return mul;
}

void *AddExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (type == MUL)
        return mul_exp->toRaw();
    assert(type == ADD);

    auto left_value = (koopa_raw_value_data_t*)add_exp->toRaw();
    auto right_value = (koopa_raw_value_data_t*)mul_exp->toRaw();

    if (left_value->kind.tag == KOOPA_RVT_INTEGER && right_value->kind.tag == KOOPA_RVT_INTEGER) {
        int left = left_value->kind.data.integer.value;
        int right = right_value->kind.data.integer.value;
        int result;
        if (op == '+') {
            result = left + right;
        } else if (op == '-') {
            result = left - right;
        } else {
            assert(false);
        }
        
        return build_number(result);
    }
    koopa_raw_binary_op bop;
    if (op == '+') {
        bop = KOOPA_RBO_ADD;
    } else if (op == '-') {
        bop = KOOPA_RBO_SUB;
    } else {
        assert(false);
    }
    auto add = build_binary(bop, left_value, right_value);
    append_value(add);
    return add;
}

void *RelExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (type == ADD)
        return add_exp->toRaw();
    assert(type == REL);

    auto left_value = (koopa_raw_value_data_t*)rel_exp->toRaw();
    auto right_value = (koopa_raw_value_data_t*)add_exp->toRaw();

    if (left_value->kind.tag == KOOPA_RVT_INTEGER && right_value->kind.tag == KOOPA_RVT_INTEGER) {
        int left = left_value->kind.data.integer.value;
        int right = right_value->kind.data.integer.value;
        int result;
        if (op == "<") {
            result = left < right;
        } else if (op == ">") {
            result = left > right;
        } else if (op == "<=") {
            result = left <= right;
        } else if (op == ">=") {
            result = left >= right;
        } else {
            assert(false);
        }
        return build_number(result);
    }
    koopa_raw_binary_op bop;
    if (op == "<") {
        bop = KOOPA_RBO_LT;
    } else if (op == ">") {
        bop = KOOPA_RBO_GT;
    } else if (op == "<=") {
        bop = KOOPA_RBO_LE;
    } else if (op == ">=") {
        bop = KOOPA_RBO_GE;
    } else {
        assert(false);
    }
    auto rel = build_binary(bop, left_value, right_value);
    append_value(rel);
    return rel;
}

void *EqExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (type == REL)
        return rel_exp->toRaw();
    assert(type == EQ);

    auto left_value = (koopa_raw_value_data_t*)eq_exp->toRaw();
    auto right_value = (koopa_raw_value_data_t*)rel_exp->toRaw();

    if (left_value->kind.tag == KOOPA_RVT_INTEGER && right_value->kind.tag == KOOPA_RVT_INTEGER) {
        int left = left_value->kind.data.integer.value;
        int right = right_value->kind.data.integer.value;
        int result;
        if (op == "==") {
            result = left == right;
        } else if (op == "!=") {
            result = left != right;
        } else {
            assert(false);
        }
        return build_number(result);
    }
    koopa_raw_binary_op bop;
    if (op == "==") {
        bop = KOOPA_RBO_EQ;
    } else if (op == "!=") {
        bop = KOOPA_RBO_NOT_EQ;
    } else {
        assert(false);
    }
    auto eq = build_binary(bop, left_value, right_value);
    append_value(eq);
    return eq;
}

void *LAndExpAST::toRaw(int n = 0, void* args[] = nullptr) const 
// (x && y) --> (x != 0) & (y != 0) (旧)
// lhs && rhs
//                                  
//                            /-- false -->  一个0
//                  /-- 能 --                                   /--  能   --> lhs && rhs
// lhs能否判断值？--           \-- true  -->  rhs能否判断值？ --
//                  \                                           \--  不能 --> ne rhs, 0
//                   \
//                    - 不能:
//                          ...(lhs)
//                      %:
//                          br lhs %rhs_entry %end(0)
//                      %rhs_entry:
//                          ...(rhs)
//                      %:
//                          %0 = ne rhs 0
//                          jmp %end(%0)
//                      %land_end(%value: i32):
//                          %value 即为表达式的值
{
    if (type == EQ)
        return eq_exp->toRaw();
    assert(type == LAND);

    auto left_value = (koopa_raw_value_data_t*)land_exp->toRaw();

    // lhs能判断值
    if (left_value->kind.tag == KOOPA_RVT_INTEGER) {
        int left = left_value->kind.data.integer.value;
        // lhs为false，放一个0
        if (left == 0) {
            return build_number(0);
        }
        // lhs为true
        auto right_value = (koopa_raw_value_data_t*)eq_exp->toRaw();
        // rhs能判断值，lhs && rhs
        if (right_value->kind.tag == KOOPA_RVT_INTEGER) {
            int right = right_value->kind.data.integer.value;
            int result = left && right;
            return build_number(result);
        }
        // rhs不能判断值，ne rhs, 0
        auto ne = build_binary(KOOPA_RBO_NOT_EQ, right_value, build_number(0));
        append_value(ne);
        return ne;
    }

    // lhs不能判断值
    // end_bb
    auto end_param = new koopa_raw_value_data_t({
        .ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_INT32,
        }),
        .name = "%value",
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .kind = {
            .tag = KOOPA_RVT_BLOCK_ARG_REF,
            .data = {
                .block_arg_ref = {
                    .index = 0,
                },
            },
        },
    });

    auto end_bb = new koopa_raw_basic_block_data_t({
        .name = "%land_end",
        .params = {
            .buffer = new const void* [1],
            .len = 1,
            .kind = KOOPA_RSIK_VALUE,
        },
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .insts = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
    });
    end_bb->params.buffer[0] = end_param;

    // br
    auto false_args = new koopa_raw_slice_t({
        .buffer = new const void*[1],
        .len = 1,
        .kind = KOOPA_RSIK_VALUE,
    });
    false_args->buffer[0] = build_number(0);

    auto rhs_entry = build_block_from_insts(nullptr, "%rhs_entry");

    auto branch = build_branch(left_value, rhs_entry, end_bb, nullptr, false_args);
    append_value(branch);

    append_bb(rhs_entry);

    auto right_value = (koopa_raw_value_data_t*)eq_exp->toRaw();

    // ne rhs, 0
    auto ne = build_binary(KOOPA_RBO_NOT_EQ, right_value, build_number(0));
    // jmp
    auto jmp_args = new koopa_raw_slice_t({
        .buffer = new const void*[1],
        .len = 1,
        .kind = KOOPA_RSIK_VALUE,
    });
    jmp_args->buffer[0] = ne;

    auto jmp = build_jump(end_bb, jmp_args);
    
    append_value(ne);
    append_value(jmp);

    append_bb(end_bb);
    return end_param;
}

void *LOrExpAST::toRaw(int n = 0, void* args[] = nullptr) const  
// (x || y) --> (x|y != 0) (旧)
// lhs || rhs
//                                  
//                            /-- true  -->  一个1
//                  /-- 能 --                                   /--  能   --> lhs || rhs
// lhs能否判断值？--           \-- false -->  rhs能否判断值？ --
//                  \                                           \--  不能 --> ne rhs, 0
//                   \
//                    - 不能:
//                          ...(lhs)
//                      %:
//                          br lhs %end(1) %rhs_entry
//                      %rhs_entry:
//                          ...(rhs)
//                      %:
//                          %0 = ne rhs 0
//                          jmp %end(%0)
//                      %lor_end(%value: i32):
//                          %value 即为表达式的值
{
    if (type == LAND)
        return land_exp->toRaw();
    assert(type == LOR);

    auto left_value = (koopa_raw_value_data_t*)lor_exp->toRaw();

    // lhs能判断值
    if (left_value->kind.tag == KOOPA_RVT_INTEGER) {
        int left = left_value->kind.data.integer.value;
        // lhs为true，放一个1
        if (left != 0) {
            return build_number(1);
        }
        // lhs为false
        auto right_value = (koopa_raw_value_data_t*)land_exp->toRaw();
        // rhs能判断值，lhs || rhs
        if (right_value->kind.tag == KOOPA_RVT_INTEGER) {
            int right = right_value->kind.data.integer.value;
            int result = left || right;
            return build_number(result);
        }
        // rhs不能判断值，ne rhs, 0
        auto ne = build_binary(KOOPA_RBO_NOT_EQ, right_value, build_number(0));
        append_value(ne);
        return ne;
    }

    // lhs不能判断值
    // end_bb
    auto end_param = new koopa_raw_value_data_t({
        .ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_INT32,
        }),
        .name = "%value",
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .kind = {
            .tag = KOOPA_RVT_BLOCK_ARG_REF,
            .data = {
                .block_arg_ref = {
                    .index = 0,
                },
            },
        },
    });

    auto end_bb = new koopa_raw_basic_block_data_t({
        .name = "%lor_end",
        .params = {
            .buffer = new const void* [1],
            .len = 1,
            .kind = KOOPA_RSIK_VALUE,
        },
        .used_by = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
        .insts = {
            .buffer = nullptr,
            .len = 0,
            .kind = KOOPA_RSIK_VALUE,
        },
    });
    end_bb->params.buffer[0] = end_param;

    // br
    auto true_args = new koopa_raw_slice_t({
        .buffer = new const void*[1],
        .len = 1,
        .kind = KOOPA_RSIK_VALUE,
    });
    true_args->buffer[0] = build_number(1);

    auto rhs_entry = build_block_from_insts(nullptr, "%rhs_entry");

    auto branch = build_branch(left_value, end_bb, rhs_entry, true_args, nullptr);
    append_value(branch);

    append_bb(rhs_entry);

    auto right_value = (koopa_raw_value_data_t*)land_exp->toRaw();

    // ne rhs, 0
    auto ne = build_binary(KOOPA_RBO_NOT_EQ, right_value, build_number(0));

    // jmp
    auto jmp_args = new koopa_raw_slice_t({
        .buffer = new const void*[1],
        .len = 1,
        .kind = KOOPA_RSIK_VALUE,
    });
    jmp_args->buffer[0] = ne;

    auto jmp = build_jump(end_bb, jmp_args);

    append_value(ne);
    append_value(jmp);

    append_bb(end_bb);
    return end_param;
}

void *DeclAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    switch (type)
    {
    case CONSTDECL:
        const_decl->toRaw(n, args);
        break;
    case VARDECL:
        var_decl->toRaw(n, args);
        break;
    default:
        assert(false);
    }
    return nullptr;
}

void *ConstDeclAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    for (auto &i : *const_def_list) {
        i->toRaw(n, args);
    }
    return nullptr;
}

void *VarDeclAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    for (auto &i : *var_def_list) {
        i->toRaw(n, args);
    }
    return nullptr;
}

void *ConstDefAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    // const_init_val的toRaw()填充args[1]的result_vec
    auto dim_vec = new std::vector<int>();
    if (dim_list != nullptr)
        for (auto i = dim_list->rbegin(); i != dim_list->rend(); i++) {
            dim_vec->push_back((long)(*i)->toRaw());
        }
    auto result_vec = new std::vector<int>();
    const_init_val->toRaw(n, new void*[2]{dim_vec, result_vec});
    if (dim_list == nullptr) { // 不是数组
        int val = result_vec->front();
        Symbol::insert(ident, Symbol::TYPE_CONST, val);
        return nullptr;
    // 以下为数组
    } else if (n == 1) { // global def
        auto global_alloc = new koopa_raw_value_data_t({
            .name = build_ident(ident, '@'),
            .used_by = {
                .buffer = nullptr,
                .len = 0,
                .kind = KOOPA_RSIK_VALUE,
            },
            .kind = {
                .tag = KOOPA_RVT_GLOBAL_ALLOC,
                .data = {
                    .global_alloc = {
                        .init = build_aggregate(dim_vec, result_vec),
                    },
                },
            },
        });
        auto ty_integer = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_INT32,
        });
        koopa_raw_type_kind_t* temp_ty = ty_integer;
        for (auto i : *dim_vec) {
            auto ty_array = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_ARRAY,
                .data = {
                    .array = {
                        .base = temp_ty,
                        .len = (unsigned long)i,
                    },
                },
            });
            temp_ty = ty_array;
        }
        global_alloc->ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_POINTER,
            .data = {
                .pointer = {
                    .base = temp_ty,
                },
            },
        });
        Symbol::insert(ident, Symbol::TYPE_ARRAY, global_alloc, dim_vec);
        global_values.push_back(global_alloc);
        return nullptr;

    } else { // local def，不能用aggregate（其实这里倒可以用，不过用了到riscv还要坐牢，先改一下吧）
        auto alloc = new koopa_raw_value_data_t({
            .name = build_ident(ident, '@'),
            .used_by = {
                .buffer = nullptr,
                .len = 0,
                .kind = KOOPA_RSIK_VALUE,
            },
            .kind = {
                .tag = KOOPA_RVT_ALLOC,
            },
        });
        auto ty_integer = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_INT32,
        });
        koopa_raw_type_kind_t* temp_ty = ty_integer;
        for (auto i : *dim_vec) {
            auto ty_array = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_ARRAY,
                .data = {
                    .array = {
                        .base = temp_ty,
                        .len = (unsigned long)i,
                    },
                },
            });
            temp_ty = ty_array;
        }
        alloc->ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_POINTER,
            .data = {
                .pointer = {
                    .base = temp_ty,
                },
            },
        });
        Symbol::insert(ident, Symbol::TYPE_ARRAY, alloc, dim_vec);
        append_value(alloc);
        store2array(alloc, result_vec, dim_vec);
        return nullptr;
    }
}

void *VarDefAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto dim_vec = new std::vector<int>();
    if (dim_list != nullptr)
        for (auto i = dim_list->rbegin(); i != dim_list->rend(); i++) {
            dim_vec->push_back((long)(*i)->toRaw());
        }
    auto result_vec = new std::vector<koopa_raw_value_data_t*>();
    if (has_init_val)
        init_val->toRaw(n, new void*[2]{dim_vec, result_vec});
    if (dim_list != nullptr) { // 是数组
        if (!has_init_val) {
            int len = 1;
            for (auto i : *dim_vec) {
                len *= i;
            }
            for (int i = 0; i < len; i++) {
                result_vec->push_back(build_number(0));
            }
        }
        if (n == 1) { // global def
            auto global_alloc = new koopa_raw_value_data_t({
                .name = build_ident(ident, '@'),
                .used_by = {
                    .buffer = nullptr,
                    .len = 0,
                    .kind = KOOPA_RSIK_VALUE,
                },
                .kind = {
                    .tag = KOOPA_RVT_GLOBAL_ALLOC,
                    .data = {
                        .global_alloc = {
                            .init = build_aggregate(dim_vec, result_vec),
                        }
                    }
                }
            });
            auto ty_integer = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_INT32,
            });
            koopa_raw_type_kind_t* temp_ty = ty_integer;
            for (auto i : *dim_vec) {
                auto ty_array = new koopa_raw_type_kind_t({
                    .tag = KOOPA_RTT_ARRAY,
                    .data = {
                        .array = {
                            .base = temp_ty,
                            .len = (unsigned long)i,
                        },
                    },
                });
                temp_ty = ty_array;
            }
            global_alloc->ty = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_POINTER,
                .data = {
                    .pointer = {
                        .base = temp_ty,
                    },
                },
            });
            global_values.push_back(global_alloc);
            Symbol::insert(ident, Symbol::TYPE_ARRAY, global_alloc, dim_vec);
            return nullptr;
        }
        else { // local array
            auto alloc = new koopa_raw_value_data_t({
                .name = build_ident(ident, '@'),
                .used_by = {
                    .buffer = nullptr,
                    .len = 0,
                    .kind = KOOPA_RSIK_VALUE,
                },
                .kind = {
                    .tag = KOOPA_RVT_ALLOC,
                },
            });
            auto ty_integer = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_INT32,
            });
            koopa_raw_type_kind_t* temp_ty = ty_integer;
            for (auto i : *dim_vec) {
                auto ty_array = new koopa_raw_type_kind_t({
                    .tag = KOOPA_RTT_ARRAY,
                    .data = {
                        .array = {
                            .base = temp_ty,
                            .len = (unsigned long)i,
                        },
                    },
                });
                temp_ty = ty_array;
            }
            alloc->ty = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_POINTER,
                .data = {
                    .pointer = {
                        .base = temp_ty,
                    },
                },
            });
            Symbol::insert(ident, Symbol::TYPE_ARRAY, alloc, dim_vec);
            append_value(alloc);
            store2array(alloc, result_vec, dim_vec);
            return nullptr;
        }
    }
    // 不是数组
    if (n == 1) {
        // global def
        auto global_alloc = new koopa_raw_value_data_t({
            .ty = new koopa_raw_type_kind_t({
                .tag = KOOPA_RTT_POINTER,
                .data = {
                    .pointer = {
                        .base = new koopa_raw_type_kind_t({
                            .tag = KOOPA_RTT_INT32,
                        }),
                    },
                },
            }),
            .name = build_ident(ident, '@'),
            .used_by = {
                .buffer = nullptr,
                .len = 0,
                .kind = KOOPA_RSIK_VALUE,
            },
            .kind = {
                .tag = KOOPA_RVT_GLOBAL_ALLOC,
            },
        });
        if (has_init_val) {
            global_alloc->kind.data.global_alloc.init = (koopa_raw_value_data_t*)result_vec->front();
        }
        else {
            global_alloc->kind.data.global_alloc.init = new koopa_raw_value_data_t({
                .ty = new koopa_raw_type_kind_t({
                    .tag = KOOPA_RTT_INT32,
                }),
                .name = nullptr,
                .used_by = {
                    .buffer = nullptr,
                    .len = 0,
                    .kind = KOOPA_RSIK_VALUE,
                },
                .kind = {
                    .tag = KOOPA_RVT_ZERO_INIT,
                },
            });
        }
        global_values.push_back(global_alloc);
        Symbol::insert(ident, Symbol::TYPE_VAR, global_alloc);
        return nullptr;
    }

    // local def
    // alloc
    auto alloc = build_alloc(build_ident(ident, '%'));
    Symbol::insert(ident, Symbol::TYPE_VAR, alloc);
    append_value(alloc);

    // store
    if (has_init_val) {
        auto value = (koopa_raw_value_data_t*)result_vec->front();
        auto store = build_store(value, alloc);
        append_value(store);
    }
    return nullptr;
}

void *ConstInitValAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto dim_vec = (std::vector<int>*)args[0];
    auto result_vec = (std::vector<int>*)args[1];
    if (!is_list) {
        result_vec->push_back((long)const_exp->toRaw());
    } else {
        int len = result_vec->size();
        assert(!(len % dim_vec->front()));
        int alignment = 1;
        for (auto i : *dim_vec) {
            if (len % (alignment * i))
                break;
            alignment *= i;
        }
        auto new_dim_vec = new std::vector<int>(*dim_vec);
        new_dim_vec->pop_back();
        for (auto& i : *const_init_val_list) {
            i->toRaw(n, new void*[2]{new_dim_vec, result_vec});
        }
        for (; (result_vec->size() % alignment) || (result_vec->size() == 0) ; result_vec->push_back(0));
    }
    return result_vec;
}

void *InitValAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto dim_vec = (std::vector<int>*)args[0];
    auto result_vec = (std::vector<koopa_raw_value_data_t*>*)args[1];
    // void* -> koopa_raw_value_data_t*
    if (!is_list) {
        result_vec->push_back((koopa_raw_value_data_t*)exp->toRaw());
    } else {
        int len = result_vec->size();
        assert(!(len % dim_vec->front()));
        int alignment = 1;
        for (auto i : *dim_vec) {
            if (len % (alignment * i))
                break;
            alignment *= i;
        }
        auto new_dim_vec = new std::vector<int>(*dim_vec);
        new_dim_vec->pop_back();
        for (auto& i : *init_val_list) {
            i->toRaw(n, new void*[2]{new_dim_vec, result_vec});
        }
        for (; result_vec->size() % alignment || (!result_vec->size()); result_vec->push_back(build_number(0)));}
    return result_vec;
}

void *LValAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto sym = Symbol::query(ident);
    switch (sym.type)
    {
        case Symbol::TYPE_POINTER:
        {
            if (index_list == nullptr) {
                return sym.allocator;
            }
            auto load = new koopa_raw_value_data_t({
                .ty = sym.allocator->ty->data.pointer.base,
                .name = nullptr,
                .used_by = {
                    .buffer = nullptr,
                    .len = 0,
                    .kind = KOOPA_RSIK_VALUE,
                },
                .kind = {
                    .tag = KOOPA_RVT_LOAD,
                    .data = {
                        .load = {
                            .src = sym.allocator,
                        },
                    },
                },
            });
            append_value(load);
            auto get_ptr = new koopa_raw_value_data_t({
                .ty = load->ty,
                .name = nullptr,
                .used_by = {
                    .buffer = nullptr,
                    .len = 0,
                    .kind = KOOPA_RSIK_VALUE,
                },
                .kind = {
                    .tag = KOOPA_RVT_GET_PTR,
                    .data = {
                        .get_ptr = {
                            .src = load,
                            .index = (koopa_raw_value_data_t*)index_list->front()->toRaw(),
                        },
                    },
                },
            });
            append_value(get_ptr);
            index_list->erase(index_list->begin());
            koopa_raw_value_data_t* temp_p = get_ptr;
            for (auto &i : *index_list) {
                auto get_elem_ptr = new koopa_raw_value_data_t({
                    .ty = new koopa_raw_type_kind_t({
                        .tag = KOOPA_RTT_POINTER,
                        .data = {
                            .pointer = {
                                .base = temp_p->ty->data.pointer.base->data.array.base,
                            },
                        },
                    }),
                    .name = nullptr,
                    .used_by = {
                        .buffer = nullptr,
                        .len = 0,
                        .kind = KOOPA_RSIK_VALUE,
                    },
                    .kind = {
                        .tag = KOOPA_RVT_GET_ELEM_PTR,
                        .data = {
                            .get_elem_ptr = {
                                .src = temp_p,
                                .index = (koopa_raw_value_data_t*)i->toRaw(),
                            },
                        },
                    },
                });
                temp_p = get_elem_ptr;
                append_value(temp_p);
            }
            return temp_p;
        }
        case Symbol::TYPE_ARRAY:
        {
            if (index_list == nullptr) {
                return sym.allocator;
            }
            auto temp_p = sym.allocator;
            for (auto &i : *index_list) {
                auto get_elem_ptr = new koopa_raw_value_data_t({
                    .ty = new koopa_raw_type_kind_t({
                        .tag = KOOPA_RTT_POINTER,
                        .data = {
                            .pointer = {
                                .base = temp_p->ty->data.pointer.base->data.array.base,
                            },
                        },
                    }),
                    .name = nullptr,
                    .used_by = {
                        .buffer = nullptr,
                        .len = 0,
                        .kind = KOOPA_RSIK_VALUE,
                    },
                    .kind = {
                        .tag = KOOPA_RVT_GET_ELEM_PTR,
                        .data = {
                            .get_elem_ptr = {
                                .src = temp_p,
                                .index = (koopa_raw_value_data_t*)i->toRaw(),
                            },
                        },
                    },
                });
                temp_p = get_elem_ptr;
                append_value(temp_p);
            }
            return temp_p;
        }
        case Symbol::TYPE_VAR:
        {
            return sym.allocator;
        }
        case Symbol::TYPE_CONST:
        {
            return build_number(sym.int_value);
        }
    default:
        assert(false);
    }
}