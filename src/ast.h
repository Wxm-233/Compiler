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

// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;

    virtual std::string toString() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    std::string toString() const override;
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    std::string toString() const override;
};

class FuncTypeAST : public BaseAST
{
public:
    std::string type;

    std::string toString() const override;
};

class BlockAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;

    std::string toString() const override;
};

class StmtAST : public BaseAST
{
public:
    int INT_CONST;

    std::string toString() const override;
};