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

#ifndef XTEE_WINE_H
#define XTEE_WINE_H

#include "xtee_prefix.h"

enum XteeWineVersionArch {
        XTEE_WINE_VERSION_ARCH_X86,
        XTEE_WINE_VERSION_ARCH_AMD64,
        XTEE_WINE_VERSION_ARCH_WOW64,
        XTEE_WINE_VERSION_ARCH_WRONG
};

typedef struct XteeWineVersionData {
        XteeString id;
        XteeString wine_path;
        XteeString wine_lib_path;
        XteeString wineboot_path;
        XteeString winecfg_path;
        XteeString wineserver_path;
        enum XteeWineVersionArch arch;
} XteeWineVersionData;

typedef struct XteeWineVersionDataBlock{
        XteeWineVersionData *block;
        size_t n;
} XteeWineVersionDataBlock;

extern XteeWineVersionDataBlock xtee_wine_versions_data;

/* Call this after xtee_system_init. */
void xtee_wine_init(void);
void xtee_wine_free(void);

XteeWineVersionData *xtee_wine_get_version_data(const char *id);
XteeStringList xtee_wine_get_versions_ids(void);

#endif
