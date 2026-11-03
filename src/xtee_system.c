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

#include "xtee_system.h"
#include "xtee_error.h"
#include "xtee_utils.h"

#include <sys/stat.h>
#include <stdint.h>
#include <unistd.h>

#if INTPTR_MAX == INT64_MAX
#define XTEE_SYSTEM_AMD64
#elif INTPTR_MAX == INT32_MAX
#define XTEE_SYSTEM_IA32
#else
#error System architecture detection failed.
#endif

bool xtee_system_is_wine_installed = false;
bool xtee_system_is_winetricks_installed = false;
XteeConstString xtee_system_wine_path = { 0 };
XteeConstString xtee_system_wine_lib_path = { 0 };
XteeConstString xtee_system_wineboot_path = { 0 };
XteeConstString xtee_system_winecfg_path = { 0 };
XteeConstString xtee_system_wineserver_path = { 0 };
XteeString xtee_system_winetricks_path = { 0 };

static bool xtee_system_is_wine_lib_present = false;
static bool xtee_system_is_prefix_usr_local = false;

static const char *usr_wine_lib_paths[] = {
        "/usr/lib64/wine/",
        "/usr/lib/wine/",
};

static const char *usr_local_wine_lib_paths[]  = {
        "/usr/local/lib64/wine/",
        "/usr/local/lib/wine/",
};

static void
set_wine_lib_directory(void)
{
        const char **lib_paths;
        size_t lib_paths_n;

        if (xtee_system_is_prefix_usr_local) {
                lib_paths = usr_local_wine_lib_paths;
                lib_paths_n = XTEE_ARRAY_LEN(usr_local_wine_lib_paths);
        } else {
                lib_paths = usr_wine_lib_paths;
                lib_paths_n = XTEE_ARRAY_LEN(usr_wine_lib_paths);
        }

        for (size_t i = 0; i < lib_paths_n && !xtee_system_is_wine_lib_present;
                        ++i) {
                struct stat buf;
                const char *current_path;

                current_path = lib_paths[i];
                if (stat(current_path, &buf) == 0 && S_ISDIR(buf.st_mode)) {
                        xtee_system_wine_lib_path.s = current_path;
                        xtee_system_wine_lib_path.n = strlen(current_path);
                        xtee_system_is_wine_lib_present = true;
                }
        }

        if (!xtee_system_is_wine_lib_present) {
                print_warning("Wine lib directory not found.");
                xtee_system_is_wine_installed = false;
        }
}

static void
set_wineboot_path(void)
{
        const char *bin_wineboot_path = "/usr/bin/wineboot";
        const char *local_bin_wineboot_path = "/usr/local/bin/wineboot";

        if (xtee_system_is_prefix_usr_local) {
                xtee_system_wineboot_path.s = local_bin_wineboot_path;
                xtee_system_wineboot_path.n = strlen(local_bin_wineboot_path);
        } else {
                xtee_system_wineboot_path.s = bin_wineboot_path;
                xtee_system_wineboot_path.n = strlen(bin_wineboot_path);
        }
}

static void
set_winecfg_path(void)
{
        const char *bin_winecfg_path = "/usr/bin/winecfg";
        const char *local_bin_winecfg_path = "/usr/local/bin/winecfg";

        if (xtee_system_is_prefix_usr_local) {
                xtee_system_winecfg_path.s = local_bin_winecfg_path;
                xtee_system_winecfg_path.n = strlen(local_bin_winecfg_path);
        } else {
                xtee_system_winecfg_path.s = bin_winecfg_path;
                xtee_system_winecfg_path.n = strlen(bin_winecfg_path);
        }
}

static void
set_wineserver_path(void)
{
        const char *bin_wineserver_path = "/usr/bin/wineserver";
        const char *local_bin_wineserver_path = "/usr/local/bin/wineserver";

        if (xtee_system_is_prefix_usr_local) {
                xtee_system_wineserver_path.s = local_bin_wineserver_path;
                xtee_system_wineserver_path.n = strlen(
                                local_bin_wineserver_path);
        } else {
                xtee_system_wineserver_path.s = bin_wineserver_path;
                xtee_system_wineserver_path.n = strlen(bin_wineserver_path);
        }
}

static bool
is_winetricks_executable(const char *s)
{
        return access(s, X_OK) == 0;
}

/* TOCTOU race condition here!
 * This is not an essential system utility so this is fine for xteetotaller.
 * Returned value must be freed if not NULL. */
static char *
get_winetricks_path(void)
{
        char *paths, *path_string, *path_separator;

        paths = xtee_strdup_or_exit(getenv("PATH"));
        path_string = paths;
        path_separator = NULL;
        do {
                char *full_path = NULL;

                path_separator = strchr(path_string, ':');
                if (path_separator) {
                        path_separator[0] = '\0';
                }
                full_path = xtee_string_merge_char_strings(path_string,
                                "/winetricks");
                if (is_winetricks_executable(full_path)) {
                        const char *log_strs[3] = {
                                "Found winetricks at ",
                                full_path,
                                NULL,
                        };
                        free(paths);
                        print_info_strs(log_strs);
                        return full_path;
                } else {
                        const char *log_strs[3] = {
                                full_path,
                                " is not executable or does not exist.",
                                NULL,
                        };

                        print_info_strs(log_strs);
                }
                free(full_path);
                full_path = NULL;
                path_string = path_separator + 1;
        } while (path_separator != NULL);

        free(paths);
        return NULL;
}

static void
set_winetricks_path(void)
{
        char *path;

        if (!(path = get_winetricks_path())) {
                return;
        }
        xtee_system_winetricks_path = XteeString_create_from_charstring(path);
        free(path);
        xtee_system_is_winetricks_installed = true;
}

void
xtee_system_init(void)
{
        const char *wine_paths[] = {
                "/usr/bin/wine",
                "/usr/local/bin/wine",
        };

        for (size_t i = 0; i < XTEE_ARRAY_LEN(wine_paths)
                        && !xtee_system_is_wine_installed; ++i) {
                const size_t usr_local_i = 1;
                struct stat buf;

                if (stat(wine_paths[i], &buf) == 0 && S_ISREG(buf.st_mode)) {
                        xtee_system_wine_path.s = wine_paths[i];
                        xtee_system_wine_path.n = strlen(wine_paths[i]);
                        xtee_system_is_wine_installed = true;
                        if (i == usr_local_i) {
                                xtee_system_is_prefix_usr_local = true;
                        }
                }
        }

        if (!xtee_system_is_wine_installed) {
                print_warning("Wine not installed on system.");
        } else {
                set_wineboot_path();
                set_winecfg_path();
                set_wineserver_path();
                set_wine_lib_directory();
        }
        set_winetricks_path();
}

bool
xtee_system_is_amd64(void)
{
#ifdef XTEE_SYSTEM_AMD64
        return true;
#else
        return false;
#endif
}
