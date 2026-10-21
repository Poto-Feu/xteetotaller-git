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

#include "xtee_state.h"

bool xtee_state_is_prefix_loaded = false;

static bool is_program_loaded = false;
static XteePrefix loaded_prefix = { 0 };
static XteeProgram *loaded_program = NULL;

void
xtee_state_free(void)
{
        if (xtee_state_is_prefix_loaded) {
                xtee_free_prefix_struct(&loaded_prefix);
        }
}

XteePrefix *
xtee_state_get_loaded_prefix(void)
{
        return &loaded_prefix;
}

void
xtee_state_set_loaded_prefix(const XteePrefix *val)
{
        if (xtee_state_is_prefix_loaded) {
                print_info("Clearing previous loaded prefix.");
                xtee_free_prefix_struct(&loaded_prefix);
        }
        loaded_prefix = *val;
        xtee_state_is_prefix_loaded = true;
}

enum XteeGetProgramDataStatus
xtee_state_set_loaded_program(const char *program_id)
{
        const char *info_strs[4];
        enum XteeGetProgramDataStatus status;

        info_strs[0] = "Loading \"";
        info_strs[1] = program_id;
        info_strs[2] = "\" program.";
        info_strs[3] = NULL;
        print_info_strs(info_strs);
        if (loaded_program) {
                xtee_free_program(loaded_program);
        }
        free(loaded_program);
        loaded_program = xtee_prefix_get_program_data(&loaded_prefix,
                        program_id, &status);
        if (!loaded_program) {
                return status;
        } else {
                is_program_loaded = true;
        }
        return XTEE_GET_PROGRAM_DATA_OK;
}

XteeProgram *
xtee_state_get_loaded_program(void)
{
        if (!is_program_loaded) {
                return NULL;
        }
        return loaded_program;
}

void
xtee_state_unload_program(void)
{
        if (is_program_loaded) {
                print_info("Unloading program");
                xtee_free_program(loaded_program);
                free(loaded_program);
                is_program_loaded = false;
        }
}

int
xtee_state_save_loaded_program(void)
{
        print_info("Saving loaded program");
        return xtee_prefix_save_program(&loaded_prefix, loaded_program);
}

XteeWineVersionData *
xtee_state_get_loaded_prefix_wine_version_data(void)
{
        return xtee_wine_get_version_data(loaded_prefix.wine_version.s);
}
