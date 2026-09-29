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

        handler->block[handler->n].name = name;
        handler->block[handler->n].value = value;
        ++handler->n;
}

void
xtee_env_data_free_handler(XteeEnvDataHandler *handler)
{
        free(handler->block);
        free(handler->winedlloverrides_str);
        free(handler->x11_display_str);
        handler->block = NULL;
        handler->n = 0;
        handler->winedlloverrides_str = NULL;
        handler->x11_display_str = NULL;
}
