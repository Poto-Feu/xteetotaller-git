#include "xtee_gui_common.h"
#include "xtee_gui.h"

extern inline void xtee_gui_hide_dialog_cancel_button(Widget w);
extern inline void xtee_gui_hide_dialog_help_button(Widget w);
extern inline void xtee_gui_disable_dialog_help_button(Widget w);

void xtee_gui_update_display(void)
{
        XmUpdateDisplay(xtee_gui_get_main_win());
}
