#include "xtee_program.h"
#include "xtee_prefix.h"
#include "xtee_utils.h"

void
xtee_init_program(XteeProgram *program, const char* id, const char *exe_path)
{
        program->id = XteeString_create_from_charstring(id);
        program->exe_path = XteeString_create_from_charstring(exe_path);
        program->overrides = NULL;
        program->overrides_n = 0;
        xtee_date_set_to_current_day(&program->creation_date);
}

void
xtee_free_program(XteeProgram *program)
{
        free_XteeString(program->id);
        free_XteeString(program->exe_path);
}
