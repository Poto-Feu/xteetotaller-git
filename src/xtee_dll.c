#include "xtee_dll.h"
#include "xtee_utils.h"

void
xtee_dll_add_override_to_list(XteeDllOverrideList *list,
                XteeConstString dll_name, enum XteeDllOverrideType type)
{
        const size_t start_capacity = 4;

        if (list->capacity == 0) {
                list->block = malloc_or_exit(
                                start_capacity * sizeof(*list->block));
                list->capacity = start_capacity;
        } else if (list->capacity == list->n) {
                XteeDllOverride *tmp_ptr;

                tmp_ptr = realloc_or_exit(list->block,
                                list->capacity * 2 * sizeof(*list->block));
                list->block = tmp_ptr;
                list->capacity *= 2;
        }
        list->block[list->n].name = XteeConstString_to_new_XteeString(
                        dll_name);
        list->block[list->n].type = type;
        ++list->n;
}

char *
xtee_dll_get_WINEDLLOVERRIDES_string(const XteeDllOverrideList *list)
{
        const char b_str[] = "=b";
        const char n_str[] = "=n";
        const char bn_str[] = "=b,n";
        const char nb_str[] = "=n,b";
        const char disable_str[] = "=";
        const size_t b_str_len = XTEE_ARRAY_STRLEN(b_str);
        const size_t n_str_len = XTEE_ARRAY_STRLEN(n_str);
        const size_t bn_str_len = XTEE_ARRAY_STRLEN(bn_str);
        const size_t nb_str_len = XTEE_ARRAY_STRLEN(nb_str);
        const size_t disable_str_len = XTEE_ARRAY_STRLEN(disable_str);
        char *rtrn_str = NULL;
        size_t str_len = 0;
        size_t str_char_i = 0;

        if (list->n == 0) {
                return NULL;
        }

        for (size_t i = 0; i < list->n; ++i) {
                str_len += list->block[i].name.n;
                /* For semicolon separator */
                if (i != 0) {
                        ++str_len;
                }

                switch(list->block[i].type) {
                        case XTEE_DLL_OVERRIDE_B:
                                str_len += b_str_len;
                                break;
                        case XTEE_DLL_OVERRIDE_N:
                                str_len += n_str_len;
                                break;
                        case XTEE_DLL_OVERRIDE_BN:
                                str_len += bn_str_len;
                                break;
                        case XTEE_DLL_OVERRIDE_NB:
                                str_len += nb_str_len;
                                break;
                        case XTEE_DLL_OVERRIDE_DISABLE:
                                str_len += disable_str_len;
                                break;
                        default:
                                fatal_error("Wrong dll override type");
                                break;
                }
        }
        rtrn_str = malloc_or_exit(str_len + 1);

        for (size_t i = 0; i < list->n; ++i) {
                if (i != 0) {
                        rtrn_str[str_char_i] = ';';
                        ++str_char_i;
                }
                memcpy(rtrn_str + str_char_i, list->block[i].name.s,
                                list->block[i].name.n);
                str_char_i += list->block[i].name.n;

                switch(list->block[i].type) {
                        case XTEE_DLL_OVERRIDE_B:
                                memcpy(rtrn_str + str_char_i, b_str,
                                                b_str_len);
                                str_char_i += b_str_len;
                                break;
                        case XTEE_DLL_OVERRIDE_N:
                                memcpy(rtrn_str + str_char_i, n_str,
                                                n_str_len);
                                str_char_i += n_str_len;
                                break;
                        case XTEE_DLL_OVERRIDE_BN:
                                memcpy(rtrn_str + str_char_i, bn_str,
                                                bn_str_len);
                                str_char_i += bn_str_len;
                                break;
                        case XTEE_DLL_OVERRIDE_NB:
                                memcpy(rtrn_str + str_char_i, nb_str,
                                                nb_str_len);
                                str_char_i += nb_str_len;
                                break;
                        case XTEE_DLL_OVERRIDE_DISABLE:
                                memcpy(rtrn_str + str_char_i, disable_str,
                                                disable_str_len);
                                str_char_i += disable_str_len;
                                break;
                        default:
                                fatal_error("Wrong dll override type");
                                break;
                }
        }
        rtrn_str[str_char_i] = '\0';
        return rtrn_str;
}

void
xtee_dll_free_list(XteeDllOverrideList *list)
{
        for (size_t i = 0; i < list->n; ++i) {
                XteeString_free(list->block[i].name);
        }
        free(list->block);
        list->n = 0;
        list->capacity = 0;
}
