#include "xtee_utils.h"

#include <stdlib.h>

extern inline char *xtee_bool_to_string(bool x);
extern inline bool xtee_string_to_bool(const char *s);
extern inline bool xtee_is_char_ascii_alphanum(char c);

void
xtee_env_data_add_var_to_handler(XteeEnvDataHandler *handler,
                char *name, char *value)
{
        XteeEnvData *tmp_ptr;

        tmp_ptr = realloc(handler->block,
                        (handler->n + 1) * sizeof(XteeEnvData));
        handler->block = tmp_ptr;
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
