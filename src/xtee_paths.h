#ifndef XTEE_PATHS_H
#define XTEE_PATHS_H

#include "xtee_string.h"

/* This function uses the same allocated string as a return value in any call.
 * This means that the returned string must not be freed except at the end of
 * the program with free_xteetotaller_dir_path. */
XteeConstString get_xteetotaller_dir_path(void);

void free_xteetotaller_dir_path(void);

#endif
