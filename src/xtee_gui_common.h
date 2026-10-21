/*
 * Copyright (C) 2026 Adrien Saad
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef XTEE_GUI_COMMON_H
#define XTEE_GUI_COMMON_H

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#define XTEE_XT_CALLBACK_FUNC(func_name) \
        void func_name (Widget w, XtPointer client_data, XtPointer call_data)

#define XTEE_XT_CALLBACK_FUNC_PTR(fp) \
        void (*fp) (Widget, XtPointer, XtPointer)

#define XTEE_BUSY_NANOSLEEP 33000000L

#define XTEE_GUI_BASICFORMDIALOG_DEFAULT_WIDTH 600
#define XTEE_GUI_BASICFORMDIALOG_DEFAULT_HEIGHT 450

#define XTEE_FILESELECTIONBOX_WIDTH 420
#define XTEE_FILESELECTIONBOX_HEIGHT 360

void xtee_gui_update_display(void);
Display *xtee_gui_get_display(void);

Widget xtee_gui_get_textfield_child_widget(Widget);
char *xtee_gui_get_textfield_child_string(Widget);
void xtee_gui_set_dialog_title_message(Widget dialog, char *title, char *msg);

XTEE_XT_CALLBACK_FUNC(xtee_gui_reset_message_area_string);
#define xtee_gui_reset_message_area_string_CALL \
        xtee_gui_reset_message_area_string(NULL, NULL, NULL)

Widget xtee_create_FileSelectionDialog(Widget parent, char *dialog_name,
                char *dialog_title, unsigned char file_type_mask);

inline XmString
xtee_new_XmString(char *s)
{
        return XmStringCreateLocalized(s);
}

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
