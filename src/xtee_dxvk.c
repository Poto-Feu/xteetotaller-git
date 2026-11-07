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

#include "xtee_dxvk.h"
#include "xtee_file.h"
#include "xtee_paths.h"

#include <sys/stat.h>

typedef struct XteeDxvkVersionDataBlock {
        XteeDxvkVersionData *block;
        size_t n;
} XteeDxvkVersionDataBlock;

XteeDxvkVersionDataBlock dxvk_versions_data = { 0 };

static bool
is_file_name_dll(XteeConstString str)
{
        return str.n > 4 && memcmp(str.s + str.n - 4, ".dll", 4) == 0;
}

static void
add_dxvk_dlls_to_list(XteeStringList *dlls_list,
                XteeConstString parent_dir, XteeStringList files_list)
{

        for (size_t i = 0; i < files_list.n; ++i) {
                XteeConstString current_file_str;
                const char *info_strs[4];
                XteeString dll_full_path;

                current_file_str = XteeString_to_XteeConstString(
                                files_list.data[i]);

                dll_full_path = XteeString_merge_XteeConstStrings(parent_dir,
                                current_file_str);
                if (!xtee_is_file_regular(dll_full_path.s)
                                || !is_file_name_dll(current_file_str)) {
                        const char *warning_strs[] = {
                                current_file_str.s,
                                " is not a valid dll file.",
                                NULL,
                        };

                        print_warning_strs(warning_strs);
                        XteeString_free(dll_full_path);
                        continue;
                }
                info_strs[0] = "Adding ";
                info_strs[1] = current_file_str.s;
                info_strs[2] = " to dll list.";
                info_strs[3] = NULL;
                print_info_strs(info_strs);
                xtee_add_charstring_to_XteeStringList(dlls_list,
                                current_file_str.s);
                XteeString_free(dll_full_path);
        }
}

static void
free_dxvk_version_data(const XteeDxvkVersionData *dxvk_version)
{
        XteeString_free(dxvk_version->id);
        XteeStringList_free(dxvk_version->x32_dll_strs);
        XteeStringList_free(dxvk_version->x64_dll_strs);
        XteeString_free(dxvk_version->x32_path);
        XteeString_free(dxvk_version->x64_path);
}

