#include "xtee_error.h"

#include <stdio.h>
#include <string.h>

static enum XteeLogLevel xtee_log_level = XTEE_LOG_ERROR;

extern inline void fatal_error(const char *s);
extern inline void *malloc_or_exit(size_t size);
extern inline void *realloc_or_exit(void *ptr, size_t size);
extern inline void *calloc_or_exit(size_t nmemb, size_t size);

void
set_log_level(void)
{
        const char *env_name = "XTEE_LOG_LEVEL";
        const char *env_value;
        size_t env_value_len;

        if (!(env_value = getenv(env_name))) {
                return;
        }
        env_value_len = strlen(env_value);

        if (env_value_len != 1) {
                xtee_log_level = XTEE_LOG_ERROR;
                return;
        }

        switch(*env_value) {
                case '3':
                        xtee_log_level = XTEE_LOG_INFO;
                        break;
                case '2':
                        xtee_log_level = XTEE_LOG_WARN;
                        break;
                case '1':
                default:
                        xtee_log_level = XTEE_LOG_ERROR;
        }
}

void
print_error(const char *s)
{
        fprintf(stderr, "ERROR: %s\n", s);
}

void
print_error_strs(const char **strs)
{
        if (!(*strs)) {
                return;
        }

        fputs("ERROR: ", stderr);
        do {
                fputs(*strs, stderr);
        } while (*(++strs));
        fputc('\n', stderr);
}

void
print_warning(const char *s)
{
        if (xtee_log_level >= XTEE_LOG_WARN) {
                fprintf(stderr, "WARNING: %s\n", s);
        }
}

void
print_warning_strs(const char **strs)
{
        if (xtee_log_level < XTEE_LOG_WARN) {
                return;
        }

        if (!(*strs)) {
                return;
        }

        fputs("WARNING: ", stderr);
        do {
                fputs(*strs, stderr);
        } while (*(++strs));
        fputc('\n', stderr);
}

void
print_info(const char *s)
{
        if (xtee_log_level == XTEE_LOG_INFO) {
                fprintf(stderr, "INFO: %s\n", s);
        }
}

void
print_info_strs(const char **strs)
{
        if (xtee_log_level < XTEE_LOG_INFO) {
                return;
        }

        if (!(*strs)) {
                return;
        }

        fputs("INFO: ", stderr);
        do {
                fputs(*strs, stderr);
        } while (*(++strs));
        fputc('\n', stderr);
}
