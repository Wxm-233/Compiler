#pragma once

#include <string>
#include <stack>
#include <memory>

namespace Symbol {
    enum Type
    {
        TYPE_CONST,
        TYPE_VAR,
    };

    struct symbol_val {
        Type type;
        void* value; 
        // for const, value is the value of the constant
        // for var, value is the address of the alloc instruction
    };

    // Type str2type(const std::string &str);
    void insert(const std::string &ident, Type type, void* value);
    std::pair<Type, void*> query(const std::string &ident);
};
