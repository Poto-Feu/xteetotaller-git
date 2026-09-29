/*
 * Copyright (C) 2026 Adrien Saad
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

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
