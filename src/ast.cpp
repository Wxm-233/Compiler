#include "ast.h"

std::string BaseAST::toString() const
{
    return "";
    // BaseAST does not have any specific content to string
}

std::string CompUnitAST::toString() const
{
    return "CompUnitAST { " + func_def->toString() + " } ";
}

std::string FuncDefAST::toString() const
{
    return "FuncDefAST { " + func_type->toString() + ", " + ident + ", " + block->toString() + " } ";
}

std::string FuncTypeAST::toString() const
{
    return "FuncTypeAST { " + type + " }";
}

std::string BlockAST::toString() const
{
    return "BlockAST { " + stmt->toString() + " } ";
}

std::string StmtAST::toString() const
{
    return "StmtAST { " + std::to_string(INT_CONST) + " } ";
}