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

#ifndef XTEE_SYSTEM_H
#define XTEE_SYSTEM_H

#include "xtee_string.h"

extern bool xtee_system_is_wine_installed;
extern bool xtee_system_is_winetricks_installed;
extern XteeConstString xtee_system_wine_path;
extern XteeConstString xtee_system_wine_lib_path;
extern XteeConstString xtee_system_wineboot_path;
extern XteeConstString xtee_system_winecfg_path;
extern XteeConstString xtee_system_wineserver_path;
extern XteeString xtee_system_winetricks_path;

void xtee_system_init(void);
bool xtee_system_is_amd64(void);

#endif
