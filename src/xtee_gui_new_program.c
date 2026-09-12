#include "xtee_gui_new_program.h"
#include "xtee_file.h"
#include "xtee_gui.h"
#include "xtee_gui_form.h"
#include "xtee_state.h"

static const size_t exe_path_field_i = 1;
static XteeGuiBasicFormDialog new_program_dialog = { 0 };

static
XTEE_XT_CALLBACK_FUNC(new_program_ok_cb)
{
        const size_t id_field_i = 0;
        XteePrefix *current_prefix;
        XteeProgram program;
        XteeConstString exe_path_xteestr;
        XteeGuiFormField *id_field, *exe_path_field;
        char *id_str, *exe_path_str;
        XteeGuiBasicFormDialog *dialog;
        Widget dialog_w;
        enum XteeAddProgramStatus status;

        (void)w;
        (void)call_data;
        dialog = (XteeGuiBasicFormDialog *)client_data;
        current_prefix = xtee_state_get_loaded_prefix();
        dialog_w = dialog->dialog_form;

        id_field = dialog->fields + id_field_i;
        id_str = get_XteeGuiFormField_string_value(id_field);
        if (!id_str) {
                fatal_error("program id_str is NULL (alloc error?)");
        } else if (id_str[0] == '\0') {
                gui_print_error_log_and_dialog("Program id is empty.",
                                dialog_w);
                XtFree(id_str);
                return;
        }

        exe_path_field = dialog->fields + exe_path_field_i;
        exe_path_str = get_XteeGuiFormField_string_value(exe_path_field);
        if (!exe_path_str) {
                fatal_error("exe_path_str is NULL (alloc error?)");
        } else if (exe_path_str[0] == '\0') {
                gui_print_error_log_and_dialog("Program executable path is "
                                "empty.",
                                dialog_w);
                XtFree(id_str);
                XtFree(exe_path_str);
                return;
        }
        exe_path_xteestr.s = exe_path_str;
        exe_path_xteestr.n = strlen(exe_path_str);
        if (exe_path_xteestr.n > XTEE_PROGRAM_EXE_PATH_MAX_LEN) {
                gui_print_error_log_and_dialog("Program executable path is "
                                "too long.",
                                dialog_w);
                XtFree(id_str);
                XtFree(exe_path_str);
                return;
        }

        if (!xtee_is_file_msi(exe_path_xteestr)) {
                enum XteeExecutableType exe_type;

                if (xtee_get_executable_type(&exe_type, exe_path_xteestr)
                                != 0) {
                        gui_print_error_log_and_dialog("Cannot read "
                                        "executable file.",
                                        dialog_w);
                        XtFree(id_str);
                        XtFree(exe_path_str);
                        return;
                } else if (exe_type == XTEE_EXECUTABLE_UNKNOWN) {
                        gui_print_error_log_and_dialog("Incorrect executable "
                                        "format.",
                                        dialog_w);
                        XtFree(id_str);
                        XtFree(exe_path_str);
                        return;
                }
        }

        xtee_init_program(&program, id_str, exe_path_str);
        status = add_program_to_prefix(current_prefix, &program);
        xtee_free_program(&program);
        XtFree(id_str);
        XtFree(exe_path_str);
        switch (status) {
                case XTEE_ADD_PROGRAM_ID_EXIST:
                        gui_print_error_log_and_dialog("Program id is already "
                                        "in use.",
                                        dialog_w);
                        return;
                case XTEE_ADD_PROGRAM_WRITE_ERROR:
                        gui_print_error_log_and_dialog("Cannot write program "
                                        "configuration.",
                                        dialog_w);
                        return;
                default:
                        xtee_gui_set_message_area_string(
                                        "New program created.");
                        XtUnmanageChild(dialog_w);
        }

        if (xtee_prefix_reload_programs_list(current_prefix) != 0) {
                gui_print_error_log_and_dialog("Cannot get prefix programs "
                                "list.",
                                xtee_gui_get_main_win());
        }
        refresh_program_list();
}

static
XTEE_XT_CALLBACK_FUNC(new_program_cancel_cb)
{
        (void)w;
        (void)call_data;
        xtee_gui_reset_message_area_string_CALL;
        XtUnmanageChild((Widget)client_data);
}

void
xtee_gui_new_program_show_dialog(void)
{
        static bool is_dialog_ready = false;

        if (!is_dialog_ready) {
                Arg text_args[4];
                Arg filesel_args[4];
                XmString tmp_str;
                unsigned int text_args_n = 0;
                unsigned int filesel_args_n = 0;

                tmp_str = XmStringCreateLocalized("Program file selection");

                create_XteeGuiBasicFormDialog(&new_program_dialog,
                                xtee_gui_get_main_win(),
                                "new_program_dialog",
                                "New Program");
                XtSetArg(text_args[text_args_n], XmNmaxLength,
                                XTEE_PROGRAM_ID_MAX_LEN); text_args_n++;
                add_label_text_form_field_to_XteeGuiBasicFormDialog(
                                &new_program_dialog,
                                "program_id_form", "Id: ",
                                text_args, text_args_n);
                text_args_n = 0;
                XtSetArg(text_args[text_args_n], XmNmaxLength,
                                XTEE_MAX_FILE_PATH_LEN); text_args_n++;
                XtSetArg(filesel_args[filesel_args_n],
                                XmNdialogTitle, tmp_str); filesel_args_n++;
                add_ltpf_field_to_XteeGuiBasicFormDialog(
                                &new_program_dialog,
                                "program_exe_path", "Executable path: ",
                                text_args, text_args_n,
                                filesel_args, filesel_args_n, XmFILE_REGULAR);
                XmStringFree(tmp_str);

                XtAddCallback(new_program_dialog.ok_btn,
                                XmNactivateCallback,
                                new_program_ok_cb,
                                &new_program_dialog);
                XtAddCallback(new_program_dialog.cancel_btn,
                                XmNactivateCallback,
                                new_program_cancel_cb,
                                new_program_dialog.dialog_form);
                is_dialog_ready = true;
        } else {
                reset_XteeGuiBasicFormDialog_fields(&new_program_dialog);
        }
        XtManageChild(new_program_dialog.dialog_form);
}

void
xtee_gui_new_program_free(void)
{
        free_XteeGuiBasicFormDialog(&new_program_dialog);
}
