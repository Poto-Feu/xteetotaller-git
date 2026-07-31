#include "xtee_gui_form.h"
#include "xtee_error.h"
#include "xtee_gui.h"

#include <Xm/ComboBox.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>

#include <stdbool.h>

extern inline void manage_XteeGuiBasicFormDialog_fields(
                XteeGuiBasicFormDialog *dialog);
extern inline void reset_XteeGuiBasicFormDialog_fields(
                XteeGuiBasicFormDialog *dialog);

static void
create_gui_label_text_form(XteeGuiLabelTextForm *form_data, Widget parent,
                char *form_name,
                char *label_name, char *label_str,
                char *text_name, int text_left_margin)
{
        XmString tmp_str;

        form_data->form = XmCreateForm(parent, form_name, NULL, 0);
        tmp_str = XmStringCreateLocalized(label_str);
        form_data->label = XmVaCreateManagedLabelGadget(form_data->form,
                        label_name,
                        XmNlabelString, tmp_str,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNtopOffset, 5,
                        XmNleftAttachment, XmATTACH_FORM,
                        NULL);
        XmStringFree(tmp_str);
        form_data->text = XmVaCreateTextField(form_data->form, text_name,
                        XmNrenderTable, xtee_gui_render_table,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, form_data->label,
                        XmNleftOffset, text_left_margin,
                        XmNrightAttachment, XmATTACH_FORM,
                        NULL);
}

static void
manage_gui_label_text_form(XteeGuiLabelTextForm *form_data)
{
        XtManageChild(form_data->text);
        XtManageChild(form_data->form);
}

static void
reset_gui_label_text_form(XteeGuiLabelTextForm *f)
{
        XtVaSetValues(f->text, XmNvalue, "", NULL);
}

static
XTEE_XT_CALLBACK_FUNC(ltpf_browse_btn_cb)
{
        Widget browse_dialog;

        (void)w;
        (void)call_data;
        browse_dialog = (Widget)client_data;
        XtManageChild(browse_dialog);
}

static
XTEE_XT_CALLBACK_FUNC(ltpf_file_selection_btns_cb)
{
        XmFileSelectionBoxCallbackStruct *cbs;
        Widget path_text;

        cbs = (XmFileSelectionBoxCallbackStruct *)call_data;
        path_text = (Widget)client_data;

        if (cbs->reason == XmCR_OK) {
                char *val_str;

                val_str = XmString_to_char_str(&cbs->value);
                if (!val_str) {
                        return;
                }
                XtVaSetValues(path_text,
                                XmNvalue, val_str,
                                NULL);
                XtFree(val_str);
                XtUnmanageChild(w);
        } else if (cbs->reason == XmCR_CANCEL) {
                XtUnmanageChild(w);
        }
}

static void
create_gui_ltpf(XteeGuiLTPF *form_data, Widget parent,
                char *form_name,
                char *label_name, char *label_str,
                char *text_name, int text_margin)
{
        Arg args[5];
        Cardinal n = 0;
        XmString tmp_str;

        form_data->form = XmCreateForm(parent, form_name, NULL, 0);
        tmp_str = XmStringCreateLocalized(label_str);
        form_data->label = XmVaCreateManagedLabelGadget(form_data->form,
                        label_name,
                        XmNlabelString, tmp_str,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNtopOffset, 5,
                        XmNleftAttachment, XmATTACH_FORM,
                        NULL);
        XmStringFree(tmp_str);

        tmp_str = XmStringCreateLocalized("...");
        form_data->browse_btn = XmVaCreatePushButton(form_data->form,
                        "browse_btn",
                        XmNlabelString, tmp_str,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNtraversalOn, False,
                        NULL);
        XmStringFree(tmp_str);
        XtSetArg(args[n], XmNpathMode, XmPATH_MODE_RELATIVE); n++;
        form_data->browse_dialog = XmCreateFileSelectionDialog(parent,
                        "browse_dialog", args, n);
        xtee_gui_disable_dialog_help_button(form_data->browse_dialog);
        XtAddCallback(form_data->browse_btn, XmNactivateCallback,
                        ltpf_browse_btn_cb,
                        (XtPointer)form_data->browse_dialog);
        form_data->text = XmVaCreateTextField(form_data->form, text_name,
                        XmNrenderTable, xtee_gui_render_table,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, form_data->label,
                        XmNleftOffset, text_margin,
                        XmNrightAttachment, XmATTACH_WIDGET,
                        XmNrightWidget, form_data->browse_btn,
                        XmNrightOffset, text_margin,
                        NULL);
        XtAddCallback(form_data->browse_dialog, XmNokCallback,
                        ltpf_file_selection_btns_cb,
                        (XtPointer)form_data->text);
        XtAddCallback(form_data->browse_dialog, XmNcancelCallback,
                        ltpf_file_selection_btns_cb,
                        (XtPointer)form_data->text);
}

