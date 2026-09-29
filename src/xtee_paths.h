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

#ifndef XTEE_PATHS_H
#define XTEE_PATHS_H

#include "xtee_string.h"

/* This function uses the same allocated string as a return value in any call.
 * This means that the returned string must not be freed except at the end of
 * the program with free_xteetotaller_dir_path.
 * Please note that the returned string will have a trailing slash so there is
 * no need to add a directory separator at the end, even though it will only
 * save one byte. */
XteeConstString get_xteetotaller_dir_path(void);

void free_xteetotaller_dir_path(void);

#endif
