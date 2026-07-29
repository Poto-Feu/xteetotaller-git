#ifndef XTEE_WINE_H
#define XTEE_WINE_H

#include "xtee_prefix.h"

enum XteeWineVersionArch {
        XTEE_WINE_VERSION_ARCH_X86,
        XTEE_WINE_VERSION_ARCH_AMD64,
        XTEE_WINE_VERSION_ARCH_WOW64,
        XTEE_WINE_VERSION_ARCH_WRONG
};

typedef struct XteeWineVersionData {
        XteeString id;
        XteeString wine_path;
        XteeString wine_lib_path;
        XteeString wineboot_path;
        XteeString winecfg_path;
        XteeString wineserver_path;
        enum XteeWineVersionArch arch;
} XteeWineVersionData;

typedef struct XteeWineVersionDataBlock{
        XteeWineVersionData *block;
        size_t n;
} XteeWineVersionDataBlock;

extern XteeWineVersionDataBlock xtee_wine_versions_data;

/* Call this after xtee_system_init. */
void xtee_wine_init(void);
void xtee_wine_free(void);

XteeWineVersionData *xtee_wine_get_version_data(const char *id);
XteeStringList xtee_wine_get_versions_ids(void);

#endif
