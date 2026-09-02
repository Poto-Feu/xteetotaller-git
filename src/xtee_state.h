#ifndef XTEE_STATE_H
#define XTEE_STATE_H

#include "xtee_prefix.h"
#include "xtee_wine.h"

extern bool xtee_state_is_prefix_loaded;

void xtee_state_free(void);

XteePrefix *xtee_state_get_loaded_prefix(void);
void xtee_state_set_loaded_prefix(const XteePrefix *val);

enum XteeGetProgramDataStatus xtee_state_set_loaded_program(
                const char *program_id);
XteeProgram *xtee_state_get_loaded_program(void);
void xtee_state_unload_program(void);
int xtee_state_save_loaded_program(void);

XteeWineVersionData *xtee_state_get_loaded_prefix_wine_version_data(void);

#endif
