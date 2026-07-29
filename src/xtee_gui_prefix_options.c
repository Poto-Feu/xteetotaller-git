#include "xtee_gui_prefix_options.h"
#include "xtee_gui.h"
#include "xtee_gui_common.h"
#include "xtee_state.h"

#include <Xm/ToggleBG.h>

static XteeGuiPrefixOptionsData xtee_gui_prefix_options_data = { 0 };

static const int disable_winemenubuilder_i = 0;

static
XTEE_XT_CALLBACK_FUNC(prefix_options_dialog_ok_cb)
{
        XteePrefix *loaded_prefix;
        XteeGuiFormField *disable_winemenubuilder_field;
        XteeGuiBasicFormDialog *dialog;
        Widget main_win;
        bool disable_winemenubuilder;

        (void)w;
        (void)client_data;
        (void)call_data;
        dialog = &xtee_gui_prefix_options_data.dialog;
        loaded_prefix = xtee_state_get_loaded_prefix();
        disable_winemenubuilder_field = dialog->fields
                + disable_winemenubuilder_i;
        disable_winemenubuilder = (bool)XmToggleButtonGadgetGetState(
                        disable_winemenubuilder_field->data.toggle_g);
        loaded_prefix->disable_winemenubuilder = disable_winemenubuilder;

        main_win = xtee_gui_get_main_win();
        if (xtee_prefix_save_config(loaded_prefix) != 0) {
                XtUnmanageChild(dialog->dialog_form);
                gui_print_error_log_and_dialog("Cannot save prefix config.",
                                main_win);
                return;
        }

        XtUnmanageChild(dialog->dialog_form);
        if (xtee_gui_load_prefix(loaded_prefix->id.s, main_win) == 0) {
                gui_set_message_area_string("Prefix configuration updated and "
                                "reloaded.");
        }
}

static
XTEE_XT_CALLBACK_FUNC(prefix_options_dialog_cancel_cb)
{
        (void)w;
        (void)call_data;
        XtUnmanageChild((Widget)client_data);
}

void
xtee_gui_show_prefix_options_dialog(Widget main_win)
{
        static bool is_dialog_ready = false;
        XteeGuiBasicFormDialog *dialog;
        XteePrefix *loaded_prefix;
        XteeGuiFormField *disable_winemenubuilder_field;

        dialog = &xtee_gui_prefix_options_data.dialog;
        loaded_prefix = xtee_state_get_loaded_prefix();

        if (!is_dialog_ready) {
                dialog = &xtee_gui_prefix_options_data.dialog;
                create_XteeGuiBasicFormDialog(dialog,
                                main_win,
                                "prefix_options_dialog",
                                "Prefix options",
                                false);
                add_simple_form_field_to_XteeGuiBasicFormDialog(
                                dialog,
                                XTEE_GUI_FORM_FIELD_TOGGLE,
                                "disable_winemenubuilder_toggle",
                                "Disable winemenubuilder");
                XtAddCallback(dialog->ok_btn, XmNactivateCallback,
                                prefix_options_dialog_ok_cb,
                                NULL);
                XtAddCallback(dialog->cancel_btn, XmNactivateCallback,
                                prefix_options_dialog_cancel_cb,
                                (XtPointer)dialog->dialog_form);
                is_dialog_ready = true;
        }

        reset_XteeGuiBasicFormDialog_fields(dialog);
        disable_winemenubuilder_field = dialog->fields
                + disable_winemenubuilder_i;
        XmToggleButtonGadgetSetState(
                        disable_winemenubuilder_field->data.toggle_g,
                        loaded_prefix->disable_winemenubuilder,
                        False);
        XtManageChild(dialog->dialog_form);
}

void
xtee_gui_prefix_options_free(void)
{
        free_XteeGuiBasicFormDialog(&xtee_gui_prefix_options_data.dialog);
}
