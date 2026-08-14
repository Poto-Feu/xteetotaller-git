#include "xtee_gui_gamescope.h"
#include "xtee_gui.h"
#include "xtee_gui_form.h"
#include "xtee_state.h"
#include "xtee_system.h"
#include "xtee_wine.h"

#include <Xm/MessageB.h>
#include <unistd.h>

static const size_t internal_width_field_i = 0;
static const size_t internal_height_field_i = 1;
static const size_t window_width_field_i = 2;
static const size_t window_height_field_i = 3;

static XteeGuiBasicFormDialog start_gamescope_form_dialog = { 0 };
static Widget start_gamescope_working_dialog = NULL;

static void
set_and_show_start_gamescope_working_dialog(char *title, char *msg)
{
        XmString title_str, msg_str;
        if (!start_gamescope_working_dialog) {
                Arg args[5];
                Cardinal n = 0;

                XtSetArg(args[n], XmNdialogStyle,
                                XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
                start_gamescope_working_dialog = XmCreateWorkingDialog(
                                start_gamescope_form_dialog.dialog_form,
                                "start_gamescope_working_dialog",
                                args, n);
                XtUnmanageChild(XtNameToWidget(start_gamescope_working_dialog,
                                        "OK"));
                xtee_gui_hide_dialog_cancel_button(
                                start_gamescope_working_dialog);
                xtee_gui_hide_dialog_help_button(
                                start_gamescope_working_dialog);
        }
        title_str = XmStringCreateLocalized(title);
        msg_str = XmStringCreateLocalized(msg);
        XtVaSetValues(start_gamescope_working_dialog,
                        XmNdialogTitle, title_str,
                        XmNmessageString, msg_str,
                        NULL);
        XmStringFree(title_str);
        XmStringFree(msg_str);
        XtManageChild(start_gamescope_working_dialog);
}

static void
hide_start_gamescope_working_dialog(void)
{
        XtUnmanageChild(start_gamescope_working_dialog);
}

static void
xtee_gui_gamescope_set_working_dir(XteeString program_path)
{
        const char *info_strs[3];
        char new_wd[XTEE_MAX_FILE_PATH_LEN] = { 0 };
        char *slash_ch;

        strcpy(new_wd, program_path.s);
        if (!(slash_ch = strrchr(new_wd, '/'))) {
                fatal_error("No slash in program file path.");
        }
        *slash_ch = '\0';
        info_strs[0] = "Setting working directory to ";
        info_strs[1] = new_wd;
        info_strs[2] = NULL;
        print_info_strs(info_strs);
        chdir(new_wd);
}

static void
run_program_with_gamescope(char *restrict internal_width_str,
                char *restrict internal_height_str,
                char *restrict window_width_str,
                char *restrict window_height_str)
{
        const char *current_wd_info_strs[3];
        Widget main_win_w;
        const size_t gamescope_args_n = 9;
        XteeEnvData env_data[1];
        XteeString program_str;
        XteeProgram *program_data;
        XteePrefix *loaded_prefix;
        XteeWineVersionData *wine_version;
        char **args_block = NULL;
        char **args_ptr = NULL;
        char *gamescope_path;
        char previous_wd[XTEE_MAX_FILE_PATH_LEN] = { 0 };
        size_t program_args_n;
        size_t total_args_n;
        int r;
        enum XteeGetProgramDataStatus get_program_data_status;

        loaded_prefix = xtee_state_get_loaded_prefix();
        wine_version = xtee_wine_get_version_data(
                        loaded_prefix->wine_version.s);
        if (!wine_version) {
                fatal_error("Prefix wine version does not match an "
                                "existing wine version.");
        }
        env_data[0].name = "WINEPREFIX";
        env_data[0].value = loaded_prefix->wineprefix_dir.s;
        xtee_prefix_set_WINEDLLOVERRIDES_env(loaded_prefix);
        gamescope_path = strdup(xtee_system_gamescope_path.s);
        main_win_w = xtee_gui_get_main_win();
        if (xtee_gui_get_selected_program_string(&program_str) != 0) {
                fatal_error("Cannot get selected program string.");
        }
        program_data = xtee_prefix_get_program_data(loaded_prefix,
                        program_str.s,
                        &get_program_data_status);
        if (!program_data) {
                fatal_error("Cannot get program data from string");
        }
        set_and_show_start_gamescope_working_dialog("Running program",
                        "Running program with gamescope");
        /* We cannot use "wine start" with gamescope as it creates a detached
         * process.
         * As a workaround, we set the working directory before forking
         * and unset it after the end of the wine process. */
        if (!(getcwd(previous_wd, XTEE_MAX_FILE_PATH_LEN))) {
                fatal_error("Working directory path is too long.");
        }
        current_wd_info_strs[0] = "Current working directory is ";
        current_wd_info_strs[1] = previous_wd;
        current_wd_info_strs[2] = NULL;
        print_info_strs(current_wd_info_strs);
        xtee_gui_gamescope_set_working_dir(program_data->exe_path);
        program_args_n = program_data->args_list.n;
        /* The 2 additional argument represent the "wine" and program
         * arguments. */
        total_args_n = gamescope_args_n + 2 + program_args_n;
        args_block = malloc_or_exit(total_args_n * sizeof(char *));
        args_ptr = args_block;
        *args_ptr = "-w";
        ++args_ptr;
        *args_ptr = internal_width_str;
        ++args_ptr;
        *args_ptr = "-h";
        ++args_ptr;
        *args_ptr = internal_height_str;
        ++args_ptr;
        *args_ptr = "-W";
        ++args_ptr;
        *args_ptr = window_width_str;
        ++args_ptr;
        *args_ptr = "-H";
        ++args_ptr;
        *args_ptr = window_height_str;
        ++args_ptr;
        *args_ptr = "--";
        ++args_ptr;
        *args_ptr = wine_version->wine_path.s;
        ++args_ptr;
        *args_ptr = program_data->exe_path.s;

        for (size_t i = 0; i < program_args_n; ++i) {
                ++args_ptr;
                *args_ptr = program_data->args_list.data[i].s;
        }
        r = xtee_gui_fork_exec(gamescope_path, args_block, total_args_n,
                        env_data, XTEE_ARRAY_LEN(env_data));
        free(args_block);
        free(gamescope_path);
        xtee_free_program(program_data);
        free(program_data);
        current_wd_info_strs[0] = "Setting working directory to ";
        current_wd_info_strs[1] = previous_wd;
        current_wd_info_strs[2] = NULL;
        chdir(previous_wd);
        hide_start_gamescope_working_dialog();
        if (r != 0) {
                gui_print_warning_dialog("Error while running program through "
                                "gamescope", main_win_w);
        }
}

static
XTEE_XT_CALLBACK_FUNC(gamescope_dialog_ok_cb)
{
        XteeGuiBasicFormDialog *dialog;
        char *internal_width_str = NULL;
        char *internal_height_str = NULL;
        char *window_width_str = NULL;
        char *window_height_str = NULL;

        dialog = &start_gamescope_form_dialog;
        (void)w;
        (void)client_data;
        (void)call_data;
        internal_width_str = get_XteeGuiFormField_string_value(
                        dialog->fields + internal_width_field_i);
        internal_height_str = get_XteeGuiFormField_string_value(
                        dialog->fields + internal_height_field_i);
        window_width_str = get_XteeGuiFormField_string_value(
                        dialog->fields + window_width_field_i);
        window_height_str = get_XteeGuiFormField_string_value(
                        dialog->fields + window_height_field_i);

        if (*internal_width_str == '\0'
                        || *internal_height_str == '\0'
                        || *window_width_str == '\0'
                        || *window_height_str == '\0') {
                gui_print_error_dialog("Some fields are empty",
                                dialog->dialog_form);
                free(internal_width_str);
                free(internal_height_str);
                free(window_width_str);
                free(window_height_str);
                return;
        } else if (!xtee_is_charstring_number(internal_width_str)
                        || !xtee_is_charstring_number(internal_height_str)
                        || !xtee_is_charstring_number(window_width_str)
                        || !xtee_is_charstring_number(window_height_str)) {
                gui_print_error_dialog("Text fields must have number values.",
                                dialog->dialog_form);
                free(internal_width_str);
                free(internal_height_str);
                free(window_width_str);
                free(window_height_str);
                return;
        }
        run_program_with_gamescope(internal_width_str, internal_height_str,
                        window_width_str, window_height_str);
        free(internal_width_str);
        free(internal_height_str);
        free(window_width_str);
        free(window_height_str);
        XtUnmanageChild(dialog->dialog_form);
}

static
XTEE_XT_CALLBACK_FUNC(gamescope_dialog_cancel_cb)
{
        XteeGuiBasicFormDialog *dialog;

        dialog = &start_gamescope_form_dialog;
        (void)w;
        (void)client_data;
        (void)call_data;
        XtUnmanageChild(dialog->dialog_form);
}

void
xtee_gui_gamescope_show_dialog(void)
{
        static bool is_dialog_ready = false;
        XteeGuiBasicFormDialog *dialog;

        dialog = &start_gamescope_form_dialog;

        if (!is_dialog_ready) {
                create_XteeGuiBasicFormDialog(dialog,
                                xtee_gui_get_main_win(),
                                "gamescope_form_dialog", "Gamescope settings");
                add_simple_form_field_to_XteeGuiBasicFormDialog(
                                dialog,
                                XTEE_GUI_FORM_FIELD_LABELTEXT,
                                "internal_width_form", "Internal width: ");
                add_simple_form_field_to_XteeGuiBasicFormDialog(
                                dialog,
                                XTEE_GUI_FORM_FIELD_LABELTEXT,
                                "internal_height_form", "Internal height: ");
                add_simple_form_field_to_XteeGuiBasicFormDialog(
                                dialog,
                                XTEE_GUI_FORM_FIELD_LABELTEXT,
                                "window_width_form", "Window width: ");
                add_simple_form_field_to_XteeGuiBasicFormDialog(
                                dialog,
                                XTEE_GUI_FORM_FIELD_LABELTEXT,
                                "window_height_form", "Window height: ");
                XtAddCallback(dialog->ok_btn, XmNactivateCallback,
                                gamescope_dialog_ok_cb,
                                NULL);
                XtAddCallback(dialog->cancel_btn, XmNactivateCallback,
                                gamescope_dialog_cancel_cb,
                                NULL);
                manage_XteeGuiBasicFormDialog_fields(dialog);
                is_dialog_ready = true;
        }
        /* Do not reset fields as it can be useful for users to re-use the same
         * values. */
        XtManageChild(dialog->dialog_form);
}
