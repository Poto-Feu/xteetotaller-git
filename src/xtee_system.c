#include "xtee_system.h"
#include "xtee_error.h"
#include "xtee_utils.h"

#include <sys/stat.h>
#include <stdint.h>

#if INTPTR_MAX == INT64_MAX
#define XTEE_SYSTEM_AMD64
#elif INTPTR_MAX == INT32_MAX
#define XTEE_SYSTEM_IA32
#else
#error System architecture detection failed.
#endif

bool xtee_system_is_wine_installed = false;
bool xtee_system_is_winetricks_installed = false;
bool xtee_system_is_gamescope_installed = false;
XteeConstString xtee_system_wine_path = { 0 };
XteeConstString xtee_system_wine_lib_path = { 0 };
XteeConstString xtee_system_wineboot_path = { 0 };
XteeConstString xtee_system_winecfg_path = { 0 };
XteeConstString xtee_system_wineserver_path = { 0 };
XteeConstString xtee_system_winetricks_path = { 0 };
XteeConstString xtee_system_gamescope_path = { 0 };

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
                        xtee_system_wine_lib_path.s = *lib_paths;
                        xtee_system_wine_lib_path.n = strlen(*lib_paths);
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

static void
set_winetricks_path(void)
{
        const char *winetricks_paths[] = {
                "/usr/bin/winetricks",
                "/usr/local/bin/winetricks",
        };

        for (size_t i = 0; XTEE_ARRAY_LEN(winetricks_paths)
                        && !xtee_system_is_winetricks_installed; ++i) {
                struct stat buf;

                if (stat(winetricks_paths[i], &buf) == 0
                                && S_ISREG(buf.st_mode)) {
                        xtee_system_winetricks_path.s = winetricks_paths[i];
                        xtee_system_winetricks_path.n = strlen(
                                        winetricks_paths[i]);
                        xtee_system_is_winetricks_installed = true;
                }
        }

        if (!xtee_system_is_winetricks_installed) {
                print_warning("Winetricks not installed on system.");
        }
}

static void
set_gamescope_path(void)
{
        const char *gamescope_paths[] = {
                "/usr/bin/gamescope",
                "/usr/local/bin/gamescope",
        };
        for (size_t i = 0; XTEE_ARRAY_LEN(gamescope_paths)
                        && !xtee_system_is_gamescope_installed; ++i) {
                struct stat buf;

                if (stat(gamescope_paths[i], &buf) == 0
                                && S_ISREG(buf.st_mode)) {
                        xtee_system_gamescope_path.s = gamescope_paths[i];
                        xtee_system_gamescope_path.n = strlen(
                                        gamescope_paths[i]);
                        xtee_system_is_gamescope_installed = true;
                }
        }

        if (!xtee_system_is_gamescope_installed) {
                print_warning("Gamescope not installed on system.");
        } else {
                const char *info_strs[3] =  {
                        "Gamescope found at ",
                        xtee_system_gamescope_path.s,
                        NULL,
                };
                print_info_strs(info_strs);
        }
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
        set_gamescope_path();
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
