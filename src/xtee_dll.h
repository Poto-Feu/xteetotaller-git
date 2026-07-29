#ifndef XTEE_DLL_H
#define XTEE_DLL_H

#include "xtee_string.h"

/* B = Builtin
 * N = Native
 * BN = Builtin then Native
 * NB = Native then Builtin */
enum XteeDllOverrideType {
        XTEE_DLL_OVERRIDE_NONE = 0,
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

#endif
