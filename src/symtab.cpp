#include "symtab.h"
#include <map>
#include <cassert>

namespace Symbol {
    std::map<std::string, std::pair<Type, int>> symtab;

    void insert(const std::string &ident, Type type, int value)
    {
        if (symtab.find(ident) != symtab.end())
            assert(false);
        symtab[ident] = {type, value};
    }
    
    std::pair<Type, int> query(const std::string &ident)
    {
        if (symtab.find(ident) == symtab.end())
            assert(false);
        return symtab[ident];
    }
}



