#include "ast.h"
#include "symtab.h"

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
            .data.integer.value = number,
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

// 其实不需要写used_by，这个函数的作用实际上是填充used_by字段中的kind，否则会出现类型错误
// 后期可以考虑删除该函数，每个value初始化的时候就填写好used_by字段
void BaseAST::set_used_by(koopa_raw_value_data_t* value, koopa_raw_value_data_t* user=nullptr)
{
    value->used_by = {
        .buffer = nullptr,
        .len = 0,
        .kind = KOOPA_RSIK_VALUE,
    };
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
            .data.jump = {
                .target = target,
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
            .data.branch = {
                .cond = cond,
                .true_bb = true_bb,
                .false_bb = false_bb,
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

// void BaseAST::append_value(std::vector<koopa_raw_basic_block_data_t*>* bbs, koopa_raw_value_data_t* value)
// {
//     auto end_bb = bbs->back();
//     auto end_value = (koopa_raw_value_data_t*)end_bb->insts.buffer[end_bb->insts.len-1];
//     if (end_value->kind.tag == KOOPA_RVT_INTEGER || end_value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF) {
//         end_bb->insts.buffer[end_bb->insts.len-1] = value;
//         return;
//     }
//     end_bb->insts.len += 1;
//     auto buffer = new const void*[end_bb->insts.len];
//     for (int i = 0; i < end_bb->insts.len - 1; i++) {
//         buffer[i] = end_bb->insts.buffer[i];
//     }
//     buffer[end_bb->insts.len-1] = value;
//     end_bb->insts.buffer = buffer;
// }

koopa_raw_value_data_t* BaseAST::build_alloc(const char* name)
{
    auto raw_alloc = new koopa_raw_value_data_t({
        .ty = new koopa_raw_type_kind_t({
            .tag = KOOPA_RTT_POINTER,
            .data.pointer = {
                .base = new koopa_raw_type_kind_t({.tag = KOOPA_RTT_INT32}),
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
            .data.store = {
                .value = value,
                .dest = dest,
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
        .data.function = {
            .params = {
                .buffer = new const void*[params.size()],
                .len = (unsigned int)params.size(),
                .kind = KOOPA_RSIK_TYPE,
            },
            .ret = ret_ty,
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
                .data.pointer = {
                    .base = new koopa_raw_type_kind_t({
                        .tag = KOOPA_RTT_INT32
                    }),
                },
            });
        }
        else assert(false);
        ty->data.function.params.buffer[i] = raw_param_ty;
    }

    raw_function->ty = ty;
    
    return raw_function;
}

void *CompUnitAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto raw_program = new koopa_raw_program_t;

    std::vector<koopa_raw_function_data_t*> funcs;
    std::vector<koopa_raw_value_data_t*> values;

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
        auto def = dynamic_cast<GlobalDefAST*>(global_def.get());
        switch (def->type) {
        case GlobalDefAST::FUNC_DEF:
        {
            auto func_def = (koopa_raw_function_data_t*)global_def->toRaw();
            funcs.push_back(func_def);
            break;
        }
        case GlobalDefAST::DECL:
        {
            auto value_list = (std::vector<koopa_raw_value_data*>*)global_def->toRaw();
            for (auto value : *value_list) {
                values.push_back(value);
            }
            break;
        }
        default:
            assert(false);
        }
    }

    raw_program->values.len = values.size();
    raw_program->values.buffer = new const void*[raw_program->values.len];
    raw_program->values.kind = KOOPA_RSIK_VALUE;
    for (int i = 0; i < values.size(); i++) {
        raw_program->values.buffer[i] = values[i];
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
            return decl->toRaw(1);
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
    auto vec_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)block->toRaw(-1);
    raw_function->bbs.len = vec_bbs->size();
    raw_function->bbs.buffer = new const void*[raw_function->bbs.len];
    raw_function->bbs.kind = KOOPA_RSIK_BASIC_BLOCK;
    for (int i = 0; i < vec_bbs->size(); i++) {
        filter_basic_block(vec_bbs->at(i));
        char *bb_name = new char[strlen(vec_bbs->at(i)->name) + ident.length() + 2];
        memset(bb_name, 0, strlen(vec_bbs->at(i)->name) + ident.length() + 2);
        bb_name[0] = '%';
        strcat(bb_name, ident.c_str());
        strcat(bb_name, "_");
        strcat(bb_name, vec_bbs->at(i)->name + 1);
        vec_bbs->at(i)->name = bb_name;
        raw_function->bbs.buffer[i] = vec_bbs->at(i);
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
    set_used_by(raw, nullptr);
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

    auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
    bbs_vec->push_back(raw_basic_block);
    for (auto& block_item : *block_item_list) {
        auto block_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)block_item->toRaw(n, args);
        for (auto block_bb : *block_bbs_vec) {
            if (block_bb->insts.len == 0 && block_bb->name == nullptr) {
                // delete block_bb;
                continue;
            }
            else if (block_bb->name == nullptr) {
                auto last = bbs_vec->back();
                last->insts = combine_slices(last->insts, block_bb->insts);
            }
            else bbs_vec->push_back(block_bb);
        }
    }
    return bbs_vec;
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

// args[0] : while_entry_bb
// args[1] : while_end_bb
{
    switch (type) {
        case IF:
        {
            koopa_raw_basic_block_data_t* end_bb = build_block_from_insts(nullptr, "%end");
            auto true_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)stmt->toRaw(n, args);
            true_bbs->front()->name = "%then";
            auto raw_jmp = build_jump(end_bb);
            auto jmp_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_jmp}));

            auto exp_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)exp->toRaw();
            auto exp_last_bb = exp_bbs_vec->back();
            auto value = (koopa_raw_value_data*)exp_last_bb->insts.buffer[exp_last_bb->insts.len-1];

            auto raw_branch = build_branch(value, true_bbs->front(), end_bb);
            auto branch_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_branch}));

            exp_bbs_vec->push_back(branch_bb);
            for (auto bb : *true_bbs) {
                exp_bbs_vec->push_back(bb);
            }
            exp_bbs_vec->push_back(jmp_bb);
            exp_bbs_vec->push_back(end_bb);
            
            return exp_bbs_vec;
        }
        case IF_ELSE:
        {
            koopa_raw_basic_block_data_t* end_bb = build_block_from_insts(new std::vector<koopa_raw_value_data*>(), "%end");
            auto true_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)closed_stmt->toRaw(n, args);
            true_bbs->front()->name = "%then";
            auto false_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)open_stmt->toRaw(n, args);
            false_bbs->front()->name = "%else";
            auto raw_jmp = build_jump(end_bb);
            auto jmp_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_jmp}));

            auto exp_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)exp->toRaw();
            auto exp_last_bb = exp_bbs_vec->back();
            auto value = (koopa_raw_value_data*)exp_last_bb->insts.buffer[exp_last_bb->insts.len-1];
            
            auto raw_branch = build_branch(value, true_bbs->front(), false_bbs->front());
            auto branch_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_branch}));

            exp_bbs_vec->push_back(branch_bb);
            for (auto bb : *true_bbs) {
                exp_bbs_vec->push_back(bb);
            }
            exp_bbs_vec->push_back(jmp_bb);
            for (auto bb : *false_bbs) {
                exp_bbs_vec->push_back(bb);
            }
            exp_bbs_vec->push_back(jmp_bb);
            exp_bbs_vec->push_back(end_bb);

            return exp_bbs_vec;
        }
        case WHILE:
        {
            koopa_raw_basic_block_data_t* end_bb = build_block_from_insts(new std::vector<koopa_raw_value_data*>(), "%while_end");

            auto exp_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)exp->toRaw();
            auto entry_bb = exp_bbs_vec->front();
            if (entry_bb->name == nullptr)
                entry_bb->name = "%while_entry";
            auto exp_last_bb = exp_bbs_vec->back();
            auto value = (koopa_raw_value_data*)exp_last_bb->insts.buffer[exp_last_bb->insts.len-1];
            
            auto raw_branch = build_branch(value, nullptr, end_bb);
            auto branch_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_branch}));

            void **new_args = new void*[2]{entry_bb, end_bb};
            auto true_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)open_stmt->toRaw(n, new_args);
            true_bbs->front()->name = "%while_body";
            auto raw_jmp = build_jump(entry_bb);
            auto jmp_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_jmp}));
            raw_branch->kind.data.branch.true_bb = true_bbs->front();

            auto init_jmp = build_jump(entry_bb);
            auto init_insts = new std::vector<koopa_raw_value_data_t*>({init_jmp});
            auto init_bb = build_block_from_insts(init_insts);

            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>({init_bb});
            for (auto bb : *exp_bbs_vec) {
                bbs_vec->push_back(bb);
            }
            bbs_vec->push_back(branch_bb);
            for (auto bb : *true_bbs) {
                bbs_vec->push_back(bb);
            }
            bbs_vec->push_back(jmp_bb);
            bbs_vec->push_back(end_bb);

            return bbs_vec;
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
            koopa_raw_basic_block_data_t* end_bb = build_block_from_insts(new std::vector<koopa_raw_value_data*>(), "%end");
            auto true_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)closed_stmt->toRaw(n, args);
            true_bbs->front()->name = "%then";
            auto false_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)closed_stmt2->toRaw(n, args);
            false_bbs->front()->name = "%else";
            auto raw_jmp = build_jump(end_bb);
            auto jmp_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_jmp}));

            auto exp_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)exp->toRaw();
            auto exp_last_bb = exp_bbs_vec->back();
            auto value = (koopa_raw_value_data*)exp_last_bb->insts.buffer[exp_last_bb->insts.len-1];
            auto raw_branch = build_branch(value, true_bbs->front(), false_bbs->front());
            auto branch_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_branch}));

            exp_bbs_vec->push_back(branch_bb);
            for (auto bb : *true_bbs) {
                exp_bbs_vec->push_back(bb);
            }
            exp_bbs_vec->push_back(jmp_bb);
            for (auto bb : *false_bbs) {
                exp_bbs_vec->push_back(bb);
            }
            exp_bbs_vec->push_back(jmp_bb);
            exp_bbs_vec->push_back(end_bb);

            return exp_bbs_vec;
        }
        case WHILE:
        {
            koopa_raw_basic_block_data_t* end_bb = build_block_from_insts(new std::vector<koopa_raw_value_data*>(), "%while_end");

            auto exp_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)exp->toRaw();
            auto entry_bb = exp_bbs_vec->front();
            if (entry_bb->name == nullptr)
                entry_bb->name = "%while_entry";
            auto exp_last_bb = exp_bbs_vec->back();
            auto value = (koopa_raw_value_data*)exp_last_bb->insts.buffer[exp_last_bb->insts.len-1];
            
            auto raw_branch = build_branch(value, nullptr, end_bb);
            auto branch_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_branch}));

            void **new_args = new void*[2]{entry_bb, end_bb};
            auto true_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)closed_stmt->toRaw(n, new_args);
            true_bbs->front()->name = "%while_body";
            auto raw_jmp = build_jump(entry_bb);
            auto jmp_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_jmp}));
            raw_branch->kind.data.branch.true_bb = true_bbs->front();

            auto init_jmp = build_jump(entry_bb);
            auto init_insts = new std::vector<koopa_raw_value_data_t*>({init_jmp});
            auto init_bb = build_block_from_insts(init_insts);

            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>({init_bb});
            for (auto bb : *exp_bbs_vec) {
                bbs_vec->push_back(bb);
            }
            bbs_vec->push_back(branch_bb);
            for (auto bb : *true_bbs) {
                bbs_vec->push_back(bb);
            }
            bbs_vec->push_back(jmp_bb);
            bbs_vec->push_back(end_bb);

            return bbs_vec;
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
            auto raw_stmt = new koopa_raw_value_data_t;
            auto ty = new koopa_raw_type_kind_t;
            ty->tag = KOOPA_RTT_INT32;
            raw_stmt->ty = ty;
            raw_stmt->name = nullptr;
            raw_stmt->kind.tag = KOOPA_RVT_STORE;

            auto ident = dynamic_cast<LValAST*>(lval.get())->ident;
            auto sym = Symbol::query(ident);
            assert(sym.type == Symbol::TYPE_VAR);
            auto raw_alloc = sym.allocator;

            auto bbs_vec_exp = (std::vector<koopa_raw_basic_block_data_t*>*)exp->toRaw();
            assert(bbs_vec_exp->size() > 0);
            auto exp_last_bb = bbs_vec_exp->back();
            auto value = (koopa_raw_value_data*)exp_last_bb->insts.buffer[exp_last_bb->insts.len-1];

            raw_stmt->kind.data.store.dest = raw_alloc;
            raw_stmt->kind.data.store.value = value;

            exp_last_bb->insts.len += 1;
            auto buffer = new const void*[exp_last_bb->insts.len];
            for (int i = 0; i < exp_last_bb->insts.len - 1; i++) {
                buffer[i] = exp_last_bb->insts.buffer[i];
            }
            buffer[exp_last_bb->insts.len-1] = raw_stmt;
            exp_last_bb->insts.buffer = buffer;

            return bbs_vec_exp;
        }
        case RETURN:
        {   
            auto raw_stmt = new koopa_raw_value_data_t;
            auto ty = new koopa_raw_type_kind_t;
            ty->tag = KOOPA_RTT_INT32;
            raw_stmt->ty = ty;
            raw_stmt->name = nullptr;
            set_used_by(raw_stmt, nullptr);
            raw_stmt->kind.tag = KOOPA_RVT_RETURN;

            auto exp_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)exp->toRaw();
            assert(exp_bbs_vec->size() > 0);
            auto exp_last_bb = exp_bbs_vec->back();
            auto value = (koopa_raw_value_data*)exp_last_bb->insts.buffer[exp_last_bb->insts.len-1];
            raw_stmt->kind.data.ret.value = value;
            
            exp_last_bb->insts.len += 1;
            auto buffer = new const void*[exp_last_bb->insts.len];
            for (int i = 0; i < exp_last_bb->insts.len - 1; i++) {
                buffer[i] = exp_last_bb->insts.buffer[i];
            }
            buffer[exp_last_bb->insts.len-1] = raw_stmt;
            exp_last_bb->insts.buffer = buffer;

            return exp_bbs_vec;
        }
        case EMPTY_RETURN:
        {
            auto raw_stmt = new koopa_raw_value_data_t;
            auto ty = new koopa_raw_type_kind_t;
            ty->tag = KOOPA_RTT_INT32;
            raw_stmt->ty = ty;
            raw_stmt->name = nullptr;
            set_used_by(raw_stmt, nullptr);
            auto insts = new std::vector<koopa_raw_value_data*>();
            raw_stmt->kind.tag = KOOPA_RVT_RETURN;
            raw_stmt->kind.data.ret.value = nullptr;
            insts->push_back(raw_stmt);
            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(build_block_from_insts(insts, nullptr));
            return bbs_vec;
        }
        case EXP:
        {
            return exp->toRaw();
        }
        case EMPTY:
        {
            auto insts = new std::vector<koopa_raw_value_data*>();
            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(build_block_from_insts(insts, nullptr));
            return bbs_vec;
        }
        case BLOCK:
        {
            Symbol::enter_scope();
            auto bbs_vec = block->toRaw(n, args);
            Symbol::leave_scope();
            return bbs_vec;
        }
        case BREAK:
        {
            auto insts = new std::vector<koopa_raw_value_data*>();
            assert(args != nullptr);
            koopa_raw_basic_block_data_t* target = (koopa_raw_basic_block_data_t*)args[1];
            assert(target != nullptr);
            auto raw_jmp = build_jump(target);
            insts->push_back(raw_jmp);
            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(build_block_from_insts(insts, nullptr));
            return bbs_vec;
        }
        case CONTINUE:
        {
            auto insts = new std::vector<koopa_raw_value_data*>();
            assert(args != nullptr);
            koopa_raw_basic_block_data_t* target = (koopa_raw_basic_block_data_t*)args[0];
            assert(target != nullptr);
            auto raw_jmp = build_jump(target);
            insts->push_back(raw_jmp);
            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(build_block_from_insts(insts, nullptr));
            return bbs_vec;
        }
        default:
            assert(false);
    }
}

