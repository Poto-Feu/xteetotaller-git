#include "xtee_gui_new_prefix.h"
#include "xtee_error.h"
#include "xtee_file.h"
#include "xtee_gui.h"
#include "xtee_state.h"
#include "xtee_system.h"
#include "xtee_wine.h"

#include <Xm/MessageB.h>
#include <Xm/ToggleBG.h>

#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define XTEE_NEW_PREFIX_ID_FIELD_I 0
#define XTEE_NEW_PREFIX_DIR_FIELD_I 1
#define XTEE_NEW_PREFIX_ISOLATE_HOME_FIELD_I 2
#define XTEE_NEW_PREFIX_WINE_VERSION_FIELD_I 4

static const int prefix_dir_field_i = XTEE_NEW_PREFIX_DIR_FIELD_I;

static char *arch_choices_str[3] = { "win32", "win64", "wow64" };
static const unsigned int arch_choices_n = XTEE_ARRAY_LEN(arch_choices_str);

static unsigned xtee_new_prefix_arch_choice_i;
static XteeGuiBasicFormDialog xtee_gui_new_prefix_dialog = { 0 };

static Widget new_prefix_working_dialog = NULL;

void
xtee_gui_new_prefix_free(void)
{
        free_XteeGuiBasicFormDialog(&xtee_gui_new_prefix_dialog);
}

static
XTEE_XT_CALLBACK_FUNC(new_prefix_dialog_cancel_cb)
{
        (void)w;
        (void)call_data;
        xtee_gui_reset_message_area_string_CALL;
        XtUnmanageChild((Widget)client_data);
}

static
XTEE_XT_CALLBACK_FUNC(new_prefix_dialog_arch_radiobox_cb)
{
        XmToggleButtonCallbackStruct *cbs;
        unsigned int *arch_choice_i;

        (void)w;
        cbs = (XmToggleButtonCallbackStruct *)call_data;

        if (cbs->set != XmSET) {
                return;
        }
        arch_choice_i = &xtee_new_prefix_arch_choice_i;


#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        *arch_choice_i = (unsigned int)client_data;
#pragma GCC diagnostic pop
}

static void
print_dir_status_error(char *s, Widget parent)
{
        gui_print_error_log_and_dialog(s, parent);
}

