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

#ifndef XTEE_DXVK_H
#define XTEE_DXVK_H

#include "xtee_string.h"

typedef struct XteeDxvkVersionData {
        XteeStringList x32_dll_strs;
        XteeStringList x64_dll_strs;
        XteeString id;
        XteeString x32_path;
        XteeString x64_path;
} XteeDxvkVersionData;

void xtee_dxvk_init(void);
void xtee_dxvk_free(void);

XteeStringList xtee_dxvk_get_ids(void);
XteeDxvkVersionData *xtee_dxvk_get_data(const char *id);

#endif
