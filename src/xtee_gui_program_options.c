#include "xtee_gui_program_options.h"
#include "xtee_gui.h"
#include "xtee_gui_common.h"
#include "xtee_state.h"

#include <Xm/Form.h>

static XteeGuiProgramOptionsData xtee_gui_program_options_data = { 0 };

static
XTEE_XT_CALLBACK_FUNC(program_options_unmap_cb)
{
        (void)w;
        (void)client_data;
        (void)call_data;
        xtee_state_unload_program();
}

void
xtee_gui_show_program_options_dialog(Widget main_win)
{
        XteeString program_str;
        static bool is_dialog_ready = false;
        XteeGuiBasicFormDialog *dialog;
        enum XteeGetProgramDataStatus status;

        dialog = &xtee_gui_program_options_data.dialog;

        if (!is_dialog_ready) {
                XteeString btns_names_array[] = {
                        XTEE_CONSTSTRING_STATIC_INIT("add_argument_button"),
                        XTEE_CONSTSTRING_STATIC_INIT("remove_argument_button"),
                };
                XteeStringList cmd_line_btns_names = {
                        btns_names_array, XTEE_ARRAY_LEN(btns_names_array)
                };
                char *cmd_line_btns_strs[] = {
                        "Add argument",
                        "Remove argument",
                };
                /* Auto-unmanage for now */
                create_XteeGuiBasicFormDialog(dialog,
                                main_win,
                                "program_options_dialog",
                                "Program options",
                                true);
                XtAddCallback(dialog->dialog_form, XmNunmapCallback,
                                program_options_unmap_cb, NULL);
                add_list_buttons_field_to_XteeGuiBasicFormDialog(dialog,
                                "cmd_line_args_form",
                                "Command line arguments: ",
                                cmd_line_btns_names,
                                cmd_line_btns_strs,
                                false);
                manage_XteeGuiBasicFormDialog_fields(
                                dialog);
                is_dialog_ready = true;
        }
        if (xtee_gui_get_selected_program_string(&program_str) != 0) {
                fatal_error("Cannot get selected program string.");
        }

        if ((status = xtee_state_set_loaded_program(program_str.s))
                        != XTEE_GET_PROGRAM_DATA_OK) {
                xtee_gui_print_get_program_data_error(status);
                return;
        }
        XtManageChild(dialog->dialog_form);
}
