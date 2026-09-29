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

#include "xtee_gui_common.h"
#include "xtee_gui.h"

#include <X11/Intrinsic.h>
#include <Xm/FileSB.h>
#include <Xm/TextF.h>

extern inline XmString xtee_new_XmString(char *s);
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

void
xtee_gui_set_dialog_title_message(Widget dialog, char *title, char *msg)
{
        XmString title_str, msg_str;

        title_str = xtee_new_XmString(title);
        msg_str = xtee_new_XmString(msg);
        XtVaSetValues(dialog, XmNdialogTitle, title_str,
                        XmNmessageString, msg_str,
                        NULL);
        XmStringFree(title_str);
        XmStringFree(msg_str);
}

/* Needed to read "gui.show_hidden_files" parameter and set the value
 * accordingly. */
static
XTEE_XT_CALLBACK_FUNC(FileSelectionDialog_map_cb)
{
        const char *gui_show_hidden_files_val;

        (void)call_data;
        (void)client_data;
        gui_show_hidden_files_val = xtee_config_get_entry_value(
                        "gui.show_hidden_files");
        if (!gui_show_hidden_files_val
                        || !xtee_string_to_bool(gui_show_hidden_files_val)) {
                XtVaSetValues(w, XmNfileFilterStyle, XmFILTER_HIDDEN_FILES,
                                NULL);
        } else {
                XtVaSetValues(w, XmNfileFilterStyle, XmFILTER_NONE, NULL);
        }
}
Widget
xtee_create_FileSelectionDialog(Widget parent, char *dialog_name,
                char *dialog_title, unsigned char file_type_mask)
{
        XmString title_str = xtee_new_XmString(dialog_title);
        Arg args[] = {
                { XmNpathMode, XmPATH_MODE_RELATIVE },
                { XmNdialogTitle, (XtArgVal)title_str },
                { XmNresizePolicy, XmRESIZE_NONE },
                { XmNwidth, XTEE_FILESELECTIONBOX_WIDTH },
                { XmNheight, XTEE_FILESELECTIONBOX_HEIGHT },
                { XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL },
                { XmNfileTypeMask, file_type_mask },
        };
        Widget fileseldialog;

        fileseldialog = XmCreateFileSelectionDialog(parent, dialog_name,
                        args, XtNumber(args));
        XmStringFree(title_str);
        XtAddCallback(fileseldialog, XmNmapCallback,
                        FileSelectionDialog_map_cb, NULL);
        return fileseldialog;
}
