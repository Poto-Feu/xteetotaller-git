#include "xtee_string.h"
#include "xtee_error.h"

#include <string.h>

extern inline void XteeString_free(XteeString xteestr);
extern inline XteeConstString XteeString_to_XteeConstString(
                const XteeString xteestr);
extern inline bool xtee_strcmp_bool(const char *s1, const char *s2);
extern inline bool xtee_is_charstring_number(const char *s);
extern inline char *xtee_strdup_or_exit(const char *s);
extern inline XteeString XteeString_duplicate(const XteeString str);
extern inline void init_XteeStringList(XteeStringList *list);

XteeString
move_string_to_XteeString(char *s)
{
        XteeString xtee_str;

        xtee_str.s = s;
        xtee_str.n = strlen(s);

        return xtee_str;
}

XteeString XteeString_create_from_charstring(const char *s)
{
        XteeString xtee_str;
        size_t len;

        len = strlen(s);
        xtee_str.s = malloc_or_exit(len + 1);
        memcpy(xtee_str.s, s, len + 1);
        xtee_str.n = len;

        return xtee_str;
}

XteeString
XteeConstString_to_new_XteeString(const XteeConstString cstr)
{
        XteeString new_str;

        new_str.s = strdup(cstr.s);
        new_str.n = cstr.n;

        return new_str;
}

XteeString
XteeString_merge_char_strings(const char *s1, const char *s2)
{
        XteeString new_str;
        size_t s1_len = strlen(s1);
        size_t s2_len = strlen(s2);
        size_t total_len = s1_len + s2_len;

        new_str.s = malloc_or_exit(total_len + 1);
        memcpy(new_str.s, s1, s1_len);
        memcpy(new_str.s + s1_len, s2, s2_len);
        new_str.s[total_len] = '\0';
        new_str.n = total_len;

        return new_str;
}

char *
xtee_string_merge_char_strings(const char *s1, const char *s2)
{
        char *rtrn_str;
        size_t total_len;

        total_len = strlen(s1) + strlen(s2);
        rtrn_str = malloc_or_exit(total_len + 1);
        rtrn_str[0] = '\0';
        strcat(rtrn_str, s1);
        strcat(rtrn_str, s2);
        return rtrn_str;
}

char *
xtee_string_merge_char_strings_block(const char **strs)
{
        const char **strs_count_ptr;
        char *rtrn_str = NULL;
        size_t final_str_len = 0;

        if (!(*strs)) {
                return NULL;
        }
        strs_count_ptr = strs;

        do {
                final_str_len += strlen(*strs_count_ptr);
        } while (*(++strs_count_ptr));

        rtrn_str = malloc_or_exit(final_str_len + 1);
        rtrn_str[0] = '\0';
        do {
                strcat(rtrn_str, *strs);
        } while (*(++strs));

        return rtrn_str;
}

XteeString
XteeString_merge_charstring_XteeConstString(const char *charstr,
                const XteeConstString cstr)
{
        XteeString new_str;
        size_t charstr_len = strlen(charstr);
        size_t total_len = charstr_len + cstr.n;

        new_str.s = malloc_or_exit(total_len + 1);
        memcpy(new_str.s, charstr, charstr_len);
        memcpy(new_str.s + charstr_len, cstr.s, cstr.n);
        new_str.s[total_len] = '\0';
        new_str.n = total_len;

        return new_str;
}

XteeString
XteeString_merge_XteeConstString_charstring(const XteeConstString cstr,
                const char *charstr)
{
        XteeString new_str;
        size_t charstr_len = strlen(charstr);
        size_t total_len = charstr_len + cstr.n;

        new_str.s = malloc_or_exit(total_len + 1);
        memcpy(new_str.s, cstr.s, cstr.n);
        memcpy(new_str.s + cstr.n, charstr, charstr_len);
        new_str.s[total_len] = '\0';
        new_str.n = total_len;

        return new_str;
}

XteeString
XteeString_merge_XteeConstStrings(const XteeConstString cstr1,
                const XteeConstString cstr2)
{
        XteeString new_str;
        size_t total_len;

        total_len = cstr1.n + cstr2.n;
        new_str.s = malloc_or_exit(total_len + 1);
        memcpy(new_str.s, cstr1.s, cstr1.n);
        memcpy(new_str.s + cstr1.n, cstr2.s, cstr2.n);
        new_str.s[total_len] = '\0';
        new_str.n = total_len;

        return new_str;
}

void
XteeStringList_free(XteeStringList list)
{
        for (size_t i = 0; i < list.n; ++i) {
                XteeString_free(list.data[i]);
        }
        free(list.data);
}

void
xtee_add_charstring_to_XteeStringList(XteeStringList *list,
                const char *s)
{
        XteeString *tmp_ptr;

        tmp_ptr = realloc_or_exit(
                        list->data, (list->n + 1) * sizeof(*tmp_ptr));
        list->data = tmp_ptr;
        list->data[list->n] = XteeString_create_from_charstring(s);
        ++list->n;
}

void
xtee_remove_string_in_XteeStringList(XteeStringList *list, size_t i)
{
        XteeString_free(list->data[i]);
        memmove(list->data + i, list->data + i + 1,
                        (list->n - i - 1) * sizeof(XteeString));
        --list->n;
}

XteeStringList
XteeStringList_duplicate(XteeStringList list)
{
        XteeStringList new_list = { 0 };

        new_list.data = malloc_or_exit(list.n * sizeof(*new_list.data));
        new_list.n = list.n;
        for (size_t i = 0; i < list.n; ++i) {
                new_list.data[i] = XteeString_duplicate(list.data[i]);
        }
        return new_list;
}

void
XteeConstStringList_free(XteeConstStringList list)
{
        free(list.data);
}
