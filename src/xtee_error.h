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

#ifndef XTEE_ERROR_H
#define XTEE_ERROR_H

#include <stdlib.h>

enum XteeLogLevel {
        XTEE_LOG_ERROR = 1,
        XTEE_LOG_WARN = 2,
        XTEE_LOG_INFO = 3
};

void set_log_level(void);

/* Functions ending with _strs print all strings in the block as a single
 * warning line.
 * The last element must be a NULL pointer.
 * No punctuation is added between strings so the space character should be
 * inserted at the end of every string barring the last. */
void print_error(const char *s);
void print_error_strs(const char **strs);
void print_warning(const char *s);
void print_warning_strs(const char **strs);
void print_info(const char *s);
void print_info_strs(const char **strs);

/* Simple utility function */
inline void
fatal_error(const char *s)
{
        print_error(s);
        exit(EXIT_FAILURE);
}

/* Since most Motif errors crash the entire program, it makes no sense to emit
 * a dignostic for an edge case such as memory allocation failure */
inline void *
malloc_or_exit(size_t size)
{
        void *ptr;

        if (!(ptr = malloc(size))) {
                fatal_error("malloc failed.");
        }

        return ptr;
}

inline void *
calloc_or_exit(size_t nmemb, size_t size)
{
        void *ptr;

        if (!(ptr = calloc(nmemb, size))) {
                fatal_error("calloc failed.");
        }

        return ptr;
}

/* A temporary pointer is not needed as the NULL check is done in this
 * function. */
inline void *
realloc_or_exit(void *ptr, size_t size)
{
        void *new_ptr;

        if (!(new_ptr = realloc(ptr, size))) {
                fatal_error("realloc failed.");
        }

        return new_ptr;
}

#endif