void *ConstExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    return exp->toRaw();
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
        auto insts = (std::vector<koopa_raw_value_data*>*)lval->toRaw();
        auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
        bbs_vec->push_back(build_block_from_insts(insts));
        return bbs_vec;
    }
    case NUMBER:
    {
        auto raw_number = build_number(number);
        auto insts = new std::vector<koopa_raw_value_data*>({raw_number});
        auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
        bbs_vec->push_back(build_block_from_insts(insts));
        return bbs_vec;
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
        auto bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)unary_exp->toRaw();
        auto last_bb = bbs_vec->back();
        auto value = (koopa_raw_value_data_t*)(last_bb->insts.buffer[last_bb->insts.len-1]);
        
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
            last_bb->insts.buffer[last_bb->insts.len-1] = build_number(val, nullptr);
            return bbs_vec;
        }

        auto raw_unary = new koopa_raw_value_data_t({
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
                .data.binary = {
                    .lhs = build_number(0, nullptr),
                    .rhs = value,
                },
            },
        });

        switch (unaryop) {
        case '-':
            raw_unary->kind.data.binary.op = KOOPA_RBO_SUB;
            break;
        case '!':
            raw_unary->kind.data.binary.op = KOOPA_RBO_EQ;
            break;
        default:
            assert(false);
        }

        auto unary_bb = build_block_from_insts(new std::vector<koopa_raw_value_data_t*>({raw_unary}));
        bbs_vec->push_back(unary_bb);
        return bbs_vec;
    }
    case FUNC_CALL:
    {
        auto raw = new koopa_raw_value_data_t({
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
                .data.call = {
                    .callee = Symbol::query(ident).function,
                    .args = {
                        .kind = KOOPA_RSIK_VALUE,
                    },
                },
            },
        });

        auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
        auto params = new std::vector<koopa_raw_value_data_t*>();
        
        for (auto &exp : *func_r_param_list) {
            auto exp_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)exp->toRaw();
            auto exp_last_bb = exp_bbs_vec->back();
            auto exp_value = (koopa_raw_value_data_t*)exp_last_bb->insts.buffer[exp_last_bb->insts.len-1];
            params->push_back(exp_value);

            for (auto bb : *exp_bbs_vec) {
                bbs_vec->push_back(bb);
            }
        }
        raw->kind.data.call.args.len = params->size();
        raw->kind.data.call.args.kind = KOOPA_RSIK_VALUE;
        raw->kind.data.call.args.buffer = new const void* [params->size()];
        for (int i = 0; i < params->size(); i++) {
            raw->kind.data.call.args.buffer[i] = params->at(i);
        }
        auto insts = new std::vector<koopa_raw_value_data_t*>();
        insts->push_back(raw);
        auto call_bb = build_block_from_insts(insts);
        bbs_vec->push_back(call_bb);
        return bbs_vec;
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

    auto left_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)mul_exp->toRaw();
    auto left_last_bb = left_bbs_vec->back();
    auto left_value = (koopa_raw_value_data_t*)left_last_bb->insts.buffer[left_last_bb->insts.len - 1];

    auto right_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)unary_exp->toRaw();
    auto right_last_bb = right_bbs_vec->back();
    auto right_value = (koopa_raw_value_data_t*)right_last_bb->insts.buffer[right_last_bb->insts.len - 1];

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

        // left_last_bb->insts.len -= 1;
        // left_last_bb->insts.buffer = new const void*[left_last_bb->insts.len];
        // for (int i = 0; i < left_last_bb->insts.len; i++) {
        //     left_last_bb->insts.buffer[i] = left_last_bb->insts.buffer[i];
        // }
        // right_last_bb->insts.buffer[right_last_bb->insts.len - 1] = build_number(result, nullptr);
        // for (auto bb : *right_bbs_vec) {
        //     left_bbs_vec->push_back(bb);
        // }
        // return left_bbs_vec;

        auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(build_number(result, nullptr));
        bbs_vec->push_back(build_block_from_insts(insts, nullptr));
        return bbs_vec;
    }

    auto raw = new koopa_raw_value_data_t;
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_INT32;
    raw->ty = ty;
    raw->name = nullptr;
    raw->kind.tag = KOOPA_RVT_BINARY;

    switch (op)
    {
    case '*':
        raw->kind.data.binary.op = KOOPA_RBO_MUL;
        break;
    case '/':
        raw->kind.data.binary.op = KOOPA_RBO_DIV;
        break;
    case '%':
        raw->kind.data.binary.op = KOOPA_RBO_MOD;
        break;
    default:
        assert(false);
    }

    raw->kind.data.binary.lhs = left_value;
    raw->kind.data.binary.rhs = right_value;

    if (left_value->kind.tag == KOOPA_RVT_INTEGER)
        left_last_bb->insts.len -= 1;
    if (right_value->kind.tag == KOOPA_RVT_INTEGER)
        right_last_bb->insts.len -= 1;
    
    for (auto bb : *right_bbs_vec) {
        left_bbs_vec->push_back(bb);
    }
    auto last_bb = left_bbs_vec->back();
    auto buffer = new const void*[last_bb->insts.len + 1];
    for (int i = 0; i < last_bb->insts.len; i++) {
        buffer[i] = last_bb->insts.buffer[i];
    }
    buffer[last_bb->insts.len] = raw;
    last_bb->insts.len += 1;
    last_bb->insts.buffer = buffer;
    return left_bbs_vec;
}

