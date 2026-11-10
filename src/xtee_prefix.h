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

#ifndef XTEE_PREFIX_H
#define XTEE_PREFIX_H

#include "xtee_config.h"
#include "xtee_dxvk.h"
#include "xtee_program.h"
#include "xtee_utils.h"

#define XTEE_PREFIX_DEFAULT_ID_MAX_LEN XTEE_MAX_ID_LEN

#define XTEE_PREFIX_VAR_NAME_MAX_LEN XTEE_VAR_NAME_MAX_LEN
#define XTEE_PREFIX_VAR_VALUE_MAX_LEN XTEE_VAR_VALUE_MAX_LEN

#if FILENAME_MAX < XTEE_PREFIX_DEFAULT_ID_MAX_LEN
#define XTEE_PREFIX_ID_MAX_LEN FILENAME_MAX
#else
#define XTEE_PREFIX_ID_MAX_LEN XTEE_PREFIX_DEFAULT_ID_MAX_LEN
#endif

#define XTEE_PREFIX_DIR_PATH_MAX_LEN (XTEE_VAR_VALUE_MAX_LEN - 1)

enum XteePrefixArch {
        XTEE_PREFIX_ARCH_WIN32,
        XTEE_PREFIX_ARCH_WIN64,
        XTEE_PREFIX_ARCH_WOW64,
        XTEE_PREFIX_ARCH_WRONG = -1
};

#define XTEE_PREFIX_ARCH_WIN32_STR "Win32"
#define XTEE_PREFIX_ARCH_WIN64_STR "Win64"
#define XTEE_PREFIX_ARCH_WOW64_STR "SysWow64"
#define XTEE_PREFIX_ARCH_WRONG_STR "Unknown Wine architecture"

enum XteeNewPrefixCreateStatus {
        XTEE_NEW_PREFIX_CREATE_OK = 0,
        XTEE_NEW_PREFIX_CREATE_ID_EXIST,
        XTEE_NEW_PREFIX_CREATE_DIR_ERROR,
        XTEE_NEW_PREFIX_CREATE_WRITE_ERROR,
        XTEE_NEW_PREFIX_OTHER_ERR,
};

enum XteeLoadPrefixStatus {
        XTEE_LOAD_PREFIX_OK = 0,
        XTEE_LOAD_PREFIX_NO_CONFIG,
        XTEE_LOAD_PREFIX_CONFIG_ACCESS_ERR,
        XTEE_LOAD_PREFIX_CONFIG_READ_ERR,
        XTEE_LOAD_PREFIX_CONFIG_MISSING_PREFIXDIR,
        XTEE_LOAD_PREFIX_DIR_NOT_EXIST,
        XTEE_LOAD_PREFIX_PROGRAMS_READ_ERR,
        XTEE_LOAD_PREFIX_WRONG_ARCH,
        XTEE_LOAD_PREFIX_WINE_VERSION_NOTEXIST,
        XTEE_LOAD_PREFIX_WINE_VERSION_INCOMP_ARCH,
        XTEE_LOAD_PREFIX_WINE_ARCH_SYSTEM_NOMATCH,
        XTEE_LOAD_PREFIX_OTHER_ERR,
};

enum XteeAddProgramStatus {
        XTEE_ADD_PROGRAM_0K = 0,
        XTEE_ADD_PROGRAM_ID_EXIST,
        XTEE_ADD_PROGRAM_WRITE_ERROR
};

enum XteeGetProgramDataStatus {
        XTEE_GET_PROGRAM_DATA_OK = 0,
        XTEE_GET_PROGRAM_DATA_NOT_EXIST,
        XTEE_GET_PROGRAM_DATA_NO_EXE_PATH,
        XTEE_GET_PROGRAM_DATA_READ_ERROR,
};

typedef struct XteePrefix {
        XteeString id;
        XteeString wineprefix_dir;
        XteeString wine_version;
        XteeDate creation_date;
        XteeString dxvk_version;
        XteeStringList dxvk_x32_dlls;
        XteeStringList dxvk_x64_dlls;
        XteeString *programs_ids;
        size_t programs_n;
        enum XteePrefixArch arch;
        bool isolate_home;
        bool disable_winemenubuilder;
        bool is_dxvk_installed;
} XteePrefix;

typedef struct XteePrefixIdBlock {
        XteeString *data;
        size_t n;
} XteePrefixIdBlock;

void xtee_prefixes_init(void);
void xtee_prefixes_free(void);

void init_XteePrefix(XteePrefix *prefix);
void xtee_free_prefix_struct(XteePrefix *prefix);

void free_XteePrefixIdBlock(XteePrefixIdBlock *);

enum XteeNewPrefixCreateStatus create_new_prefix(const char *id,
                const char *dir, const char *wine_version,
                enum XteePrefixArch arch, bool isolate_home);
enum XteeLoadPrefixStatus xtee_prefix_load(XteePrefix *prefix,
                const char *id);
int xtee_prefix_save_config(XteePrefix *prefix);

int get_prefixes_ids(XteePrefixIdBlock *block);

/* We cannot use const for program as its values are used in an
 * XteeConfigurationEntry struct, which discards the const qualifier. */
enum XteeAddProgramStatus add_program_to_prefix(XteePrefix *prefix,
                XteeProgram *program);

int xtee_prefix_reload_programs_list(XteePrefix *prefix);

enum XteePrefixArch get_prefix_arch_from_str(const char *s);

/* Returned string must be freed except if NULL is returned. */
char *xtee_prefix_get_WINEDLLOVERRIDES_env_value(const XteePrefix *prefix);

int xtee_prefix_save_program(XteePrefix *prefix, XteeProgram *program);

/* The returned pointer is dynamically allocated.
 * Don't forget to call xtee_free_program and free on the pointer! */
XteeProgram *xtee_prefix_get_program_data(const XteePrefix *prefix,
                const char *program_id,
                enum XteeGetProgramDataStatus *status);

/* Both xtee_prefix_install_dxvk and xtee_prefix_remove_dxvk call
 * xtee_gui_update_display as the process is not forked when calling these
 * functions. */
int xtee_prefix_install_dxvk(XteePrefix *prefix,
                const XteeDxvkVersionData *dxvk_data);
int xtee_prefix_remove_dxvk(XteePrefix *prefix);

/* Do not free returned string as they are pointers to string literals. */
inline char *
xtee_prefix_get_arch_string(enum XteePrefixArch arch)
{
        switch (arch) {
                case XTEE_PREFIX_ARCH_WIN32:
                        return XTEE_PREFIX_ARCH_WIN32_STR;
                case XTEE_PREFIX_ARCH_WIN64:
                        return XTEE_PREFIX_ARCH_WIN64_STR;
                case XTEE_PREFIX_ARCH_WOW64:
                        return XTEE_PREFIX_ARCH_WOW64_STR;
                default:
                        return XTEE_PREFIX_ARCH_WRONG_STR;
        }
}

#endif