void
xtee_dxvk_init(void)
{
        const char *dxvk_components_suffix = "components/dxvk/";
        XteeStringList files_list = { 0 };
        XteeConstString xtee_dir_path = { 0 };
        XteeString dxvk_comp_path = { 0 };

        xtee_dir_path = get_xteetotaller_dir_path();
        dxvk_comp_path = XteeString_merge_XteeConstString_charstring(
                        xtee_dir_path, dxvk_components_suffix);

        if (xtee_file_get_dir_files_list(
                                XteeString_to_XteeConstString(dxvk_comp_path),
                                &files_list)
                                != 0) {
                print_error("Cannot read dxvk components directory");
                goto error_rtrn;
        }

        if (!files_list.n) {
                XteeStringList_free(files_list);
                print_info("No wine component installed.");
                goto direct_rtrn;
        }
        /* See comment inx tee_wine_add_installed, except there are no
         * system installation of dxvk. */
        dxvk_versions_data.block = realloc_or_exit(dxvk_versions_data.block,
                        files_list.n * sizeof(XteeDxvkVersionData));
        for (size_t i = 0; i < files_list.n; ++i) {
                const char *reading_path_info_strs[5];
                const char *x32_dir_suffix = "/x32/";
                const char *x64_dir_suffix = "/x64/";
                XteeDxvkVersionData *version_data;
                XteeConstString current_file_str;
                XteeString dxvk_root_path;
                XteeString x32_dir_path;
                XteeString x64_dir_path;
                XteeStringList dlls_files_list;
                size_t block_i;

                block_i = dxvk_versions_data.n;
                version_data = dxvk_versions_data.block + block_i;
                current_file_str = XteeString_to_XteeConstString(
                                files_list.data[i]);
                dxvk_root_path = XteeString_merge_XteeConstStrings(
                                XteeString_to_XteeConstString(dxvk_comp_path),
                                current_file_str);

                if (!xtee_is_file_directory(dxvk_root_path.s)) {
                        const char *warning_strs[] = {
                                current_file_str.s,
                                " is not a readable directory.",
                                NULL
                        };

                        print_warning_strs(warning_strs);
                        XteeString_free(dxvk_root_path);
                        continue;
                }
                reading_path_info_strs[0] = "Reading ";
                reading_path_info_strs[1] = files_list.data[i].s;
                reading_path_info_strs[2] = " at ";
                reading_path_info_strs[3] = dxvk_root_path.s;
                reading_path_info_strs[4] = NULL;
                print_info_strs(reading_path_info_strs);
                /* See xtee_wine_add_installed comment. */
                memset(version_data, 0, sizeof(XteeDxvkVersionData));
                x32_dir_path = XteeString_merge_XteeConstString_charstring(
                                XteeString_to_XteeConstString(dxvk_root_path),
                                x32_dir_suffix);
                if (xtee_file_get_dir_files_list(
                                        XteeString_to_XteeConstString(
                                                x32_dir_path),
                                        &dlls_files_list) != 0) {
                        XteeString_free(x32_dir_path);
                        XteeString_free(dxvk_root_path);
                        print_error("Cannot read dxvk x32 directory.");
                        continue;
                }

                if (!dlls_files_list.n) {
                        print_error("No x32 dxvk dlls");
                        XteeString_free(x32_dir_path);
                        XteeString_free(dxvk_root_path);
                        continue;
                }
                print_info("Reading dxvk x32 directory");
                add_dxvk_dlls_to_list(&version_data->x32_dll_strs,
                                XteeString_to_XteeConstString(x32_dir_path),
                                dlls_files_list);
                /* Move instead of duplicate to avoid another allocation */
                version_data->x32_path = x32_dir_path;
                XteeStringList_free(dlls_files_list);

                /* Reset dlls_files_list to reuse it for x64 dlls */
                dlls_files_list.data = NULL;
                dlls_files_list.n = 0;

                x64_dir_path = XteeString_merge_XteeConstString_charstring(
                                XteeString_to_XteeConstString(dxvk_root_path),
                                x64_dir_suffix);
                XteeString_free(dxvk_root_path);
                if (xtee_file_get_dir_files_list(
                                        XteeString_to_XteeConstString(
                                                x64_dir_path),
                                        &dlls_files_list) != 0) {
                        XteeString_free(x64_dir_path);
                        free_dxvk_version_data(version_data);
                        print_error("Cannot read dxvk x64 directory.");
                        continue;
                }
                if (!dlls_files_list.n) {
                        print_error("No x64 dxvk dlls");
                        XteeString_free(x64_dir_path);
                        free_dxvk_version_data(version_data);
                }
                print_info("Reading dxvk x64 directory");
                add_dxvk_dlls_to_list(&version_data->x64_dll_strs,
                                XteeString_to_XteeConstString(x64_dir_path),
                                dlls_files_list);
                XteeStringList_free(dlls_files_list);
                version_data->x64_path = x64_dir_path;
                version_data->id = XteeConstString_to_new_XteeString(
                                current_file_str);
                ++dxvk_versions_data.n;
        }
        XteeStringList_free(files_list);
error_rtrn:
direct_rtrn:
        XteeString_free(dxvk_comp_path);
}

void
xtee_dxvk_free(void)
{
        for (size_t i = 0; i < dxvk_versions_data.n; ++i) {
                free_dxvk_version_data(dxvk_versions_data.block + i);
        }
        free(dxvk_versions_data.block);
}

XteeStringList
xtee_dxvk_get_ids(void)
{
        XteeStringList strings_list;

        strings_list.n = dxvk_versions_data.n;
        strings_list.data = malloc(strings_list.n * sizeof(XteeConstString));
        for (size_t i = 0; i < dxvk_versions_data.n; ++i) {
                strings_list.data[i].s = strdup(
                                dxvk_versions_data.block[i].id.s);
                strings_list.data[i].n = dxvk_versions_data.block[i].id.n;
        }
        return strings_list;
}

XteeDxvkVersionData *
xtee_dxvk_get_data(const char *id)
{
        for (size_t i = 0; i < dxvk_versions_data.n; ++i) {
                XteeDxvkVersionData *version_data;

                version_data = &dxvk_versions_data.block[i];
                if (xtee_strcmp_bool(id, version_data->id.s)) {
                        return version_data;
                }
        }
        return NULL;
}
