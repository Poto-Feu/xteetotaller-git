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

#include "xteetotaller.h"
#include "xtee_config.h"
#include "xtee_dxvk.h"
#include "xtee_error.h"
#include "xtee_file.h"
#include "xtee_gui.h"
#include "xtee_paths.h"
#include "xtee_prefix.h"
#include "xtee_state.h"
#include "xtee_system.h"
#include "xtee_wine.h"

#include <X11/StringDefs.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

static int
create_xteetotaller_dirs(const XteeConstString xtee_dir_path)
{
        XteeConstString components_dir_name =
                XTEE_CONSTSTRING_STATIC_INIT("components/");
        XteeConstString wine_dir_path_end =
                XTEE_CONSTSTRING_STATIC_INIT("components/wine/");
        XteeConstString dxvk_dir_path_end =
                XTEE_CONSTSTRING_STATIC_INIT("components/dxvk/");
        XteeConstString prefixes_dir_path_end =
                XTEE_CONSTSTRING_STATIC_INIT("prefixes/");
        XteeString components_dir_str, wine_dir_str, dxvk_dir_str;
        XteeString prefixes_dir_str;
        XteeConstString components_dir_cstr, wine_dir_cstr, dxvk_dir_cstr;
        XteeConstString prefixes_dir_cstr;

        if (xtee_directory_exist_or_create(xtee_dir_path) != 0) {
                print_error("Cannot create .xteetotaller directory.");
                return -1;
        }

        components_dir_str = XteeString_merge_XteeConstStrings(xtee_dir_path,
                        components_dir_name);
        components_dir_cstr = XteeString_to_XteeConstString(
                        components_dir_str);
        if (xtee_directory_exist_or_create(components_dir_cstr) != 0) {
                print_error("Cannot create components directory.");
                return -1;
        }
        XteeString_free(components_dir_str);

        wine_dir_str = XteeString_merge_XteeConstStrings(xtee_dir_path,
                        wine_dir_path_end);
        wine_dir_cstr = XteeString_to_XteeConstString(wine_dir_str);
        if (xtee_directory_exist_or_create(wine_dir_cstr) != 0) {
                print_error("Cannot create wine components directory.");
                return -1;
        }
        XteeString_free(wine_dir_str);

        dxvk_dir_str = XteeString_merge_XteeConstStrings(xtee_dir_path,
                        dxvk_dir_path_end);
        dxvk_dir_cstr = XteeString_to_XteeConstString(dxvk_dir_str);
        if (xtee_directory_exist_or_create(dxvk_dir_cstr) != 0) {
                print_error("Cannot create dxvk components directory.");
                return -1;
        }
        XteeString_free(dxvk_dir_str);

        prefixes_dir_str = XteeString_merge_XteeConstStrings(
                        xtee_dir_path,
                        prefixes_dir_path_end);
        prefixes_dir_cstr = XteeString_to_XteeConstString(
                        prefixes_dir_str);
        if (xtee_directory_exist_or_create(prefixes_dir_cstr) != 0) {
                print_error("Cannot create wine_prefixes directory.");
                return -1;
        }
        XteeString_free(prefixes_dir_str);

        return 0;
}

void
xtee_clean()
{
        xtee_prefixes_free();
        xtee_config_free();
        xtee_dxvk_free();
        xtee_wine_free();
        xtee_state_free();
        free_xteetotaller_dir_path();
}

int
main(int argc, char *argv[])
{
        XteeConstString xtee_dir_path;
        int status = EXIT_FAILURE;

        set_log_level();
        xtee_dir_path = get_xteetotaller_dir_path();
        if (create_xteetotaller_dirs(xtee_dir_path) != 0) {
                goto err_exit;
        }

        xtee_system_init();
        xtee_wine_init();
        xtee_dxvk_init();
        if (xtee_config_init(xtee_dir_path) != 0) {
                return EXIT_FAILURE;
        }
        xtee_prefixes_init();

        xtee_gui_init(&argc, argv);
        xtee_gui_run();
        /* The program exits with WM_DELETE_WINDOW callback,
         * so no need to free components there. */
err_exit:
        free_xteetotaller_dir_path();
        return status;
}

