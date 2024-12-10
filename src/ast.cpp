#include "ast.h"
#include "symtab.h"

koopa_raw_value_data_t* BaseAST::build_number(int number, koopa_raw_value_data_t* user=nullptr)
{
    auto num = new koopa_raw_value_data_t;
    auto num_ty = new koopa_raw_type_kind_t;
    num_ty->tag = KOOPA_RTT_INT32;
    num->ty = num_ty;

    num->name = nullptr;
    num->kind.tag = KOOPA_RVT_INTEGER;
    num->kind.data.integer.value = number;

    set_used_by(num, user);

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

// 其实不需要写used_by，哈哈，这个只是个充样子的函数
void BaseAST::set_used_by(koopa_raw_value_data_t* value, koopa_raw_value_data_t* user=nullptr)
{
    value->used_by.kind = KOOPA_RSIK_VALUE;
    value->used_by.len = 0;
    value->used_by.buffer = nullptr;
}

koopa_raw_basic_block_data_t* BaseAST::build_block_from_insts(std::vector<koopa_raw_value_data_t*>* insts, const char* block_name=nullptr)
{
    auto raw_block = new koopa_raw_basic_block_data_t;

    raw_block->name = block_name;

    raw_block->params.buffer = nullptr;
    raw_block->params.len = 0;
    raw_block->params.kind = KOOPA_RSIK_VALUE;

    raw_block->used_by.buffer = nullptr;
    raw_block->used_by.len = 0;
    raw_block->used_by.kind = KOOPA_RSIK_VALUE;

    raw_block->insts.kind = KOOPA_RSIK_VALUE;
    if (insts->size() == 0) {
        raw_block->insts.buffer = nullptr;
        raw_block->insts.len = 0;
        return raw_block;
    }
    raw_block->insts.len = insts->size();
    raw_block->insts.buffer = new const void*[raw_block->insts.len];
    for (int i = 0; i < raw_block->insts.len; i++) {
        raw_block->insts.buffer[i] = insts->at(i);
    }
    return raw_block;
}

koopa_raw_value_data_t* BaseAST::build_jump(koopa_raw_basic_block_t target, koopa_raw_value_data_t* user=nullptr, const char* name=nullptr)
{
    auto raw_jump = new koopa_raw_value_data_t;
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_UNIT;
    raw_jump->ty = ty;

    raw_jump->name = name;
    raw_jump->kind.tag = KOOPA_RVT_JUMP;
    raw_jump->kind.data.jump.target = target;
    raw_jump->kind.data.jump.args.buffer = nullptr;
    raw_jump->kind.data.jump.args.kind = KOOPA_RSIK_VALUE;
    raw_jump->kind.data.jump.args.len = 0;

    set_used_by(raw_jump, user);

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
        if (inst != nullptr && inst->kind.tag != KOOPA_RVT_INTEGER)
            insts->push_back(inst);
        if (inst->kind.tag == KOOPA_RVT_RETURN || inst->kind.tag == KOOPA_RVT_BRANCH || inst->kind.tag == KOOPA_RVT_JUMP)
            break;
    }
    bb->insts.len = insts->size();
    bb->insts.buffer = new const void*[bb->insts.len];
    for (int i = 0; i < bb->insts.len; i++) {
        bb->insts.buffer[i] = insts->at(i);
    }
}

koopa_raw_value_data_t* BaseAST::build_branch(koopa_raw_value_data* cond, koopa_raw_basic_block_data_t* true_bb, koopa_raw_basic_block_data_t* false_bb)
{
    auto raw_stmt = new koopa_raw_value_data_t;
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_INT32;
    raw_stmt->ty = ty;
    raw_stmt->name = nullptr;
    set_used_by(raw_stmt, nullptr);
    raw_stmt->kind.tag = KOOPA_RVT_BRANCH;
    set_used_by(cond, raw_stmt);

    raw_stmt->kind.data.branch.cond = cond;
    raw_stmt->kind.data.branch.true_bb = true_bb;
    raw_stmt->kind.data.branch.false_bb = false_bb;
    raw_stmt->kind.data.branch.false_args.buffer = nullptr;
    raw_stmt->kind.data.branch.false_args.len = 0;
    raw_stmt->kind.data.branch.false_args.kind = KOOPA_RSIK_VALUE;
    raw_stmt->kind.data.branch.true_args.buffer = nullptr;
    raw_stmt->kind.data.branch.true_args.len = 0;
    raw_stmt->kind.data.branch.true_args.kind = KOOPA_RSIK_VALUE;

    return raw_stmt;
}

