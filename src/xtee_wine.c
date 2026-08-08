#include "xtee_wine.h"
#include "xtee_error.h"
#include "xtee_file.h"
#include "xtee_paths.h"
#include "xtee_system.h"
#include "xtee_utils.h"

#include <sys/stat.h>

#define I386_UNIX_WINE_DIR_NAME "i386-unix"
#define X86_64_UNIX_WINE_DIR_NAME "x86_64-unix"

XteeWineVersionDataBlock xtee_wine_versions_data = { 0 };

static void
set_wine_system_version_data(void)
{
        XteeString i386_unix_dir_path, x86_64_unix_dir_path;
        XteeWineVersionData *system_wine_data;

        i386_unix_dir_path = XteeString_merge_XteeConstString_charstring(
                        xtee_system_wine_lib_path, I386_UNIX_WINE_DIR_NAME);
        x86_64_unix_dir_path = XteeString_merge_XteeConstString_charstring(
                        xtee_system_wine_lib_path, X86_64_UNIX_WINE_DIR_NAME);

        xtee_wine_versions_data.block = malloc_or_exit(
                        sizeof(XteeWineVersionData));
        xtee_wine_versions_data.n = 1;
        system_wine_data = &xtee_wine_versions_data.block[0];

        if (xtee_does_dir_exist(i386_unix_dir_path.s)) {
                if (xtee_does_dir_exist(x86_64_unix_dir_path.s)) {
                        system_wine_data->arch = XTEE_WINE_VERSION_ARCH_AMD64;
                        print_info("System Wine arch is AMD64.");
                } else {
                        system_wine_data->arch = XTEE_WINE_VERSION_ARCH_X86;
                        print_info("System Wine arch is x86.");
                }
        } else if (xtee_does_dir_exist(x86_64_unix_dir_path.s)) {
                system_wine_data->arch = XTEE_WINE_VERSION_ARCH_WOW64;
                print_info("System Wine arch is WOW64.");
        } else {
                system_wine_data->arch = XTEE_WINE_VERSION_ARCH_WRONG;
                print_error("Wrong system Wine arch");
        }
        system_wine_data->id = XteeString_create_from_charstring("System");
        system_wine_data->wine_path = XteeConstString_to_new_XteeString(
                        xtee_system_wine_path);
        system_wine_data->wine_lib_path = XteeConstString_to_new_XteeString(
                        xtee_system_wine_lib_path);
        system_wine_data->wineboot_path = XteeConstString_to_new_XteeString(
                        xtee_system_wineboot_path);
        system_wine_data->winecfg_path = XteeConstString_to_new_XteeString(
                        xtee_system_winecfg_path);
        system_wine_data->wineserver_path = XteeConstString_to_new_XteeString(
                        xtee_system_wineserver_path);

        XteeString_free(i386_unix_dir_path);
        XteeString_free(x86_64_unix_dir_path);
}

static enum XteeWineVersionArch
get_installed_wine_arch(XteeConstString wine_root_dir)
{
        enum XteeWineVersionArch rtrn_val;
        XteeString wine_lib_path;
        XteeString i386_unix_dir_path, x86_64_unix_dir_path;

        wine_lib_path = XteeString_merge_XteeConstString_charstring(
                        wine_root_dir, "/lib/wine/");
        i386_unix_dir_path = XteeString_merge_XteeConstString_charstring(
                        XteeString_to_XteeConstString(wine_lib_path),
                        I386_UNIX_WINE_DIR_NAME);
        x86_64_unix_dir_path = XteeString_merge_XteeConstString_charstring(
                        XteeString_to_XteeConstString(wine_lib_path),
                        X86_64_UNIX_WINE_DIR_NAME);
        XteeString_free(wine_lib_path);

        if (xtee_does_dir_exist(i386_unix_dir_path.s)) {
                if (xtee_does_dir_exist(x86_64_unix_dir_path.s)) {
                        rtrn_val = XTEE_WINE_VERSION_ARCH_AMD64;
                } else {
                        rtrn_val = XTEE_WINE_VERSION_ARCH_X86;
                }
        } else if (xtee_does_dir_exist(x86_64_unix_dir_path.s)) {
                rtrn_val = XTEE_WINE_VERSION_ARCH_WOW64;
        } else {
                rtrn_val = XTEE_WINE_VERSION_ARCH_WRONG;
        }

        XteeString_free(x86_64_unix_dir_path);
        XteeString_free(i386_unix_dir_path);

        return rtrn_val;
}

static void
xtee_wine_version_set_paths(XteeWineVersionData *data,
                XteeConstString wine_root_dir)
{
        const char *wine_exe_suffix = "/bin/wine";
        const char *lib_suffix = "/lib/wine/";
        const char *wineboot_suffix = "/bin/wineboot";
        const char *winecfg_suffix = "/bin/winecfg";
        const char *wineserver_suffix = "/bin/wineserver";

        data->wine_path = XteeString_merge_XteeConstString_charstring(
                        wine_root_dir, wine_exe_suffix);
        data->wine_lib_path = XteeString_merge_XteeConstString_charstring(
                        wine_root_dir, lib_suffix);
        data->wineboot_path = XteeString_merge_XteeConstString_charstring(
                        wine_root_dir, wineboot_suffix);
        data->winecfg_path = XteeString_merge_XteeConstString_charstring(
                        wine_root_dir, winecfg_suffix);
        data->wineserver_path = XteeString_merge_XteeConstString_charstring(
                        wine_root_dir, wineserver_suffix);
}

