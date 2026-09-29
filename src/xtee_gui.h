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

#ifndef XTEE_GUI_H
#define XTEE_GUI_H

#include "xtee_gui_common.h"
#include "xtee_prefix.h"
#include "xtee_utils.h"

extern XmRenderTable xtee_gui_render_table;

void xtee_gui_init(int *argc, char *argv[]);
void xtee_gui_run(void);

char *XmString_to_char_str(XmString *xmstr);

void xtee_gui_set_message_area_string(char *s);
void refresh_programs_list(void);

int xtee_gui_load_prefix(const char *prefix_id, Widget parent);

void xtee_gui_set_motif_label_string(Widget w, char *s);

void gui_print_error_dialog(char *msg, Widget parent);
void gui_print_error_log_and_dialog(char *msg, Widget parent);
void gui_print_warning_dialog(char *msg, Widget parent);

Widget xtee_gui_get_main_win(void);

/* The block pointed by args MUST NOT have a NULL pointer at its end and
 * args_n should also reflect this.
 * The first argument MUST NOT be the path to the program, as the function
 * will handle it by allocating its own arguments block and append the
 * contents of args to it. */
int xtee_gui_fork_exec(char *pathname, char **args, size_t args_n,
                const XteeEnvData *env, size_t env_n);

/* If there are no selected program, a non-zero value is returned.
 * DO NOT free XteeString after usage. */
int xtee_gui_get_selected_program_string(XteeString *str);

void xtee_gui_print_get_program_data_error(
                enum XteeGetProgramDataStatus status);

#endif
