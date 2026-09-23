#ifndef XTEE_DLL_H
#define XTEE_DLL_H

#include "xtee_string.h"

/* B = Builtin
 * N = Native
 * BN = Builtin then Native
 * NB = Native then Builtin */
enum XteeDllOverrideType {
        XTEE_DLL_OVERRIDE_B,
        XTEE_DLL_OVERRIDE_N,
        XTEE_DLL_OVERRIDE_BN,
        XTEE_DLL_OVERRIDE_NB,
        XTEE_DLL_OVERRIDE_DISABLE,
};

typedef struct XteeDllOverride {
        XteeString name;
        enum XteeDllOverrideType type;
} XteeDllOverride;

typedef struct XteeDllOverrideList {
        XteeDllOverride *block;
        size_t n;
        size_t capacity;
} XteeDllOverrideList;

void xtee_dll_add_override_to_list(XteeDllOverrideList *list,
                XteeConstString dll_name, enum XteeDllOverrideType type);
/* NULL is returned if list has no items */
char *xtee_dll_get_WINEDLLOVERRIDES_string(const XteeDllOverrideList *list);
void xtee_dll_free_list(XteeDllOverrideList *list);

#endif
