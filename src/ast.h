#pragma once

#include <string>
#include <memory>
#include <iostream>
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

    virtual std::string toString() const = 0;
    virtual std::string toIRString() const = 0;
    virtual void* toRaw() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    std::string toString() const override;
    std::string toIRString() const override;
    void* toRaw() const override;
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    std::string toString() const override;
    std::string toIRString() const override;
    void* toRaw() const override;
};

class FuncTypeAST : public BaseAST
{
public:
    std::string type;

    std::string toString() const override;
    std::string toIRString() const override;
    void* toRaw() const override;
};

class BlockAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;

    std::string toString() const override;
    std::string toIRString() const override;
    void* toRaw() const override;
};

class StmtAST : public BaseAST
{
public:
    int INT_CONST;

    std::string toString() const override;
    std::string toIRString() const override;
    void* toRaw() const override;
};

class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> unary_exp;

    // std::string toString() const override;
    std::string toIRString() const override;
    void* toRaw() const override;
};

class PrimaryExpAST : public BaseAST
{
public:
    bool is_number;
    std::unique_ptr<BaseAST> exp;
    int number;

    // std::string toString() const override;
    std::string toIRString() const override;
    void* toRaw() const override;
};

class UnaryExpAST : public BaseAST
{
public:
    bool is_primary_exp;
    std::unique_ptr<BaseAST> primary_exp;
    char unaryop;
    std::unique_ptr<BaseAST> unary_exp; 

    // std::string toString() const override;
    std::string toIRString() const override;
    void* toRaw() const override;
};