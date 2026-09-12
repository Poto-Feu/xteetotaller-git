#include "xtee_gui_program_options.h"
#include "xtee_gui.h"
#include "xtee_gui_common.h"
#include "xtee_state.h"

#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/SelectioB.h>
#include <Xm/TextF.h>

static XteeGuiBasicFormDialog xtee_gui_program_options_dialog = { 0 };
static Widget new_arg_dialog = NULL;

static const size_t exe_path_field_i = 0;

static const size_t args_field_i = 1;
static const size_t add_arg_btn_i = 0;
static const size_t remove_arg_btn_i = 1;

static const size_t x11_display_field_i = 2;

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
        char *x11_display_string;
        char *exe_path_error_str;
        char *exe_path_str;
        char *tmp_ptr;
        XteeGuiBasicFormDialog *dialog;
        XteeProgram *program;
        XteeGuiFormField *exe_path_field, *x11_display_field;

        (void)w;
        (void)call_data;
        dialog = (XteeGuiBasicFormDialog *)client_data;
        program = xtee_state_get_loaded_program();

        exe_path_field = XteeGuiBasicFormDialog_get_field(dialog,
                        exe_path_field_i);
        exe_path_str = XmTextFieldGetString(exe_path_field->data.ltf.text);
        if (xtee_program_set_executable_path(program, exe_path_str,
                                &exe_path_error_str) != 0) {
                free(exe_path_str);
                gui_print_error_log_and_dialog(exe_path_error_str,
                                dialog->dialog_form);
                return;
        }
        free(exe_path_str);

        x11_display_field = XteeGuiBasicFormDialog_get_field(dialog,
                        x11_display_field_i);
        x11_display_string = get_XteeGuiFormField_string_value(
                        x11_display_field);
        tmp_ptr = x11_display_string;
        while (isspace(*tmp_ptr)) {
                ++tmp_ptr;
        }

        if (program->is_x11_display_set) {
                XteeString_free(program->x11_display_str);
        }

        if (*tmp_ptr != '\0') {
                program->x11_display_str = XteeString_create_from_charstring(
                                tmp_ptr);
                program->is_x11_display_set = true;
        } else {
                program->is_x11_display_set = false;
        }
        free(x11_display_string);

        if (xtee_state_save_loaded_program() != 0) {
                gui_print_error_log_and_dialog("Cannot save program data.",
                                dialog->dialog_form);
        } else {
                xtee_gui_set_message_area_string("Program data saved.");
                XtUnmanageChild(dialog->dialog_form);
        }
}

static
XTEE_XT_CALLBACK_FUNC(program_options_dialog_cancel_cb)
{
        (void)w;
        (void)client_data;
        (void)call_data;
        xtee_gui_reset_message_area_string_CALL;
        XtUnmanageChild(xtee_gui_program_options_dialog.dialog_form);
}

static void
reload_program_cmd_line_args_list(XteeGuiBasicFormDialog *dialog)
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
        XmListDeselectAllItems(args_field->list);
        xtee_gui_free_XmStringTable_items(args_strs, program->args_list.n);
        free(args_strs);
        /* As all items are deselected, disable "Remove argument" button. */
        XteeGuiListButtonsForm_button_set_sensitive(args_field,
                        remove_arg_btn_i, False);
}

static
XTEE_XT_CALLBACK_FUNC(args_list_selection_cb)
{
        XteeGuiBasicFormDialog *dialog;
        XmListCallbackStruct *cbs;
        XteeGuiListButtonsForm *field;

        (void)w;
        cbs = (XmListCallbackStruct *)call_data;
        dialog = (XteeGuiBasicFormDialog *)client_data;
        field = &dialog->fields[args_field_i].data.listbuttons_form;
        switch (cbs->reason) {
                case XmCR_SINGLE_SELECT:
                        XteeGuiListButtonsForm_button_set_sensitive(field,
                                        remove_arg_btn_i, True);
                        break;
                default:
                        break;
        }
}

static
XTEE_XT_CALLBACK_FUNC(add_argument_dialog_cb)
{
        XteeProgram *program;
        XmSelectionBoxCallbackStruct *cbs;
        XteeGuiBasicFormDialog *dialog;
        char *textfield_str;

        (void)w;
        cbs = (XmSelectionBoxCallbackStruct *)call_data;
        dialog = (XteeGuiBasicFormDialog *)client_data;
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
        xtee_program_add_argument(program, textfield_str);
        XtFree(textfield_str);
        reload_program_cmd_line_args_list(dialog);
        XtUnmanageChild(w);
}

static
XTEE_XT_CALLBACK_FUNC(add_arg_btn_cb)
{
        XteeGuiBasicFormDialog *dialog;
        Widget textfield_w;

        (void)w;
        (void)call_data;
        print_info("Opening Add argument dialog.");
        dialog = (XteeGuiBasicFormDialog *)client_data;

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
                                add_argument_dialog_cb,
                                &xtee_gui_program_options_dialog);
                XtAddCallback(new_arg_dialog, XmNcancelCallback,
                                add_argument_dialog_cb,
                                &xtee_gui_program_options_dialog);
                xtee_gui_hide_dialog_help_button(new_arg_dialog);
        }
        textfield_w = xtee_gui_get_textfield_child_widget(new_arg_dialog);
        XmTextFieldSetString(textfield_w, "");
        XtManageChild(new_arg_dialog);
}