static void
manage_gui_ltpf(XteeGuiLTPF *form_data)
{
        XtManageChild(form_data->text);
        XtManageChild(form_data->browse_btn);
        XtManageChild(form_data->form);
}

static void
reset_gui_ltpf(XteeGuiLTPF *f)
{
        XtVaSetValues(f->text, XmNvalue, "", NULL);
}

static void
create_gui_toggle_field(Widget *toggle_g, Widget parent,
                char *field_name, char *label_str)
{
        XmString tmp_str;

        tmp_str = XmStringCreateLocalized(label_str);
        *toggle_g = XmVaCreateManagedToggleButtonGadget(parent,
                        field_name,
                        XmNlabelString, tmp_str,
                        XmNalignment, XmALIGNMENT_BEGINNING,
                        NULL);
        XmStringFree(tmp_str);
}

void
manage_gui_radiobox_form(XteeGuiRadioBoxForm *field)
{
        XtManageChild(field->row_column);
        XtManageChild(field->form);
}

void
manage_gui_dropdownlist_form(XteeGuiDropDownListForm *field)
{
        XtManageChild(field->form);
}

void
manage_gui_listbuttons_form(XteeGuiListButtonsForm *field)
{
        XtManageChild(field->form);
}

static bool
is_form_field_type_correct(enum XteeGuiFormFieldType type)
{
        return !!(type < XTEE_GUI_FORM_FIELD_LAST && type >= 0);
}

static void
manage_XteeGuiFormField(XteeGuiFormField *field)
{
        enum XteeGuiFormFieldType type;

        type = field->type;
        if (!is_form_field_type_correct(type)) {
                print_warning("Incorrect XteeGuiFormFieldType value.");
                type = XTEE_GUI_FORM_FIELD_LABELTEXT;
        }

        switch (type) {
                case XTEE_GUI_FORM_FIELD_LABELTEXT:
                        manage_gui_label_text_form(&field->data.ltf);
                        break;
                case XTEE_GUI_FORM_FIELD_LTPF:
                        manage_gui_ltpf(&field->data.ltpf);
                        break;
                case XTEE_GUI_FORM_FIELD_TOGGLE:
                        /* Already managed at creation. */
                        break;
                case XTEE_GUI_FORM_FIELD_RADIOBOX:
                        manage_gui_radiobox_form(&field->data.radiobox_form);
                        break;
                case XTEE_GUI_FORM_FIELD_DROPDOWNLIST:
                        manage_gui_dropdownlist_form(
                                        &field->data.dropdownlist_form);
                        break;
                case XTEE_GUI_FORM_FIELD_LIST_BUTTONS:
                        manage_gui_listbuttons_form(
                                        &field->data.listbuttons_form);
                        break;
                default:
                        break;
        }
}

void
manage_XteeGuiBasicFormDialog_fields(XteeGuiBasicFormDialog *dialog)
{
        Widget last_field_form;

        for (size_t i = 0; i < dialog->fields_n; ++i) {
                manage_XteeGuiFormField(&dialog->fields[i]);
        }

        if (!(last_field_form = get_last_XteeGuiFormField_widget(dialog))) {
                return;
        }
        XtVaSetValues(last_field_form,
                        XmNbottomAttachment, XmATTACH_WIDGET,
                        XmNbottomWidget, dialog->ok_cancel_column,
                        XmNbottomOffset, 5,
                        NULL);
}

char *
get_XteeGuiFormField_string_value(XteeGuiFormField *field)
{
        if (field->type == XTEE_GUI_FORM_FIELD_LABELTEXT) {
                return XmTextFieldGetString(field->data.ltf.text);
        } else if (field->type == XTEE_GUI_FORM_FIELD_LTPF) {
                return XmTextFieldGetString(field->data.ltpf.text);
        } else if (field->type == XTEE_GUI_FORM_FIELD_DROPDOWNLIST) {
                return XmTextFieldGetString(
                                XtNameToWidget(
                                        field->data.dropdownlist_form.list,
                                        "*Text"));
        }
        return NULL;
}