static void
xtee_wine_add_installed(void)
{
        const char *wine_components_suffix = "/components/wine/";
        XteeStringList files_list = { 0 };
        XteeConstString xtee_dir_path = { 0 };
        XteeString wine_comp_path = { 0 };
        XteeWineVersionData *tmp_ptr;

        xtee_dir_path = get_xteetotaller_dir_path();
        wine_comp_path = XteeString_merge_XteeConstString_charstring(
                        xtee_dir_path, wine_components_suffix);

        if (xtee_file_get_dir_files_list(
                                XteeString_to_XteeConstString(wine_comp_path),
                                &files_list)
                                != 0) {
                print_error("Cannot read wine components directory");
                goto error_rtrn;
        }

        if (!files_list.n) {
                print_info("No wine component installed.");
                goto direct_rtrn;
        }
        /* There cannot be more wine components installed than the number of
         * files in the directory so it is safe to use files_list.n value.
         * It is unlikely that an user would have a lot of other unrelated
         * files in this directory, so there will not be a lot of memory
         * wasted.
         * Add one to size to take into account system wine data. */
        tmp_ptr = realloc_or_exit(xtee_wine_versions_data.block,
                        (files_list.n + 1) * sizeof(XteeWineVersionData));
        xtee_wine_versions_data.block = tmp_ptr;

        for (size_t i = 0; i < files_list.n; ++i) {
                XteeWineVersionData *version_data;
                struct stat sb;
                XteeString wine_root_dir;
                size_t wine_version_i;

                wine_version_i = xtee_wine_versions_data.n;
                version_data = xtee_wine_versions_data.block + wine_version_i;

                wine_root_dir = XteeString_merge_XteeConstStrings(
                                XteeString_to_XteeConstString(wine_comp_path),
                                XteeString_to_XteeConstString(
                                        files_list.data[i]));
                if (stat(wine_root_dir.s, &sb) != 0) {
                        print_warning_strs(
                                        (const char *[]){
                                         "Cannot read wine version "
                                         "directory: ",
                                        files_list.data[i].s,
                                        NULL
                                        });
                        XteeString_free(wine_root_dir);
                        continue;
                } else if (!S_ISDIR(sb.st_mode)) {
                        print_warning_strs(
                                        (const char *[]){
                                        "Wine component is not a directory: ",
                                        files_list.data[i].s,
                                        NULL
                                        });
                        XteeString_free(wine_root_dir);
                        continue;
                } else if (xtee_strcmp_bool(files_list.data[i].s,
                                        "System")) {
                        print_warning("Ignoring Wine version named "
                                        "\"System\" as it conflicts with "
                                        "the system-wide Wine id.");
                        continue;
                }
                print_info_strs(
                                (const char *[]){
                                "Reading installed wine version: ",
                                files_list.data[i].s,
                                NULL
                                });
                /* Used to set pointers to NULL.
                 * Not guaranteed by ISO C nor by POSIX but architectures
                 * running this program have this behavior. */
                memset(version_data, 0, sizeof(XteeWineVersionData));
                version_data->arch = get_installed_wine_arch(
                                XteeString_to_XteeConstString(wine_root_dir));
                switch(version_data->arch) {
                        case XTEE_WINE_VERSION_ARCH_X86:
                                print_info("Installed Wine arch is x86.");
                                break;
                        case XTEE_WINE_VERSION_ARCH_AMD64:
                                print_info("Installed Wine arch is AMD64.");
                                break;
                        case XTEE_WINE_VERSION_ARCH_WOW64:
                                print_info("Installed Wine arch is WOW64.");
                                break;
                        case XTEE_WINE_VERSION_ARCH_WRONG:
                                print_error("Wrong installed Wine arch");
                                XteeString_free(wine_root_dir);
                                continue;
                }

                version_data->id = XteeString_duplicate(files_list.data[i]);
                xtee_wine_version_set_paths(version_data,
                                XteeString_to_XteeConstString(wine_root_dir));
                XteeString_free(wine_root_dir);
                ++xtee_wine_versions_data.n;
        }
        XteeStringList_free(files_list);
error_rtrn:
direct_rtrn:
        XteeString_free(wine_comp_path);
}

void
xtee_wine_init(void)
{
        if (xtee_system_is_wine_installed) {
                set_wine_system_version_data();
        }
        xtee_wine_add_installed();
}

void
xtee_wine_free(void)
{
        for (size_t i = 0; i < xtee_wine_versions_data.n; ++i) {
                XteeWineVersionData *version_data;

                version_data = xtee_wine_versions_data.block + i;
                XteeString_free(version_data->id);
                XteeString_free(version_data->wine_lib_path);
                XteeString_free(version_data->wine_path);
                XteeString_free(version_data->wineboot_path);
                XteeString_free(version_data->winecfg_path);
                XteeString_free(version_data->wineserver_path);
        }
        free(xtee_wine_versions_data.block);
}

XteeWineVersionData *
xtee_wine_get_version_data(const char *id)
{
        for (size_t i = 0; i < xtee_wine_versions_data.n; ++i) {
                XteeWineVersionData *version_data;

                version_data = &xtee_wine_versions_data.block[i];
                if (xtee_strcmp_bool(id, version_data->id.s)) {
                        return version_data;
                }
        }
        return NULL;
}

XteeStringList
xtee_wine_get_versions_ids(void)
{
        XteeStringList list;

        list.n = xtee_wine_versions_data.n;
        list.data = malloc_or_exit(list.n * sizeof (XteeString));

        for (size_t i = 0; i < xtee_wine_versions_data.n; ++i) {
                XteeWineVersionData *version_data;

                version_data = xtee_wine_versions_data.block + i;
                list.data[i] = XteeString_duplicate(version_data->id);
        }
        return list;
}
