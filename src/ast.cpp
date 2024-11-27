#include "ast.h"
#include "symtab.h"

koopa_raw_value_data_t* BaseAST::build_number(int number, koopa_raw_value_data_t* user)
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

char* BaseAST::build_ident(const std::string& ident)
{
    auto name = new char[ident.size() + 2];
    name[0] = '@';
    std::copy(ident.begin(), ident.end(), name + 1);
    name[ident.size() + 1] = '\0';
    return name;
}

// 其实不需要写used_by，哈哈
void BaseAST::set_used_by(koopa_raw_value_data_t* value, koopa_raw_value_data_t* user)
{
    value->used_by.kind = KOOPA_RSIK_VALUE;
    value->used_by.len = 0;
}

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

    raw_function->name = build_ident(ident);

    raw_function->params.len = 0;
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
    auto insts_vec = new std::vector<koopa_raw_value_data*>();
    int index = 0;
    bool return_detected = false;
    for (auto &i : *block_item_list) {
        auto ret_vec = (std::vector<koopa_raw_value_data*>*)i->toRaw();
        if (ret_vec == nullptr)
            continue;
        for (auto &j : *ret_vec) {
            // 判断条件正确吗？
            if (j == nullptr || j->kind.tag != KOOPA_RVT_INTEGER) {
                insts_vec->push_back(j);
                index++;
            }
            if (j->kind.tag == KOOPA_RVT_RETURN) {
                return_detected = true;
                break;
            }
        }
        if (return_detected)
            break;
    }
    raw_block->insts.len = index;
    raw_block->insts.buffer = new const void*[raw_block->insts.len];
    index = 0;
    for (auto inst: *insts_vec) {
        raw_block->insts.buffer[index++] = inst;
    }

    return raw_block;
}

void *BlockItemAST::toRaw() const
{
    switch (type) {
        case DECL:
            return decl->toRaw();
        case STMT:
            return stmt->toRaw();
        default:
            assert(false);
    }
}

void *StmtAST::toRaw() const
{
    auto raw_stmt = new koopa_raw_value_data_t;

    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_INT32;
    raw_stmt->ty = ty;
    raw_stmt->name = nullptr;
    set_used_by(raw_stmt, nullptr);

    std::vector<koopa_raw_value_data*>* insts_exp;
    std::string ident;
    

    switch (type)
    {
        case LVAL_ASSIGN:
        {
            auto insts = new std::vector<koopa_raw_value_data*>();
            raw_stmt->kind.tag = KOOPA_RVT_STORE;

            ident = dynamic_cast<LValAST*>(lval.get())->ident;
            auto sym = Symbol::query(ident);
            assert(sym.type == Symbol::TYPE_VAR);
            auto raw_alloc = sym.allocator;

            insts_exp = (std::vector<koopa_raw_value_data*>*)exp->toRaw();
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
            return insts;
        }
        case RETURN:
        {   
            raw_stmt->kind.tag = KOOPA_RVT_RETURN;
            insts_exp = (std::vector<koopa_raw_value_data*>*)exp->toRaw();
            assert(insts_exp->size() > 0);
            auto value = *insts_exp->rbegin();
            set_used_by(value, raw_stmt);
            raw_stmt->kind.data.ret.value = value;
            insts_exp->push_back(raw_stmt);
            return insts_exp;
        }
        case EMPTY_RETURN:
        {
            auto insts = new std::vector<koopa_raw_value_data*>();
            raw_stmt->kind.tag = KOOPA_RVT_RETURN;
            raw_stmt->kind.data.ret.value = nullptr;
            insts->push_back(raw_stmt);
            return insts;
        }
        case BLOCK:
        {
            return block->toRaw();
        }
        case EXP:
        {
            return exp->toRaw();
        }
        case EMPTY:
        {
            return new std::vector<koopa_raw_value_data*>();
        }
        default:
            assert(false);
    }
}

