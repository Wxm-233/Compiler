#include "symtab.h"
#include <map>
#include <cassert>

namespace Symbol {
    std::map<std::string, symbol_val> symtab;

    void insert(const std::string &ident, Type type, int int_value)
    {
        if (symtab.find(ident) != symtab.end())
            assert(false);
        symtab[ident] = {type, int_value, nullptr};
    }
    
    void insert(const std::string &ident, Type type, koopa_raw_value_data_t* allocator)
    {
        if (symtab.find(ident) != symtab.end())
            assert(false);
        symtab[ident] = {type, 0, allocator};
    }

    symbol_val query(const std::string &ident)
    {
        if (symtab.find(ident) == symtab.end())
            return {TYPE_VAR, 0, nullptr};
        return symtab[ident];
    }
}



