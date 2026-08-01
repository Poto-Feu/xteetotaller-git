#include "xtee_gui_program_options.h"
#include "xtee_gui.h"
#include "xtee_gui_common.h"
#include "xtee_state.h"

#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/SelectioB.h>
#include <Xm/TextF.h>

static XteeGuiProgramOptionsData xtee_gui_program_options_data = { 0 };
static Widget new_arg_dialog = NULL;
static const size_t args_field_i = 0;

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
                gui_set_message_area_string("Program data saved.");
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

static
XTEE_XT_CALLBACK_FUNC(add_argument_dialog_cb)
{
        XteeProgram *program;
        XteeGuiBasicFormDialog *dialog;
        XteeGuiListButtonsForm *listbtns_form;
        XmSelectionBoxCallbackStruct *cbs;
        char *textfield_str;
        XmString temp_str;

        (void)w;
        (void)client_data;
        cbs = (XmSelectionBoxCallbackStruct *)call_data;
        dialog = &xtee_gui_program_options_data.dialog;
        switch (cbs->reason) {
                case XmCR_OK:
                        print_info("Adding new argument.");
                        break;
                case XmCR_CANCEL:
                        XtUnmanageChild(w);
                        return;
                default:
                        return;
        }
        textfield_str = xtee_gui_get_textfield_child_string(w);
        if (textfield_str[0] == '\0') {
                gui_print_error_log_and_dialog("Argument value is empty.",
                                xtee_gui_get_main_win());
                XtFree(textfield_str);
                return;
        }
        program = xtee_state_get_loaded_program();
        listbtns_form = &dialog->fields[args_field_i].data.listbuttons_form;
        xtee_program_add_argument(program, textfield_str);
        temp_str = XmStringCreateLocalized(textfield_str);
        XtFree(textfield_str);
        XmListAddItem(listbtns_form->list, temp_str, 0);
        XmStringFree(temp_str);
        XtUnmanageChild(w);
}

static
XTEE_XT_CALLBACK_FUNC(program_options_dialog_add_arg_cb)
{
        XteeGuiBasicFormDialog *dialog;
        Widget textfield_w;

        (void)w;
        (void)client_data;
        (void)call_data;
        print_info("Opening Add argument dialog.");
        dialog = &xtee_gui_program_options_data.dialog;

        if (!new_arg_dialog) {
                Arg args[5];
                Cardinal n = 0;

                XtSetArg(args[n], XmNdialogStyle,
                                XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
                XtSetArg(args[n], XmNtitle, "Add argument"); n++;
                XtSetArg(args[n], XmNautoUnmanage, False); n++;
                new_arg_dialog = XmCreatePromptDialog(dialog->dialog_form,
                                "add_argument_dialog",
                                args,
                                n);
                XtAddCallback(new_arg_dialog, XmNokCallback,
                                add_argument_dialog_cb, NULL);
                XtAddCallback(new_arg_dialog, XmNcancelCallback,
                                add_argument_dialog_cb, NULL);
                xtee_gui_hide_dialog_help_button(new_arg_dialog);
        }
        textfield_w = xtee_gui_get_textfield_child_widget(new_arg_dialog);
        XmTextFieldSetString(textfield_w, "");
        XtManageChild(new_arg_dialog);
}

static void
add_program_cmd_line_args_to_list(XteeGuiBasicFormDialog *dialog)
{
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

static void
set_program_args_widgets(void)
{
        const size_t add_arg_btn_i = 0;
        const size_t remove_arg_btn_i = 1;
        XteeGuiBasicFormDialog *dialog;
        XteeGuiListButtonsForm *listbuttons_form;
        XteeGuiFormField *last_field;
        Widget add_arg_btn, remove_arg_btn;

        dialog = &xtee_gui_program_options_data.dialog;
        last_field = get_last_XteeGuiFormField_data(dialog);
        listbuttons_form = &last_field->data.listbuttons_form;
        remove_arg_btn = listbuttons_form->buttons[remove_arg_btn_i];
        add_arg_btn = listbuttons_form->buttons[add_arg_btn_i];
        XtSetSensitive(remove_arg_btn, False);
        XtAddCallback(add_arg_btn, XmNactivateCallback,
                        program_options_dialog_add_arg_cb,
                        NULL);
}
void
xtee_gui_show_program_options_dialog(Widget main_win)
{
        XteeString program_str;
        static bool is_dialog_ready = false;
        XteeGuiBasicFormDialog *dialog;
        enum XteeGetProgramDataStatus status;

        /* TODO: implement Remove Argument button */
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
                                true);
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
                                true);
                set_program_args_widgets();
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
