#ifndef XTEE_PATHS_H
#define XTEE_PATHS_H

#include "xtee_string.h"

/* This function uses the same allocated string as a return value in any call.
 * This means that the returned string must not be freed except at the end of
 * the program with free_xteetotaller_dir_path.
 * Please note that the returned string will have a trailing slash so there is
 * no need to add a directory separator at the end, even though it will only
 * save one byte. */
XteeConstString get_xteetotaller_dir_path(void);

void free_xteetotaller_dir_path(void);

#endif