void *ExpAST::toRaw() const
{
    switch (type)
    {
    case LOR:
        return lor_exp->toRaw();
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

void *PrimaryExpAST::toRaw() const
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

void *UnaryExpAST::toRaw() const
{
    if (type == NUMBER) {
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(build_number(number, nullptr));
        return insts;
    }
    if (type == PRIMARY) {
        return primary_exp->toRaw();
    }
    if (unaryop == '+') {
        return unary_exp->toRaw();
    }

    assert(type == UNARY);

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
        raw->kind.data.binary.op = KOOPA_RBO_EQ;
        break;
    default:
        assert(false);
    }

    auto zero = build_number(0, raw);

    raw->kind.data.binary.lhs = zero;

    auto insts = (std::vector<koopa_raw_value_data*>*)unary_exp->toRaw();
    assert(insts->size() > 0);
    auto value = *insts->rbegin();
    raw->kind.data.binary.rhs = value;
    set_used_by(value, raw);

    insts->push_back(raw);
    return insts;
}

void *MulExpAST::toRaw() const
{
    switch (type)
    {
    case UNARY:
        return unary_exp->toRaw();
    case NUMBER:
    {
        auto raw = build_number(number, nullptr);
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(raw);
        return insts;
    }
    case MUL:
    {
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

        auto left_insts = (std::vector<koopa_raw_value_data*>*)mul_exp->toRaw();
        assert(left_insts->size() > 0);
        auto left_value = *left_insts->rbegin();
        set_used_by(left_value, raw);
        raw->kind.data.binary.lhs = left_value;

        auto right_insts = (std::vector<koopa_raw_value_data*>*)unary_exp->toRaw();
        assert(right_insts->size() > 0);
        auto right_value = *right_insts->rbegin();
        set_used_by(right_value, raw);
        raw->kind.data.binary.rhs = right_value;
        
        for (auto inst : *right_insts) {
            left_insts->push_back(inst);
        }
        left_insts->push_back(raw);

        return left_insts;
    }
    default:
        assert(false);
    }
}

void *AddExpAST::toRaw() const
{
    switch (type) {
    case MUL:
        return mul_exp->toRaw();
    case NUMBER:
    {
        auto raw = build_number(number, nullptr);
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(raw);
        return insts;
    }
    case ADD:
    {
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

        auto left_insts = (std::vector<koopa_raw_value_data*>*)add_exp->toRaw();
        assert(left_insts->size() > 0);
        auto left_value = *left_insts->rbegin();
        set_used_by(left_value, raw);
        raw->kind.data.binary.lhs = left_value;

        auto right_insts = (std::vector<koopa_raw_value_data*>*)mul_exp->toRaw();
        assert(right_insts->size() > 0);
        auto right_value = *right_insts->rbegin();
        set_used_by(right_value, raw);
        raw->kind.data.binary.rhs = right_value;
        
        for (auto inst : *right_insts) {
            left_insts->push_back(inst);
        }
        left_insts->push_back(raw);

        return left_insts;
    }

    default:
        assert(false);
    }
}

void *RelExpAST::toRaw() const
{
    switch (type) {
    case ADD:
        return add_exp->toRaw();
    case NUMBER:
    {
        auto raw = build_number(number, nullptr);
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(raw);
        return insts;
    }
    case REL:
    {
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

        auto left_insts = (std::vector<koopa_raw_value_data*>*)rel_exp->toRaw();
        assert(left_insts->size() > 0);
        auto left_value = *left_insts->rbegin();
        set_used_by(left_value, raw);
        raw->kind.data.binary.lhs = left_value;

        auto right_insts = (std::vector<koopa_raw_value_data*>*)add_exp->toRaw();
        assert(right_insts->size() > 0);
        auto right_value = *right_insts->rbegin();
        set_used_by(right_value, raw);
        raw->kind.data.binary.rhs = right_value;
        
        for (auto inst : *right_insts) {
            left_insts->push_back(inst);
        }
        left_insts->push_back(raw);

        return left_insts;
    }
    default:
        assert(false);
    }
}

void *EqExpAST::toRaw() const
{
    switch (type) {
    case REL:
        return rel_exp->toRaw();
    case NUMBER:
    {
        auto raw = build_number(number, nullptr);
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(raw);
        return insts;
    }
    case EQ:
    {
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

        auto left_insts = (std::vector<koopa_raw_value_data*>*)eq_exp->toRaw();
        assert(left_insts->size() > 0);
        auto left_value = *left_insts->rbegin();
        set_used_by(left_value, raw);
        raw->kind.data.binary.lhs = left_value;

        auto right_insts = (std::vector<koopa_raw_value_data*>*)rel_exp->toRaw();
        assert(right_insts->size() > 0);
        auto right_value = *right_insts->rbegin();
        set_used_by(right_value, raw);
        raw->kind.data.binary.rhs = right_value;
        
        for (auto inst : *right_insts) {
            left_insts->push_back(inst);
        }
        left_insts->push_back(raw);

        return left_insts;
    }
    default:
        assert(false);
    }
}

void *LAndExpAST::toRaw() const // (x && y) --> (x != 0) & (y != 0)
{
    switch (type) {
    case EQ:
        return eq_exp->toRaw();
    case NUMBER:
    {
        auto raw = build_number(number, nullptr);
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(raw);
        return insts;
    }
    case LAND:
    {
        auto raw_exp = new koopa_raw_value_data_t;
        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_INT32;
        raw_exp->ty = ty;
        raw_exp->name = nullptr;
        raw_exp->kind.tag = KOOPA_RVT_BINARY;
        raw_exp->kind.data.binary.op = KOOPA_RBO_AND;

        auto left_insts = (std::vector<koopa_raw_value_data*>*)land_exp->toRaw();
        assert(left_insts->size() > 0);
        auto left_value = *left_insts->rbegin();

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

        auto right_insts = (std::vector<koopa_raw_value_data*>*)eq_exp->toRaw();
        assert(right_insts->size() > 0);
        auto right_value = *right_insts->rbegin();

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
    default:
        assert(false);
    }
}

void *LOrExpAST::toRaw() const // (x || y) --> (x|y != 0)
{
    switch (type) {
    case LAND:
        return land_exp->toRaw();
    case NUMBER:
    {
        auto raw = build_number(number, nullptr);
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(raw);
        return insts;
    }
    case LOR:
    {
        auto raw_exp = new koopa_raw_value_data_t;
        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_INT32;
        raw_exp->ty = ty;
        raw_exp->name = nullptr;
        raw_exp->kind.tag = KOOPA_RVT_BINARY;
        raw_exp->kind.data.binary.op = KOOPA_RBO_OR;

        auto left_insts = (std::vector<koopa_raw_value_data*>*)lor_exp->toRaw();
        assert(left_insts->size() > 0);
        auto left_value = *left_insts->rbegin();
        set_used_by(left_value, raw_exp);
        raw_exp->kind.data.binary.lhs = left_value;

        auto right_insts = (std::vector<koopa_raw_value_data*>*)land_exp->toRaw();
        assert(right_insts->size() > 0);
        auto right_value = *right_insts->rbegin();
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
    default:
        assert(false);
    }
}

void *DeclAST::toRaw() const
{
    switch (type)
    {
    case CONSTDECL:
        return const_decl->toRaw();
    case VARDECL:
        return var_decl->toRaw();
    default:
        assert(false);
    }
}

void *ConstDeclAST::toRaw() const
{
    return nullptr;
}

void *VarDeclAST::toRaw() const
{
    auto insts = new std::vector<koopa_raw_value_data*>();
    for (auto &i : *var_def_list) {
        auto ret_vec = (std::vector<koopa_raw_value_data*>*)i->toRaw();
        if (ret_vec == nullptr)
            continue;
        for (auto &j : *ret_vec) {
            insts->push_back(j);
        }
    }
    return insts;
}

void *ConstDefAST::toRaw() const
{
    return nullptr;
}

void *VarDefAST::toRaw() const
{
    auto insts = new std::vector<koopa_raw_value_data*>();

    // alloc
    auto raw_alloc = new koopa_raw_value_data_t;
    
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = KOOPA_RTT_POINTER;
    auto raw_base = new koopa_raw_type_kind_t;
    raw_base->tag = KOOPA_RTT_INT32;
    ty->data.pointer.base = raw_base;

    raw_alloc->ty = ty;
    raw_alloc->name = build_ident(ident);
    raw_alloc->kind.tag = KOOPA_RVT_ALLOC;

    insts->push_back(raw_alloc);

    // store
    if (has_init_val) {
        auto raw_store = new koopa_raw_value_data_t;

        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_INT32;

        raw_store->ty = ty;
        raw_store->name = build_ident(ident);
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

void *InitValAST::toRaw() const
{
    if (type == EXP) {
        return exp->toRaw();
    } else {
        auto raw = build_number(number, nullptr);
        auto insts = new std::vector<koopa_raw_value_data*>();
        insts->push_back(raw);
        return insts;
    }
}

void *LValAST::toRaw() const
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