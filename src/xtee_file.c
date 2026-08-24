#include "xtee_file.h"
#include "xtee_utils.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

int
xtee_directory_exist_or_create(const XteeConstString path)
{
        if (mkdir(path.s, XTEE_NEW_DIR_MODE) != 0 && errno != EEXIST) {
                return -1;
        }
        return 0;
}

bool
xtee_is_windows_illegal_character(char c)
{
        switch (c) {
                case '<':
                case '>':
                case ':':
                case '"':
                case '/':
                case '\\':
                case '|':
                case '?':
                case '*':
                        return true;
                        break;
                default:
                        break;
        }
        return false;
}

/* The following function is not written in a very efficient way with its
 * multiple use of rewind(3).
 * However, this is negligeable as this function is rarely called and
 * often only after an user action.
 * Moreover at this scale it is unlikely to be noticeable. */

#define E_LFANEW_LOCATION 0x3C

int
xtee_get_executable_type(enum XteeExecutableType *type,
                const XteeConstString cstr)
{
        unsigned char dos_header_buf[E_LFANEW_LOCATION + 4];
        char magic_number[2];
        char signature[4];
        long file_size;
        int32_t e_lfanew;
        FILE *f = NULL;

        if (!(f = fopen(cstr.s, "rb"))) {
                return -1;
        }

        if ((fseek(f, 0, SEEK_END)) == -1) {
                goto error_rtrn;
        }
        file_size = ftell(f);
        if (file_size < 0) {
                goto error_rtrn;
        }
        rewind(f);

        /* Look for the DOS "MZ" magic number. */
        if ((fread(magic_number, 1, 2, f)) != 2) {
                goto error_rtrn;
        }
        *type = XTEE_EXECUTABLE_UNKNOWN;

        if (magic_number[0] != 'M' || magic_number[1] != 'Z') {
                *type = XTEE_EXECUTABLE_UNKNOWN;
                goto early_rtrn;
        }
        rewind(f);

        if (fread(dos_header_buf, 1, sizeof(dos_header_buf), f)
                        != sizeof(dos_header_buf)) {
                goto error_rtrn;
        }
        memcpy(&e_lfanew, dos_header_buf + E_LFANEW_LOCATION, 4);

        /* If e_lfanew is not pointing to any correct location in the file,
         * this is probably a DOS executable.
         * However, there are also DOS executable in NE forms.
         * More checking is thus required. */
        if (e_lfanew <= 0 || file_size < (long)e_lfanew) {
                *type = XTEE_EXECUTABLE_DOS_MZ;
                goto early_rtrn;
        }
        fseek(f, e_lfanew, SEEK_SET);
        if (fread(signature, 1, 4, f) != 4) {
                goto error_rtrn;
        }

        if (signature[0] == 'N' && signature[1] == 'E') {
                *type = XTEE_EXECUTABLE_NE;
        } else if (signature[0] == 'L' && signature[1] == 'E') {
                *type = XTEE_EXECUTABLE_LE;
        } else if (signature[0] == 'P' && signature[1] == 'E') {
                const uint16_t i386_type = 0x14C;
                const uint16_t x64_type = 0x8664;
                uint16_t machine;

                if (fread(&machine, 1, sizeof(machine), f)
                                != sizeof(machine)) {
                        goto error_rtrn;
                }

                if (machine == i386_type) {
                        *type = XTEE_EXECUTABLE_PE32;
                } else if (machine == x64_type) {
                        *type = XTEE_EXECUTABLE_PE32PLUS;
                } else {
                        *type = XTEE_EXECUTABLE_UNKNOWN;
                }
        } else {
                *type = XTEE_EXECUTABLE_UNKNOWN;
        }

early_rtrn:
        if (f) {
                fclose(f);
        }
        return 0;

error_rtrn:
        if (f) {
                fclose(f);
        }
        return -1;
}

bool
xtee_is_file_msi(const XteeConstString cstr)
{
        /* 3 characters for the msi extension, one for the dot and at least 1
         * for the rest of the filename. */
        if (cstr.n < 5) {
                return false;
        }

        /* Check for extension case */
        if (tolower(cstr.s[cstr.n - 1]) != 'i'
                        || tolower(cstr.s[cstr.n - 2]) != 's'
                        || tolower(cstr.s[cstr.n - 3]) != 'm'
                        || cstr.s[cstr.n - 4] != '.'
                        || xtee_is_windows_illegal_character(
                                cstr.s[cstr.n - 5])) {
                return false;
        }
        return true;
}