void BaseAST::append_jump(std::vector<koopa_raw_basic_block_data_t*>* bbs, koopa_raw_value_data_t* jmp)
{
    auto end_bb = bbs->back();
    end_bb->insts.len += 1;
    auto buffer = new const void*[end_bb->insts.len];
    for (int i = 0; i < end_bb->insts.len - 1; i++) {
        buffer[i] = end_bb->insts.buffer[i];
    }
    buffer[end_bb->insts.len-1] = jmp;
    end_bb->insts.buffer = buffer;
}

void *CompUnitAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto raw_program = new koopa_raw_program_t;

    std::vector<koopa_raw_function_data_t*> funcs;
    std::vector<koopa_raw_value_data_t*> values;

    Symbol::enter_scope();

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
    raw_program->values.buffer = values.size() > 0 ? new const void*[raw_program->values.len] : nullptr;
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
    ty->data.function.params.buffer = nullptr;
    ty->data.function.params.len = 0;
    ty->data.function.params.kind = KOOPA_RSIK_TYPE;
    {
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
    }

    raw_function->ty = ty;

    raw_function->name = build_ident(ident, '@');

    raw_function->params.len = 0;
    raw_function->params.buffer = nullptr;
    raw_function->params.kind = KOOPA_RSIK_VALUE;

    Symbol::enter_scope();

    auto vec_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)block->toRaw(-1);
    raw_function->bbs.len = vec_bbs->size();
    raw_function->bbs.buffer = new const void*[raw_function->bbs.len];
    raw_function->bbs.kind = KOOPA_RSIK_BASIC_BLOCK;
    for (int i = 0; i < vec_bbs->size(); i++) {
        filter_basic_block(vec_bbs->at(i));
        raw_function->bbs.buffer[i] = vec_bbs->at(i);
    }
    auto last_bb = (koopa_raw_basic_block_data_t*)raw_function->bbs.buffer[raw_function->bbs.len-1];
    if (last_bb->insts.len == 0) {
        last_bb->insts.len = 1;
        last_bb->insts.buffer = new const void*[last_bb->insts.len];
        last_bb->insts.kind = KOOPA_RSIK_VALUE;
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
        last_bb->insts.buffer[0] = raw_ret;
    }

    Symbol::leave_scope();

    return raw_function;
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
    raw_basic_block->params.buffer = nullptr;
    raw_basic_block->params.len = 0;
    raw_basic_block->params.kind = KOOPA_RSIK_VALUE;
    raw_basic_block->used_by.buffer = nullptr;
    raw_basic_block->used_by.len = 0;
    raw_basic_block->used_by.kind = KOOPA_RSIK_VALUE;
    raw_basic_block->insts.kind = KOOPA_RSIK_VALUE;
    raw_basic_block->insts.buffer = nullptr;
    raw_basic_block->insts.len = 0;

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
        {
            auto insts = (std::vector<koopa_raw_value_data*>*)decl->toRaw();
            auto block = build_block_from_insts(insts, nullptr);
            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(block);
            return bbs_vec;
        }
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
            void **new_args;
            koopa_raw_basic_block_data_t* end_bb = build_block_from_insts(new std::vector<koopa_raw_value_data*>(), "%end");
            if (args != nullptr) { // 是某个stmt的子句
                new_args = args;
            } else { // 无从属关系
                new_args = new void*[2]{nullptr, nullptr};
            }
            
            auto true_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)stmt->toRaw(0, new_args);
            true_bbs->front()->name = "%then";
            auto raw_jmp = build_jump(end_bb);
            append_jump(true_bbs, raw_jmp);
            auto insts_exp = (std::vector<koopa_raw_value_data*>*)exp->toRaw();
            assert(insts_exp->size() > 0);
            auto value = insts_exp->back();
            auto raw_stmt = build_branch(value, true_bbs->front(), end_bb);
            insts_exp->push_back(raw_stmt);
            auto if_bb = build_block_from_insts(insts_exp, nullptr);

            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(if_bb);
            for (auto bb : *true_bbs) {
                bbs_vec->push_back(bb);
            }
            bbs_vec->push_back(end_bb);
            
            return bbs_vec;
        }
        case IF_ELSE:
        {
            void **new_args;
            koopa_raw_basic_block_data_t* end_bb = build_block_from_insts(new std::vector<koopa_raw_value_data*>(), "%end");
            if (args != nullptr) { // 是某个stmt的子句
                new_args = args;
            } else { // 无从属关系
                new_args = new void*[2]{nullptr, nullptr};
            }

            auto true_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)closed_stmt->toRaw(0, new_args);
            true_bbs->front()->name = "%then";
            auto false_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)open_stmt->toRaw(0, new_args);
            false_bbs->front()->name = "%else";
            auto raw_jmp = build_jump(end_bb);
            append_jump(true_bbs, raw_jmp);
            append_jump(false_bbs, raw_jmp);
            auto insts_exp = (std::vector<koopa_raw_value_data*>*)exp->toRaw();
            assert(insts_exp->size() > 0);
            auto value = insts_exp->back();
            auto raw_stmt = build_branch(value, true_bbs->front(), false_bbs->front());
            insts_exp->push_back(raw_stmt);
            auto if_bb = build_block_from_insts(insts_exp, nullptr);

            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(if_bb);
            for (auto bb : *true_bbs) {
                bbs_vec->push_back(bb);
            }
            for (auto bb : *false_bbs) {
                bbs_vec->push_back(bb);
            }
            bbs_vec->push_back(end_bb);

            return bbs_vec;
        }
        case WHILE:
        {
            koopa_raw_basic_block_data_t* end_bb = build_block_from_insts(new std::vector<koopa_raw_value_data*>(), "%end");

            auto insts_exp = (std::vector<koopa_raw_value_data*>*)exp->toRaw();
            assert(insts_exp->size() > 0);
            auto value = insts_exp->back();
            auto raw_branch = build_branch(value, nullptr, end_bb);
            insts_exp->push_back(raw_branch);
            auto entry_bb = build_block_from_insts(insts_exp, "%while_entry");
            void **new_args= new void*[2]{entry_bb, end_bb};
            auto true_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)open_stmt->toRaw(3, new_args);
            true_bbs->front()->name = "%while_body";
            auto raw_jmp = build_jump(entry_bb);
            append_jump(true_bbs, raw_jmp);
            raw_branch->kind.data.branch.true_bb = true_bbs->front();

            auto init_jmp = build_jump(entry_bb, nullptr);
            auto first_insts = new std::vector<koopa_raw_value_data*>();
            first_insts->push_back(init_jmp);
            auto first_bb = build_block_from_insts(first_insts, nullptr);

            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(first_bb);
            bbs_vec->push_back(entry_bb);
            for (auto bb : *true_bbs) {
                bbs_vec->push_back(bb);
            }
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
            void **new_args;
            koopa_raw_basic_block_data_t* end_bb = build_block_from_insts(new std::vector<koopa_raw_value_data*>(), "%end");
            if (args != nullptr) { // 是某个stmt的子句
                new_args = args;
            } else { // 无从属关系
                new_args = new void*[2]{nullptr, nullptr};
            }

            auto true_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)closed_stmt->toRaw(2, new_args);
            true_bbs->front()->name = "%then";
            auto false_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)closed_stmt2->toRaw(2, new_args);
            false_bbs->front()->name = "%else";
            auto raw_jmp = build_jump(end_bb);
            append_jump(true_bbs, raw_jmp);
            append_jump(false_bbs, raw_jmp);

            auto insts_exp = (std::vector<koopa_raw_value_data*>*)exp->toRaw();
            assert(insts_exp->size() > 0);
            auto value = insts_exp->back();
            auto raw_stmt = build_branch(value, true_bbs->front(), false_bbs->front());
            insts_exp->push_back(raw_stmt);
            auto if_bb = build_block_from_insts(insts_exp, nullptr);

            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(if_bb);
            for (auto bb : *true_bbs) {
                bbs_vec->push_back(bb);
            }
            for (auto bb : *false_bbs) {
                bbs_vec->push_back(bb);
            }
            bbs_vec->push_back(end_bb);

            return bbs_vec;
        }
        case WHILE:
        {
            koopa_raw_basic_block_data_t* end_bb = build_block_from_insts(new std::vector<koopa_raw_value_data*>(), "%end");


            auto insts_exp = (std::vector<koopa_raw_value_data*>*)exp->toRaw();
            assert(insts_exp->size() > 0);
            auto value = insts_exp->back();
            auto raw_branch = build_branch(value, nullptr, end_bb);
            insts_exp->push_back(raw_branch);
            auto entry_bb = build_block_from_insts(insts_exp, "%while_entry");

            void **new_args = new void*[2]{entry_bb, end_bb};
            auto true_bbs = (std::vector<koopa_raw_basic_block_data_t*>*)closed_stmt->toRaw(3, new_args);
            true_bbs->front()->name = "%while_body";
            auto raw_jmp = build_jump(entry_bb);
            append_jump(true_bbs, raw_jmp);
            raw_branch->kind.data.branch.true_bb = true_bbs->front();

            auto init_jmp = build_jump(entry_bb, nullptr);
            auto first_insts = new std::vector<koopa_raw_value_data*>();
            first_insts->push_back(init_jmp);
            auto first_bb = build_block_from_insts(first_insts, nullptr);

            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(first_bb);
            bbs_vec->push_back(entry_bb);
            for (auto bb : *true_bbs) {
                bbs_vec->push_back(bb);
            }
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
            set_used_by(raw_stmt, nullptr);
            auto insts = new std::vector<koopa_raw_value_data*>();
            raw_stmt->kind.tag = KOOPA_RVT_STORE;

            auto ident = dynamic_cast<LValAST*>(lval.get())->ident;
            auto sym = Symbol::query(ident);
            assert(sym.type == Symbol::TYPE_VAR);
            auto raw_alloc = sym.allocator;

            auto insts_exp = (std::vector<koopa_raw_value_data*>*)exp->toRaw();
            assert(insts_exp->size() > 0);
            auto value = *insts_exp->rbegin();
            set_used_by(value, raw_stmt);
            set_used_by(raw_alloc, raw_stmt);

            raw_stmt->kind.data.store.dest = raw_alloc;
            raw_stmt->kind.data.store.value = value;
            for (auto inst : *insts_exp) {
                insts->push_back(inst);
            }
            insts->push_back(raw_stmt);
            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(build_block_from_insts(insts, nullptr));
            return bbs_vec;
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
            auto insts_exp = (std::vector<koopa_raw_value_data*>*)exp->toRaw();
            assert(insts_exp->size() > 0);
            auto value = *insts_exp->rbegin();
            set_used_by(value, raw_stmt);
            raw_stmt->kind.data.ret.value = value;
            insts_exp->push_back(raw_stmt);
            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(build_block_from_insts(insts_exp, nullptr));
            return bbs_vec;
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
            auto insts = (std::vector<koopa_raw_value_data*>*)exp->toRaw();
            auto bbs_vec = new std::vector<koopa_raw_basic_block_data_t*>();
            bbs_vec->push_back(build_block_from_insts(insts, nullptr));
            return bbs_vec;
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
        return lval->toRaw();
    case NUMBER:
    {
        auto raw = build_number(number, nullptr);
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(raw);
        return insts;
    }
    default:
        assert(false);
    }
}

