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
        program->is_x11_display_set = false;
        program->x11_display_str.s = NULL;
        program->x11_display_str.n = 0;
        xtee_date_set_to_current_day(&program->creation_date);
        init_XteeStringList(&program->args_list);
}

void
xtee_free_program(XteeProgram *program)
{
        XteeString_free(program->id);
        XteeString_free(program->exe_path);
        XteeStringList_free(program->args_list);

        if (program->is_x11_display_set) {
                XteeString_free(program->x11_display_str);
        }
}

void
xtee_program_add_argument(XteeProgram *restrict program,
                const char *restrict arg_str)
{
        print_info("Adding program command line argument.");
        xtee_add_charstring_to_XteeStringList(&program->args_list, arg_str);
}

void
xtee_program_remove_argument(XteeProgram *program, size_t i)
{
        print_info("Removing program command line argument.");
        xtee_remove_string_in_XteeStringList(&program->args_list, i);
}