void *AddExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (type == MUL)
        return mul_exp->toRaw();
    
    assert(type == ADD);

    auto left_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)add_exp->toRaw();
    auto left_last_bb = left_bbs_vec->back();
    auto left_value = (koopa_raw_value_data_t*)left_last_bb->insts.buffer[left_last_bb->insts.len - 1];

    auto right_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)mul_exp->toRaw();
    auto right_last_bb = right_bbs_vec->back();
    auto right_value = (koopa_raw_value_data_t*)right_last_bb->insts.buffer[right_last_bb->insts.len - 1];

    if (left_value->kind.tag == KOOPA_RVT_INTEGER && right_value->kind.tag == KOOPA_RVT_INTEGER) {
        int left = left_value->kind.data.integer.value;
        int right = right_value->kind.data.integer.value;
        int result;
        switch (op) {
        case '+':
            result = left + right;
            break;
        case '-':
            result = left - right;
            break;
        default:
            assert(false);
        }

        auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(build_number(result, nullptr));
        bbs_vec->push_back(build_block_from_insts(insts, nullptr));
        return bbs_vec;
    }

    auto raw = new koopa_raw_value_data_t;
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_INT32;
    raw->ty = ty;
    raw->name = nullptr;
    raw->kind.tag = KOOPA_RVT_BINARY;
    
    switch (op)
    {
    case '+':
        raw->kind.data.binary.op = KOOPA_RBO_ADD;
        break;
    case '-':
        raw->kind.data.binary.op = KOOPA_RBO_SUB;
        break;
    default:
        assert(false);
    }

    raw->kind.data.binary.lhs = left_value;
    raw->kind.data.binary.rhs = right_value;

    if (left_value->kind.tag == KOOPA_RVT_INTEGER)
        left_last_bb->insts.len -= 1;
    if (right_value->kind.tag == KOOPA_RVT_INTEGER)
        right_last_bb->insts.len -= 1;

    for (auto bb : *right_bbs_vec) {
        left_bbs_vec->push_back(bb);
    }
    auto last_bb = left_bbs_vec->back();
    auto buffer = new const void*[last_bb->insts.len + 1];
    for (int i = 0; i < last_bb->insts.len; i++) {
        buffer[i] = last_bb->insts.buffer[i];
    }
    buffer[last_bb->insts.len] = raw;
    last_bb->insts.len += 1;
    last_bb->insts.buffer = buffer;
    return left_bbs_vec;
}