void *UnaryExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (type == PRIMARY) {
        return primary_exp->toRaw();
    }
    if (unaryop == '+') {
        return unary_exp->toRaw();
    }

    assert(type == UNARY);

    auto insts = (std::vector<koopa_raw_value_data*>*)unary_exp->toRaw();
    assert(insts->size() > 0);
    auto value = *insts->rbegin();
    
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
        insts->pop_back();
        insts->push_back(build_number(val, nullptr));
        return insts;
    }

    auto raw = new koopa_raw_value_data_t;
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_INT32;
    raw->ty = ty;
    raw->name = nullptr;

    raw->kind.tag = KOOPA_RVT_BINARY;
    raw->kind.data.binary.rhs = value;
    switch (unaryop) {
    case '-':
        raw->kind.data.binary.op = KOOPA_RBO_SUB;
        break;
    case '!':
        raw->kind.data.binary.op = KOOPA_RBO_EQ;
        break;
    default:
        assert(false);
    }

    auto zero = build_number(0, raw);
    raw->kind.data.binary.lhs = zero;
    set_used_by(value, raw);
    insts->push_back(raw);
    return insts;
}

void *MulExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (type == UNARY)
        return unary_exp->toRaw();

    assert(type == MUL);

    auto left_insts = (std::vector<koopa_raw_value_data*>*)mul_exp->toRaw();
    assert(left_insts->size() > 0);
    auto left_value = *left_insts->rbegin();

    auto right_insts = (std::vector<koopa_raw_value_data*>*)unary_exp->toRaw();
    assert(right_insts->size() > 0);
    auto right_value = *right_insts->rbegin();

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
        left_insts->pop_back();
        right_insts->pop_back();
        for (auto inst : *right_insts) {
            left_insts->push_back(inst);
        }
        left_insts->push_back(build_number(result, nullptr));
        return left_insts;
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

    set_used_by(left_value, raw);
    raw->kind.data.binary.lhs = left_value;
    set_used_by(right_value, raw);
    raw->kind.data.binary.rhs = right_value;
    
    for (auto inst : *right_insts) {
        left_insts->push_back(inst);
    }
    left_insts->push_back(raw);

    return left_insts;

}