enum XteeDirStatus
xtee_get_dir_status(const char *path)
{
        DIR *dir = NULL;
        struct dirent *dir_entry;
        int prev_errno;

        if (path[0] != '/') {
                return XTEE_DIR_STATUS_PATH_NOT_VALID;
        }

        if (!(dir = opendir(path))) {
                switch (errno) {
                        case ENOENT:
                                return XTEE_DIR_STATUS_NOT_EXIST;
                        case ENOTDIR:
                                return XTEE_DIR_STATUS_NOT_DIR;
                        case EACCES:
                                return XTEE_DIR_STATUS_READ_ERROR;
                        default:
                                return XTEE_DIR_STATUS_OTHER_ERROR;
                }
        }
        prev_errno = errno;

        /* This loop checks if the directory is empty. */
        while ((dir_entry = readdir(dir))) {
                if (xtee_strcmp_bool(".", dir_entry->d_name)
                                || xtee_strcmp_bool("..", dir_entry->d_name)) {
                        continue;
                } else {
                        closedir(dir);
                        return XTEE_DIR_STATUS_NOT_EMPTY;
                }
        }
        closedir(dir);
        if (errno != prev_errno) {
                return XTEE_DIR_STATUS_READ_ERROR;
        }

        return XTEE_DIR_STATUS_EMPTY;
}

bool
xtee_does_dir_exist(const char *path)
{
        struct stat buf;

        return !!(stat(path, &buf) == 0 && S_ISDIR(buf.st_mode));
}

int
xtee_file_get_dir_files_list(XteeConstString dir_path,
                XteeStringList *list)
{
        const size_t start_listcapacity = 5;
        DIR *dir = NULL;
        struct dirent *dir_entry;
        size_t listcapacity = 0;
        int prev_errno;

        list->data = NULL;
        list->n = 0;

        if (!(dir = opendir(dir_path.s))) {
                print_error_strs(
                                (const char *[]){
                                "Cannot read directory: ",
                                dir_path.s,
                                NULL
                                });
                return -1;
        }

        list->data = malloc_or_exit(start_listcapacity * sizeof(XteeString));
        listcapacity = start_listcapacity;

        /* See get_prefixes_id comment for more details on prev_errno. */
        prev_errno = errno;
        while ((dir_entry = readdir(dir))) {
                if ((xtee_strcmp_bool(".", dir_entry->d_name))
                                || xtee_strcmp_bool("..", dir_entry->d_name)) {
                        continue;
                }

                if (list->n == listcapacity) {
                        XteeString *tmp_ptr;

                        listcapacity *= 2;
                        tmp_ptr = realloc_or_exit(list->data,
                                        listcapacity * sizeof(XteeString));
                        list->data = tmp_ptr;
                }
                list->data[list->n] = XteeString_create_from_charstring(
                                dir_entry->d_name);
                ++list->n;
        }

        closedir(dir);
        if (prev_errno != errno) {
                print_error_strs(
                                (const char *[]){
                                 "Cannot read directory files: ",
                                dir_path.s,
                                NULL
                                });
                return -1;
        }

        return 0;
}

bool
xtee_is_file_regular(const char *path)
{
        struct stat sb;

        if (stat(path, &sb) != 0) {
                return false;
        } else if (!S_ISREG(sb.st_mode)) {
                return false;
        }
        return true;
}

bool
xtee_is_file_directory(const char *path)
{
        struct stat sb;

        if (stat(path, &sb) != 0) {
                return false;
        } else if (!S_ISDIR(sb.st_mode)) {
                return false;
        }
        return true;
}

int
xtee_file_copy(const char *restrict dest, const char *restrict file_path)
{
        FILE *input_f;
        FILE *output_f;
        char buf[4096];

        input_f = fopen(file_path, "rb");
        output_f = fopen(dest, "wb");

        while (!feof(input_f)) {
                size_t n;

                n = fread(buf, 1, sizeof(buf), input_f);
                if (n) {
                        fwrite(buf, 1, n, output_f);
                }
        }
        fclose(input_f);
        fclose(output_f);
        return 0;
}
