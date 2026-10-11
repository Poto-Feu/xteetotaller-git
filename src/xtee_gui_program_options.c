/*
 * Copyright (C) 2026 Adrien Saad
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "xtee_gui_program_options.h"
#include "xtee_gui.h"
#include "xtee_gui_common.h"
#include "xtee_state.h"

#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/SelectioB.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>

static const size_t exe_path_field_i = 0;

static const size_t args_field_i = 1;
static const size_t add_arg_btn_i = 0;
static const size_t remove_arg_btn_i = 1;

static const size_t env_vars_field_i = 5;
static const size_t add_var_btn_i = 0;
static const size_t remove_var_btn_i = 1;

static const size_t x11_display_field_i = 2;
static const size_t enable_damavand_field_i = 3;
static const size_t force_vsync_field_i = 4;

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
        const char *tmp_ptr;
        XteeGuiBasicFormDialog *dialog;
        XteeProgram *program;
        XteeGuiFormField *exe_path_field, *x11_display_field,
                         *enable_damavand_field, *force_vsync_field;

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
        enable_damavand_field = dialog->fields + enable_damavand_field_i;
        program->is_damavand_on = (bool)XmToggleButtonGadgetGetState(
                        enable_damavand_field->data.toggle_g);
        force_vsync_field = dialog->fields + force_vsync_field_i;
        program->is_force_vsync_on = (bool)XmToggleButtonGadgetGetState(
                        force_vsync_field->data.toggle_g);

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
        XteeGuiBasicFormDialog *dialog;
        (void)w;
        (void)call_data;
        dialog = (XteeGuiBasicFormDialog *)client_data;
        xtee_gui_reset_message_area_string_CALL;
        XtUnmanageChild(dialog->dialog_form);
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
                args_strs[i] = xtee_new_XmString(program->args_list.data[i].s);
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
        const XmListCallbackStruct *cbs;
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
        const XmSelectionBoxCallbackStruct *cbs;
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
        static Widget new_arg_dialog = NULL;
        XteeGuiBasicFormDialog *dialog;
        Widget textfield_w;

        (void)w;
        (void)call_data;
        print_info("Opening Add argument dialog.");
        dialog = (XteeGuiBasicFormDialog *)client_data;

        if (!new_arg_dialog) {
                Arg args[] = {
                        { XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL },
                        { XmNtitle, (XtArgVal)"Add argument" },
                        { XmNautoUnmanage, False },
                };
                new_arg_dialog = XmCreatePromptDialog(dialog->dialog_form,
                                "add_argument_dialog", args, XtNumber(args));
                XtAddCallback(new_arg_dialog, XmNokCallback,
                                add_argument_dialog_cb,
                                dialog);
                XtAddCallback(new_arg_dialog, XmNcancelCallback,
                                add_argument_dialog_cb,
                                dialog);
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
        const XmPushButtonCallbackStruct *cbs;
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
set_program_args_widgets(XteeGuiBasicFormDialog *dialog,
                XteeGuiFormField *args_field)
{
        XteeGuiListButtonsForm *listbuttons_form;
        Widget add_arg_btn;
        Widget remove_arg_btn;

        listbuttons_form = &args_field->data.listbuttons_form;
        add_arg_btn = listbuttons_form->buttons[add_arg_btn_i];
        remove_arg_btn = listbuttons_form->buttons[remove_arg_btn_i];
        XtAddCallback(add_arg_btn, XmNactivateCallback, add_arg_btn_cb,
                        dialog);
        XtAddCallback(remove_arg_btn, XmNactivateCallback, remove_arg_btn_cb,
                        dialog);
        /* Disable "Remove argument" button as nothing is selected at first. */
        XteeGuiListButtonsForm_button_set_sensitive(listbuttons_form,
                        remove_arg_btn_i, False);
}

typedef struct NewEnvVarOkData {
        XteeGuiBasicFormDialog *dialog;
        XteeGuiBasicFormDialog *parent_dialog;
} NewEnvVarOkData;

static bool
charstring_has_spaces(const char *s)
{
        while (*s != '\0') {
                if (isspace(*s)) {
                        return true;
                }
                ++s;
        }
        return false;
}

