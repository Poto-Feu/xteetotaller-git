#ifndef XTEE_PROGRAM_H
#define XTEE_PROGRAM_H

#include "xtee_date.h"
#include "xtee_dll.h"
#include "xtee_string.h"
#include "xtee_utils.h"

#define XTEE_PROGRAM_ID_MAX_LEN XTEE_MAX_ID_LEN

typedef struct XteeProgram {
        XteeString id;
        XteeString exe_path;
        XteeDate creation_date;
        XteeDllOverride *overrides;
        size_t overrides_n;
        XteeStringList args_list;
} XteeProgram;

void xtee_init_program(XteeProgram *program,
                const char* id, const char *exe_path);
void xtee_free_program(XteeProgram *program);
void xtee_program_add_argument(XteeProgram *restrict program,
                const char *restrict arg_str);
void xtee_program_remove_argument(XteeProgram *program, size_t i);

#endif
