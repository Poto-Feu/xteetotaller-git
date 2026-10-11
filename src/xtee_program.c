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

#include "xtee_program.h"
#include "xtee_file.h"
#include "xtee_prefix.h"
#include "xtee_utils.h"

void
xtee_init_program(XteeProgram *program, const char* id, const char *exe_path)
{
        program->id = XteeString_create_from_charstring(id);
        program->exe_path = XteeString_create_from_charstring(exe_path);
        program->overrides = NULL;
        program->overrides_n = 0;
        program->is_x11_display_set = false;
        program->x11_display_str.s = NULL;
        program->x11_display_str.n = 0;
        program->is_damavand_on = false;
        program->is_force_vsync_on = false;
        xtee_date_set_to_current_day(&program->creation_date);
        init_XteeStringList(&program->args_list);
}

void
xtee_free_program(XteeProgram *program)
{
        XteeString_free(program->id);
        XteeString_free(program->exe_path);
        XteeStringList_free(program->args_list);
        xtee_env_data_free_handler(&program->env_data);

        if (program->is_x11_display_set) {
                XteeString_free(program->x11_display_str);
        }
}

void
xtee_program_add_argument(XteeProgram *restrict program,
                const char *restrict arg_str)
{
        print_info("Adding program command line argument.");
        xtee_add_charstring_to_XteeStringList(&program->args_list, arg_str);
}

void
xtee_program_remove_argument(XteeProgram *program, size_t i)
{
        print_info("Removing program command line argument.");
        xtee_remove_string_in_XteeStringList(&program->args_list, i);
}

void
xtee_program_add_env_var(XteeProgram *restrict program,
                const char *restrict env_name,
                const char *restrict env_value)
{
        xtee_env_data_add_var_to_handler(&program->env_data,
                        env_name, env_value);
}

int
xtee_program_set_executable_path(XteeProgram *program,
                const char *exe_path, char **error_str)
{
        XteeString exe_path_xteestr = { 0 };
        XteeConstString exe_path_conststr = { 0 };

        if (!exe_path || exe_path[0] == '\0') {
                *error_str = "Program executable path is empty.";
                return -1;
        }
        exe_path_conststr.s = exe_path;
        exe_path_conststr.n = strlen(exe_path);

        if (exe_path_conststr.n > XTEE_PROGRAM_EXE_PATH_MAX_LEN) {
                *error_str = "Program executable path is too long.";
                return -1;
        }

        if (!xtee_is_file_msi(exe_path_conststr)) {
                enum XteeExecutableType exe_type;

                if (xtee_get_executable_type(&exe_type, exe_path_conststr)
                                != 0) {
                        *error_str = "Cannot read executable file.";
                        return -1;
                } else if (exe_type == XTEE_EXECUTABLE_UNKNOWN) {
                        *error_str = "Incorrect executable format.";
                        return -1;
                }
        }
        exe_path_xteestr.s = xtee_strdup_or_exit(exe_path);
        exe_path_xteestr.n = exe_path_conststr.n;

        XteeString_free(program->exe_path);
        program->exe_path = exe_path_xteestr;
        return 0;
}