/* Environment variables in the list will be shown in the "name=value"
 * format. */
static void
reload_program_env_vars_list(XteeGuiBasicFormDialog *dialog)
{
        XteeGuiFormField *env_vars_field;
        Widget list_w;
        XteeProgram *program;
        XmString *env_strs;

        program = xtee_state_get_loaded_program();
        env_vars_field = XteeGuiBasicFormDialog_get_field(dialog,
                        env_vars_field_i);
        list_w = env_vars_field->data.listbuttons_form.list;
        if (program->env_data.n == 0) {
                XmListDeleteAllItems(list_w);
                return;
        }
        env_strs = malloc_or_exit(program->env_data.n * sizeof(XmString));
        for (size_t i = 0; i < program->env_data.n; ++i) {
                XteeEnvData *current_env_data;
                char *combined_str;
                size_t var_name_len, var_value_len, total_str_len;

                current_env_data = program->env_data.block + i;
                var_name_len = strlen(current_env_data->name);
                var_value_len = strlen(current_env_data->value);
                total_str_len = var_name_len + var_value_len + 1;
                combined_str = malloc_or_exit(total_str_len + 1);
                memcpy(combined_str, current_env_data->name, var_name_len);
                combined_str[var_name_len] = '=';
                memcpy(combined_str + var_name_len + 1,
                                current_env_data->value, var_value_len);
                combined_str[total_str_len] = '\0';
                env_strs[i] = xtee_new_XmString(combined_str);
                free(combined_str);
        }
        XtVaSetValues(list_w,
                        XmNitems, env_strs,
                        XmNitemCount, (int)program->env_data.n,
                        NULL);
        xtee_gui_free_XmStringTable_items(env_strs, program->env_data.n);
        free(env_strs);
}

static
XTEE_XT_CALLBACK_FUNC(new_env_var_ok_cb)
{
        NewEnvVarOkData *new_env_var_ok_data;
        XteeGuiBasicFormDialog *dialog, *parent_dialog;
        XteeGuiFormField *name_field, *value_field;
        XteeProgram *program;
        char *var_name, *var_value;

        (void)w;
        (void)call_data;
        new_env_var_ok_data = (NewEnvVarOkData *)client_data;
        dialog = new_env_var_ok_data->dialog;
        parent_dialog = new_env_var_ok_data->parent_dialog;
        name_field = XteeGuiBasicFormDialog_get_field(dialog, 0);
        var_name = get_XteeGuiFormField_string_value(name_field);
        if (charstring_has_spaces(var_name)) {
                gui_print_error_dialog("Environment variable name must not "
                                "have any spaces.", dialog->dialog_form);
                free(var_name);
                return;
        }
        value_field = XteeGuiBasicFormDialog_get_field(dialog, 1);
        var_value = get_XteeGuiFormField_string_value(value_field);
        if (isspace(*var_value)) {
                gui_print_error_dialog("Environment variable value "
                                "must not start with a space character.",
                                dialog->dialog_form);
                free(var_name);
                free(var_value);
                return;
        } else if (*var_value == '\0') {
                gui_print_error_dialog("Environment variable value "
                                "is empty.", dialog->dialog_form);
                free(var_name);
                free(var_value);
                return;
        }

        program = xtee_state_get_loaded_program();
        xtee_program_add_env_var(program, var_name, var_value);
        free(var_name);
        free(var_value);
        reload_program_env_vars_list(parent_dialog);
        XtUnmanageChild(dialog->dialog_form);
}

static
XTEE_XT_CALLBACK_FUNC(new_env_var_cancel_cb)
{
        XteeGuiBasicFormDialog *dialog;
        (void)w;
        (void)call_data;
        dialog = (XteeGuiBasicFormDialog *)client_data;
        XtUnmanageChild(dialog->dialog_form);
}

