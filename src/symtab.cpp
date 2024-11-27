#include "symtab.h"
#include <map>
#include <cassert>

namespace Symbol {
    std::map<std::string, std::pair<Type, void*>> symtab;

    void insert(const std::string &ident, Type type, void* value)
    {
        if (symtab.find(ident) != symtab.end())
            assert(false);
        symtab[ident] = {type, value};
    }
    
    std::pair<Type, void*> query(const std::string &ident)
    {
        if (symtab.find(ident) == symtab.end())
            return std::pair<Type, void*>(TYPE_VAR, 0);
        return symtab[ident];
    }
}