static
XTEE_XT_CALLBACK_FUNC(remove_arg_btn_cb)
{
        XteeGuiBasicFormDialog *dialog;
        XmPushButtonCallbackStruct *cbs;
        XteeProgram *program;
        unsigned int *list_positions;
        int list_positions_n;
        Widget list_w;

        (void)w;
        print_info("Remove argument button clicked.");

        cbs = (XmPushButtonCallbackStruct *)call_data;
        dialog = (XteeGuiBasicFormDialog *)client_data;
        list_w = dialog->fields[args_field_i].data.listbuttons_form.list;
        if (cbs->reason != XmCR_ACTIVATE) {
                return;
        }
        XtVaGetValues(list_w,
                        XmNselectedPositionCount, &list_positions_n,
                        XmNselectedPositions, &list_positions,
                        NULL);
        if (list_positions_n > 1) {
                fatal_error("Multiple selections in args list.");
        }
        program = xtee_state_get_loaded_program();
        xtee_program_remove_argument(program, list_positions[0] - 1);
        reload_program_cmd_line_args_list(dialog);
}

static void
set_program_args_widgets(XteeGuiFormField *args_field)
{
        XteeGuiListButtonsForm *listbuttons_form;
        Widget add_arg_btn;
        Widget remove_arg_btn;

        listbuttons_form = &args_field->data.listbuttons_form;
        add_arg_btn = listbuttons_form->buttons[add_arg_btn_i];
        remove_arg_btn = listbuttons_form->buttons[remove_arg_btn_i];
        XtAddCallback(add_arg_btn, XmNactivateCallback, add_arg_btn_cb,
                        &xtee_gui_program_options_dialog);
        XtAddCallback(remove_arg_btn, XmNactivateCallback, remove_arg_btn_cb,
                        &xtee_gui_program_options_dialog);
        /* Disable "Remove argument" button as nothing is selected at first. */
        XteeGuiListButtonsForm_button_set_sensitive(listbuttons_form,
                        remove_arg_btn_i, False);
}

void
set_program_exe_path_field(XteeProgram *program)
{
        XteeGuiFormField *exe_path_field;

        exe_path_field = XteeGuiBasicFormDialog_get_field(
                        &xtee_gui_program_options_dialog,
                        exe_path_field_i);
        XmTextFieldSetString(exe_path_field->data.ltf.text,
                        program->exe_path.s);
}

void
xtee_gui_show_program_options_dialog(Widget main_win)
{
        XteeString program_str;
        static bool is_dialog_ready = false;
        XteeGuiBasicFormDialog *dialog;
        XteeProgram *loaded_program;
        enum XteeGetProgramDataStatus status;

        dialog = &xtee_gui_program_options_dialog;

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
                Arg text_args[4];
                Arg filesel_args[4];
                XmString tmp_str;
                XteeGuiFormField *args_field;
                unsigned text_args_n = 0;
                unsigned filesel_args_n = 0;

                tmp_str = XmStringCreateLocalized("Program file selection");
                XtSetArg(text_args[text_args_n], XmNmaxLength,
                                XTEE_MAX_FILE_PATH_LEN); text_args_n++;
                XtSetArg(filesel_args[filesel_args_n],
                                XmNdialogTitle, tmp_str); filesel_args_n++;

                create_XteeGuiBasicFormDialog(dialog,
                                main_win,
                                "program_options_dialog",
                                "Program options");
                XtAddCallback(dialog->dialog_form, XmNunmapCallback,
                                program_options_unmap_cb, NULL);
                XtAddCallback(dialog->ok_btn, XmNactivateCallback,
                                program_options_dialog_ok_cb,
                                &xtee_gui_program_options_dialog);
                XtAddCallback(dialog->cancel_btn, XmNactivateCallback,
                                program_options_dialog_cancel_cb,
                                NULL);
                add_ltpf_field_to_XteeGuiBasicFormDialog(dialog,
                                "exe_path_form",
                                "Executable path: ",
                                text_args, text_args_n,
                                filesel_args, filesel_args_n, XmFILE_REGULAR);
                XmStringFree(tmp_str);
                add_list_buttons_field_to_XteeGuiBasicFormDialog(dialog,
                                "cmd_line_args_form",
                                "Command line arguments: ",
                                cmd_line_btns_names,
                                cmd_line_btns_strs,
                                true);
                args_field = XteeGuiBasicFormDialog_get_field(dialog,
                                args_field_i);
                XtAddCallback(args_field->data.listbuttons_form.list,
                                XmNsingleSelectionCallback,
                                args_list_selection_cb,
                                &xtee_gui_program_options_dialog);
                set_program_args_widgets(args_field);
                add_label_text_form_field_to_XteeGuiBasicFormDialog(dialog,
                                "x11_display_text",
                                "X11 Display (leave empty for default)",
                                NULL, 0);
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

        loaded_program = xtee_state_get_loaded_program();
        set_program_exe_path_field(loaded_program);
        reload_program_cmd_line_args_list(dialog);

        /* Set x11_display_field if set */
        if (loaded_program->is_x11_display_set) {
                XteeGuiFormField *x11_display_field;

                x11_display_field = dialog->fields + x11_display_field_i;
                XmTextFieldSetString(x11_display_field->data.ltf.text,
                                loaded_program->x11_display_str.s);
        }

        XtManageChild(dialog->dialog_form);
}

void
xtee_gui_program_options_free(void)
{
        free_XteeGuiBasicFormDialog(&xtee_gui_program_options_dialog);
}
