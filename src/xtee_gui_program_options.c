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

static
XTEE_XT_CALLBACK_FUNC(program_options_dialog_ok_cb)
{
        XteeGuiBasicFormDialog *dialog;

        dialog = &xtee_gui_program_options_data.dialog;
        (void)w;
        (void)client_data;
        (void)call_data;
        if (xtee_state_save_loaded_program() != 0) {
                gui_print_error_log_and_dialog("Cannot save program data.",
                                dialog->dialog_form);
        } else {
                XtUnmanageChild(dialog->dialog_form);
        }
}

static
XTEE_XT_CALLBACK_FUNC(program_options_dialog_cancel_cb)
{
        XteeGuiBasicFormDialog *dialog;

        dialog = &xtee_gui_program_options_data.dialog;
        (void)w;
        (void)client_data;
        (void)call_data;
        XtUnmanageChild(dialog->dialog_form);
}


static void
add_program_cmd_line_args_to_list(XteeGuiBasicFormDialog *dialog)
{
        const size_t args_field_i = 0;
        XteeGuiListButtonsForm *args_field;
        XteeProgram *program;
        XmString *args_strs;

        program = xtee_state_get_loaded_program();
        args_field = &dialog->fields[args_field_i].data.listbuttons_form;
        args_strs = malloc_or_exit(program->args_list.n * sizeof(XmString));
        for (size_t i = 0; i < program->args_list.n; ++i) {
                args_strs[i] = XmStringCreateLocalized(
                                program->args_list.data[i].s);
        }
        XtVaSetValues(args_field->list,
                        XmNitems, args_strs,
                        XmNitemCount, (int)program->args_list.n,
                        NULL);
        xtee_gui_free_XmStringTable_items(args_strs, program->args_list.n);
        free(args_strs);
}

void
xtee_gui_show_program_options_dialog(Widget main_win)
{
        XteeString program_str;
        static bool is_dialog_ready = false;
        XteeGuiBasicFormDialog *dialog;
        enum XteeGetProgramDataStatus status;

        /* TODO: implement Add Argument button */
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
                create_XteeGuiBasicFormDialog(dialog,
                                main_win,
                                "program_options_dialog",
                                "Program options",
                                false);
                XtAddCallback(dialog->dialog_form, XmNunmapCallback,
                                program_options_unmap_cb, NULL);
                XtAddCallback(dialog->ok_btn, XmNactivateCallback,
                                program_options_dialog_ok_cb,
                                NULL);
                XtAddCallback(dialog->cancel_btn, XmNactivateCallback,
                                program_options_dialog_cancel_cb,
                                (XtPointer)dialog->dialog_form);
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
        reset_XteeGuiBasicFormDialog_fields(dialog);
        if (xtee_gui_get_selected_program_string(&program_str) != 0) {
                fatal_error("Cannot get selected program string.");
        }

        if ((status = xtee_state_set_loaded_program(program_str.s))
                        != XTEE_GET_PROGRAM_DATA_OK) {
                xtee_gui_print_get_program_data_error(status);
                return;
        }
        add_program_cmd_line_args_to_list(dialog);
        XtManageChild(dialog->dialog_form);
}
