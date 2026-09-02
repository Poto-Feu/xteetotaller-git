#include "xtee_gui_common.h"
#include "xtee_gui.h"

#include <X11/Intrinsic.h>
#include <Xm/TextF.h>

extern inline void xtee_gui_hide_dialog_cancel_button(Widget w);
extern inline void xtee_gui_hide_dialog_help_button(Widget w);
extern inline void xtee_gui_disable_dialog_help_button(Widget w);
extern inline void xtee_gui_free_XmStringTable_items(XmString *table,
                size_t n);

void
xtee_gui_update_display(void)
{
        XmUpdateDisplay(xtee_gui_get_main_win());
}

Display *
xtee_gui_get_display(void)
{
        return XtDisplay(xtee_gui_get_main_win());
}

Widget
xtee_gui_get_textfield_child_widget(Widget w)
{
        return XtNameToWidget(w, "*Text");
}

char *
xtee_gui_get_textfield_child_string(Widget w)
{
        return XmTextFieldGetString(xtee_gui_get_textfield_child_widget(w));
}

XTEE_XT_CALLBACK_FUNC(xtee_gui_reset_message_area_string)
{
        (void)w;
        (void)call_data;
        (void)client_data;
        xtee_gui_set_message_area_string("Ready");
}