void
reset_XteeGuiFormField(XteeGuiFormField *field)
{
        switch (field->type) {
                case XTEE_GUI_FORM_FIELD_LABELTEXT:
                        reset_gui_label_text_form(&field->data.ltf);
                        break;
                case XTEE_GUI_FORM_FIELD_LTPF:
                        reset_gui_ltpf(&field->data.ltpf);
                        break;
                case XTEE_GUI_FORM_FIELD_TOGGLE:
                        XmToggleButtonGadgetSetState(field->data.toggle_g,
                                        False, False);
                        break;
                case XTEE_GUI_FORM_FIELD_LIST_BUTTONS:
                        XmListDeleteAllItems(
                                        field->data.listbuttons_form.list);
                        break;
                default:
                        break;
        }
}

Widget
get_XteeGuiFormField_widget(XteeGuiFormField *field)
{
        enum XteeGuiFormFieldType type;

        type = field->type;

        if (type != XTEE_GUI_FORM_FIELD_LABELTEXT
                        && type != XTEE_GUI_FORM_FIELD_LTPF) {
                type = XTEE_GUI_FORM_FIELD_LABELTEXT;
        }

        switch (type) {
                case XTEE_GUI_FORM_FIELD_LABELTEXT:
                        return field->data.ltf.form;
                case XTEE_GUI_FORM_FIELD_LTPF:
                        return field->data.ltpf.form;
                case XTEE_GUI_FORM_FIELD_TOGGLE:
                        return field->data.toggle_g;
                case XTEE_GUI_FORM_FIELD_RADIOBOX:
                        return field->data.radiobox_form.form;
                default:
                        return NULL;
        }
        return field->data.ltpf.form;
}

/* PushButtons and RowColumn widgets are managed at creation. */
static void
add_ok_cancel_buttons_to_form_dialog(Widget form, Widget *ok_cancel_column,
                Widget *ok_btn,
                Widget *cancel_btn)
{
        *ok_cancel_column = XtVaCreateWidget("ok_cancel_column",
                        xmRowColumnWidgetClass, form,
                        XmNorientation, XmHORIZONTAL,
                        XmNmarginWidth, 0,
                        XmNmarginHeight, 0,
                        XmNspacing, 8,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNbottomOffset, 12,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNleftOffset, XTEE_FORM_LR_MARGIN,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNrightOffset, XTEE_FORM_LR_MARGIN,
                        NULL);
        *ok_btn = XtVaCreateManagedWidget("OK", xmPushButtonWidgetClass,
                        *ok_cancel_column,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNbottomOffset, 12,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNleftOffset, XTEE_FORM_LR_MARGIN,
                        NULL);
        *cancel_btn = XtVaCreateManagedWidget("Cancel",
                        xmPushButtonWidgetClass, *ok_cancel_column,
                        XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNbottomWidget, *ok_btn,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, *ok_btn,
                        XmNleftOffset, 8,
                        NULL);
        XtManageChild(*ok_cancel_column);
}

