#ifndef FM2_CSTRPTR_H
#define FM2_CSTRPTR_H

#include <memory>
#include <glib.h>

namespace Fm {

struct CStrDeleter {
    void operator()(char* ptr) const {
        g_free(ptr);
    }
};

// smart pointer for C string (char*) which should be freed by free()
typedef std::unique_ptr<char[], CStrDeleter> CStrPtr;

struct CStrHash {
    std::size_t operator()(const char* str) const {
        return g_str_hash(str);
    }
};

struct CStrEqual {
    bool operator()(const char* str1, const char* str2) const {
        return g_str_equal(str1, str2);
    }
};

struct CStrVDeleter {
    void operator()(char** ptr) const {
        g_strfreev(ptr);
    }
};

// smart pointer for C string array (char**) which should be freed by g_strfreev() of glib
typedef std::unique_ptr<char*[], CStrVDeleter> CStrArrayPtr;


} // namespace Fm

#endif // FM2_CSTRPTR_H
