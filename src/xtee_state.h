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

#ifndef XTEE_STATE_H
#define XTEE_STATE_H

#include "xtee_prefix.h"
#include "xtee_wine.h"

extern bool xtee_state_is_prefix_loaded;

void xtee_state_free(void);

XteePrefix *xtee_state_get_loaded_prefix(void);
void xtee_state_set_loaded_prefix(const XteePrefix *val);

enum XteeGetProgramDataStatus xtee_state_set_loaded_program(
                const char *program_id);
XteeProgram *xtee_state_get_loaded_program(void);
void xtee_state_unload_program(void);
int xtee_state_save_loaded_program(void);

XteeWineVersionData *xtee_state_get_loaded_prefix_wine_version_data(void);

#endif
