#ifndef XTEE_GUI_H
#define XTEE_GUI_H

#include "xtee_gui_common.h"
#include "xtee_prefix.h"
#include "xtee_utils.h"

extern XmRenderTable xtee_gui_render_table;

void xtee_gui_init(int *argc, char *argv[]);
void xtee_gui_run(void);
void xtee_gui_free(void);

char *XmString_to_char_str(XmString *xmstr);

void xtee_gui_set_message_area_string(char *s);
int xtee_gui_load_prefix(const char *prefix_id, Widget parent);

void xtee_gui_set_motif_label_string(Widget w, char *s);

void gui_print_error_dialog(char *msg, Widget parent);
void gui_print_error_log_and_dialog(char *msg, Widget parent);
void gui_print_warning_dialog(char *msg, Widget parent);

Widget xtee_gui_get_main_win(void);

/* Mostly used for working dialogs */
void xtee_gui_handle_busy_events(void);

/* The block pointed by args MUST NOT have a NULL pointer at its end and
 * args_n should also reflect this.
 * The first argument MUST NOT be the path to the program, as the function
 * will handle it by allocating its own arguments block and append the
 * contents of args to it. */
int xtee_gui_fork_exec(char *pathname, char **args, size_t args_n,
                const XteeEnvData *env, size_t env_n);

/* If there are no selected program, a non-zero value is returned. */
int xtee_gui_get_selected_program_string(XteeString *str);

void xtee_gui_print_get_program_data_error(
                enum XteeGetProgramDataStatus status);

#endif
