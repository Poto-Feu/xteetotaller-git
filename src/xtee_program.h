#ifndef XTEE_PROGRAM_H
#define XTEE_PROGRAM_H

#include "xtee_config.h"
#include "xtee_date.h"
#include "xtee_dll.h"
#include "xtee_string.h"
#include "xtee_utils.h"

#define XTEE_PROGRAM_ID_MAX_LEN XTEE_MAX_ID_LEN

#define XTEE_PROGRAM_EXE_PATH_MAX_LEN (XTEE_VAR_VALUE_MAX_LEN - 1)

typedef struct XteeProgram {
        XteeString id;
        XteeString exe_path;
        XteeString x11_display_str;
        XteeDate creation_date;
        XteeDllOverride *overrides;
        size_t overrides_n;
        XteeStringList args_list;
        bool is_x11_display_set;
        bool is_damavand_on;
} XteeProgram;

void xtee_init_program(XteeProgram *program,
                const char* id, const char *exe_path);
void xtee_free_program(XteeProgram *program);
void xtee_program_add_argument(XteeProgram *restrict program,
                const char *restrict arg_str);
void xtee_program_remove_argument(XteeProgram *program, size_t i);

/* On success, 0 is returned and error_str is left untouched.
 * Else, error_str will be set to point to a string with the error message. */
int xtee_program_set_executable_path(XteeProgram *program,
                const char *exe_path, char **error_str);

#endif