void *RelExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (type == ADD)
        return add_exp->toRaw();
    
    assert(type == REL);

    auto left_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)rel_exp->toRaw();
    auto left_last_bb = left_bbs_vec->back();
    auto left_value = (koopa_raw_value_data_t*)left_last_bb->insts.buffer[left_last_bb->insts.len - 1];

    auto right_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)add_exp->toRaw();
    auto right_last_bb = right_bbs_vec->back();
    auto right_value = (koopa_raw_value_data_t*)right_last_bb->insts.buffer[right_last_bb->insts.len - 1];

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
        
        auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(build_number(result, nullptr));
        bbs_vec->push_back(build_block_from_insts(insts, nullptr));
        return bbs_vec;
    }

    auto raw = new koopa_raw_value_data_t;
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_INT32;
    raw->ty = ty;
    raw->name = nullptr;
    raw->kind.tag = KOOPA_RVT_BINARY;

    if (op == "<") {
        raw->kind.data.binary.op = KOOPA_RBO_LT;
    } else if (op == ">") {
        raw->kind.data.binary.op = KOOPA_RBO_GT;
    } else if (op == "<=") {
        raw->kind.data.binary.op = KOOPA_RBO_LE;
    } else if (op == ">=") {
        raw->kind.data.binary.op = KOOPA_RBO_GE;
    } else {
        assert(false);
    }

    raw->kind.data.binary.lhs = left_value;
    raw->kind.data.binary.rhs = right_value;

    if (left_value->kind.tag == KOOPA_RVT_INTEGER)
        left_last_bb->insts.len -= 1;
    if (right_value->kind.tag == KOOPA_RVT_INTEGER)
        right_last_bb->insts.len -= 1;

    for (auto bb : *right_bbs_vec) {
        left_bbs_vec->push_back(bb);
    }
    auto last_bb = left_bbs_vec->back();
    auto buffer = new const void*[last_bb->insts.len + 1];
    for (int i = 0; i < last_bb->insts.len; i++) {
        buffer[i] = last_bb->insts.buffer[i];
    }
    buffer[last_bb->insts.len] = raw;
    last_bb->insts.len += 1;
    last_bb->insts.buffer = buffer;
    return left_bbs_vec;
}

