#ifndef XTEE_GUI_COMMON_H
#define XTEE_GUI_COMMON_H

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#define XTEE_XT_CALLBACK_FUNC(func_name) \
        void func_name (Widget w, XtPointer client_data, XtPointer call_data)

#define XTEE_XT_CALLBACK_FUNC_PTR(fp) \
        void (*fp) (Widget, XtPointer, XtPointer)

#define XTEE_FILESELECTIONBOX_WIDTH 350
#define XTEE_FILESELECTIONBOX_HEIGHT 380

void xtee_gui_update_display(void);
Display *xtee_gui_get_display(void);

Widget xtee_gui_get_textfield_child_widget(Widget);
char *xtee_gui_get_textfield_child_string(Widget);

inline void
xtee_gui_hide_dialog_cancel_button(Widget w)
{
        XtUnmanageChild(XtNameToWidget(w, "Cancel"));
}

inline void
xtee_gui_hide_dialog_help_button(Widget w)
{
        XtUnmanageChild(XtNameToWidget(w, "Help"));
}

inline void
xtee_gui_disable_dialog_help_button(Widget w)
{
        XtSetSensitive(XtNameToWidget(w, "Help"), False);
}

inline void
xtee_gui_free_XmStringTable_items(XmString *table, size_t n)
{
        for (size_t i = 0; i < n; ++i) {
                XmStringFree(table[i]);
        }
}

#endif
