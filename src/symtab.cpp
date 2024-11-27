#include "symtab.h"
#include <map>
#include <vector>
#include <cassert>

namespace Symbol {
    int scope_level = -1;
    std::vector<std::map<std::string, symbol_val>> symtab_stack;

    void insert(const std::string &ident, Type type, int int_value)
    {
        if (symtab_stack[scope_level].find(ident) != symtab_stack[scope_level].end())
            assert(false);
        symtab_stack[scope_level][ident] = {type, int_value, nullptr};
    }
    
    void insert(const std::string &ident, Type type, koopa_raw_value_data_t* allocator)
    {
        if (symtab_stack[scope_level].find(ident) != symtab_stack[scope_level].end())
            assert(false);
        symtab_stack[scope_level][ident] = {type, 0, allocator};
    }

    symbol_val query(const std::string &ident)
    {
        for (int i = scope_level; i >= 0; i--)
        {
            if (symtab_stack[i].find(ident) != symtab_stack[i].end())
                return symtab_stack[i][ident];
        }
        return {TYPE_VAR, 0, nullptr};
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