void
create_XteeGuiBasicFormDialog(XteeGuiBasicFormDialog *dialog, Widget parent,
                char *name, char *title, bool auto_unmanage)
{
        Arg args[10];
        XmString dialog_title;
        Cardinal n = 0;

        dialog_title = XmStringCreateLocalized(title);
        XtSetArg(args[n], XmNdialogTitle, dialog_title); n++;
        XtSetArg(args[n], XmNdialogStyle,
                        XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
        XtSetArg(args[n], XmNwidth, 600); n++;
        XtSetArg(args[n], XmNheight, 450); n++;
        if (auto_unmanage) {
                XtSetArg(args[n], XmNautoUnmanage, True); n++;
        } else {
                XtSetArg(args[n], XmNautoUnmanage, False); n++;
        }

        dialog->dialog_form = XmCreateFormDialog(parent, name, args, n);
        add_ok_cancel_buttons_to_form_dialog(dialog->dialog_form,
                        &dialog->ok_cancel_column,
                        &dialog->ok_btn,
                        &dialog->cancel_btn);
        dialog->fields = NULL;
        dialog->fields_n = 0;
}

void
free_XteeGuiBasicFormDialog(XteeGuiBasicFormDialog *dialog)
{
        free(dialog->fields);
}

XteeGuiFormField *
get_last_XteeGuiFormField_data(XteeGuiBasicFormDialog *dialog)
{
        if (!dialog->fields_n) {
                return NULL;
        }
        return &dialog->fields[dialog->fields_n - 1];
}

Widget
get_last_XteeGuiFormField_widget(XteeGuiBasicFormDialog *dialog)
{
        if (!dialog->fields_n) {
                return NULL;
        }

        return get_XteeGuiFormField_widget(
                        &dialog->fields[dialog->fields_n - 1]);
}

static void
set_form_attach_resources(Widget new_w, Widget prev_w)
{
        XtVaSetValues(new_w,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNleftOffset, XTEE_FORM_LR_MARGIN,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNrightOffset, XTEE_FORM_LR_MARGIN,
                        NULL);

        if (!prev_w) {
                XtVaSetValues(new_w,
                                XmNtopAttachment, XmATTACH_FORM,
                                XmNtopOffset, 20,
                                NULL);
        } else {
                XtVaSetValues(new_w,
                                XmNtopAttachment, XmATTACH_WIDGET,
                                XmNtopWidget, prev_w,
                                XmNtopOffset, 5,
                                NULL);
        }
}

void
add_simple_form_field_to_XteeGuiBasicFormDialog(XteeGuiBasicFormDialog *dialog,
                enum XteeGuiFormFieldType type, char *w_name,
                char *label_str)
{
        const int text_w_margin = 10;
        XteeGuiFormField *tmp_ptr;
        bool is_first_field;
        size_t field_i;
        Widget new_w = NULL;
        Widget prev_w = NULL;

        if (type != XTEE_GUI_FORM_FIELD_LABELTEXT
                        && type != XTEE_GUI_FORM_FIELD_LTPF
                        && type != XTEE_GUI_FORM_FIELD_TOGGLE) {
                type = XTEE_GUI_FORM_FIELD_LABELTEXT;
        }

        is_first_field = dialog->fields_n ? false : true;
        if (!is_first_field) {
                prev_w = get_last_XteeGuiFormField_widget(dialog);
        }


        tmp_ptr = realloc_or_exit(dialog->fields,
                        ++dialog->fields_n * sizeof(XteeGuiFormField));
        dialog->fields = tmp_ptr;
        field_i = dialog->fields_n - 1;

        if (type == XTEE_GUI_FORM_FIELD_LABELTEXT) {
                create_gui_label_text_form(
                                &dialog->fields[field_i].data.ltf,
                                dialog->dialog_form,
                                w_name,
                                "form_label", label_str,
                                "form_text", text_w_margin);
                new_w = dialog->fields[field_i].data.ltf.form;
        } else if (type == XTEE_GUI_FORM_FIELD_LTPF) {
                create_gui_ltpf(&dialog->fields[field_i].data.ltpf,
                                dialog->dialog_form,
                                w_name,
                                "form_label", label_str,
                                "form_text", text_w_margin);
                new_w = dialog->fields[field_i].data.ltpf.form;
        } else if (type == XTEE_GUI_FORM_FIELD_TOGGLE) {
                create_gui_toggle_field(&dialog->fields[field_i].data.toggle_g,
                                dialog->dialog_form,
                                w_name, label_str);
                new_w = dialog->fields[field_i].data.toggle_g;
        } else {
                /* If this happens, there is a bug somewhere */
                fatal_error("Unknown form field type.");
        }
        dialog->fields[field_i].type = type;
        set_form_attach_resources(new_w, prev_w);
}

void
add_radiobox_field_to_XteeGuiBasicFormDialog(XteeGuiBasicFormDialog *dialog,
                char *w_name, char *label_str,
                char **choices_str, unsigned int choices_n)
{
        const int radiobox_w_margin = 10;
        XteeGuiFormField *tmp_ptr;
        XteeGuiRadioBoxForm *form_data;
        Widget prev_w = NULL;
        XmString tmp_str;
        size_t field_i;
        bool is_first_field;

        is_first_field = dialog->fields_n ? false : true;
        if (!is_first_field) {
                prev_w = get_last_XteeGuiFormField_widget(dialog);
        }

        tmp_ptr = realloc_or_exit(dialog->fields,
                        ++dialog->fields_n * sizeof(XteeGuiFormField));
        dialog->fields = tmp_ptr;
        field_i = dialog->fields_n - 1;

        form_data = &dialog->fields[field_i].data.radiobox_form;
        dialog->fields[field_i].type = XTEE_GUI_FORM_FIELD_RADIOBOX;
        form_data->form = XtCreateWidget(w_name, xmFormWidgetClass,
                        dialog->dialog_form, NULL, 0);

        tmp_str = XmStringCreateLocalized(label_str);
        form_data->label = XtVaCreateManagedWidget(
                        "form_label",
                        xmLabelGadgetClass,
                        form_data->form,
                        XmNlabelString, tmp_str,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNtopOffset, 5,
                        XmNleftAttachment, XmATTACH_FORM,
                        NULL);
        XmStringFree(tmp_str);

        form_data->choices_w = malloc_or_exit(choices_n * sizeof(Widget));
        form_data->choices_n = choices_n;
        form_data->row_column = XmVaCreateSimpleRadioBox(form_data->form,
                        "radio_box", 0, NULL,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, form_data->label,
                        XmNleftOffset, radiobox_w_margin,
                        NULL);
        for (size_t i = 0; i < choices_n; ++i) {
                form_data->choices_w[i] = XtCreateManagedWidget(
                                choices_str[i], xmToggleButtonGadgetClass,
                                form_data->row_column, NULL, 0);
        }

        set_form_attach_resources(form_data->form, prev_w);

}

void
add_dropdownlist_field_to_XteeGuiBasicFormDialog(
                XteeGuiBasicFormDialog *dialog, char *w_name, char *label_str,
                XteeStringList choices_strs)
{
        const int dropdownlist_w_margin = 10;
        Arg args[10];
        Cardinal n = 0;
        XmStringTable items_table = NULL;
        XteeGuiFormField *tmp_ptr;
        XteeGuiDropDownListForm *form_data;
        Widget prev_w = NULL;
        XmString tmp_str;
        size_t field_i;
        bool is_first_field;

        is_first_field = dialog->fields_n ? false : true;
        if (!is_first_field) {
                prev_w = get_last_XteeGuiFormField_widget(dialog);
        }

        tmp_ptr = realloc_or_exit(dialog->fields,
                        ++dialog->fields_n * sizeof(XteeGuiFormField));
        dialog->fields = tmp_ptr;
        field_i = dialog->fields_n - 1;

        form_data = &dialog->fields[field_i].data.dropdownlist_form;
        dialog->fields[field_i].type = XTEE_GUI_FORM_FIELD_DROPDOWNLIST;
        form_data->form = XtCreateWidget(w_name, xmFormWidgetClass,
                        dialog->dialog_form, NULL, 0);

        XtVaSetValues(form_data->form,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNleftOffset, XTEE_FORM_LR_MARGIN,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNrightOffset, XTEE_FORM_LR_MARGIN,
                        NULL);
        if (is_first_field) {
                XtVaSetValues(form_data->form,
                                XmNtopAttachment, XmATTACH_FORM,
                                XmNtopOffset, 20,
                                NULL);
        } else {
                XtVaSetValues(form_data->form,
                                XmNtopAttachment, XmATTACH_WIDGET,
                                XmNtopWidget, prev_w,
                                XmNtopOffset, 5,
                                NULL);
        }

        tmp_str = XmStringCreateLocalized(label_str);
        form_data->label = XtVaCreateManagedWidget(
                        "form_label",
                        xmLabelGadgetClass,
                        form_data->form,
                        XmNlabelString, tmp_str,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNtopOffset, 5,
                        XmNleftAttachment, XmATTACH_FORM,
                        NULL);
        XmStringFree(tmp_str);

        items_table = malloc_or_exit(choices_strs.n * sizeof(XmString));
        for (size_t i = 0; i < choices_strs.n; ++i) {
                items_table[i] = XmStringCreateLocalized(
                                choices_strs.data[i].s);
        }
        XtSetArg(args[n], XmNcomboBoxType, XmDROP_DOWN_LIST); n++;
        XtSetArg(args[n], XmNitems, items_table); n++;
        XtSetArg(args[n], XmNitemCount, (int)choices_strs.n); n++;
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
        XtSetArg(args[n], XmNleftWidget, form_data->label); n++;
        XtSetArg(args[n], XmNleftOffset, dropdownlist_w_margin); n++;
        form_data->list = XtCreateManagedWidget("dropdownlist",
                        xmComboBoxWidgetClass, form_data->form, args, n);
        for (size_t i = 0; i < choices_strs.n; ++i) {
                XmStringFree(items_table[i]);
        }
        free(items_table);

        set_form_attach_resources(form_data->form, prev_w);
}

void
add_list_buttons_field_to_XteeGuiBasicFormDialog(
                XteeGuiBasicFormDialog *dialog, char *w_name, char *label_str,
                XteeStringList buttons_names,
                char **buttons_strs,
                bool are_btns_enabled)
{
        Arg args[5];
        Cardinal n = 0;
        const int list_w_margin = 10;
        XteeGuiFormField *tmp_ptr;
        XteeGuiListButtonsForm *form_data;
        Widget prev_w = NULL;
        Widget list_parent;
        XmString tmp_str;
        size_t field_i;
        bool is_first_field;

        is_first_field = dialog->fields_n ? false : true;
        if (!is_first_field) {
                prev_w = get_last_XteeGuiFormField_widget(dialog);
        }
        tmp_ptr = realloc_or_exit(dialog->fields,
                        ++dialog->fields_n * sizeof(XteeGuiFormField));
        dialog->fields = tmp_ptr;
        field_i = dialog->fields_n - 1;
        form_data = &dialog->fields[field_i].data.listbuttons_form;
        form_data->form = XtCreateWidget(w_name, xmFormWidgetClass,
                        dialog->dialog_form, NULL, 0);
        dialog->fields[field_i].type = XTEE_GUI_FORM_FIELD_LIST_BUTTONS;
        XtVaSetValues(form_data->form,
                        XmNfractionBase, 8,
                        NULL);
        if (is_first_field) {
                XtVaSetValues(form_data->form,
                                XmNtopAttachment, XmATTACH_FORM,
                                XmNtopOffset, 20,
                                NULL);
        } else {
                XtVaSetValues(form_data->form,
                                XmNtopAttachment, XmATTACH_WIDGET,
                                XmNtopWidget, prev_w,
                                XmNtopOffset, 5,
                                NULL);
        }

        tmp_str = XmStringCreateLocalized(label_str);
        form_data->label = XtVaCreateManagedWidget(
                        "form_label",
                        xmLabelGadgetClass,
                        form_data->form,
                        XmNlabelString, tmp_str,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNtopOffset, 5,
                        XmNleftAttachment, XmATTACH_FORM,
                        NULL);
        XmStringFree(tmp_str);
        XtSetArg(args[n], XmNselectionPolicy, XmSINGLE_SELECT); ++n;
        XtSetArg(args[n], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE); ++n;
        form_data->list = XmCreateScrolledList(form_data->form,
                        "form_field_list", args, n);
        list_parent = XtParent(form_data->list);
        XtVaSetValues(list_parent,
                        XmNheight, 150,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, form_data->label,
                        XmNleftOffset, list_w_margin,
                        XmNrightAttachment, XmATTACH_POSITION,
                        XmNrightPosition, 6,
                        XmNtopAttachment, XmATTACH_FORM,
                        NULL);
        XtManageChild(form_data->list);
        form_data->row_column = XmVaCreateRowColumn(
                        form_data->form, "form_buttons_row_column",
                        XmNorientation, XmVERTICAL,
                        XmNisAligned, False,
                        XmNentryAlignment, XmALIGNMENT_CENTER,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, list_parent,
                        XmNleftOffset, list_w_margin,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNbottomWidget, list_parent,
                        NULL);
        form_data->buttons_n = buttons_names.n;
        form_data->buttons = malloc_or_exit(
                        form_data->buttons_n * sizeof(Widget));
        for (size_t i = 0; i < form_data->buttons_n; ++i) {
                XmString tmp_btn_str;

                tmp_btn_str = XmStringCreateLocalized(buttons_strs[i]);
                form_data->buttons[i] = XmVaCreatePushButton(
                                form_data->row_column,
                                buttons_names.data[i].s,
                                XmNlabelString, tmp_btn_str,
                                NULL);
                XmStringFree(tmp_btn_str);
                if (!are_btns_enabled) {
                        XtSetSensitive(form_data->buttons[i], False);
                }
                XtManageChild(form_data->buttons[i]);
        }
        XtManageChild(form_data->row_column);
        set_form_attach_resources(form_data->form, prev_w);
}

void
XteeGuiRadioBoxForm_add_callback(XteeGuiRadioBoxForm *radiobox_form,
                void (*cb_func)(Widget, XtPointer, XtPointer))
{
        for (unsigned int i = 0; i < (unsigned int)radiobox_form->choices_n;
                        ++i) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
                XtAddCallback(radiobox_form->choices_w[i],
                                XmNvalueChangedCallback, cb_func,
                                (XtPointer)(i + 1));
#pragma GCC diagnostic pop
        }
}
