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

#include "xtee_utils.h"
#include "xtee_error.h"
#include "xtee_string.h"

extern inline char *xtee_bool_to_string(bool x);
extern inline bool xtee_string_to_bool(const char *s);

void
xtee_env_data_add_var_to_handler(XteeEnvDataHandler *handler,
                const char *name, const char *value)
{
        if (!handler->n) {
                size_t block_initial_size = 6;

                handler->block_size = block_initial_size;
                handler->block = malloc_or_exit(
                                handler->block_size * sizeof(XteeEnvData));
        } else if (handler->n + 1 > handler->block_size) {
                print_info("Growing XteeEnvDataHandler.");
                /* This may cause unsigned integer overflow but since there are
                 * no way for user to add environment variables for now, this
                 * is OK.
                 * When adding support for user-defined environment variable,
                 * we should add a limit to avoid this issue. */
                handler->block_size *= 2;
                handler->block = realloc_or_exit(handler->block,
                                handler->block_size * sizeof(XteeEnvData));
        }

        handler->block[handler->n].name = xtee_strdup_or_exit(name);
        handler->block[handler->n].value = xtee_strdup_or_exit(value);
        ++handler->n;
}

void
xtee_env_data_free_handler(XteeEnvDataHandler *handler)
{
        for (size_t i = 0; i < handler->n; ++i) {
                free(handler->block[i].name);
                free(handler->block[i].value);
        }
        free(handler->block);
        handler->block = NULL;
        handler->n = 0;
}
