#pragma once

#include <string>
#include <stack>
#include <memory>
#include <koopa.h>

namespace Symbol {
    enum Type
    {
        TYPE_CONST,
        TYPE_VAR,
    };

    struct symbol_val {
        Type type;
        int int_value;
        koopa_raw_value_data_t* allocator;
    };

    void insert(const std::string &ident, Type type, int int_value);
    void insert(const std::string &ident, Type type, koopa_raw_value_data_t* allocator);
    symbol_val query(const std::string &ident);
};
