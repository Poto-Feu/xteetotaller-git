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

#include "xtee_paths.h"
#include "xtee_error.h"

#define TEETOTALLER_DIR_NAME ".xteetotaller"

static XteeString xteetotaller_dir_path = { 0 };
static bool is_xteetotaller_dir_path_ready = false;
static bool is_data_dir_env_set = false;

static const char *
get_xteetotaller_dir_path_start(void)
{
        const char *env_val;

        if ((env_val = getenv("XTEE_DATA_DIR"))) {
                is_data_dir_env_set = true;
        } else if((env_val = getenv("HOME"))) {
                is_data_dir_env_set = false;
        } else {
                fatal_error("Cannot get HOME environment variable value.");
        }

        return env_val;
}

static void
alloc_xteetotaller_dir_path(void)
{
        const char *env_val;
        char *dir_path;
        struct XteeString dir_path_xstr;
        size_t env_val_len, dir_path_len, dir_name_len;

        dir_name_len = sizeof(TEETOTALLER_DIR_NAME) - 1;
        /* get_xteetotaller_dir_path_start never returns NULL since it crashes
         * on error. */
        env_val = get_xteetotaller_dir_path_start();
        env_val_len = strlen(env_val);
        if (is_data_dir_env_set) {
                dir_path_len = env_val_len + 1;
                dir_path = malloc_or_exit(dir_path_len + 1);
                memcpy(dir_path, env_val, env_val_len);
        } else {
                /* Adding 2 bytes for the directory separators after $HOME and
                 * after ".xteetotaller" */
                dir_path_len = env_val_len + dir_name_len + 2;

                dir_path = malloc_or_exit(dir_path_len + 1);
                memcpy(dir_path, env_val, env_val_len);
                dir_path[env_val_len] = '/';
                memcpy(dir_path + env_val_len + 1, TEETOTALLER_DIR_NAME,
                                dir_name_len);
        }
        dir_path[dir_path_len - 1] = '/';
        dir_path[dir_path_len] = '\0';
        dir_path_xstr = move_string_to_XteeString(dir_path);
        xteetotaller_dir_path = dir_path_xstr;
        is_xteetotaller_dir_path_ready = true;
}

XteeConstString
get_xteetotaller_dir_path(void)
{
        if (!is_xteetotaller_dir_path_ready) {
                alloc_xteetotaller_dir_path();
        }
        return XteeString_to_XteeConstString(xteetotaller_dir_path);
}

void
free_xteetotaller_dir_path(void)
{
        if (!is_xteetotaller_dir_path_ready) {
                return;
        }
        XteeString_free(xteetotaller_dir_path);
}