void *EqExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (type == REL)
        return rel_exp->toRaw();
    
    assert(type == EQ);

    auto left_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)eq_exp->toRaw();
    auto left_last_bb = left_bbs_vec->back();
    auto left_value = (koopa_raw_value_data_t*)left_last_bb->insts.buffer[left_last_bb->insts.len - 1];

    auto right_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)rel_exp->toRaw();
    auto right_last_bb = right_bbs_vec->back();
    auto right_value = (koopa_raw_value_data_t*)right_last_bb->insts.buffer[right_last_bb->insts.len - 1];

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
        
        auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(build_number(result, nullptr));
        bbs_vec->push_back(build_block_from_insts(insts, nullptr));
        return bbs_vec;
    }

    auto raw = new koopa_raw_value_data_t;
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_INT32;
    raw->ty = ty;
    raw->name = nullptr;
    raw->kind.tag = KOOPA_RVT_BINARY;

    if (op == "==") {
        raw->kind.data.binary.op = KOOPA_RBO_EQ;
    } else if (op == "!=") {
        raw->kind.data.binary.op = KOOPA_RBO_NOT_EQ;
    } else {
        assert(false);
    }

    raw->kind.data.binary.lhs = left_value;
    raw->kind.data.binary.rhs = right_value;

    if (left_value->kind.tag == KOOPA_RVT_INTEGER)
        left_last_bb->insts.len -= 1;
    if (right_value->kind.tag == KOOPA_RVT_INTEGER)
        right_last_bb->insts.len -= 1;

    for (auto bb : *right_bbs_vec) {
        left_bbs_vec->push_back(bb);
    }
    auto last_bb = left_bbs_vec->back();
    auto buffer = new const void*[last_bb->insts.len + 1];
    for (int i = 0; i < last_bb->insts.len; i++) {
        buffer[i] = last_bb->insts.buffer[i];
    }
    buffer[last_bb->insts.len] = raw;
    last_bb->insts.len += 1;
    last_bb->insts.buffer = buffer;
    return left_bbs_vec;
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
//                      %lhs:
//                          ...
//                          br lhs %rhs %end(0)
//                      %rhs:
//                          ...
//                          %0 = ne rhs 0
//                          jmp %end(%0)
//                      %end(%value: i32):
//                          %1 = %value
{
    if (type == EQ)
        return eq_exp->toRaw();
    
    assert(type == LAND);

    auto left_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)land_exp->toRaw();
    auto left_last_bb = left_bbs_vec->back();
    auto left_value = (koopa_raw_value_data_t*)left_last_bb->insts.buffer[left_last_bb->insts.len - 1];

    auto right_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)eq_exp->toRaw();
    auto right_last_bb = right_bbs_vec->back();
    auto right_value = (koopa_raw_value_data_t*)right_last_bb->insts.buffer[right_last_bb->insts.len - 1];

    // lhs能判断值
    if (left_value->kind.tag == KOOPA_RVT_INTEGER) {
        int left = left_value->kind.data.integer.value;
        // lhs为false，放一个0
        if (left == 0) {
            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            auto insts = new std::vector<koopa_raw_value_data*>();
            insts->push_back(build_number(0, nullptr));
            bbs_vec->push_back(build_block_from_insts(insts, nullptr));
            return bbs_vec;
        }
        // lhs为true
        // rhs能判断值，lhs && rhs
        if (right_value->kind.tag == KOOPA_RVT_INTEGER) {
            int right = right_value->kind.data.integer.value;
            int result = left && right;
            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            auto insts = new std::vector<koopa_raw_value_data*>();
            insts->push_back(build_number(result, nullptr));
            bbs_vec->push_back(build_block_from_insts(insts, nullptr));
            return bbs_vec;
        }
        // rhs不能判断值，ne rhs, 0
        auto raw_right = new koopa_raw_value_data_t;
        auto ty_right = new koopa_raw_type_kind_t;
        ty_right->tag = KOOPA_RTT_INT32;
        raw_right->ty = ty_right;
        raw_right->name = nullptr;
        raw_right->kind.tag = KOOPA_RVT_BINARY;
        raw_right->kind.data.binary.op = KOOPA_RBO_NOT_EQ;

        auto zero_right = build_number(0);
        raw_right->kind.data.binary.lhs = zero_right;
        raw_right->kind.data.binary.rhs = right_value;

        right_last_bb->insts.len += 1;
        auto buffer = new const void*[right_last_bb->insts.len];
        for (int i = 0; i < right_last_bb->insts.len - 1; i++) {
            buffer[i] = right_last_bb->insts.buffer[i];
        }
        buffer[right_last_bb->insts.len - 1] = raw_right;
        right_last_bb->insts.buffer = buffer;
        return right_bbs_vec;
    }

    // lhs不能判断值（其实还可以做rhs的判断，但比较麻烦，先不写了）
    // end_bb
    auto raw_param = new koopa_raw_value_data_t;
    auto ty_param = new koopa_raw_type_kind_t;
    ty_param->tag = KOOPA_RTT_INT32;
    raw_param->ty = ty_param;
    raw_param->name = "%value";
    raw_param->kind.tag = KOOPA_RVT_BLOCK_ARG_REF;
    raw_param->kind.data.block_arg_ref.index = 0;
    raw_param->used_by = {
        .buffer = nullptr,
        .len = 0,
        .kind = KOOPA_RSIK_VALUE,
    };

    auto end_bb = new koopa_raw_basic_block_data_t;
    end_bb->name = "%end";
    end_bb->insts.len = 1;
    end_bb->insts.buffer = new const void*[1];
    end_bb->insts.buffer[0] = raw_param;
    end_bb->insts.kind = KOOPA_RSIK_VALUE;

    end_bb->params.len = 1;
    end_bb->params.buffer = new const void*[1];
    end_bb->params.buffer[0] = raw_param;
    end_bb->params.kind = KOOPA_RSIK_VALUE;

    if (left_value->kind.tag == KOOPA_RVT_INTEGER || left_value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF)
        left_last_bb->insts.len -= 1;
    if (right_value->kind.tag == KOOPA_RVT_INTEGER || right_value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF)
        right_last_bb->insts.len -= 1;

    // lhs
    auto false_args = new koopa_raw_slice_t;
    false_args->len = 1;
    false_args->kind = KOOPA_RSIK_VALUE;
    false_args->buffer = new const void*[1];
    false_args->buffer[0] = build_number(0);

    left_last_bb->insts.len += 1;
    auto left_buffer = new const void*[left_last_bb->insts.len];
    for (int i = 0; i < left_last_bb->insts.len - 1; i++) {
        left_buffer[i] = left_last_bb->insts.buffer[i];
    }
    left_buffer[left_last_bb->insts.len - 1] = build_branch(left_value, right_bbs_vec->front(), end_bb, nullptr, false_args);
    left_last_bb->insts.buffer = left_buffer;

    // rhs
    auto raw_right = new koopa_raw_value_data_t;
    auto ty_right = new koopa_raw_type_kind_t;
    ty_right->tag = KOOPA_RTT_INT32;
    raw_right->ty = ty_right;
    raw_right->name = nullptr;
    raw_right->kind.tag = KOOPA_RVT_BINARY;
    raw_right->kind.data.binary.op = KOOPA_RBO_NOT_EQ;

    auto zero_right = build_number(0, raw_right);
    raw_right->kind.data.binary.lhs = zero_right;
    raw_right->kind.data.binary.rhs = right_value;
    set_used_by(right_value, raw_right);

    auto jmp_args = new koopa_raw_slice_t;
    jmp_args->len = 1;
    jmp_args->kind = KOOPA_RSIK_VALUE;
    jmp_args->buffer = new const void*[1];
    jmp_args->buffer[0] = raw_right;

    right_last_bb->insts.len += 2;
    auto right_buffer = new const void*[right_last_bb->insts.len];
    for (int i = 0; i < right_last_bb->insts.len - 2; i++) {
        right_buffer[i] = right_last_bb->insts.buffer[i];
    }
    right_buffer[right_last_bb->insts.len - 2] = raw_right;
    right_buffer[right_last_bb->insts.len - 1] = build_jump(end_bb, jmp_args);
    right_last_bb->insts.buffer = right_buffer;

    auto right_first_bb = right_bbs_vec->front();
    if (right_first_bb->name == nullptr)
        right_first_bb->name = "%rhs";

    for (auto bb : *right_bbs_vec) {
        left_bbs_vec->push_back(bb);
    }
    left_bbs_vec->push_back(end_bb);
    return left_bbs_vec;
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
//                      %lhs:
//                          ...
//                          br lhs %end(1) %rhs
//                      %rhs:
//                          ...
//                          %0 = ne rhs 0
//                          jmp %end(%0)
//                      %end(%value: i32):
//                          %1 = %value
{
    if (type == LAND)
        return land_exp->toRaw();

    assert(type == LOR);

    auto left_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)lor_exp->toRaw();
    auto left_last_bb = left_bbs_vec->back();
    auto left_value = (koopa_raw_value_data_t*)left_last_bb->insts.buffer[left_last_bb->insts.len - 1];

    auto right_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)land_exp->toRaw();
    auto right_last_bb = right_bbs_vec->back();
    auto right_value = (koopa_raw_value_data_t*)right_last_bb->insts.buffer[right_last_bb->insts.len - 1];

    // lhs能判断值
    if (left_value->kind.tag == KOOPA_RVT_INTEGER) {
        int left = left_value->kind.data.integer.value;
        // lhs为true，放一个1
        if (left != 0) {
            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            auto insts = new std::vector<koopa_raw_value_data*>();
            insts->push_back(build_number(1));
            bbs_vec->push_back(build_block_from_insts(insts));
            return bbs_vec;
        }
        // lhs为false
        // rhs能判断值，lhs || rhs
        if (right_value->kind.tag == KOOPA_RVT_INTEGER) {
            int right = right_value->kind.data.integer.value;
            int result = left || right;
            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            auto insts = new std::vector<koopa_raw_value_data*>();
            insts->push_back(build_number(result));
            bbs_vec->push_back(build_block_from_insts(insts));
            return bbs_vec;
        }
        // rhs不能判断值，ne rhs, 0
        auto raw_right = new koopa_raw_value_data_t;
        auto ty_right = new koopa_raw_type_kind_t;
        ty_right->tag = KOOPA_RTT_INT32;
        raw_right->ty = ty_right;
        raw_right->name = nullptr;
        raw_right->kind.tag = KOOPA_RVT_BINARY;
        raw_right->kind.data.binary.op = KOOPA_RBO_NOT_EQ;

        auto zero_right = build_number(0);
        raw_right->kind.data.binary.lhs = right_value;
        raw_right->kind.data.binary.rhs = zero_right;

        right_last_bb->insts.len += 1;
        auto buffer = new const void*[right_last_bb->insts.len];
        for (int i = 0; i < right_last_bb->insts.len - 1; i++) {
            buffer[i] = right_last_bb->insts.buffer[i];
        }
        buffer[right_last_bb->insts.len - 1] = raw_right;
        right_last_bb->insts.buffer = buffer;
        return right_bbs_vec;
    }

    // lhs不能判断值
    // end_bb
    auto raw_param = new koopa_raw_value_data_t;
    auto ty_param = new koopa_raw_type_kind_t;
    ty_param->tag = KOOPA_RTT_INT32;
    raw_param->ty = ty_param;
    raw_param->name = "%value";
    raw_param->kind.tag = KOOPA_RVT_BLOCK_ARG_REF;
    raw_param->kind.data.block_arg_ref.index = 0;
    raw_param->used_by = {
        .buffer = nullptr,
        .len = 0,
        .kind = KOOPA_RSIK_VALUE,
    };

    auto end_bb = new koopa_raw_basic_block_data_t;
    end_bb->name = "%end";
    end_bb->insts.len = 1;
    end_bb->insts.buffer = new const void*[1];
    end_bb->insts.buffer[0] = raw_param;
    end_bb->insts.kind = KOOPA_RSIK_VALUE;

    end_bb->params.len = 1;
    end_bb->params.buffer = new const void*[1];
    end_bb->params.buffer[0] = raw_param;
    end_bb->params.kind = KOOPA_RSIK_VALUE;

    if (left_value->kind.tag == KOOPA_RVT_INTEGER || left_value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF)
        left_last_bb->insts.len -= 1;
    if (right_value->kind.tag == KOOPA_RVT_INTEGER || right_value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF)
        right_last_bb->insts.len -= 1;

    // lhs
    auto true_args = new koopa_raw_slice_t;
    true_args->len = 1;
    true_args->kind = KOOPA_RSIK_VALUE;
    true_args->buffer = new const void*[1];
    true_args->buffer[0] = build_number(1);
    
    left_last_bb->insts.len += 1;
    auto left_buffer = new const void*[left_last_bb->insts.len];
    for (int i = 0; i < left_last_bb->insts.len - 1; i++) {
        left_buffer[i] = left_last_bb->insts.buffer[i];
    }
    left_buffer[left_last_bb->insts.len - 1] = build_branch(left_value, end_bb, right_bbs_vec->front(), true_args);
    left_last_bb->insts.buffer = left_buffer;

    // rhs
    auto raw_right = new koopa_raw_value_data_t;
    auto ty_right = new koopa_raw_type_kind_t;
    ty_right->tag = KOOPA_RTT_INT32;
    raw_right->ty = ty_right;
    raw_right->name = nullptr;
    raw_right->kind.tag = KOOPA_RVT_BINARY;
    raw_right->kind.data.binary.op = KOOPA_RBO_NOT_EQ;

    auto zero_right = build_number(0, raw_right);
    raw_right->kind.data.binary.lhs = zero_right;
    raw_right->kind.data.binary.rhs = right_value;
    set_used_by(right_value, raw_right);
    
    auto jmp_args = new koopa_raw_slice_t;
    jmp_args->len = 1;
    jmp_args->kind = KOOPA_RSIK_VALUE;
    jmp_args->buffer = new const void*[1];
    jmp_args->buffer[0] = raw_right;

    right_last_bb->insts.len += 2;
    auto right_buffer = new const void*[right_last_bb->insts.len];
    for (int i = 0; i < right_last_bb->insts.len - 2; i++) {
        right_buffer[i] = right_last_bb->insts.buffer[i];
    }
    right_buffer[right_last_bb->insts.len - 2] = raw_right;
    right_buffer[right_last_bb->insts.len - 1] = build_jump(end_bb, jmp_args);
    right_last_bb->insts.buffer = right_buffer;

    auto right_first_bb = right_bbs_vec->front();
    if (right_first_bb->name == nullptr)
        right_first_bb->name = "%rhs";

    for (auto bb : *right_bbs_vec) {
        left_bbs_vec->push_back(bb);
    }
    left_bbs_vec->push_back(end_bb);

    return left_bbs_vec;
}

