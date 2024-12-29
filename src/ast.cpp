#include "ast.h"
#include "symtab.h"

std::vector<koopa_raw_basic_block_data_t*> current_bbs;
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
        auto sub_vector = new std::vector<int>();
        for (int j = i * sub_len; j < (i + 1) * sub_len; j++) {
            sub_vector->push_back(result_vec->at(j));
        }
        raw_aggregate->kind.data.aggregate.elems.buffer[i] = build_aggregate(sub_dim_vec, sub_vector);
    }
        
    return raw_aggregate;
}

koopa_raw_value_data_t* BaseAST::build_aggregate(std::vector<int>* dim_vec, std::vector<void*>* result_vec)
{
    auto int_vec = new std::vector<int>();
    for (auto i : *result_vec) {
        auto inst = (koopa_raw_value_data_t*)i;
        assert(inst->kind.tag == KOOPA_RVT_INTEGER);
        int_vec->push_back(inst->kind.data.integer.value);
    }
    return build_aggregate(dim_vec, int_vec);
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
        ty->data.function.params.buffer[i] = f_param->ty;
        raw_function->params.buffer[i] = f_param;
        auto name = build_ident(f_param->name + 1, '%');
        auto raw_alloc = build_alloc(name);
        var_decl_insts->push_back(raw_alloc);
        var_decl_insts->push_back(build_store(f_param, raw_alloc));
        Symbol::insert(f_param->name + 1, Symbol::TYPE_VAR, raw_alloc);
    }
    raw_function->ty = ty;
    raw_function->name = build_ident(ident, '@');
    current_bbs.clear();
    block->toRaw(-1);
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
    auto raw = new koopa_raw_value_data_t;
    raw->name = build_ident(ident, '@');
    auto ty = new koopa_raw_type_kind_t;
    if (type == "int")
        ty->tag = KOOPA_RTT_INT32;
    else assert(false);
    raw->ty = ty;
    raw->kind.tag = KOOPA_RVT_FUNC_ARG_REF;
    raw->kind.data.func_arg_ref.index = n;
    raw->used_by = {
        .buffer = nullptr,
        .len = 0,
        .kind = KOOPA_RSIK_VALUE,
    };
    return raw;
}