static
XTEE_XT_CALLBACK_FUNC(add_var_btn_cb)
{
        static XteeGuiBasicFormDialog new_var_dialog = { 0 };
        static bool is_dialog_ready = false;

        (void)w;
        (void)call_data;
        print_info("Opening Add variable dialog.");

        if (!is_dialog_ready) {
                static NewEnvVarOkData new_env_var_ok_data;
                XteeGuiBasicFormDialog *parent_dialog;
                Dimension parent_dialog_width;
                Dimension dialog_width, dialog_height;

                parent_dialog = (XteeGuiBasicFormDialog *)client_data;
                create_XteeGuiBasicFormDialog(&new_var_dialog,
                                parent_dialog->dialog_form,
                                "new_env_var_dialog",
                                "New environment variable");
                new_env_var_ok_data.parent_dialog = parent_dialog;
                new_env_var_ok_data.dialog = &new_var_dialog;
                XtAddCallback(new_var_dialog.ok_btn, XmNactivateCallback,
                                new_env_var_ok_cb,
                                &new_env_var_ok_data);
                XtAddCallback(new_var_dialog.cancel_btn, XmNactivateCallback,
                                new_env_var_cancel_cb,
                                &new_var_dialog);
                add_label_text_form_field_to_XteeGuiBasicFormDialog(
                                &new_var_dialog,
                                "env_var_name_text",
                                "Name: ",
                                NULL, 0);
                add_label_text_form_field_to_XteeGuiBasicFormDialog(
                                &new_var_dialog,
                                "env_var_value_text",
                                "Value: ",
                                NULL, 0);
                XtVaGetValues(parent_dialog->dialog_form,
                                XmNwidth, &parent_dialog_width,
                                NULL);
                dialog_width = parent_dialog_width - 150;
                dialog_height = 175;
                XtVaSetValues(new_var_dialog.dialog_form,
                                XmNwidth, dialog_width,
                                XmNheight, dialog_height,
                                NULL);
                is_dialog_ready = true;
        }
        reset_XteeGuiBasicFormDialog_fields(&new_var_dialog);
        XtManageChild(new_var_dialog.dialog_form);
}

static
XTEE_XT_CALLBACK_FUNC(remove_var_btn_cb)
{
        XteeGuiBasicFormDialog *dialog;
        const XmPushButtonCallbackStruct *cbs;
        XteeProgram *program;
        unsigned int *list_positions;
        int list_positions_n;
        Widget list_w;

        (void)w;
        print_info("Remove variable button clicked.");

        cbs = (XmPushButtonCallbackStruct *)call_data;
        dialog = (XteeGuiBasicFormDialog *)client_data;
        list_w = dialog->fields[env_vars_field_i].data.listbuttons_form.list;
        program = xtee_state_get_loaded_program();
        if (cbs->reason != XmCR_ACTIVATE) {
                return;
        }
        XtVaGetValues(list_w,
                        XmNselectedPositionCount, &list_positions_n,
                        XmNselectedPositions, &list_positions,
                        NULL);
        if (!list_positions_n || list_positions_n > 1) {
                puts("lol");
                return;
        }
        xtee_env_data_remove_var_from_handler(&program->env_data,
                        *list_positions - 1);
        reload_program_env_vars_list(dialog);
}

static void
set_program_env_var_widgets(XteeGuiBasicFormDialog *dialog,
                XteeGuiFormField *env_var_field)
{
        XteeGuiListButtonsForm *listbuttons_form;
        Widget add_var_btn;
        Widget remove_var_btn;

        listbuttons_form = &env_var_field->data.listbuttons_form;
        add_var_btn = listbuttons_form->buttons[add_var_btn_i];
        remove_var_btn = listbuttons_form->buttons[remove_var_btn_i];
        XtAddCallback(add_var_btn, XmNactivateCallback, add_var_btn_cb,
                        dialog);
        XtAddCallback(remove_var_btn, XmNactivateCallback, remove_var_btn_cb,
                        dialog);
}

static void
set_program_exe_path_field(const XteeGuiBasicFormDialog *dialog,
                XteeProgram *program)
{
        XteeGuiFormField *exe_path_field;

        exe_path_field = XteeGuiBasicFormDialog_get_field(
                        dialog, exe_path_field_i);
        XmTextFieldSetString(exe_path_field->data.ltf.text,
                        program->exe_path.s);
}