static void
show_new_prefix_working_dialog(void)
{
        XteeGuiBasicFormDialog *parent;

        parent = &xtee_gui_new_prefix_dialog;

        if (!new_prefix_working_dialog) {
                Arg args[10];
                Cardinal n = 0;
                XmString dialog_title;
                XmString message_string;

                dialog_title = XmStringCreateLocalized(
                                "Prefix initialization");
                message_string = XmStringCreateLocalized(
                                "Wine prefix is being initialized.");

                XtSetArg(args[n], XmNdialogTitle, dialog_title); n++;
                XtSetArg(args[n], XmNmessageString, message_string); n++;
                XtSetArg(args[n], XmNdialogStyle,
                                XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
                new_prefix_working_dialog = XmCreateWorkingDialog(
                                parent->dialog_form,
                                "prefix_init_working_dialog",
                                args, n);
                XmStringFree(dialog_title);
                XmStringFree(message_string);
                XtUnmanageChild(XtNameToWidget(new_prefix_working_dialog,
                                        "OK"));
                xtee_gui_disable_dialog_help_button(new_prefix_working_dialog);
                xtee_gui_hide_dialog_cancel_button(new_prefix_working_dialog);
                xtee_gui_hide_dialog_help_button(new_prefix_working_dialog);
        }
        XtManageChild(new_prefix_working_dialog);
}

static void
hide_new_prefix_working_dialog(void)
{
        XtUnmanageChild(new_prefix_working_dialog);
}

static int
fork_exec_isolate_home(XteePrefix *prefix)
{
        XteeEnvData env_data[3];
        char *args[2] = { "-q", "isolate_home" };
        XteeWineVersionData *version_data;

        print_info("Running winetricks -q isolate_home");
        version_data = xtee_wine_get_version_data(prefix->wine_version.s);
        env_data[0].name = "WINE";
        env_data[0].value = version_data->wine_path.s;
        env_data[1].name = "WINESERVER";
        env_data[1].value = version_data->wineserver_path.s;
        env_data[2].name = "WINEPREFIX";
        env_data[2].value = prefix->wineprefix_dir.s;

        return xtee_gui_fork_exec((char *)xtee_system_winetricks_path.s,
                        args, XTEE_ARRAY_LEN(args), env_data,
                        XTEE_ARRAY_LEN(env_data));
}

static int
fork_exec_wineboot_init(XteePrefix *prefix)
{
        XteeEnvData env_data[2];
        char *args[1] = { "-i" };
        XteeWineVersionData *version_data;
        size_t args_n;

        print_info("Running wineboot -i");
        version_data = xtee_wine_get_version_data(prefix->wine_version.s);
        args_n = XTEE_ARRAY_LEN(args);
        env_data[0].name = "WINEARCH";
        switch(prefix->arch) {
                case XTEE_PREFIX_ARCH_WIN32:
                        env_data[0].value = "win32";
                        break;
                case XTEE_PREFIX_ARCH_WIN64:
                        env_data[0].value = "win64";
                        break;
                case XTEE_PREFIX_ARCH_WOW64:
                        env_data[0].value = "wow64";
                        break;
                default:
                        fatal_error("Incorrect Wine prefix arch.");
                        break;
        }
        env_data[1].name = "WINEPREFIX";
        env_data[1].value = prefix->wineprefix_dir.s;
        return xtee_gui_fork_exec(version_data->wineboot_path.s, args,
                        args_n, env_data, XTEE_ARRAY_LEN(env_data));
}

static
XTEE_XT_CALLBACK_FUNC(new_prefix_dialog_ok_cb)
{
        const int prefix_id_field_i = XTEE_NEW_PREFIX_ID_FIELD_I;
        const int isolate_home_field_i = XTEE_NEW_PREFIX_ISOLATE_HOME_FIELD_I;
        char *wine_version_charstr = NULL;
        XteeWineVersionData *wine_version_data;
        XteeGuiBasicFormDialog *dialog;
        char *prefix_arch_str;
        char *prefix_id_str, *prefix_dir_str;
        enum XteeNewPrefixCreateStatus status;
        enum XteeDirStatus dir_status;
        bool isolate_home;
        unsigned int arch_choice_i;
        enum XteePrefixArch arch_enum_val;
        XteeGuiFormField *wine_version_form;
        XteePrefix *loaded_prefix;
        int r;

        (void)w;
        (void)call_data;

        dialog = (XteeGuiBasicFormDialog *)client_data;
        wine_version_form =
                dialog->fields + XTEE_NEW_PREFIX_WINE_VERSION_FIELD_I;
        arch_choice_i = xtee_new_prefix_arch_choice_i;
        isolate_home = (bool)XmToggleButtonGadgetGetState(
                        dialog->fields[isolate_home_field_i].data.toggle_g);
        prefix_id_str = get_XteeGuiFormField_string_value(
                        &dialog->fields[prefix_id_field_i]);
        if (!prefix_id_str) {
                fatal_error("prefix_id_str is NULL (alloc error?)");
        } else if (prefix_id_str[0] == '\0') {
                gui_print_error_log_and_dialog("Prefix id is empty.",
                                dialog->dialog_form);
                free(prefix_id_str);
                return;
        } else if (arch_choice_i == 0) {
                gui_print_error_log_and_dialog("No prefix architecture "
                                "selected.",
                                dialog->dialog_form);
                free(prefix_id_str);
                return;
        } else if (isolate_home && !xtee_system_is_winetricks_installed) {
                gui_print_error_log_and_dialog("Isolate home option "
                                "only available with winetricks installed.",
                                dialog->dialog_form);
                free(prefix_id_str);
                return;
        }

        prefix_arch_str = arch_choices_str[arch_choice_i - 1];
        arch_enum_val = get_prefix_arch_from_str(prefix_arch_str);
        if (arch_enum_val == XTEE_PREFIX_ARCH_WRONG) {
                /* Should not happen. */
                fatal_error("Impossible new prefix architecture selected.");
        } else if (arch_enum_val != XTEE_PREFIX_ARCH_WIN32
                        && !xtee_system_is_amd64()) {
                gui_print_error_log_and_dialog("win64 and wow64 prefixes can "
                                "not be created on 32-bits systems.",
                                dialog->dialog_form);
                free(prefix_id_str);
                return;
        }

        wine_version_charstr = get_XteeGuiFormField_string_value(
                        wine_version_form);
        wine_version_data = xtee_wine_get_version_data(wine_version_charstr);
        if (!wine_version_data) {
                gui_print_error_log_and_dialog("Wine version data could not "
                                "be fetched",
                                dialog->dialog_form);
                free(wine_version_charstr);
                free(prefix_id_str);
                return;
        }

        if (arch_enum_val == XTEE_PREFIX_ARCH_WIN32) {
                if (wine_version_data->arch == XTEE_WINE_VERSION_ARCH_WOW64) {
                        gui_print_error_log_and_dialog("Selected Wine version "
                                        "is WoW64. "
                                        "You cannot create a win32 prefix "
                                        "with this version.\n"
                                        "Select wow64 instead.",
                                        dialog->dialog_form);
                        free(wine_version_charstr);
                        free(prefix_id_str);
                        return;
                }
        } else if (arch_enum_val == XTEE_PREFIX_ARCH_WIN64) {
                if (wine_version_data->arch == XTEE_WINE_VERSION_ARCH_X86) {
                        gui_print_error_log_and_dialog("Selected Wine version "
                                        "is x86. "
                                        "You can only use 32-bits programs "
                                        "and prefixes with this version.",
                                        dialog->dialog_form);
                        free(wine_version_charstr);
                        free(prefix_id_str);
                        return;
                } else if (wine_version_data->arch
                                == XTEE_WINE_VERSION_ARCH_WOW64) {
                        gui_print_error_log_and_dialog("Selected Wine version "
                                        "is WoW64.\n"
                                        "Select wow64 instead.",
                                        dialog->dialog_form);
                        free(wine_version_charstr);
                        free(prefix_id_str);
                        return;
                }
        } else if (arch_enum_val == XTEE_PREFIX_ARCH_WOW64) {
                if (wine_version_data->arch
                                != XTEE_WINE_VERSION_ARCH_WOW64) {
                        gui_print_error_log_and_dialog("Selected Wine "
                                        "architecture requires a WoW64 "
                                        "Wine version.",
                                        dialog->dialog_form);
                        free(wine_version_charstr);
                        free(prefix_id_str);
                        return;
                }
        }

        /* prefix_dir_str is the directory where Wine initialize the prefix,
         * not where xteetotaller store the prefix data. */
        prefix_dir_str = get_XteeGuiFormField_string_value(
                        &dialog->fields[prefix_dir_field_i]);
        if (!prefix_dir_str) {
                fatal_error("prefix_dir_str is NULL (alloc error?)");
        } else if (prefix_dir_str[0] == '\0') {
                gui_print_error_log_and_dialog("Prefix directory path is "
                                "empty.",
                                dialog->dialog_form);
                free(wine_version_charstr);
                free(prefix_id_str);
                free(prefix_dir_str);
                return;
        } else if(strlen(prefix_dir_str) > XTEE_PREFIX_DIR_PATH_MAX_LEN) {
                gui_print_error_log_and_dialog("Prefix directory path is "
                                "too long.",
                                dialog->dialog_form);
                free(wine_version_charstr);
                free(prefix_id_str);
                free(prefix_dir_str);
                return;
        }

        dir_status = xtee_get_dir_status(prefix_dir_str);
        switch (dir_status) {
                case XTEE_DIR_STATUS_EMPTY:
                        /* This is the "no error" value so just break */
                        break;
                case XTEE_DIR_STATUS_PATH_NOT_VALID:
                        print_dir_status_error("Prefix directory path is not "
                                        "a valid UNIX path.",
                                        dialog->dialog_form);
                        goto dir_status_error_rtrn;
                case XTEE_DIR_STATUS_NOT_EXIST:
                        print_dir_status_error("Prefix directory does not "
                                        "exist.",
                                        dialog->dialog_form);
                        goto dir_status_error_rtrn;
                case XTEE_DIR_STATUS_NOT_EMPTY:
                        print_dir_status_error("Prefix directory is not "
                                        "empty.",
                                        dialog->dialog_form);
                        goto dir_status_error_rtrn;
                default:
                        gui_print_error_log_and_dialog("Prefix directory "
                                        "error.",
                                        dialog->dialog_form);
                        goto dir_status_error_rtrn;
        }

        show_new_prefix_working_dialog();
        status = create_new_prefix(prefix_id_str, prefix_dir_str,
                        wine_version_charstr, arch_enum_val, isolate_home);
        free(wine_version_charstr);
        wine_version_charstr = NULL;
        free(prefix_dir_str);
        if (status != XTEE_NEW_PREFIX_CREATE_OK) {
                if (status == XTEE_NEW_PREFIX_CREATE_ID_EXIST) {
                        gui_print_error_log_and_dialog("Prefix id already in "
                                        "use.",
                                        dialog->dialog_form);
                        goto create_new_prefix_error;
                } else if (status == XTEE_NEW_PREFIX_CREATE_DIR_ERROR) {
                        gui_print_error_log_and_dialog(
                                        "Cannot create prefix data "
                                        "directory.",
                                        dialog->dialog_form);
                        goto create_new_prefix_error;
                } else if (status == XTEE_NEW_PREFIX_CREATE_WRITE_ERROR) {
                        gui_print_error_log_and_dialog(
                                        "Cannot write to new prefix "
                                        "directory.",
                                        dialog->dialog_form);
                        goto create_new_prefix_error;
                } else {
                        gui_print_error_dialog("Cannot create new prefix.",
                                        dialog->dialog_form);
                        goto create_new_prefix_error;
                }
        }

        r = xtee_gui_load_prefix(prefix_id_str, xtee_gui_get_main_win());
        free(prefix_id_str);
        if (r != 0) {
                gui_print_error_log_and_dialog("Cannot load new prefix.",
                                xtee_gui_get_main_win());
                goto load_prefix_error;
        }
        xtee_gui_set_message_area_string("Prefix created and loaded.");
        loaded_prefix = xtee_state_get_loaded_prefix();
        xtee_gui_update_display();
        print_info("Running wineboot -i");
        if (fork_exec_wineboot_init(loaded_prefix) != 0) {
                XtUnmanageChild(dialog->dialog_form);
                gui_print_error_log_and_dialog("Cannot initialize Wine prefix "
                                "with wineboot.",
                                xtee_gui_get_main_win());
                xtee_gui_set_message_area_string(
                                "Wine prefix creation failed.");
                goto load_prefix_error;
        }
        if (loaded_prefix->isolate_home) {
                if (fork_exec_isolate_home(loaded_prefix) != 0) {
                        XtUnmanageChild(dialog->dialog_form);
                        gui_print_error_log_and_dialog("Cannot run winetricks "
                                        "isolate_home.",
                                        xtee_gui_get_main_win());
                        xtee_gui_set_message_area_string(
                                        "Wine prefix creation failed.");
                        goto load_prefix_error;
                }
        }
        hide_new_prefix_working_dialog();
        XtUnmanageChild(dialog->dialog_form);
        return;

dir_status_error_rtrn:
        free(wine_version_charstr);
        free(prefix_id_str);
        free(prefix_dir_str);
        return;
create_new_prefix_error:
        hide_new_prefix_working_dialog();
        free(prefix_id_str);
        return;
load_prefix_error:
        hide_new_prefix_working_dialog();
        XtUnmanageChild(dialog->dialog_form);
        return;
}

void
create_new_prefix_arch_radiobox(XteeGuiBasicFormDialog *dialog)
{
        XteeGuiFormField *last_field;

        add_radiobox_field_to_XteeGuiBasicFormDialog(dialog,
                        "arch_radiobox", "Prefix architecture: ",
                        arch_choices_str, arch_choices_n);
        last_field = get_last_XteeGuiFormField_data(dialog);
        XteeGuiRadioBoxForm_add_callback(&last_field->data.radiobox_form,
                        new_prefix_dialog_arch_radiobox_cb);
}

void
create_new_prefix_wine_version_dropdownlist(XteeGuiBasicFormDialog *dialog)
{
        XteeStringList items_list;

        items_list = xtee_wine_get_versions_ids();
        add_dropdownlist_field_to_XteeGuiBasicFormDialog(dialog,
                        "wine_version_form", "Wine version: ",
                        items_list);
        XteeStringList_free(items_list);
}

void
show_new_prefix_dialog(void)
{
        XteeGuiBasicFormDialog *dialog;
        static bool is_dialog_ready = false;

        dialog = &xtee_gui_new_prefix_dialog;

        if (xtee_wine_versions_data.n == 0
                        && !xtee_system_is_wine_installed) {
                gui_print_error_dialog("No wine installation present in "
                                "components directory and no system-wide "
                                "wine is installed.\n"
                                "A prefix cannot be created.",
                                xtee_gui_get_main_win());
                return;
        }

        if (!is_dialog_ready) {
                Arg text_args[4];
                Arg filesel_args[4];
                XmString tmp_str;
                XteeGuiLabelTextForm *prefix_id_form;
                XteeGuiFormField *prefix_id_form_field;
                unsigned int text_args_n = 0;
                unsigned int filesel_args_n = 0;

                tmp_str = XmStringCreateLocalized("Prefix directory "
                                "selection");
                XtSetArg(text_args[text_args_n], XmNmaxLength,
                                XTEE_MAX_FILE_PATH_LEN); text_args_n++;
                XtSetArg(filesel_args[filesel_args_n], XmNfileTypeMask,
                                XmFILE_DIRECTORY); filesel_args_n++;
                XtSetArg(filesel_args[filesel_args_n], XmNdialogTitle,
                                tmp_str); filesel_args_n++;
                create_XteeGuiBasicFormDialog(dialog,
                                xtee_gui_get_main_win(),
                                "new_prefix_dialog",
                                "New Prefix");
                add_simple_form_field_to_XteeGuiBasicFormDialog(
                                dialog,
                                XTEE_GUI_FORM_FIELD_LABELTEXT,
                                "new_prefix_id_form", "Id: ");
                add_ltpf_field_to_XteeGuiBasicFormDialog(dialog,
                                "prefix_dir_form", "Prefix directory: ",
                                text_args, text_args_n,
                                filesel_args, filesel_args_n);
                XmStringFree(tmp_str);
                add_simple_form_field_to_XteeGuiBasicFormDialog(
                                dialog,
                                XTEE_GUI_FORM_FIELD_TOGGLE,
                                "isolate_home_toggle", "Isolate home");
                /* Set maximum length for prefix id */
                prefix_id_form_field = XteeGuiBasicFormDialog_get_field(dialog,
                                XTEE_NEW_PREFIX_ID_FIELD_I);
                prefix_id_form = &prefix_id_form_field->data.ltf;
                XtVaSetValues(prefix_id_form->text,
                                XmNmaxLength, XTEE_PREFIX_ID_MAX_LEN,
                                NULL);

                XtAddCallback(dialog->ok_btn, XmNactivateCallback,
                                new_prefix_dialog_ok_cb,
                                (XtPointer)dialog);
                XtAddCallback(dialog->cancel_btn, XmNactivateCallback,
                                new_prefix_dialog_cancel_cb,
                                (XtPointer)dialog->dialog_form);
                create_new_prefix_arch_radiobox(dialog);
                create_new_prefix_wine_version_dropdownlist(dialog);
                is_dialog_ready = true;
        } else {
                /* Reset forms only if dialog was already existing */
                reset_XteeGuiBasicFormDialog_fields(dialog);
        }
        XtManageChild(dialog->dialog_form);
}
