#include "xtee_gui_prefix_options.h"
#include "xtee_gui.h"
#include "xtee_gui_common.h"
#include "xtee_state.h"
#include "xtee_wine.h"

#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/ToggleBG.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static bool xtee_gui_is_dxvk_installed = false;

static Widget wine_version_selection_box = NULL;
static Widget dxvk_version_selection_box = NULL;
static Widget install_dxvk_working_dialog = NULL;

static const int disable_winemenubuilder_i = 0;
static const int change_wine_version_i = 1;
static const int install_dxvk_i = 2;

static
XTEE_XT_CALLBACK_FUNC(prefix_options_dialog_ok_cb)
{
        XteePrefix *loaded_prefix;
        XteeGuiFormField *disable_winemenubuilder_field;
        XteeGuiFormField *change_wine_field;
        XteeGuiStringAndButtonForm *change_wine_form_data;
        XteeGuiBasicFormDialog *dialog;
        XmString new_wine_version_xmstr;
        char *new_wine_version_str;
        Widget main_win;
        bool disable_winemenubuilder;

        (void)w;
        (void)call_data;
        dialog = (XteeGuiBasicFormDialog *)client_data;
        loaded_prefix = xtee_state_get_loaded_prefix();
        disable_winemenubuilder_field = dialog->fields
                + disable_winemenubuilder_i;
        disable_winemenubuilder = (bool)XmToggleButtonGadgetGetState(
                        disable_winemenubuilder_field->data.toggle_g);
        loaded_prefix->disable_winemenubuilder = disable_winemenubuilder;

        main_win = xtee_gui_get_main_win();
        change_wine_field = dialog->fields + change_wine_version_i;
        change_wine_form_data = &change_wine_field->data.stringandbutton_form;
        XtVaGetValues(change_wine_form_data->string_label,
                        XmNlabelString, &new_wine_version_xmstr,
                        NULL);
        new_wine_version_str = XmString_to_char_str(&new_wine_version_xmstr);
        XmStringFree(new_wine_version_xmstr);
        XteeString_free(loaded_prefix->wine_version);
        loaded_prefix->wine_version.s = new_wine_version_str;
        loaded_prefix->wine_version.n = strlen(new_wine_version_str);
        if (xtee_prefix_save_config(loaded_prefix) != 0) {
                XtUnmanageChild(dialog->dialog_form);
                gui_print_error_log_and_dialog("Cannot save prefix config.",
                                main_win);
                return;
        }

        XtUnmanageChild(dialog->dialog_form);
        if (xtee_gui_load_prefix(loaded_prefix->id.s, main_win) == 0) {
                xtee_gui_set_message_area_string(
                                "Prefix configuration updated and reloaded.");
        }
}

static
XTEE_XT_CALLBACK_FUNC(prefix_options_dialog_cancel_cb)
{
        (void)w;
        (void)call_data;
        xtee_gui_reset_message_area_string_CALL;
        XtUnmanageChild((Widget)client_data);
}

