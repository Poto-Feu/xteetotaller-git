#ifndef XTEE_GUI_NEW_PREFIX_H
#define XTEE_GUI_NEW_PREFIX_H

#include "xtee_gui_common.h"
#include "xtee_gui_form.h"
#include "xtee_prefix.h"
#include "xtee_utils.h"

typedef struct XteeGuiNewPrefixData {
        XteeGuiBasicFormDialog dialog;
} XteeGuiNewPrefixData;

extern unsigned xtee_new_prefix_arch_choice_i;
extern char *arch_choices_str[3];
extern const unsigned int arch_choices_n;
extern XteeGuiNewPrefixData xtee_gui_new_prefix_data;

XTEE_XT_CALLBACK_FUNC(new_prefix_dialog_cancel_cb);
XTEE_XT_CALLBACK_FUNC(new_prefix_dialog_arch_radiobox_cb);

void show_new_prefix_dialog(void);
void xtee_gui_new_prefix_free(void);
void create_new_prefix_arch_radiobox(XteeGuiBasicFormDialog *dialog);

#endif
