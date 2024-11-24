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
        int value;
    };

    // Type str2type(const std::string &str);
    void insert(const std::string &ident, Type type, int value);
    std::pair<Type, int> query(const std::string &ident);
};
