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