void *AddExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (type == MUL)
        return mul_exp->toRaw();
    
    assert(type == ADD);

    auto left_insts = (std::vector<koopa_raw_value_data*>*)add_exp->toRaw();
    assert(left_insts->size() > 0);
    auto left_value = *left_insts->rbegin();

    auto right_insts = (std::vector<koopa_raw_value_data*>*)mul_exp->toRaw();
    assert(right_insts->size() > 0);
    auto right_value = *right_insts->rbegin();

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
        left_insts->pop_back();
        right_insts->pop_back();
        for (auto inst : *right_insts) {
            left_insts->push_back(inst);
        }
        left_insts->push_back(build_number(result, nullptr));
        return left_insts;
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

    set_used_by(left_value, raw);
    raw->kind.data.binary.lhs = left_value;
    set_used_by(right_value, raw);
    raw->kind.data.binary.rhs = right_value;

    for (auto inst : *right_insts) {
        left_insts->push_back(inst);
    }
    left_insts->push_back(raw);

    return left_insts;
}

void *RelExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (type == ADD)
        return add_exp->toRaw();
    
    assert(type == REL);

    auto left_insts = (std::vector<koopa_raw_value_data*>*)rel_exp->toRaw();
    assert(left_insts->size() > 0);
    auto left_value = *left_insts->rbegin();

    auto right_insts = (std::vector<koopa_raw_value_data*>*)add_exp->toRaw();
    assert(right_insts->size() > 0);
    auto right_value = *right_insts->rbegin();

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
        left_insts->pop_back();
        right_insts->pop_back();
        for (auto inst : *right_insts) {
            left_insts->push_back(inst);
        }
        left_insts->push_back(build_number(result, nullptr));
        return left_insts;
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

    set_used_by(left_value, raw);
    raw->kind.data.binary.lhs = left_value;
    set_used_by(right_value, raw);
    raw->kind.data.binary.rhs = right_value;

    for (auto inst : *right_insts) {
        left_insts->push_back(inst);
    }
    left_insts->push_back(raw);

    return left_insts;
}

