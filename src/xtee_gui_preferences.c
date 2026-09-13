#include "xtee_gui_preferences.h"
#include "xtee_gui.h"
#include "xtee_gui_form.h"

#include <Xm/ToggleBG.h>

static XteeGuiBasicFormDialog xtee_gui_preferences_dialog = { 0 };
static const size_t show_hidden_files_field_i = 0;

static
XTEE_XT_CALLBACK_FUNC(prefs_dialog_ok_cb)
{
        XteeGuiBasicFormDialog *prefs_dialog;
        XteeGuiFormField *show_hidden_files_field;
        Boolean show_hidden_files_bool;
        const char *show_hidden_files_str;

        (void)w;
        (void)call_data;
        prefs_dialog = (XteeGuiBasicFormDialog *)client_data;;
        show_hidden_files_field = prefs_dialog->fields
                + show_hidden_files_field_i;
        show_hidden_files_bool = !!(XmToggleButtonGadgetGetState(
                        show_hidden_files_field->data.toggle_g));
        show_hidden_files_str = xtee_bool_to_string(show_hidden_files_bool);
        xtee_config_set_entry_value_and_write("gui.show_hidden_files",
                        show_hidden_files_str);
        xtee_gui_set_message_area_string("Configuration modified and saved.");
        XtUnmanageChild(prefs_dialog->dialog_form);
}

static
XTEE_XT_CALLBACK_FUNC(prefs_dialog_cancel_cb)
{
        XteeGuiBasicFormDialog *dialog;

        (void)w;
        (void)call_data;
        dialog = (XteeGuiBasicFormDialog *)client_data;
        xtee_gui_reset_message_area_string_CALL;
        XtUnmanageChild(dialog->dialog_form);
}

void
xtee_gui_preferences_show_dialog(void)
{
        static bool is_dialog_ready = false;
        XteeGuiBasicFormDialog *prefs_dialog;
        XteeGuiFormField *show_hidden_files_field;
        const char *gui_show_hidden_files_val;

        prefs_dialog = &xtee_gui_preferences_dialog;
        if (!is_dialog_ready) {
                create_XteeGuiBasicFormDialog(prefs_dialog,
                                xtee_gui_get_main_win(),
                                "preferences_dialog",
                                "XTeetotaller preferences");
                add_toggle_form_field_to_XteeGuiBasicFormDialog(
                                prefs_dialog,
                                "show_hidden_files_toggle",
                                "Show hidden files on File Selection "
                                "dialogs.");
                XtAddCallback(prefs_dialog->ok_btn,
                                XmNactivateCallback,
                                prefs_dialog_ok_cb,
                                &xtee_gui_preferences_dialog);
                XtAddCallback(prefs_dialog->cancel_btn,
                                XmNactivateCallback,
                                prefs_dialog_cancel_cb,
                                &xtee_gui_preferences_dialog);
                is_dialog_ready = true;
        }
        show_hidden_files_field = prefs_dialog->fields
                + show_hidden_files_field_i;
        reset_XteeGuiBasicFormDialog_fields(prefs_dialog);
        gui_show_hidden_files_val = xtee_config_get_entry_value(
                        "gui.show_hidden_files");
        if (!gui_show_hidden_files_val
                        || !xtee_string_to_bool(gui_show_hidden_files_val)) {
                XmToggleButtonGadgetSetState(
                                show_hidden_files_field->data.toggle_g,
                                False, False);
        } else {
                XmToggleButtonGadgetSetState(
                                show_hidden_files_field->data.toggle_g,
                                True, False);
        }
        XtManageChild(xtee_gui_preferences_dialog.dialog_form);
}
