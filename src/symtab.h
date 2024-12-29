#pragma once

#include <string>
#include <stack>
#include <memory>
#include <vector>
#include <koopa.h>

namespace Symbol {
    enum Type
    {
        TYPE_CONST,
        TYPE_VAR,
        TYPE_FUNCTION,
        TYPE_ARRAY,
    };

    struct symbol_val {
        Type type;
        int int_value;
        koopa_raw_value_data_t* allocator;
        koopa_raw_function_data_t* function;
        std::vector<int>* dim_vec;
    };

    void insert(const std::string &ident, Type type, int int_value);
    void insert(const std::string &ident, Type type, koopa_raw_value_data_t* allocator);
    void insert(const std::string &ident, Type type, koopa_raw_function_data_t* function);
    void insert(const std::string &ident, Type type, koopa_raw_value_data_t* allocator, std::vector<int>* dim_vec);
    symbol_val query(const std::string &ident);
    void enter_scope();
    void leave_scope();
    void enter_loop(koopa_raw_basic_block_data_t* loop_header, koopa_raw_basic_block_data_t* loop_exit);
    void leave_loop();
    koopa_raw_basic_block_data_t* get_loop_header();
    koopa_raw_basic_block_data_t* get_loop_exit();
};

// namespace ConstSymbol {
//     void insert(const std::string &ident, int int_value);
//     int query(const std::string &ident);

//     void enter_scope();
//     void exit_scope();
// }
