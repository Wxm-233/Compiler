#pragma once

#include <string>
#include <cstring>
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
class SimpleStmtAST;
class ExpAST;
class PrimaryExpAST;
class UnaryExpAST;

// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;

    virtual void* toRaw(int n = 0, void* args[] = nullptr) const = 0;
    static koopa_raw_value_data_t* build_number(int number, koopa_raw_value_data_t* user);
    static char* build_ident(const std::string& ident, char c);
    static void set_used_by(koopa_raw_value_data_t* value, koopa_raw_value_data_t* user);
    static koopa_raw_basic_block_data_t* build_block_from_insts(std::vector<koopa_raw_value_data_t*>* insts, const char* block_name);
    static koopa_raw_value_data_t* build_jump(koopa_raw_basic_block_t target, koopa_raw_value_data_t* user, const char* name);
    static koopa_raw_slice_t combine_slices(koopa_raw_slice_t& s1, koopa_raw_slice_t& s2);
    static void filter_basic_block(koopa_raw_basic_block_data_t* bb);
    static koopa_raw_value_data_t* build_branch(koopa_raw_value_data* cond, koopa_raw_basic_block_data_t* true_bb, koopa_raw_basic_block_data_t* false_bb);
    static void append_jump(std::vector<koopa_raw_basic_block_data_t*>* bbs, koopa_raw_value_data_t* jmp);
};

class CompUnitAST : public BaseAST
{
public:
    std::vector<std::unique_ptr<BaseAST>>* global_def_list;

    void* toRaw(int n, void* args[]) const override;
};

class GlobalDefAST : public BaseAST
{
public:
    enum Type {
        FUNC_DEF,
        DECL
    } type;
    std::unique_ptr<BaseAST> func_def;
    std::unique_ptr<BaseAST> decl;

    void* toRaw(int n, void* args[]) const override;
};

class FuncDefAST : public BaseAST
{
public:
    std::string func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void* toRaw(int n, void* args[]) const override;
};

class FuncTypeAST : public BaseAST
{
public:
    std::string type;

    void* toRaw(int n, void* args[]) const override;
};

class BlockAST : public BaseAST
{
public:
    std::vector<std::unique_ptr<BaseAST>> *block_item_list;

    void* toRaw(int n, void* args[]) const override;
};

class BlockItemAST : public BaseAST
{
public:
    enum Type {
        DECL,
        STMT
    } type;
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;

    void* toRaw(int n, void* args[]) const override;
};

class StmtAST : public BaseAST
{
public:
    enum Type {
        OPEN_STMT,
        CLOSED_STMT
    } type;

    std::unique_ptr<BaseAST> open_stmt;
    std::unique_ptr<BaseAST> closed_stmt;

    void* toRaw(int n, void* args[]) const override;
};

class OpenStmtAST : public BaseAST
{
public:
    enum Type {
        IF,
        IF_ELSE,
        WHILE
    } type;

    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> stmt;
    std::unique_ptr<BaseAST> closed_stmt;
    std::unique_ptr<BaseAST> open_stmt;

    void* toRaw(int n, void* args[]) const override;
};

class ClosedStmtAST : public BaseAST
{
public:
    enum Type {
        SIMPLE_STMT,
        IF_ELSE,
        WHILE
    } type;

    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> simple_stmt;
    std::unique_ptr<BaseAST> closed_stmt;
    std::unique_ptr<BaseAST> closed_stmt2;

    void* toRaw(int n, void* args[]) const override;
};

class SimpleStmtAST : public BaseAST
{
public:
    enum Type {
        LVAL_ASSIGN,
        RETURN,
        EMPTY_RETURN,
        EMPTY,
        EXP,
        BLOCK,
        BREAK,
        CONTINUE
    } type;

    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> block;

    void* toRaw(int n, void* args[]) const override;
};

class ConstExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;

    void* toRaw(int n, void* args[]) const override;
};

class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;

    void* toRaw(int n, void* args[]) const override;
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

    void* toRaw(int n, void* args[]) const override;
};

class UnaryExpAST : public BaseAST
{
public:
    enum Type {
        PRIMARY,
        UNARY
    } type;

    std::unique_ptr<BaseAST> primary_exp;
    char unaryop;
    std::unique_ptr<BaseAST> unary_exp; 

    void* toRaw(int n, void* args[]) const override;
};

class MulExpAST : public BaseAST
{
public:
    enum Type
    {
        UNARY,
        MUL
    } type;
    std::unique_ptr<BaseAST> unary_exp;
    char op;
    std::unique_ptr<BaseAST> mul_exp;

    void* toRaw(int n, void* args[]) const override;
};

class AddExpAST : public BaseAST
{
public:
    enum Type
    {
        MUL,
        ADD
    } type;
    std::unique_ptr<BaseAST> mul_exp;
    char op;
    std::unique_ptr<BaseAST> add_exp;

    void* toRaw(int n, void* args[]) const override;
};

class RelExpAST : public BaseAST
{
public:
    enum Type
    {
        ADD,
        REL
    } type;
    std::unique_ptr<BaseAST> add_exp;
    std::string op;
    std::unique_ptr<BaseAST> rel_exp;

    void* toRaw(int n, void* args[]) const override;
};

class EqExpAST : public BaseAST
{
public:
    enum Type
    {
        REL,
        EQ
    } type;
    std::unique_ptr<BaseAST> rel_exp;
    std::string op;
    std::unique_ptr<BaseAST> eq_exp;

    void* toRaw(int n, void* args[]) const override;
};

class LAndExpAST : public BaseAST
{
public:
    enum Type
    {
        EQ,
        LAND
    } type;
    std::unique_ptr<BaseAST> eq_exp;
    std::unique_ptr<BaseAST> land_exp;

    void* toRaw(int n, void* args[]) const override;
};

class LOrExpAST : public BaseAST
{
public:
    enum Type
    {
        LAND,
        LOR
    } type;
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> lor_exp;

    void *toRaw(int n, void* args[]) const override;
};

class DeclAST : public BaseAST
{
public:
    enum Type
    {
        CONSTDECL,
        VARDECL
    } type;
    std::unique_ptr<BaseAST> const_decl;
    std::unique_ptr<BaseAST> var_decl;

    void* toRaw(int n, void* args[]) const override;
};

class ConstDeclAST : public BaseAST
{
public:
    std::string btype;
    std::vector<std::unique_ptr<BaseAST>> *const_def_list;

    void* toRaw(int n, void* args[]) const override;
};

class VarDeclAST : public BaseAST
{
public:
    std::string btype;
    std::vector<std::unique_ptr<BaseAST>> *var_def_list;

    void* toRaw(int n, void* args[]) const override;
};

class ConstDefAST : public BaseAST
{
public:
    std::string ident;
    std::unique_ptr<BaseAST> const_init_val;

    void* toRaw(int n, void* args[]) const override;
};

class VarDefAST : public BaseAST
{
public:
    bool has_init_val;
    std::string ident;
    std::unique_ptr<BaseAST> init_val;

    void* toRaw(int n, void* args[]) const override;
};

class ConstInitValAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> const_exp;

    void* toRaw(int n, void* args[]) const override;
};

class InitValAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void* toRaw(int n, void* args[]) const override;
};

class LValAST : public BaseAST
{
public:
    std::string ident;

    void* toRaw(int n, void* args[]) const override;
};

