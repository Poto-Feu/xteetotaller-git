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

#ifndef XTEE_FILE_H
#define XTEE_FILE_H

#include "xtee_string.h"

#define XTEE_NEW_FILE_MODE (mode_t)0777
#define XTEE_NEW_DIR_MODE XTEE_NEW_FILE_MODE

enum XteeExecutableType {
        XTEE_EXECUTABLE_DOS_MZ,
        XTEE_EXECUTABLE_NE,
        XTEE_EXECUTABLE_LE,
        XTEE_EXECUTABLE_PE32,
        XTEE_EXECUTABLE_PE32PLUS,
        XTEE_EXECUTABLE_UNKNOWN
};

enum XteeDirStatus {
        XTEE_DIR_STATUS_NOT_EXIST,
        XTEE_DIR_STATUS_NOT_DIR,
        XTEE_DIR_STATUS_EMPTY,
        XTEE_DIR_STATUS_NOT_EMPTY,
        XTEE_DIR_STATUS_READ_ERROR,
        XTEE_DIR_STATUS_PATH_NOT_VALID,
        XTEE_DIR_STATUS_OTHER_ERROR
};

int xtee_directory_exist_or_create(const XteeConstString path);

/* If a non-zero value is returned, a reading error occured and the type
 * argument is not guaranteed to be set. */
int xtee_get_executable_type(enum XteeExecutableType *type,
                const XteeConstString cstr);

bool xtee_is_file_msi(const XteeConstString cstr);

enum XteeDirStatus xtee_get_dir_status(const char *path);

bool xtee_does_dir_exist(const char *path);

/* If there are no files in the specified directory, list->n is set to 0 and
 * list->data is set to NULL.
 * A non-zero value is returned on error. */
int xtee_file_get_dir_files_list(XteeConstString dir_path,
                XteeStringList *list);

bool xtee_is_file_regular(const char *path);
bool xtee_is_file_directory(const char *path);

int xtee_file_copy(const char *restrict dest, const char *restrict file_path);

#endif