void
xtee_gui_show_program_options_dialog(Widget main_win,
                XteeGuiBasicFormDialog *dialog)
{
        static bool is_dialog_ready = false;
        XteeString program_str;
        XteeProgram *loaded_program;
        XteeGuiFormField *enable_damavand_field, *force_vsync_field;
        enum XteeGetProgramDataStatus status;

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
                XteeString env_var_btns_names_array[] = {
                        XTEE_CONSTSTRING_STATIC_INIT("add_env_var_button"),
                        XTEE_CONSTSTRING_STATIC_INIT("remove_env_var_button"),
                };
                XteeStringList env_var_btns_names = {
                        env_var_btns_names_array,
                        XTEE_ARRAY_LEN(env_var_btns_names_array)
                };
                char *env_var_btns_strs[] = {
                        "Add variable",
                        "Remove variable",
                };
                Arg text_args[] = {
                        { XmNmaxLength, XTEE_MAX_FILE_PATH_LEN },
                };
                Arg filesel_args[4];
                XmString tmp_str;
                XteeGuiFormField *args_field, *env_vars_field;
                unsigned int filesel_args_n = 0;

                tmp_str = xtee_new_XmString("Program file selection");
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
                                dialog);
                XtAddCallback(dialog->cancel_btn, XmNactivateCallback,
                                program_options_dialog_cancel_cb,
                                dialog);
                add_ltpf_field_to_XteeGuiBasicFormDialog(dialog,
                                "exe_path_form",
                                "Executable path: ",
                                text_args, XtNumber(text_args),
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
                                dialog);
                set_program_args_widgets(dialog, args_field);
                add_label_text_form_field_to_XteeGuiBasicFormDialog(dialog,
                                "x11_display_text",
                                "X11 Display (leave empty for default)",
                                NULL, 0);
                add_toggle_form_field_to_XteeGuiBasicFormDialog(dialog,
                                "enable_damavand_toggle",
                                "Enable WineD3D Damavand Vulkan backend "
                                " (for Wine >= 8.0)");
                add_toggle_form_field_to_XteeGuiBasicFormDialog(dialog,
                                "force_vsync_toggle",
                                "Force VSync (for OpenGL and Vulkan wtih Mesa "
                                "only)");
                add_list_buttons_field_to_XteeGuiBasicFormDialog(dialog,
                                "env_vars_form",
                                "Environment variables: ",
                                env_var_btns_names,
                                env_var_btns_strs,
                                true);
                env_vars_field = XteeGuiBasicFormDialog_get_field(dialog,
                                env_vars_field_i);
                set_program_env_var_widgets(dialog, env_vars_field);
                is_dialog_ready = true;
        }
        if (xtee_gui_get_selected_program_string(&program_str) != 0) {
                print_warning("No selected item in programs list");
                return;
        }
        reset_XteeGuiBasicFormDialog_fields(dialog);

        if ((status = xtee_state_set_loaded_program(program_str.s))
                        != XTEE_GET_PROGRAM_DATA_OK) {
                xtee_gui_print_get_program_data_error(status);
                return;
        }

        loaded_program = xtee_state_get_loaded_program();
        set_program_exe_path_field(dialog, loaded_program);
        reload_program_cmd_line_args_list(dialog);
        reload_program_env_vars_list(dialog);

        /* Set x11_display_field if set */
        if (loaded_program->is_x11_display_set) {
                XteeGuiFormField *x11_display_field;

                x11_display_field = dialog->fields + x11_display_field_i;
                XteeGuiLabelTextForm_set_text_string(
                                &x11_display_field->data.ltf,
                                loaded_program->x11_display_str.s);
        }

        /* Set enable_damavand_toggle */
        enable_damavand_field = dialog->fields + enable_damavand_field_i;
        XmToggleButtonGadgetSetState(enable_damavand_field->data.toggle_g,
                        loaded_program->is_damavand_on,
                        False);
        /* Set force_vsync_toggle */
        force_vsync_field = dialog->fields + force_vsync_field_i;
        XmToggleButtonGadgetSetState(force_vsync_field->data.toggle_g,
                        loaded_program->is_force_vsync_on,
                        False);

        XtManageChild(dialog->dialog_form);
}