void *DeclAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    switch (type)
    {
    case CONSTDECL:
        return const_decl->toRaw(n, args);
    case VARDECL:
        return var_decl->toRaw(n, args);
    default:
        assert(false);
    }
}

void *ConstDeclAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    for (auto &i : *const_def_list) {
        i->toRaw();
    }
    return build_block_from_insts();
}

void *VarDeclAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
    for (auto &i : *var_def_list) {
        auto def_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)i->toRaw(n, args);
        for (auto bb : *def_bbs_vec) {
            bbs_vec->push_back(bb);
        }
    }
    return bbs_vec;
}

void *ConstDefAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    int val = (long)(const_init_val->toRaw());
    Symbol::insert(ident, Symbol::TYPE_CONST, val);
    return nullptr;
}

void *VarDefAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (n == 1) {
        // global def, global_alloc, 返回的是insts
        auto insts = new std::vector<koopa_raw_value_data*>();
        auto raw_global_alloc = new koopa_raw_value_data_t;
        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_POINTER;
        auto raw_base = new koopa_raw_type_kind_t;
        raw_base->tag = KOOPA_RTT_INT32;
        ty->data.pointer.base = raw_base;

        raw_global_alloc->ty = ty;
        raw_global_alloc->name = build_ident(ident, '@');
        raw_global_alloc->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
        if (has_init_val) {
            auto exp_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)init_val->toRaw();
            auto exp_last_bb = exp_bbs->back();
            auto val = (koopa_raw_value_data_t*)exp_last_bb->insts.buffer[exp_last_bb->insts.len - 1];
            assert(val->kind.tag == KOOPA_RVT_INTEGER);
            raw_global_alloc->kind.data.global_alloc.init = build_number(val->kind.data.integer.value);
        }
        else {
            auto raw_zero_init = new koopa_raw_value_data_t;
            raw_zero_init->name = nullptr;
            {
                auto ty = new koopa_raw_type_kind_t;
                ty->tag = KOOPA_RTT_INT32;
                raw_zero_init->ty = ty;
            }
            raw_zero_init->kind.tag = KOOPA_RVT_ZERO_INIT;
            raw_global_alloc->kind.data.global_alloc.init = raw_zero_init;
        }
        Symbol::insert(ident, Symbol::TYPE_VAR, raw_global_alloc);
        insts->push_back(raw_global_alloc);
        return insts;
    }

    // local def, alloc, store, 返回的是bbs_vec
    // alloc
    auto raw_alloc = build_alloc(build_ident(ident, '%'));
    Symbol::insert(ident, Symbol::TYPE_VAR, raw_alloc);

    // store
    if (has_init_val) {
        auto init_val_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)init_val->toRaw();
        auto init_val_last_bb = init_val_bbs_vec->back();
        auto value = (koopa_raw_value_data_t*)init_val_last_bb->insts.buffer[init_val_last_bb->insts.len - 1];
        auto raw_store = build_store(value, raw_alloc);

        if (value->kind.tag == KOOPA_RVT_INTEGER || value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF) {
            init_val_last_bb->insts.len -= 1;
        }
        
        init_val_last_bb->insts.len += 2;
        auto buffer = new const void*[init_val_last_bb->insts.len];
        for (int i = 0; i < init_val_last_bb->insts.len - 2; i++) {
            buffer[i] = init_val_last_bb->insts.buffer[i];
        }
        buffer[init_val_last_bb->insts.len - 2] = raw_alloc;
        buffer[init_val_last_bb->insts.len - 1] = raw_store;

        init_val_last_bb->insts.buffer = buffer;

        return init_val_bbs_vec;
    }
    // no init_val
    auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
    auto insts = new std::vector<koopa_raw_value_data*>();
    insts->push_back(raw_alloc);
    bbs_vec->push_back(build_block_from_insts(insts));
    return bbs_vec;
}