static void
set_and_show_install_dxvk_working_dialog(Widget parent, char *title, char *msg)
{
        if (!install_dxvk_working_dialog) {
                Arg args[5];
                Cardinal n = 0;

                XtSetArg(args[n], XmNdialogStyle,
                                XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
                install_dxvk_working_dialog = XmCreateWorkingDialog(
                                parent,
                                "install_dxvk_working_dialog",
                                args, n);
                XtUnmanageChild(XtNameToWidget(install_dxvk_working_dialog,
                                        "OK"));
                xtee_gui_hide_dialog_cancel_button(
                                install_dxvk_working_dialog);
                xtee_gui_hide_dialog_help_button(install_dxvk_working_dialog);
        }
        xtee_gui_set_dialog_title_message(install_dxvk_working_dialog, title,
                        msg);
        XtManageChild(install_dxvk_working_dialog);
}

static void
hide_install_dxvk_working_dialog(void)
{
        XtUnmanageChild(install_dxvk_working_dialog);
}

static int
xtee_gui_install_dxvk(XteeGuiBasicFormDialog *dialog,
                XteePrefix *restrict prefix, const char *restrict dxvk_str)
{
        const XteeDxvkVersionData *dxvk_data;

        if(!(dxvk_data = xtee_dxvk_get_data(dxvk_str))) {
                return -1;
        }
        set_and_show_install_dxvk_working_dialog(dialog->dialog_form,
                        "Installing DXVK",
                        "DXVK is being installed into the prefix.");
        XmUpdateDisplay(install_dxvk_working_dialog);
        if (xtee_prefix_install_dxvk(prefix, dxvk_data) != 0) {
                hide_install_dxvk_working_dialog();
                gui_print_error_log_and_dialog("Cannot install DXVK into "
                                "prefix.", dxvk_version_selection_box);
                return -1;
        }
        hide_install_dxvk_working_dialog();
        return 0;
}

static void
set_installed_dxvk_field(const XteeGuiBasicFormDialog *dialog)
{
        XteePrefix *loaded_prefix;
        XteeGuiFormField *install_dxvk_field;
        XteeGuiStringAndButtonForm *form_data;
        XmString label_str;
        XmString button_label;

        install_dxvk_field = dialog->fields + install_dxvk_i;
        form_data = &install_dxvk_field->data.stringandbutton_form;
        loaded_prefix = xtee_state_get_loaded_prefix();
        if (loaded_prefix->is_dxvk_installed) {
                label_str = XmStringCreateLocalized(
                                loaded_prefix->dxvk_version.s);
                button_label = XmStringCreateLocalized(
                                "Remove DXVK");
                xtee_gui_is_dxvk_installed = true;
        } else {
                label_str = XmStringCreateLocalized("(not installed)");
                button_label = XmStringCreateLocalized(
                                "Install DXVK");
                xtee_gui_is_dxvk_installed = false;
        }
        XtVaSetValues(form_data->string_label,
                        XmNlabelString, label_str,
                        NULL);
        XtVaSetValues(form_data->button,
                        XmNlabelString, button_label,
                        NULL);
        XmStringFree(label_str);
        XmStringFree(button_label);
}

static
XTEE_XT_CALLBACK_FUNC(dxvk_version_selection_box_cb)
{
        XteeGuiBasicFormDialog *dialog;
        XteePrefix *loaded_prefix;
        XmSelectionBoxCallbackStruct *cbs;
        char *dxvk_str;

        cbs = (XmSelectionBoxCallbackStruct *)call_data;
        switch (cbs->reason) {
                case XmCR_OK:
                        /* See rest of function */
                        break;
                case XmCR_CANCEL:
                        XtUnmanageChild(w);
                        return;
                case XmCR_NO_MATCH:
                        gui_print_error_log_and_dialog("DXVK version name is "
                                        "not valid.",
                                        w);
                        return;
                default:
                        return;
        }
        dialog = (XteeGuiBasicFormDialog *)client_data;
        loaded_prefix = xtee_state_get_loaded_prefix();
        dxvk_str = XmString_to_char_str(&cbs->value);
        if (xtee_gui_install_dxvk(dialog, loaded_prefix, dxvk_str) != 0) {
                gui_print_error_log_and_dialog("Cannot install DXVK in "
                                "prefix.", w);
                free(dxvk_str);
                return;
        }
        set_installed_dxvk_field(dialog);
        free(dxvk_str);
        XtUnmanageChild(w);
}

static int
xtee_gui_remove_dxvk(const XteeGuiBasicFormDialog *dialog, XteePrefix *prefix)
{
        if (xtee_prefix_remove_dxvk(prefix) != 0) {
                return -1;
        }
        set_installed_dxvk_field(dialog);
        return 0;
}

static
XTEE_XT_CALLBACK_FUNC(prefix_options_install_dxvk_cb)
{
        XteeGuiBasicFormDialog *dialog;

        (void)w;
        (void)call_data;
        dialog = (XteeGuiBasicFormDialog *)client_data;

        if (xtee_gui_is_dxvk_installed) {
                set_and_show_install_dxvk_working_dialog(dialog->dialog_form,
                                "Removing DXVK",
                                "DXVK is being removed from prefix.");
                XmUpdateDisplay(install_dxvk_working_dialog);
                if (xtee_gui_remove_dxvk(dialog,
                                        xtee_state_get_loaded_prefix()) != 0) {
                        hide_install_dxvk_working_dialog();
                        gui_print_error_log_and_dialog("An error occured "
                                        "while removing DXVK",
                                        dialog->dialog_form);
                }
                XmUpdateDisplay(install_dxvk_working_dialog);
                hide_install_dxvk_working_dialog();
                set_installed_dxvk_field(dialog);
                return;
        }

        if (!dxvk_version_selection_box) {
                XteeStringList dxvk_versions_list;
                Arg args[5];
                Cardinal n = 0;
                Widget items_list;

                XtSetArg(args[n], XmNdialogStyle,
                                XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
                XtSetArg(args[n], XmNmustMatch, True); n++;
                XtSetArg(args[n], XmNautoUnmanage, False); n++;
                dxvk_version_selection_box = XmCreateSelectionDialog(
                                dialog->dialog_form,
                                "dxvk_version_selection_box",
                                args, n);
                xtee_gui_disable_dialog_help_button(
                                dxvk_version_selection_box);
                XtUnmanageChild(XtNameToWidget(dxvk_version_selection_box,
                                        "Apply"));
                dxvk_versions_list = xtee_dxvk_get_ids();
                items_list = XtNameToWidget(dxvk_version_selection_box,
                                "*ItemsList");
                if (!items_list) {
                        fatal_error("No ItemsList child in XmSelectionBox.");
                }
                for (size_t i = 0; i < dxvk_versions_list.n; ++i) {
                        XmString item_str;

                        item_str = XmStringCreateLocalized(
                                        dxvk_versions_list.data[i].s);
                        XmListAddItem(items_list, item_str, 0);
                }
                XteeStringList_free(dxvk_versions_list);
                XtAddCallback(dxvk_version_selection_box, XmNokCallback,
                                dxvk_version_selection_box_cb,
                                dialog);
                XtAddCallback(dxvk_version_selection_box, XmNcancelCallback,
                                dxvk_version_selection_box_cb,
                                dialog);
                XtAddCallback(dxvk_version_selection_box, XmNnoMatchCallback,
                                dxvk_version_selection_box_cb,
                                dialog);
        }
        XtManageChild(dxvk_version_selection_box);
}

static
XTEE_XT_CALLBACK_FUNC(wine_version_selection_box_cb)
{
        const XteeGuiBasicFormDialog *dialog;
        XteeGuiFormField *change_wine_field;
        XteeGuiStringAndButtonForm *form_data;
        const XteePrefix *loaded_prefix;
        const XteeWineVersionData *current_wine;
        const XteeWineVersionData *new_wine;
        char *new_wine_str;
        XmSelectionBoxCallbackStruct *cbs;

        cbs = (XmSelectionBoxCallbackStruct *)call_data;
        switch (cbs->reason) {
                case XmCR_OK:
                        /* See rest of function */
                        break;
                case XmCR_CANCEL:
                        XtUnmanageChild(w);
                        return;
                case XmCR_NO_MATCH:
                        gui_print_error_log_and_dialog("Wine version name "
                                        "not valid.",
                                        w);
                        return;
                default:
                        return;
        }
        dialog = (XteeGuiBasicFormDialog *)client_data;
        change_wine_field = dialog->fields + change_wine_version_i;
        form_data = &change_wine_field->data.stringandbutton_form;
        loaded_prefix = xtee_state_get_loaded_prefix();
        current_wine = xtee_wine_get_version_data(
                        loaded_prefix->wine_version.s);
        new_wine_str = XmString_to_char_str(&cbs->value);
        new_wine = xtee_wine_get_version_data(new_wine_str);
        if (current_wine->arch != new_wine->arch) {
                gui_print_error_log_and_dialog("Selected wine version has a "
                                "different arch than prefix wine version.",
                                w);
                free(new_wine_str);
                return;
        }
        xtee_gui_set_motif_label_string(form_data->string_label,
                        new_wine_str);
        free(new_wine_str);
        XtUnmanageChild(w);
}

static
XTEE_XT_CALLBACK_FUNC(prefix_options_change_wine_cb)
{
        XteeGuiBasicFormDialog *dialog;

        (void)w;
        (void)call_data;
        dialog = (XteeGuiBasicFormDialog *)client_data;
        if (!wine_version_selection_box) {
                XteeStringList wine_versions_list;
                Arg args[5];
                Cardinal n = 0;
                Widget items_list;

                XtSetArg(args[n], XmNdialogStyle,
                                XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
                XtSetArg(args[n], XmNmustMatch, True); n++;
                XtSetArg(args[n], XmNautoUnmanage, False); n++;
                wine_version_selection_box = XmCreateSelectionDialog(
                                dialog->dialog_form,
                                "wine_version_selection_box",
                                args, n);
                xtee_gui_disable_dialog_help_button(
                                wine_version_selection_box);
                XtUnmanageChild(XtNameToWidget(wine_version_selection_box,
                                        "Apply"));
                wine_versions_list = xtee_wine_get_versions_ids();
                items_list = XtNameToWidget(wine_version_selection_box,
                                "*ItemsList");
                if (!items_list) {
                        fatal_error("No ItemsList child in XmSelectionBox.");
                }
                for (size_t i = 0; i < wine_versions_list.n; ++i) {
                        XmString item_str;

                        item_str = XmStringCreateLocalized(
                                        wine_versions_list.data[i].s);
                        XmListAddItem(items_list, item_str, 0);
                }
                XteeStringList_free(wine_versions_list);
                XtAddCallback(wine_version_selection_box, XmNokCallback,
                                wine_version_selection_box_cb, dialog);
                XtAddCallback(wine_version_selection_box, XmNcancelCallback,
                                wine_version_selection_box_cb, dialog);
                XtAddCallback(wine_version_selection_box, XmNnoMatchCallback,
                                wine_version_selection_box_cb, dialog);
        }
        XtManageChild(wine_version_selection_box);
}

static void
set_change_wine_string(const XteeGuiBasicFormDialog *dialog)
{
        XteeGuiFormField *change_wine_field;
        XteeGuiStringAndButtonForm *form_data;
        XteePrefix *loaded_prefix;

        change_wine_field = dialog->fields + change_wine_version_i;
        form_data = &change_wine_field->data.stringandbutton_form;
        loaded_prefix = xtee_state_get_loaded_prefix();
        xtee_gui_set_motif_label_string(form_data->string_label,
                        loaded_prefix->wine_version.s);
}

void
xtee_gui_show_prefix_options_dialog(Widget main_win,
                XteeGuiBasicFormDialog *dialog)
{
        static bool is_dialog_ready = false;
        XteePrefix *loaded_prefix;
        XteeGuiFormField *disable_winemenubuilder_field;

        loaded_prefix = xtee_state_get_loaded_prefix();

        if (!is_dialog_ready) {
                create_XteeGuiBasicFormDialog(dialog,
                                main_win,
                                "prefix_options_dialog",
                                "Prefix options");
                add_toggle_form_field_to_XteeGuiBasicFormDialog(
                                dialog,
                                "disable_winemenubuilder_toggle",
                                "Disable winemenubuilder");
                XtAddCallback(dialog->ok_btn, XmNactivateCallback,
                                prefix_options_dialog_ok_cb,
                                dialog);
                XtAddCallback(dialog->cancel_btn, XmNactivateCallback,
                                prefix_options_dialog_cancel_cb,
                                (XtPointer)dialog->dialog_form);
                add_string_and_button_field_to_XteeGuiBasicFormDialog(dialog,
                                "wine_version_form", "Wine version: ",
                                "wine_version_string_label",
                                loaded_prefix->wine_version.s,
                                "change_wine_button", "Change Wine version",
                                prefix_options_change_wine_cb,
                                dialog);
                add_string_and_button_field_to_XteeGuiBasicFormDialog(dialog,
                                "install_dxvk_form", "DXVK version: ",
                                "dxvk_version_string_label", "Lorem ipsum",
                                "install_dxvk_button", "Install DXVK",
                                prefix_options_install_dxvk_cb,
                                dialog);
                is_dialog_ready = true;
        }

        reset_XteeGuiBasicFormDialog_fields(dialog);
        disable_winemenubuilder_field = dialog->fields
                + disable_winemenubuilder_i;
        XmToggleButtonGadgetSetState(
                        disable_winemenubuilder_field->data.toggle_g,
                        loaded_prefix->disable_winemenubuilder,
                        False);
        set_installed_dxvk_field(dialog);
        set_change_wine_string(dialog);
        XtManageChild(dialog->dialog_form);
}
