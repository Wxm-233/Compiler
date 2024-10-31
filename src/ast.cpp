#include "ast.h"

std::string CompUnitAST::toString() const
{
    return "CompUnitAST { " + func_def->toString() + " } ";
}

std::string CompUnitAST::toIRString() const
{
    return func_def->toIRString();
}

std::string FuncDefAST::toString() const
{
    return "FuncDefAST { " + func_type->toString() + ", " + ident + ", " + block->toString() + " } ";
}

std::string FuncDefAST::toIRString() const
{
    return "fun @" + ident + "(): " + func_type->toIRString() + " {\n" + block->toIRString() + "}\n";
}

std::string FuncTypeAST::toString() const
{
    return "FuncTypeAST { " + type + " }";
}

std::string FuncTypeAST::toIRString() const
{
    if (type == "int") {
        return "i32";
    } else {
        return "void";
    }
}

std::string BlockAST::toString() const
{
    return "BlockAST { " + stmt->toString() + " } ";
}

std::string BlockAST::toIRString() const
{
    return "%entry:\n" + stmt->toIRString();
}

std::string StmtAST::toString() const
{
    return "StmtAST { " + std::to_string(INT_CONST) + " } ";
}

std::string StmtAST::toIRString() const
{
    return "    ret " + std::to_string(INT_CONST) + "\n";
}

std::string ExpAST::toIRString() const
{
    return unary_exp->toIRString();
}

std::string UnaryExpAST::toIRString() const
{
    if (is_primary_exp) {
        return primary_exp->toIRString();
    } else {
        switch (unaryop) {
            case '+':
                return unary_exp->toIRString();
            case '-':
                return "    %sub = sub 0, " + unary_exp->toIRString() + "\n";
        }
    }
}