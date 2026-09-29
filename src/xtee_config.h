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

#ifndef XTEE_CONFIG_H
#define XTEE_CONFIG_H

#include "xtee_string.h"

#include <stdio.h>

/* Variable name and value length is limited.
 * Since we are in early stages of development, this is OK for now. */
#define XTEE_VAR_NAME_MAX_LEN 256
#define XTEE_VAR_VALUE_MAX_LEN 4097

typedef struct XteeConfigurationVariable {
        XteeConstString name;
        XteeConstString default_val;
} XteeConfigurationVariable;

/* While these structs were made for xteetotaller configuration file, they can
 * also be used for prefixes and programs configuration. */
typedef struct XteeConfigurationEntry {
        const char *name;
        char *val;
} XteeConfigurationEntry;

typedef struct XteeConfigurationEntryList {
        XteeConfigurationEntry *entries;
        size_t n;
} XteeConfigurationEntryList;

/* Returns a non-zero value on error.
 * If 0 is returned, check if EOF is reached.
 * If either max_name_len or max_val_len is lower than 3, behavior is
 * undefined. */
int xtee_config_read_variable_line(char *name, char *value,
                size_t max_name_len, size_t max_val_len, FILE* file);

int xtee_write_config_entry(FILE *f, const XteeConfigurationEntry entry);

int xtee_config_init(XteeConstString xtee_dir_path);
void xtee_config_free(void);

/* If NULL is returned, the configuration entry does not exist.
 * DO NOT free the returned pointer as it points to the entry string. */
char *xtee_config_get_entry_value(const char *name);

/* If the entry does not exist, it prints an error message but returns 0.
 * Calls exit(-1) on writing errors. */
void xtee_config_set_entry_value_and_write(const char *restrict name,
                const char *restrict val);

#endif
