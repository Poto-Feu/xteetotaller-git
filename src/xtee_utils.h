#ifndef XTEE_UTILS_H
#define XTEE_UTILS_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define XTEE_ARRAY_LEN(x) (sizeof(x)/sizeof((x)[0]))
#define XTEE_STRINGLIT_LEN(x) (XTEE_ARRAY_LEN(x) - 1)
#define XTEE_ARRAY_LAST_I(x) (XTEE_ARRAY_LEN(x) - 1)
#define XTEE_ARRAY_STRLEN(x) (XTEE_ARRAY_LEN(x) - 1)

#define XTEE_MAX_FILENAME_LEN 256

#if defined(PATH_MAX) && PATH_MAX > 4096
#define XTEE_MAX_FILE_PATH_LEN 4096
#else
#define XTEE_MAX_FILE_PATH_LEN PATH_MAX
#endif

/* Setting this value for prefix and programs id length.
 * We select 200 as it is very unlikely that the user will run this program
 * on a file system with a lower maximum file name length limit.
 * The only known filesystem with a lower limit are FAT32 before Windows 95
 * and FAT16.
 * These filesystems are virtually unused nowadays. */
#define XTEE_DEFAULT_MAX_ID_LEN 200

#if defined(FILENAME_MAX) && FILENAME_MAX < XTEE_DEFAULT_MAX_ID_LEN
#define XTEE_MAX_ID_LEN FILENAME_MAX
#else
#define XTEE_MAX_ID_LEN XTEE_DEFAULT_MAX_ID_LEN
#endif

typedef struct XteeEnvData {
        const char *name;
        const char *value;
} XteeEnvData;

typedef struct XteeEnvDataHandler {
        XteeEnvData *block;
        size_t n;
        size_t block_size;
        char *winedlloverrides_str;
        char *x11_display_str;
} XteeEnvDataHandler;

inline char *
xtee_bool_to_string(bool x)
{
        return x ? "true" : "false";
}

inline bool
xtee_string_to_bool(const char *s)
{
        return (strcmp("false", s) == 0) ? false : true;
}

/* Make sure that all env_data_handler members are initialized to zero when
 * calling this function for the first time. */
void xtee_env_data_add_var_to_handler(XteeEnvDataHandler *handler,
                const char *name, const char *value);
void xtee_env_data_free_handler(XteeEnvDataHandler *);

#endif