void *EqExpAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    if (type == REL)
        return rel_exp->toRaw();
    
    assert(type == EQ);

    auto left_insts = (std::vector<koopa_raw_value_data*>*)eq_exp->toRaw();
    assert(left_insts->size() > 0);
    auto left_value = *left_insts->rbegin();

    auto right_insts = (std::vector<koopa_raw_value_data*>*)rel_exp->toRaw();
    assert(right_insts->size() > 0);
    auto right_value = *right_insts->rbegin();

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
        left_insts->pop_back();
        right_insts->pop_back();
        for (auto inst : *right_insts) {
            left_insts->push_back(inst);
        }
        left_insts->push_back(build_number(result, nullptr));
        return left_insts;
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

    set_used_by(left_value, raw);
    raw->kind.data.binary.lhs = left_value;
    set_used_by(right_value, raw);
    raw->kind.data.binary.rhs = right_value;

    for (auto inst : *right_insts) {
        left_insts->push_back(inst);
    }
    left_insts->push_back(raw);

    return left_insts;
}

void *LAndExpAST::toRaw(int n = 0, void* args[] = nullptr) const 
// (x && y) --> (x != 0) & (y != 0) (x)
// (x && y) --> if (x==0) return 0; else return y!=0, 短路求值
{
    if (type == EQ)
        return eq_exp->toRaw();
    
    assert(type == LAND);

    auto left_insts = (std::vector<koopa_raw_value_data*>*)land_exp->toRaw();
    assert(left_insts->size() > 0);
    auto left_value = *left_insts->rbegin();

    auto right_insts = (std::vector<koopa_raw_value_data*>*)eq_exp->toRaw();
    assert(right_insts->size() > 0);
    auto right_value = *right_insts->rbegin();

    if (left_value->kind.tag == KOOPA_RVT_INTEGER && right_value->kind.tag == KOOPA_RVT_INTEGER) {
        int left = left_value->kind.data.integer.value;
        int right = right_value->kind.data.integer.value;
        int result;
        result = left && right;
        left_insts->pop_back();
        right_insts->pop_back();
        for (auto inst : *right_insts) {
            left_insts->push_back(inst);
        }
        left_insts->push_back(build_number(result, nullptr));
        return left_insts;
    }

    auto raw_exp = new koopa_raw_value_data_t;
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_INT32;
    raw_exp->ty = ty;
    raw_exp->name = nullptr;
    raw_exp->kind.tag = KOOPA_RVT_BINARY;
    raw_exp->kind.data.binary.op = KOOPA_RBO_AND;

    auto raw_left = new koopa_raw_value_data_t;
    set_used_by(raw_left, raw_exp);
    auto ty_left = new koopa_raw_type_kind_t;

    ty_left->tag = KOOPA_RTT_INT32;
    raw_left->ty = ty_left;
    raw_left->name = nullptr;
    raw_left->kind.tag = KOOPA_RVT_BINARY;
    raw_left->kind.data.binary.op = KOOPA_RBO_NOT_EQ;
    
    auto zero_left = build_number(0, raw_left);
    raw_left->kind.data.binary.lhs = zero_left;
    raw_left->kind.data.binary.rhs = left_value;
    set_used_by(left_value, raw_left);
    left_insts->push_back(raw_left);

    raw_exp->kind.data.binary.lhs = raw_left;

    auto raw_right = new koopa_raw_value_data_t;
    set_used_by(raw_right, raw_exp);
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
    right_insts->push_back(raw_right);

    raw_exp->kind.data.binary.rhs = raw_right;

    for (auto inst : *right_insts) {
        left_insts->push_back(inst);
    }
    left_insts->push_back(raw_exp);
    
    return left_insts;
}

