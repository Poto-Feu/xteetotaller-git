#ifndef XTEE_UTILS_H
#define XTEE_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define XTEE_ARRAY_LEN(x) (sizeof(x)/sizeof((x)[0]))
#define XTEE_STRINGLIT_LEN(x) (XTEE_ARRAY_LEN(x) - 1)

typedef struct XteeEnvData {
        const char *name;
        const char *value;
} XteeEnvData;

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

inline bool
xtee_is_char_ascii_alphanum(char c)
{
        return ((c > 47 && c < 58) || (c > 64 && c < 91)
                        || (c > 96 && c < 123));
}

#endif
