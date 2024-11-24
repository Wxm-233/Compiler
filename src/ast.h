#pragma once

#include <string>
#include <memory>
#include <iostream>
#include <cassert>
#include <vector>
#include "koopa.h"

class BaseAST;
class CompUnitAST;
class FuncDefAST;
class FuncTypeAST;
class BlockAST;
class StmtAST;
class ExpAST;
class PrimaryExpAST;
class UnaryExpAST;

// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;

    virtual void* toRaw() const = 0;

    static koopa_raw_value_data_t* make_zero(koopa_raw_value_data_t* raw);
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    void* toRaw() const override;
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void* toRaw() const override;
};

class FuncTypeAST : public BaseAST
{
public:
    std::string type;

    void* toRaw() const override;
};

class BlockAST : public BaseAST
{
public:
    std::vector<std::unique_ptr<BaseAST>> *block_item_list;

    void* toRaw() const override;
};

class BlockItemAST : public BaseAST
{
public:
    bool is_decl;
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;

    void* toRaw() const override;
};

class StmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;

    void* toRaw() const override;
};

class ConstExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;

    void* toRaw() const override;
};

class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;

    void* toRaw() const override;
};

class PrimaryExpAST : public BaseAST
{
public:
    enum Type
    {
        EXP,
        LVAL,
        NUMBER
    } type;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> lval;
    int number;

    void* toRaw() const override;
};

class UnaryExpAST : public BaseAST
{
public:
    bool is_primary_exp;
    std::unique_ptr<BaseAST> primary_exp;
    char unaryop;
    std::unique_ptr<BaseAST> unary_exp; 

    void* toRaw() const override;
};

class MulExpAST : public BaseAST
{
public:
    bool is_unary_exp;
    std::unique_ptr<BaseAST> unary_exp;
    char op;
    std::unique_ptr<BaseAST> mul_exp;

    void* toRaw() const override;
};

class AddExpAST : public BaseAST
{
public:
    bool is_mul_exp;
    std::unique_ptr<BaseAST> mul_exp;
    char op;
    std::unique_ptr<BaseAST> add_exp;

    void* toRaw() const override;
};

class RelExpAST : public BaseAST
{
public:
    bool is_add_exp;
    std::unique_ptr<BaseAST> add_exp;
    std::string op;
    std::unique_ptr<BaseAST> rel_exp;

    void* toRaw() const override;
};

class EqExpAST : public BaseAST
{
public:
    bool is_rel_exp;
    std::unique_ptr<BaseAST> rel_exp;
    std::string op;
    std::unique_ptr<BaseAST> eq_exp;

    void* toRaw() const override;
};

class LAndExpAST : public BaseAST
{
public:
    bool is_eq_exp;
    std::unique_ptr<BaseAST> eq_exp;
    std::unique_ptr<BaseAST> land_exp;

    void* toRaw() const override;
};

class LOrExpAST : public BaseAST
{
public:
    bool is_land_exp;
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> lor_exp;

    void *toRaw() const override;
};

class DeclAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> const_decl;

    void* toRaw() const override;
};

class ConstDeclAST : public BaseAST
{
public:
    std::string btype;
    std::vector<std::unique_ptr<BaseAST>> *const_def_list;

    void* toRaw() const override;
};

class ConstDefAST : public BaseAST
{
public:
    std::string ident;
    std::unique_ptr<BaseAST> const_init_val;

    void* toRaw() const override;
};

class ConstInitValAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> const_exp;

    void* toRaw() const override;
};

class LValAST : public BaseAST
{
public:
    std::string ident;

    void* toRaw() const override;
};