void *LOrExpAST::toRaw(int n = 0, void* args[] = nullptr) const  
// (x || y) --> (x|y != 0) (x)
// (x || y) --> if (x == 1) return 1; else return y;
{
    if (type == LAND)
        return land_exp->toRaw();

    assert(type == LOR);

    auto left_insts = (std::vector<koopa_raw_value_data*>*)lor_exp->toRaw();
    assert(left_insts->size() > 0);
    auto left_value = *left_insts->rbegin();

    auto right_insts = (std::vector<koopa_raw_value_data*>*)land_exp->toRaw();
    assert(right_insts->size() > 0);
    auto right_value = *right_insts->rbegin();

    if (left_value->kind.tag == KOOPA_RVT_INTEGER && right_value->kind.tag == KOOPA_RVT_INTEGER) {
        int left = left_value->kind.data.integer.value;
        int right = right_value->kind.data.integer.value;
        int result;
        result = left || right;
        left_insts->pop_back();
        right_insts->pop_back();
        for (auto inst : *right_insts) {
            left_insts->push_back(inst);
        }
        left_insts->push_back(build_number(result, nullptr));
        return left_insts;
    }

    auto raw_exp = new koopa_raw_value_data_t;
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_INT32;
    raw_exp->ty = ty;
    raw_exp->name = nullptr;
    raw_exp->kind.tag = KOOPA_RVT_BINARY;
    raw_exp->kind.data.binary.op = KOOPA_RBO_OR;

    set_used_by(left_value, raw_exp);
    raw_exp->kind.data.binary.lhs = left_value;
    set_used_by(right_value, raw_exp);
    raw_exp->kind.data.binary.rhs = right_value;
    
    for (auto inst : *right_insts) {
        left_insts->push_back(inst);
    }
    left_insts->push_back(raw_exp);
    {
        auto raw = new koopa_raw_value_data_t;
        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_INT32;
        raw->ty = ty;
        raw->name = nullptr;
        raw->kind.tag = KOOPA_RVT_BINARY;
        raw->kind.data.binary.op = KOOPA_RBO_NOT_EQ;

        auto zero = build_number(0, raw);
        raw->kind.data.binary.lhs = zero;
        set_used_by(raw_exp, raw);
        raw->kind.data.binary.rhs = raw_exp;

        left_insts->push_back(raw);
    }
    return left_insts;
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
    return new std::vector<koopa_raw_value_t>();
}