void *ConstInitValAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto const_exp_bbs_vec = (std::vector<koopa_raw_basic_block_data_t*>*)const_exp->toRaw();
    auto const_exp_last_bb = const_exp_bbs_vec->back();
    auto value = (koopa_raw_value_data_t*)const_exp_last_bb->insts.buffer[const_exp_last_bb->insts.len - 1];

    assert(value->kind.tag == KOOPA_RVT_INTEGER);

    int val = value->kind.data.integer.value;
    return (void*)val;
}

void *InitValAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    return exp->toRaw();
}

void *LValAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto sym = Symbol::query(ident);
    if (sym.type == Symbol::TYPE_CONST) {
        auto raw = build_number(sym.int_value, nullptr);
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(raw);
        return insts;
    }

    auto raw = new koopa_raw_value_data_t;

    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_POINTER;
    auto raw_base = new koopa_raw_type_kind_t;
    raw_base->tag = KOOPA_RTT_INT32;
    ty->data.pointer.base = raw_base;

    raw->ty = ty;
    raw->name = nullptr;
    raw->kind.tag = KOOPA_RVT_LOAD;

    raw->used_by = {
        .buffer = nullptr,
        .len = 0,
        .kind = KOOPA_RSIK_VALUE,
    };
    
    raw->kind.data.load.src = sym.allocator;

    auto insts = new std::vector<koopa_raw_value_data*>();
    insts->push_back(raw);
    return insts;
}