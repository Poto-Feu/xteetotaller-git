#ifndef XTEE_GUI_FORMS_H
#define XTEE_GUI_FORMS_H

#include "xtee_string.h"

#include <X11/Intrinsic.h>

#include <stdbool.h>
#include <stdlib.h>

#define XTEE_FORM_LR_MARGIN 25

typedef struct XteeGuiLabelTextForm {
        Widget form;
        Widget label;
        Widget text;
} XteeGuiLabelTextForm;

/* LTPF = LabelTextPathForm. It is too long to not be abbreviated. */
typedef struct XteeGuiLTPF {
        Widget form;
        Widget label;
        Widget text;
        Widget browse_btn;
        Widget browse_dialog;
} XteeGuiLTPF;

typedef struct XteeGuiRadioBoxForm {
        Widget form;
        Widget label;
        Widget row_column;
        Widget *choices_w;
        size_t choices_n;
} XteeGuiRadioBoxForm;

typedef struct XteeGuiDropDownListForm {
        Widget form;
        Widget label;
        Widget list;
} XteeGuiDropDownListForm;

typedef struct XteeGuiListButtonsForm {
        Widget form;
        Widget label;
        Widget list;
        Widget row_column;
        Widget *buttons;
        size_t buttons_n;
} XteeGuiListButtonsForm;

enum XteeGuiFormFieldType {
        XTEE_GUI_FORM_FIELD_LABELTEXT = 0,
        XTEE_GUI_FORM_FIELD_LTPF,
        XTEE_GUI_FORM_FIELD_TOGGLE,
        XTEE_GUI_FORM_FIELD_RADIOBOX,
        XTEE_GUI_FORM_FIELD_DROPDOWNLIST,
        XTEE_GUI_FORM_FIELD_LIST_BUTTONS,
        /* Not used, only set to compare it with previous */
        XTEE_GUI_FORM_FIELD_LAST,
};

typedef struct XteeGuiFormField {
        union {
                XteeGuiLabelTextForm ltf;
                XteeGuiLTPF ltpf;
                Widget toggle_g;
                XteeGuiRadioBoxForm radiobox_form;
                XteeGuiDropDownListForm dropdownlist_form;
                XteeGuiListButtonsForm listbuttons_form;
        } data;
        enum XteeGuiFormFieldType type;
} XteeGuiFormField;

typedef struct XteeGuiBasicFormDialog {
        Widget dialog_form;
        Widget ok_btn;
        Widget cancel_btn;
        Widget ok_cancel_column;
        XteeGuiFormField *fields;
        size_t fields_n;
} XteeGuiBasicFormDialog;

void manage_XteeGuiBasicFormDialog_fields(XteeGuiBasicFormDialog *dialog);
void free_XteeGuiFormField(XteeGuiFormField *field);
/* Only use on LabelTextForm, LTPF and ComboBox fields. */
char *get_XteeGuiFormField_string_value(XteeGuiFormField *field);
void reset_XteeGuiFormField(XteeGuiFormField *field);

/* Ok and cancel buttons callback must be set manually if auto_unmanage is set
 * to false. */
void create_XteeGuiBasicFormDialog(XteeGuiBasicFormDialog *dialog,
                Widget parent, char *name, char *title, bool auto_unmanage);
void free_XteeGuiBasicFormDialog(XteeGuiBasicFormDialog *dialog);
XteeGuiFormField *get_last_XteeGuiFormField_data(
                XteeGuiBasicFormDialog *dialog);
Widget get_last_XteeGuiFormField_widget(XteeGuiBasicFormDialog *dialog);

/* Can only add LabelTextForm, LTPF and Toggle fields */
void add_simple_form_field_to_XteeGuiBasicFormDialog(
                XteeGuiBasicFormDialog *dialog,
                enum XteeGuiFormFieldType type, char *w_name, char *label_str);
void add_radiobox_field_to_XteeGuiBasicFormDialog(
                XteeGuiBasicFormDialog *dialog,
                char *w_name, char *label_str,
                char **choices_str, unsigned int choices_n);
void add_dropdownlist_field_to_XteeGuiBasicFormDialog(
                XteeGuiBasicFormDialog *dialog,
                char *w_name, char *label_str,
                XteeStringList choices_strs);
void add_list_buttons_field_to_XteeGuiBasicFormDialog(
                XteeGuiBasicFormDialog *dialog, char *w_name, char *label_str,
                XteeStringList buttons_names,
                char **buttons_strs, bool are_btns_enabled);

/* Add a XmNvalueChangedCallback to each RadioBox children.
 * The client_data will contain the id number of the toggle that triggered it
 * casted to an XtPointer.
 * Id number of each button starts from 1, and not 0 - the latter can be used
 * as a value for a global state variable as no RadioBox toggles are set in
 * their initial state. */
void
XteeGuiRadioBoxForm_add_callback(XteeGuiRadioBoxForm *radiobox_form,
                void (*cb_func)(Widget, XtPointer, XtPointer));



inline void
reset_XteeGuiBasicFormDialog_fields(XteeGuiBasicFormDialog *dialog)
{
        for (size_t i = 0; i < dialog->fields_n; ++i) {
                reset_XteeGuiFormField(&dialog->fields[i]);
        }
}

#endif
