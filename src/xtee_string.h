#ifndef XTEE_STRING_H
#define XTEE_STRING_H

#include "xtee_error.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* These struct types are basically wrapper around char strings.
 * They are mostly used to avoid repeated calls to strlen and to more easily
 * compare strings.
 * As both of these structs type are quite small, passing a pointer to a
 * function is only useful if structs of these type are meant to be modified
 * by this function. */

/* XteeString is used for dynamically-allocated strings. */
typedef struct XteeString {
        char *s;
        size_t n;
} XteeString;

/* XteeConstString can be used either as a "reference" to an already existing
 * XteeString or simply be initialized with a string literal.
 * It does not manage any memory allocation and is mostly used as a generic
 * string wrapper. */
typedef struct XteeConstString {
        const char *s;
        size_t n;
} XteeConstString;

typedef struct XteeStringList {
        XteeString *data;
        size_t n;
} XteeStringList;

#define XTEE_CONSTSTRING_STATIC_INIT(x) { x, sizeof(x) - 1 }

/* When using this function, only use memory allocated char strings and DO NOT
 * free it after calling this function.
 * The memory allocation is "moved" to and managed through the returned
 * XteeString. */
XteeString move_string_to_XteeString(char *s);

XteeString XteeString_create_from_charstring(const char *s);

XteeString XteeConstString_to_new_XteeString(const XteeConstString);
XteeString XteeString_merge_char_strings(const char *s1, const char *s2);
XteeString XteeString_merge_charstring_XteeConstString(
                const char *charstr,
                const XteeConstString cstr);
XteeString XteeString_merge_XteeConstString_charstring(
                const XteeConstString cstr, const char *charstr);
XteeString XteeString_merge_XteeConstStrings(
                const XteeConstString cstr1, const XteeConstString cstr2);
char *xtee_string_merge_char_strings(const char *s1, const char *s2);
/* Merge all char strings in block until NULL is encountered. */
char *xtee_string_merge_char_strings_block(const char **strs);

void free_XteeStringList(XteeStringList list);
void xtee_add_charstring_to_XteeStringList(XteeStringList *list,
                const char *s);
void xtee_remove_string_in_XteeStringList(XteeStringList *list, size_t i);

inline void
free_XteeString(XteeString xteestr)
{
        free(xteestr.s);
}

inline XteeConstString
XteeString_to_XteeConstString(const XteeString xteestr)
{
        XteeConstString const_xteestr;

        const_xteestr.s = xteestr.s;
        const_xteestr.n = xteestr.n;

        return const_xteestr;
}

inline XteeString
XteeString_duplicate(const XteeString str)
{
        return XteeConstString_to_new_XteeString(
                        XteeString_to_XteeConstString(str));
}

inline bool
xtee_strcmp_bool(const char *s1, const char *s2)
{
        return (strcmp(s1, s2) == 0);
}

inline bool
xtee_is_charstring_number(const char *s)
{
        while(*s != '\0') {
                if (!isdigit((int)*s)) {
                        return false;
                }
                ++s;
        }
        return true;
}

inline char *
xtee_strdup_or_exit(const char *s)
{
        char *r;

        if (!(r = strdup(s))) {
                fatal_error("strdup failed");
        }
        return r;
}

inline void
init_XteeStringList(XteeStringList *list)
{
        list->data = NULL;
        list->n = 0;
}


#endif