void *BlockAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto raw_basic_block = new koopa_raw_basic_block_data_t;
    if (n == -1) {
        raw_basic_block->name = "%entry";
        n = 0;
    }
    else {
        raw_basic_block->name = nullptr;
    }
    raw_basic_block->params = {
        .buffer = nullptr,
        .len = 0,
        .kind = KOOPA_RSIK_VALUE,
    };
    raw_basic_block->used_by = {
        .buffer = nullptr,
        .len = 0,
        .kind = KOOPA_RSIK_VALUE,
    };
    raw_basic_block->insts = {
        .buffer = nullptr,
        .len = 0,
        .kind = KOOPA_RSIK_VALUE,
    };

    current_bbs.push_back(raw_basic_block);
    for (auto& block_item : *block_item_list) {
        block_item->toRaw(n, args);
    }
    return raw_basic_block;
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
            auto entry_bb = build_block_from_insts(nullptr);
            current_bbs.push_back(entry_bb);
            auto cond = (koopa_raw_value_data_t*)exp->toRaw();
            auto branch = build_branch(cond, nullptr, nullptr);
            auto branch_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({branch}));
            current_bbs.push_back(branch_bb);
            auto true_entry_bb = (koopa_raw_basic_block_data_t*)stmt->toRaw(n, args);
            true_entry_bb->name = "%if_body";
            branch->kind.data.branch.true_bb = true_entry_bb;
            auto true_jmp = build_jump(nullptr);
            auto true_jmp_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({true_jmp}));
            current_bbs.push_back(true_jmp_bb);
            auto end_bb = build_block_from_insts(nullptr, "%if_end");
            current_bbs.push_back(end_bb);
            branch->kind.data.branch.false_bb = end_bb;
            true_jmp->kind.data.jump.target = end_bb;
            return entry_bb;
        }
        case IF_ELSE:
        {
            auto entry_bb = build_block_from_insts(nullptr);
            current_bbs.push_back(entry_bb);
            auto cond = (koopa_raw_value_data_t*)exp->toRaw();
            auto branch = build_branch(cond, nullptr, nullptr);
            auto branch_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({branch}));
            current_bbs.push_back(branch_bb);
            auto true_entry_bb = (koopa_raw_basic_block_data_t*)closed_stmt->toRaw(n, args);
            true_entry_bb->name = "%if_true";
            branch->kind.data.branch.true_bb = true_entry_bb;
            auto true_jmp = build_jump(nullptr);
            auto true_jmp_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({true_jmp}));
            current_bbs.push_back(true_jmp_bb);
            auto false_entry_bb = (koopa_raw_basic_block_data_t*)open_stmt->toRaw(n, args);
            false_entry_bb->name = "%if_false";
            branch->kind.data.branch.false_bb = false_entry_bb;
            auto false_jmp = build_jump(nullptr);
            auto false_jmp_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({false_jmp}));
            current_bbs.push_back(false_jmp_bb);
            auto end_bb = build_block_from_insts(nullptr, "%if_end");
            current_bbs.push_back(end_bb);
            true_jmp->kind.data.jump.target = end_bb;
            false_jmp->kind.data.jump.target = end_bb;
            return entry_bb;
        }
        case WHILE:
        {
            auto entry_bb = build_block_from_insts(nullptr, "%while_entry");
            auto init_jmp = build_jump(entry_bb);
            current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({init_jmp})));
            auto end_bb = build_block_from_insts(nullptr, "%while_end");
            Symbol::enter_loop(entry_bb, end_bb);
            current_bbs.push_back(entry_bb);
            auto cond = (koopa_raw_value_data_t*)exp->toRaw();
            auto branch = build_branch(cond, nullptr, end_bb);
            auto branch_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({branch}));
            current_bbs.push_back(branch_bb);
            auto stmt_bb = (koopa_raw_basic_block_data_t*)stmt->toRaw(n, args);
            stmt_bb->name = "while_body";
            branch->kind.data.branch.true_bb = stmt_bb;
            auto jmp = build_jump(entry_bb);
            auto jmp_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({jmp}));
            current_bbs.push_back(jmp_bb);
            current_bbs.push_back(end_bb);
            Symbol::leave_loop();
            return entry_bb;
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
            auto entry_bb = build_block_from_insts(nullptr);
            current_bbs.push_back(entry_bb);
            auto cond = (koopa_raw_value_data_t*)exp->toRaw();
            auto branch = build_branch(cond, nullptr, nullptr);
            auto branch_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({branch}));
            current_bbs.push_back(branch_bb);
            auto true_entry_bb = (koopa_raw_basic_block_data_t*)closed_stmt->toRaw(n, args);
            true_entry_bb->name = "%if_true";
            branch->kind.data.branch.true_bb = true_entry_bb;
            auto true_jmp = build_jump(nullptr);
            auto true_jmp_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({true_jmp}));
            current_bbs.push_back(true_jmp_bb);
            auto false_entry_bb = (koopa_raw_basic_block_data_t*)closed_stmt2->toRaw(n, args);
            false_entry_bb->name = "%if_false";
            branch->kind.data.branch.false_bb = false_entry_bb;
            auto false_jmp = build_jump(nullptr);
            auto false_jmp_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({false_jmp}));
            current_bbs.push_back(false_jmp_bb);
            auto end_bb = build_block_from_insts(nullptr, "%if_end");
            current_bbs.push_back(end_bb);
            true_jmp->kind.data.jump.target = end_bb;
            false_jmp->kind.data.jump.target = end_bb;
            return entry_bb;
        }
        case WHILE:
        {
            auto entry_bb = build_block_from_insts(nullptr, "%while_entry");
            auto init_jmp = build_jump(entry_bb);
            current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({init_jmp})));
            auto end_bb = build_block_from_insts(nullptr, "%while_end");
            Symbol::enter_loop(entry_bb, end_bb);
            current_bbs.push_back(entry_bb);
            auto cond = (koopa_raw_value_data_t*)exp->toRaw();
            auto branch = build_branch(cond, nullptr, end_bb);
            auto branch_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({branch}));
            current_bbs.push_back(branch_bb);
            auto stmt_bb = (koopa_raw_basic_block_data_t*)closed_stmt->toRaw(n, args);
            stmt_bb->name = "while_body";
            branch->kind.data.branch.true_bb = stmt_bb;
            auto jmp = build_jump(entry_bb);
            auto jmp_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({jmp}));
            current_bbs.push_back(jmp_bb);
            current_bbs.push_back(end_bb);
            Symbol::leave_loop();
            return entry_bb;
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
            auto bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_store}));
            current_bbs.push_back(bb);
            return bb;
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
            auto bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_return}));
            current_bbs.push_back(bb);
            return bb;
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
            auto bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_return}));
            current_bbs.push_back(bb);
            return bb;
        }
        case EXP:
        {
            exp->toRaw();
            return build_block_from_insts(nullptr);
        }
        case EMPTY:
        {
            return build_block_from_insts(nullptr);
        }
        case BLOCK:
        {
            Symbol::enter_scope();
            auto bb = block->toRaw(n, args);
            Symbol::leave_scope();
            return bb;
        }
        case BREAK:
        {
            koopa_raw_basic_block_data_t* target = Symbol::get_loop_exit();
            assert(target != nullptr);
            auto raw_jmp = build_jump(target);
            auto bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_jmp}));
            current_bbs.push_back(bb);
            return bb;
        }
        case CONTINUE:
        {
            koopa_raw_basic_block_data_t* target = Symbol::get_loop_header();
            assert(target != nullptr);
            auto raw_jmp = build_jump(target);
            auto bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_jmp}));
            current_bbs.push_back(bb);
            return bb;
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
            case KOOPA_RVT_GET_ELEM_PTR:
            {
                auto load = new koopa_raw_value_data_t({
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
                        .tag = KOOPA_RVT_LOAD,
                        .data = {
                            .load = {
                                .src = value,
                            },
                        },
                    },
                });
                current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({load})));
                return load;
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

        auto unary = new koopa_raw_value_data_t({
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
                        .lhs = build_number(0, nullptr),
                        .rhs = value,
                    },
                },
            },
        });

        switch (unaryop) {
        case '-':
            unary->kind.data.binary.op = KOOPA_RBO_SUB;
            break;
        case '!':
            unary->kind.data.binary.op = KOOPA_RBO_EQ;
            break;
        default:
            assert(false);
        }
        current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({unary})));
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
        current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({call})));
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

    auto mul = new koopa_raw_value_data_t({
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
                    .lhs = left_value,
                    .rhs = right_value,
                },
            },
        },
    });

    switch (op)
    {
    case '*':
        mul->kind.data.binary.op = KOOPA_RBO_MUL;
        break;
    case '/':
        mul->kind.data.binary.op = KOOPA_RBO_DIV;
        break;
    case '%':
        mul->kind.data.binary.op = KOOPA_RBO_MOD;
        break;
    default:
        assert(false);
    }
    current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({mul})));
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

    auto add = new koopa_raw_value_data_t({
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
                    .lhs = left_value,
                    .rhs = right_value,
                },
            },
        },
    });

    if (op == '+') {
        add->kind.data.binary.op = KOOPA_RBO_ADD;
    } else if (op == '-') {
        add->kind.data.binary.op = KOOPA_RBO_SUB;
    } else {
        assert(false);
    }
    current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({add})));
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

    auto rel = new koopa_raw_value_data_t({
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
                    .lhs = left_value,
                    .rhs = right_value,
                },
            },
        },
    });

    if (op == "<") {
        rel->kind.data.binary.op = KOOPA_RBO_LT;
    } else if (op == ">") {
        rel->kind.data.binary.op = KOOPA_RBO_GT;
    } else if (op == "<=") {
        rel->kind.data.binary.op = KOOPA_RBO_LE;
    } else if (op == ">=") {
        rel->kind.data.binary.op = KOOPA_RBO_GE;
    } else {
        assert(false);
    }
    current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({rel})));
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

    auto eq = new koopa_raw_value_data_t({
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
                    .lhs = left_value,
                    .rhs = right_value,
                },
            },
        },
    });

    if (op == "==") {
        eq->kind.data.binary.op = KOOPA_RBO_EQ;
    } else if (op == "!=") {
        eq->kind.data.binary.op = KOOPA_RBO_NOT_EQ;
    } else {
        assert(false);
    }
    current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({eq})));
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
        auto ne = new koopa_raw_value_data_t({
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
                        .op = KOOPA_RBO_NOT_EQ,
                        .lhs = right_value,
                        .rhs = build_number(0),
                    },
                },
            },
        });
        current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({ne})));
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
    current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({branch})));

    current_bbs.push_back(rhs_entry);

    auto right_value = (koopa_raw_value_data_t*)eq_exp->toRaw();

    // ne  
    auto ne = new koopa_raw_value_data_t({
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
                    .op = KOOPA_RBO_NOT_EQ,
                    .lhs = right_value,
                    .rhs = build_number(0),
                },
            },
        },
    });
    // jmp
    auto jmp_args = new koopa_raw_slice_t({
        .buffer = new const void*[1],
        .len = 1,
        .kind = KOOPA_RSIK_VALUE,
    });
    jmp_args->buffer[0] = ne;

    auto jmp = build_jump(end_bb, jmp_args);
    
    auto insts = new std::vector<koopa_raw_value_data_t*>({ne, jmp});
    current_bbs.push_back(build_block_from_insts(insts));

    current_bbs.push_back(end_bb);
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
        auto ne = new koopa_raw_value_data_t({
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
                        .op = KOOPA_RBO_NOT_EQ,
                        .lhs = right_value,
                        .rhs = build_number(0),
                    },
                },
            },
        });
        current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({ne})));
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
    current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({branch})));

    current_bbs.push_back(rhs_entry);

    auto right_value = (koopa_raw_value_data_t*)land_exp->toRaw();

    // ne
    auto ne = new koopa_raw_value_data_t({
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
                    .op = KOOPA_RBO_NOT_EQ,
                    .lhs = right_value,
                    .rhs = build_number(0),
                },
            },
        },
    });

    // jmp
    auto jmp_args = new koopa_raw_slice_t({
        .buffer = new const void*[1],
        .len = 1,
        .kind = KOOPA_RSIK_VALUE,
    });
    jmp_args->buffer[0] = ne;

    auto jmp = build_jump(end_bb, jmp_args);

    auto insts = new std::vector<koopa_raw_value_data_t*>({ne, jmp});
    current_bbs.push_back(build_block_from_insts(insts));

    current_bbs.push_back(end_bb);
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
    // const_init_val的toRaw()返回的是一个std::vector<int>*(或者说，把传入的result_vec填充好了)
    auto dim_vec = new std::vector<int>();
    if (dim_list != nullptr)
        for (auto i = dim_list->rbegin(); i != dim_list->rend(); i++) {
            dim_vec->push_back((long)(*i)->toRaw());
        }
    auto result_vec = new std::vector<int>();
    const_init_val->toRaw(n, new void*[2]{dim_vec, result_vec});
    if (dim_list == nullptr) {
        int val = result_vec->front();
        Symbol::insert(ident, Symbol::TYPE_CONST, val);
        return nullptr;
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
        Symbol::insert(ident, Symbol::TYPE_CONST, global_alloc);
        global_values.push_back(global_alloc);
        return nullptr;

    } else {
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
        Symbol::insert(ident, Symbol::TYPE_CONST, alloc);
        auto aggregate = build_aggregate(dim_vec, result_vec);
        auto store = build_store(aggregate, alloc);
        current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({alloc, store})));
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
    auto result_vec = new std::vector<void*>();
    if (has_init_val)
        init_val->toRaw(n, new void*[2]{dim_vec, result_vec});
    if (dim_list != nullptr) {
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
            Symbol::insert(ident, Symbol::TYPE_CONST, global_alloc);
            return nullptr;
        }
        else {
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
            Symbol::insert(ident, Symbol::TYPE_CONST, alloc);
            auto aggregate = build_aggregate(dim_vec, result_vec);
            auto raw_store = build_store(aggregate, alloc);
            current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({alloc, raw_store})));
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
    current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({alloc})));

    // store
    if (has_init_val) {
        auto value = (koopa_raw_value_data_t*)result_vec->front();
        auto raw_store = build_store(value, alloc);
        current_bbs.push_back(build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_store})));
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
        for (auto& i : *const_init_val_list) {
            i->toRaw(n, new void*[2]{dim_vec, result_vec});
        }
        for (; (result_vec->size() % alignment) || (!result_vec->size()) ; result_vec->push_back(0));
    }
    return result_vec;
}

void *InitValAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto dim_vec = (std::vector<int>*)args[0];
    auto result_vec = (std::vector<void*>*)args[1];
    // void* -> std::vector<koopa_raw_basic_block_data_t*>*
    if (!is_list) {
        result_vec->push_back(exp->toRaw());
    } else {
        int len = result_vec->size();
        assert(!(len % dim_vec->front()));
        int alignment = 1;
        for (auto i : *dim_vec) {
            if (len % (alignment * i))
                break;
            alignment *= i;
        }
        for (auto& i : *init_val_list) {
            i->toRaw(n, new void*[2]{dim_vec, result_vec});
        }
        for (; result_vec->size() % alignment || (!result_vec->size()); result_vec->push_back(build_number(0)));}
    return result_vec;
}

void *LValAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (index_list != nullptr) { // IDENT[Exp]

    }
    auto sym = Symbol::query(ident);
    if (sym.type == Symbol::TYPE_CONST) {
        return build_number(sym.int_value);
    }
    return sym.allocator;
}