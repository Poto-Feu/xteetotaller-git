#ifndef XTEE_GUI_PROGRAM_OPTIONS_H
#define XTEE_GUI_PROGRAM_OPTIONS_H

#include "xtee_gui_form.h"

typedef struct XteeGuiProgramOptionsData {
        XteeGuiBasicFormDialog dialog;
} XteeGuiProgramOptionsData;

void xtee_gui_show_program_options_dialog(Widget main_win);

#endif
