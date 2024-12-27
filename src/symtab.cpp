#include "symtab.h"
#include <map>
#include <vector>
#include <cassert>

namespace Symbol {
    int scope_level = -1;
    std::vector<std::map<std::string, symbol_val>> symtab_stack;
    std::stack<std::pair<koopa_raw_basic_block_data_t*, koopa_raw_basic_block_data_t*>> loop_stack;

    void insert(const std::string &ident, Type type, int int_value)
    {
        if (symtab_stack[scope_level].find(ident) != symtab_stack[scope_level].end())
            assert(false);
        symtab_stack[scope_level][ident] = {type, int_value, nullptr, nullptr};
    }
    
    void insert(const std::string &ident, Type type, koopa_raw_value_data_t* allocator)
    {
        if (symtab_stack[scope_level].find(ident) != symtab_stack[scope_level].end())
            assert(false);
        symtab_stack[scope_level][ident] = {type, 0, allocator, nullptr};
    }

    void insert(const std::string &ident, Type type, koopa_raw_function_data_t* function)
    {
        if (symtab_stack[scope_level].find(ident) != symtab_stack[scope_level].end())
            assert(false);
        symtab_stack[scope_level][ident] = {type, 0, nullptr, function};
    }

    symbol_val query(const std::string &ident)
    {
        for (int i = scope_level; i >= 0; i--)
        {
            if (symtab_stack[i].find(ident) != symtab_stack[i].end())
                return symtab_stack[i][ident];
        }
        return {TYPE_VAR, 0, nullptr, nullptr};
    }

    void enter_scope()
    {
        symtab_stack.push_back(std::map<std::string, symbol_val>());
        scope_level++;
    }

    void leave_scope()
    {
        symtab_stack.pop_back();
        scope_level--;
    }

    void enter_loop(koopa_raw_basic_block_data_t* loop_header, koopa_raw_basic_block_data_t* loop_exit)
    {
        loop_stack.push({loop_header, loop_exit});
    }
    void leave_loop()
    {
        loop_stack.pop();
    }
    koopa_raw_basic_block_data_t* get_loop_header()
    {
        return loop_stack.top().first;
    }
    koopa_raw_basic_block_data_t* get_loop_exit()
    {
        return loop_stack.top().second;
    }
}

// namespace ConstSymbol {
//     int scope_level = 0;
//     std::vector<std::map<std::string, int>> symtab_stack;

//     void insert(const std::string &ident, int int_value)
//     {
//         if (symtab_stack[scope_level].find(ident) != symtab_stack[scope_level].end())
//             assert(false);
//         symtab_stack[scope_level][ident] = int_value;
//     }

//     int query(const std::string &ident)
//     {
//         for (int i = scope_level; i >= 0; i--)
//         {
//             if (symtab_stack[i].find(ident) != symtab_stack[i].end())
//                 return symtab_stack[i][ident];
//         }
//         return 0;
//     }
// }

