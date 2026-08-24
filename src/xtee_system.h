#ifndef XTEE_SYSTEM_H
#define XTEE_SYSTEM_H

#include "xtee_string.h"

extern bool xtee_system_is_wine_installed;
extern bool xtee_system_is_winetricks_installed;
extern XteeConstString xtee_system_wine_path;
extern XteeConstString xtee_system_wine_lib_path;
extern XteeConstString xtee_system_wineboot_path;
extern XteeConstString xtee_system_winecfg_path;
extern XteeConstString xtee_system_wineserver_path;
extern XteeConstString xtee_system_winetricks_path;

void xtee_system_init(void);
bool xtee_system_is_amd64(void);

#endif
