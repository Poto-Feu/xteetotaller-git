#ifndef XTEE_DXVK_H
#define XTEE_DXVK_H

#include "xtee_string.h"

typedef struct XteeDxvkVersionData {
        XteeStringList x32_dll_strs;
        XteeStringList x64_dll_strs;
        XteeString id;
        XteeString x32_path;
        XteeString x64_path;
} XteeDxvkVersionData;

void xtee_dxvk_init(void);
void xtee_dxvk_free(void);

XteeStringList xtee_dxvk_get_ids(void);
XteeDxvkVersionData *xtee_dxvk_get_data(const char *id);

#endif