void *VarDeclAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto insts = new std::vector<koopa_raw_value_data*>();
    for (auto &i : *var_def_list) {
        auto ret_vec = (std::vector<koopa_raw_value_data*>*)i->toRaw(n, args);
        if (ret_vec == nullptr)
            continue;
        for (auto &j : *ret_vec) {
            insts->push_back(j);
        }
    }
    return insts;
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
        auto insts = new std::vector<koopa_raw_value_data*>();
        auto raw_global_alloc = new koopa_raw_value_data_t;
        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_POINTER;
        auto raw_base = new koopa_raw_type_kind_t;
        raw_base->tag = KOOPA_RTT_INT32;
        ty->data.pointer.base = raw_base;

        raw_global_alloc->ty = ty;
        raw_global_alloc->name = build_ident(ident, '%');
        raw_global_alloc->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
        if (has_init_val) {
            auto val_insts = (std::vector<koopa_raw_value_data_t*>*)init_val->toRaw();
            assert(val_insts->size() == 1);
            raw_global_alloc->kind.data.global_alloc.init = build_number(val_insts->at(0)->kind.data.integer.value);
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
        insts->push_back(raw_global_alloc);
        return insts;
    }
    auto insts = new std::vector<koopa_raw_value_data*>();

    // alloc
    auto raw_alloc = new koopa_raw_value_data_t;
    
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_POINTER;
    auto raw_base = new koopa_raw_type_kind_t;
    raw_base->tag = KOOPA_RTT_INT32;
    ty->data.pointer.base = raw_base;

    raw_alloc->ty = ty;
    raw_alloc->name = build_ident(ident, '%');
    raw_alloc->kind.tag = KOOPA_RVT_ALLOC;

    insts->push_back(raw_alloc);

    // store
    if (has_init_val) {
        auto raw_store = new koopa_raw_value_data_t;

        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_INT32;

        raw_store->ty = ty;
        raw_store->name = nullptr;
        raw_store->kind.tag = KOOPA_RVT_STORE;
        auto ret_insts = (std::vector<koopa_raw_value_data*>*)init_val->toRaw();
        assert(ret_insts->size() > 0);
        for (auto inst : *ret_insts) {
            insts->push_back(inst);
        }
        auto value = *ret_insts->rbegin();
        set_used_by(value, raw_store);
        set_used_by(raw_alloc, raw_store);
        raw_store->kind.data.store.value = value;
        raw_store->kind.data.store.dest = raw_alloc;
        insts->push_back(raw_store);
    }

    Symbol::insert(ident, Symbol::TYPE_VAR, raw_alloc);

    return insts;
}

void *ConstInitValAST::toRaw(int n = 0, void* args[] = nullptr) const
{
    auto insts = (std::vector<koopa_raw_value_data*>*)const_exp->toRaw();
    assert(insts->size() > 0);
    auto value = *insts->rbegin();
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

    auto raw_src = sym.allocator;
    
    raw->kind.data.load.src = raw_src;

    auto insts = new std::vector<koopa_raw_value_data*>();
    insts->push_back(raw);
    return insts;
}