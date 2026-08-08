#include "xtee_prefix.h"
#include "xtee_config.h"
#include "xtee_error.h"
#include "xtee_file.h"
#include "xtee_paths.h"
#include "xtee_utils.h"
#include "xtee_wine.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

static const char *prefixes_data_dir_end = "prefixes/";
static XteeString prefixes_data_dir_path = { 0 };

extern inline char *xtee_prefix_get_arch_string(enum XteePrefixArch arch);

enum XteePrefixArch
xtee_prefix_get_arch_enum_value(const char *s)
{
        if (xtee_strcmp_bool(s, XTEE_PREFIX_ARCH_WIN32_STR)) {
                return XTEE_PREFIX_ARCH_WIN32;
        } else if (xtee_strcmp_bool(s, XTEE_PREFIX_ARCH_WIN64_STR)) {
                return XTEE_PREFIX_ARCH_WIN64;
        } else if (xtee_strcmp_bool(s, XTEE_PREFIX_ARCH_WOW64_STR)) {
                return XTEE_PREFIX_ARCH_WOW64;
        }
        return XTEE_PREFIX_ARCH_WRONG;
}

/* Returned XteeString struct must be freed */
XteeString
get_xteetotaller_prefixes_dir_path(void)
{
        XteeConstString xtee_dir_path;
        XteeString full_path;

        xtee_dir_path = get_xteetotaller_dir_path();
        full_path = XteeString_merge_XteeConstString_charstring(
                        xtee_dir_path, prefixes_data_dir_end);
        return full_path;
}

void
xtee_prefixes_init(void)
{
        prefixes_data_dir_path = get_xteetotaller_prefixes_dir_path();
}

void
xtee_free_prefix_struct(XteePrefix *prefix)
{
        XteeString_free(prefix->id);
        XteeString_free(prefix->wineprefix_dir);
        XteeString_free(prefix->wine_version);

        for (size_t i = 0; i < prefix->programs_n; ++i) {
                XteeString_free(prefix->programs_ids[i]);
        }
        free(prefix->programs_ids);
}

void
xtee_prefixes_free(void)
{
        XteeString_free(prefixes_data_dir_path);
}

void
init_XteePrefix(XteePrefix *prefix)
{
        prefix->id.s = NULL;
        prefix->id.n = 0;
        prefix->wineprefix_dir.s = NULL;
        prefix->wineprefix_dir.n = 0;
        prefix->wine_version.s = NULL;
        prefix->wine_version.n = 0;
        prefix->programs_ids = NULL;
        prefix->programs_n = 0;
        /* Temporary placeholder value, ignore it for now */
        prefix->arch = XTEE_PREFIX_ARCH_WOW64;
}

void
free_XteePrefixIdBlock(XteePrefixIdBlock *block)
{
        for (size_t i = 0; i < block->n; ++i) {
                XteeString_free(block->data[i]);
        }
        free(block->data);
        block->n = 0;
}

static XteeString
get_prefix_config_file_path(const char *prefix_data_dir_path)
{
        return XteeString_merge_char_strings(prefix_data_dir_path,
                        "config");
}

static int
write_prefix_config_file(XteePrefix *prefix, const char *prefix_data_dir_path)
{
        XteeConfigurationEntry conf_entries[] = {
                { "wineprefix_dir", prefix->wineprefix_dir.s },
                { "wine_version", prefix->wine_version.s },
                { "arch", xtee_prefix_get_arch_string(prefix->arch) },
                { "creation_year", prefix->creation_date.year },
                { "creation_month", prefix->creation_date.month },
                { "creation_day", prefix->creation_date.day },
                { "isolate_home", xtee_bool_to_string(prefix->isolate_home) },
                { "disable_winemenubuilder",
                        xtee_bool_to_string(prefix->disable_winemenubuilder) },
        };
        const size_t conf_entries_len = XTEE_ARRAY_LEN(conf_entries);
        XteeString prefix_config_path;
        FILE *f;

        prefix_config_path = get_prefix_config_file_path(prefix_data_dir_path);
        f = fopen(prefix_config_path.s, "wb");
        XteeString_free(prefix_config_path);
        if (!f) {
                print_error("Cannot write to prefix config file");
                return -1;
        }

        for (size_t i = 0; i < conf_entries_len; ++i) {
                if (xtee_write_config_entry(f, conf_entries[i]) != 0) {
                        fclose(f);
                        return -1;
                }
        }
        fclose(f);

        return 0;
}

static XteeString
get_prefix_data_dir(const char *id)
{
        XteeString prefix_data_dir;
        size_t total_len;

        /* 1 byte for directory separator */
        total_len = prefixes_data_dir_path.n + strlen(id) + 1;
        prefix_data_dir.s = malloc_or_exit(total_len + 1);
        strcpy(prefix_data_dir.s, prefixes_data_dir_path.s);
        strcat(prefix_data_dir.s, id);
        prefix_data_dir.s[total_len - 1] = '/';
        prefix_data_dir.s[total_len] = '\0';
        prefix_data_dir.n = total_len;
        return prefix_data_dir;
}

static XteeString
get_prefix_data_programs_dir(XteeConstString prefix_data_dir)
{
        const char *programs_dir_suffix = "programs/";

        return XteeString_merge_XteeConstString_charstring(prefix_data_dir,
                        programs_dir_suffix);
}

enum XteeNewPrefixCreateStatus
create_new_prefix(const char *id, const char *dir, const char *wine_version,
                enum XteePrefixArch arch, bool isolate_home)
{
        XteePrefix new_prefix = { 0 };
        XteeString full_dir_path, programs_dir_path;
        int mkdir_ret;

        init_XteePrefix(&new_prefix);
        new_prefix.arch = arch;

        if (arch != XTEE_PREFIX_ARCH_WIN32
                        && arch != XTEE_PREFIX_ARCH_WIN64
                        && arch != XTEE_PREFIX_ARCH_WOW64) {
                fatal_error("Wrong architecture specified during prefix "
                                "creation.");
        }

        new_prefix.id = XteeString_create_from_charstring(id);
        full_dir_path = get_prefix_data_dir(id);

        if (mkdir(full_dir_path.s, XTEE_NEW_DIR_MODE) != 0) {
                XteeString_free(new_prefix.id);
                XteeString_free(full_dir_path);
                if (errno == EEXIST) {
                        return XTEE_NEW_PREFIX_CREATE_ID_EXIST;
                }
                return XTEE_NEW_PREFIX_CREATE_DIR_ERROR;
        }
        programs_dir_path = get_prefix_data_programs_dir(
                        XteeString_to_XteeConstString(full_dir_path));
        mkdir_ret = mkdir(programs_dir_path.s, XTEE_NEW_DIR_MODE);
        XteeString_free(programs_dir_path);
        if (mkdir_ret != 0) {
                rmdir(full_dir_path.s);
                XteeString_free(new_prefix.id);
                XteeString_free(full_dir_path);
                return XTEE_NEW_PREFIX_CREATE_DIR_ERROR;
        }

        new_prefix.wineprefix_dir = XteeString_create_from_charstring(dir);
        new_prefix.wine_version = XteeString_create_from_charstring(
                        wine_version);
        new_prefix.isolate_home = isolate_home;

        xtee_date_set_to_current_day(&new_prefix.creation_date);

        if (write_prefix_config_file(&new_prefix, full_dir_path.s) != 0) {
                XteeString_free(full_dir_path);
                xtee_free_prefix_struct(&new_prefix);
                return XTEE_NEW_PREFIX_CREATE_WRITE_ERROR;
        }
        xtee_free_prefix_struct(&new_prefix);
        XteeString_free(full_dir_path);

        return XTEE_NEW_PREFIX_CREATE_OK;
}

static void
print_multiple_config_fields_instances(const char *field_name)
{
        char *msg;
        char warning_suffix[] = " is set more than once - ignoring all "
                "instances after the first one.";

        msg = malloc_or_exit(strlen(field_name) + sizeof(warning_suffix));
        msg[0] = '\0';
        strcpy(msg, field_name);
        strcat(msg, warning_suffix);
        print_warning(msg);
        free(msg);
}

static bool
is_prefix_compatible_with_wine_version(const XteePrefix *prefix,
                const XteeWineVersionData *version_data)
{
        if (prefix->arch == XTEE_PREFIX_ARCH_WIN32) {
                return version_data->arch == XTEE_WINE_VERSION_ARCH_X86
                        || version_data->arch == XTEE_WINE_VERSION_ARCH_AMD64;
        } else if (prefix->arch == XTEE_PREFIX_ARCH_WIN64) {
                return version_data->arch == XTEE_WINE_VERSION_ARCH_AMD64;
        } else if (prefix->arch == XTEE_PREFIX_ARCH_WOW64) {
                return version_data->arch == XTEE_WINE_VERSION_ARCH_WOW64;
        }
        return false;
}

enum XteeLoadPrefixStatus
xtee_prefix_load(XteePrefix *prefix, const char *id)
{
        char var_name[XTEE_PREFIX_VAR_NAME_MAX_LEN];
        char var_value[XTEE_PREFIX_VAR_VALUE_MAX_LEN];
        struct stat statbuf;
        XteeDate tmp_date;
        XteeString prefix_data_dir, config_file_path;
        XteeWineVersionData *version_data;
        FILE *config_file;
        bool is_wineprefix_dir_set = false;
        bool is_wine_version_set = false;
        bool is_creation_year_set = false;
        bool is_creation_month_set = false;
        bool is_creation_day_set = false;
        bool is_creation_date_set = false;
        bool is_isolate_home_set = false;
        bool is_disable_winemenubuilder_set = false;
        int r = 0;

        /* Set all fields to 0 to allow it to free the structure on error. */
        memset(prefix, 0, sizeof(XteePrefix));
        prefix_data_dir = get_prefix_data_dir(id);
        config_file_path = get_prefix_config_file_path(prefix_data_dir.s);
        if (!(config_file = fopen(config_file_path.s, "r"))) {
                XteeString_free(config_file_path);
                XteeString_free(prefix_data_dir);
                switch (errno) {
                        case EACCES:
                                return XTEE_LOAD_PREFIX_CONFIG_ACCESS_ERR;
                        case ENOENT:
                                return XTEE_LOAD_PREFIX_NO_CONFIG;
                        default:
                                return XTEE_LOAD_PREFIX_CONFIG_READ_ERR;
                }
        }
        XteeString_free(config_file_path);
        XteeString_free(prefix_data_dir);

        while ((r = xtee_config_read_variable_line(var_name, var_value,
                                        sizeof(var_name), sizeof(var_value),
                                        config_file))
                        == 0) {
                if (feof(config_file)) {
                        break;
                }

                if (xtee_strcmp_bool(var_name, "wineprefix_dir")) {
                        if (is_wineprefix_dir_set) {
                                print_multiple_config_fields_instances(
                                                "wineprefix_dir");
                        } else {
                                prefix->wineprefix_dir =
                                        XteeString_create_from_charstring(
                                                        var_value);
                                is_wineprefix_dir_set = true;
                        }
                } else if (xtee_strcmp_bool(var_name, "wine_version")) {
                        if (is_wine_version_set) {
                                print_multiple_config_fields_instances(
                                                "wine_version");
                        } else {
                                prefix->wine_version =
                                        XteeString_create_from_charstring(
                                                        var_value);
                                is_wine_version_set = true;
                        }
                } else if (xtee_strcmp_bool(var_name, "creation_year")) {
                        if (is_creation_year_set) {
                                print_multiple_config_fields_instances(
                                                "creation_year");
                                continue;
                        }

                        if (strlen(var_value) > sizeof(tmp_date.year) - 1) {
                                print_warning("creation_year is too long, "
                                                "ignoring.");
                                continue;
                        }

                        if (!xtee_is_charstring_number(var_value)) {
                                print_warning("creation_year is not a "
                                                "number.");
                                continue;
                        }
                        strcpy(tmp_date.year, var_value);
                        is_creation_year_set = true;
                } else if (xtee_strcmp_bool(var_name, "creation_month")) {
                        if (is_creation_month_set) {
                                print_multiple_config_fields_instances(
                                                "creation_month");
                                continue;
                        }

                        if (strlen(var_value) > sizeof(tmp_date.month) - 1) {
                                print_warning("creation_month is too long, "
                                                "ignoring.");
                                continue;
                        }

                        if (!xtee_is_charstring_number(var_value)) {
                                print_warning("creation_month is not a "
                                                "number.");
                                continue;
                        }
                        strcpy(tmp_date.month, var_value);
                        is_creation_month_set = true;
                } else if (xtee_strcmp_bool(var_name, "creation_day")) {
                        if (is_creation_day_set) {
                                print_multiple_config_fields_instances(
                                                "creation_day");
                                continue;
                        }

                        if (strlen(var_value) > sizeof(tmp_date.day) - 1) {
                                print_warning("creation_day is too long, "
                                                "ignoring.");
                                continue;
                        }

                        if (!xtee_is_charstring_number(var_value)) {
                                print_warning("creation_day is not a number.");
                                continue;
                        }
                        strcpy(tmp_date.day, var_value);
                        is_creation_day_set = true;
                } else if (xtee_strcmp_bool(var_name, "arch")) {
                        prefix->arch = xtee_prefix_get_arch_enum_value(
                                        var_value);
                } else if (xtee_strcmp_bool(var_name, "isolate_home")) {
                        prefix->isolate_home = xtee_string_to_bool(var_value);
                        is_isolate_home_set = true;
                } else if(xtee_strcmp_bool(var_name,
                                        "disable_winemenubuilder")) {
                        prefix->disable_winemenubuilder =
                                xtee_string_to_bool(var_value);
                        is_disable_winemenubuilder_set = true;
                } else {
                        print_warning("Unknown var name in prefix "
                                        "configuration file.");
                }

        }
        fclose(config_file);

        is_creation_date_set = (is_creation_year_set && is_creation_month_set
                        && is_creation_day_set);
        if (is_creation_date_set) {
                prefix->creation_date = tmp_date;
        }

        if (r != 0) {
                xtee_free_prefix_struct(prefix);
                return XTEE_LOAD_PREFIX_CONFIG_READ_ERR;
        } else if (prefix->arch == XTEE_PREFIX_ARCH_WRONG) {
                xtee_free_prefix_struct(prefix);
                return XTEE_LOAD_PREFIX_WRONG_ARCH;
        }

        if (!is_wine_version_set) {
                print_warning("Wine version not set, defaulting to System.");
                prefix->wine_version = XteeString_create_from_charstring(
                                "System");
        }

        if (!(version_data = xtee_wine_get_version_data(
                                        prefix->wine_version.s))) {
                xtee_free_prefix_struct(prefix);
                return XTEE_LOAD_PREFIX_WINE_VERSION_NOTEXIST;
        } else if (!is_prefix_compatible_with_wine_version(prefix,
                                version_data)) {
                return XTEE_LOAD_PREFIX_WINE_VERSION_INCOMP_ARCH;
        }

        if (!is_wineprefix_dir_set) {
                xtee_free_prefix_struct(prefix);
                return XTEE_LOAD_PREFIX_CONFIG_MISSING_PREFIXDIR;
        }

        if (!is_creation_date_set) {
                /* Set it to UNIX epoch */
                strcpy(prefix->creation_date.year, "1970");
                strcpy(prefix->creation_date.month, "01");
                strcpy(prefix->creation_date.day, "01");
        }

        if (!is_isolate_home_set) {
                prefix->isolate_home = false;
        }

        if (!is_disable_winemenubuilder_set) {
                prefix->disable_winemenubuilder = false;
        }
        prefix->id = XteeString_create_from_charstring(id);

        if (xtee_prefix_reload_programs_list(prefix) != 0) {
                xtee_free_prefix_struct(prefix);
                return XTEE_LOAD_PREFIX_PROGRAMS_READ_ERR;
        }

        if (stat(prefix->wineprefix_dir.s, &statbuf) != 0
                        || !S_ISDIR(statbuf.st_mode)) {
                xtee_free_prefix_struct(prefix);
                return XTEE_LOAD_PREFIX_DIR_NOT_EXIST;
        }

        return XTEE_LOAD_PREFIX_OK;
}

int
xtee_prefix_save_config(XteePrefix *prefix)
{
        int r;
        XteeString prefix_data_dir;

        prefix_data_dir = get_prefix_data_dir(prefix->id.s);
        r = write_prefix_config_file(prefix, prefix_data_dir.s);
        XteeString_free(prefix_data_dir);
        return r;
}

static void
add_item_to_XteePrefixIdBlock(XteePrefixIdBlock *block,
                const char *restrict id)
{
        XteeString *tmp_ptr;

        tmp_ptr = realloc_or_exit(block->data,
                        (block->n + 1) * sizeof(XteeString));
        block->data = tmp_ptr;
        block->data[block->n] = XteeString_create_from_charstring(id);
        ++block->n;
}

/* The two functions below are not very efficient but since they are called
 * after a GUI interaction, it should not be very noticeable. */
int
get_prefixes_ids(XteePrefixIdBlock *block)
{
        struct dirent *dir_entry;
        DIR *prefixes_dir;
        int prev_errno;
        int r = -1;

        block->data = NULL;
        block->n = 0;

        if (!(prefixes_dir = opendir(prefixes_data_dir_path.s))) {
                print_error("Cannot read prefixes data directory.");
                return -1;
        }

        /* readdir returns NULL on both error and end of directory.
         * The only difference is that errno is not set.
         * This is why the previous value of errno is stored and compared. */
        prev_errno = errno;
        while ((dir_entry = readdir(prefixes_dir))) {
                struct stat statbuf;
                char *prefix_data_dir_path;
                size_t prefix_data_dir_path_len;
                int stat_rtrn;

                /* Skip current and parent directory entries */
                if ((xtee_strcmp_bool(".", dir_entry->d_name))
                                || xtee_strcmp_bool("..", dir_entry->d_name)) {
                        continue;
                }

                /* + 1 for the dir separator after prefix data directory */
                prefix_data_dir_path_len = prefixes_data_dir_path.n
                        + strlen(dir_entry->d_name);
                prefix_data_dir_path = malloc_or_exit(
                                prefix_data_dir_path_len + 1);
                prefix_data_dir_path[0] = '\0';
                strcat(prefix_data_dir_path, prefixes_data_dir_path.s);
                strcat(prefix_data_dir_path, dir_entry->d_name);

                stat_rtrn = stat(prefix_data_dir_path, &statbuf);
                free(prefix_data_dir_path);
                if (stat_rtrn != 0) {
                        print_error("Cannot use stat on prefix directory.");
                        goto err_return;
                }
                if ((statbuf.st_mode & S_IFMT) != S_IFDIR) {
                        print_warning("Non-directory file in prefixes data "
                                        "directory.");
                        continue;
                }

                add_item_to_XteePrefixIdBlock(block, dir_entry->d_name);
        }

        if (prev_errno != errno) {
                print_error("Cannot read prefixes directory entry.");
                closedir(prefixes_dir);
                return -1;
        }
        r = 0;
err_return:
        if (r != 0) {
                free_XteePrefixIdBlock(block);
        }
        closedir(prefixes_dir);
        return r;
}

int
get_prefix_programs_ids(XteeConstString prefix_id, XteePrefixIdBlock *block)
{
        XteeString prefix_data_dir_path, programs_dir_path;
        DIR *programs_dir;
        struct dirent *dir_entry;
        int prev_errno;
        int r = -1;

        block->data = NULL;
        block->n = 0;

        prefix_data_dir_path = get_prefix_data_dir(prefix_id.s);
        programs_dir_path = get_prefix_data_programs_dir(
                        XteeString_to_XteeConstString(prefix_data_dir_path));
        XteeString_free(prefix_data_dir_path);

        if (!(programs_dir = opendir(programs_dir_path.s))) {
                XteeString_free(programs_dir_path);
                print_error("Cannot read prefix programs directory.");
                return -1;
        }

        prev_errno = errno;
        while ((dir_entry = readdir(programs_dir))) {
                struct stat statbuf;
                char *program_path;
                size_t program_path_len;
                int stat_rtrn;

                if ((xtee_strcmp_bool(".", dir_entry->d_name))
                                || xtee_strcmp_bool("..", dir_entry->d_name)) {
                        continue;
                }
                program_path_len = programs_dir_path.n
                        + strlen(dir_entry->d_name);
                program_path = malloc_or_exit(program_path_len + 1);
                program_path[0] = '\0';
                strcat(program_path, programs_dir_path.s);
                strcat(program_path, dir_entry->d_name);

                stat_rtrn = stat(program_path, &statbuf);
                free(program_path);
                if (stat_rtrn != 0) {
                        print_error("Cannot use stat on programs directory.");
                        goto err_return;
                }
                if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
                        print_warning("Non-regular file in programs "
                                        "directory.");
                        continue;
                }

                add_item_to_XteePrefixIdBlock(block, dir_entry->d_name);
        }
        if (prev_errno != errno) {
                print_error("Cannot read program directory entry.");
                closedir(programs_dir);
                return -1;
        }
        r = 0;
err_return:
        XteeString_free(programs_dir_path);
        if (r != 0) {
                free_XteePrefixIdBlock(block);
        }
        closedir(programs_dir);
        return r;
}

static int
write_program_config_file(XteeProgram *program,
                FILE *f)
{
        XteeConfigurationEntry conf_entries[] = {
                { "creation_year", program->creation_date.year },
                { "creation_month", program->creation_date.month },
                { "creation_day", program->creation_date.day },
                { "exe_path", program->exe_path.s }
        };
        const size_t conf_entries_len = XTEE_ARRAY_LEN(conf_entries);

        for (size_t i = 0; i < conf_entries_len; ++i) {
                if (xtee_write_config_entry(f, conf_entries[i]) != 0) {
                        return -1;
                }
        }

        for (size_t i = 0; i < program->args_list.n; ++i) {
                XteeConfigurationEntry conf_entry = {
                        "cmd_line_arg", program->args_list.data[i].s
                };
                if (xtee_write_config_entry(f, conf_entry) != 0) {
                        return -1;
                }
        }

        return 0;
}

XteeString
xtee_prefix_get_program_config_path(XteePrefix *prefix,
                XteeProgram *program)
{
        XteeString prefix_data_dir_path;
        XteeString programs_dir_path;
        XteeString program_config_path;

        prefix_data_dir_path = get_prefix_data_dir(prefix->id.s);
        programs_dir_path = get_prefix_data_programs_dir(
                        XteeString_to_XteeConstString(prefix_data_dir_path));
        XteeString_free(prefix_data_dir_path);
        program_config_path.n = programs_dir_path.n + program->id.n;
        program_config_path.s = malloc(program_config_path.n + 1);
        strcpy(program_config_path.s, programs_dir_path.s);
        strcat(program_config_path.s, program->id.s);
        XteeString_free(programs_dir_path);
        return program_config_path;
}

enum
XteeAddProgramStatus add_program_to_prefix(XteePrefix *prefix,
                XteeProgram *program)
{
        const char *error_str = "Cannot write to program config file.";
        XteeString new_program_config_path;
        XteeString *tmp_ptr;
        FILE *f;
        int fd;
        int r;

        new_program_config_path = xtee_prefix_get_program_config_path(prefix,
                        program);

        /* We use open() at first to write to the file only if it does not
         * exist with the flag O_EXCL.
         * This avoids TOCTOU race conditions.
         * After that, it is converted to a FILE pointer with fdopen(). */
        fd = open(new_program_config_path.s, O_WRONLY | O_CREAT | O_EXCL,
                        XTEE_NEW_FILE_MODE);
        XteeString_free(new_program_config_path);
        if (fd == -1) {
                print_error(error_str);
                if (errno == EEXIST) {
                        return XTEE_ADD_PROGRAM_ID_EXIST;
                }
                return XTEE_ADD_PROGRAM_WRITE_ERROR;
        }
        if (!(f = fdopen(fd, "w"))) {
                print_error(error_str);
                close(fd);
                return XTEE_ADD_PROGRAM_WRITE_ERROR;
        }

        r = write_program_config_file(program, f);
        fclose(f);
        if (r != 0) {
                return XTEE_ADD_PROGRAM_WRITE_ERROR;
        }
        tmp_ptr = realloc_or_exit(prefix->programs_ids,
                        ++prefix->programs_n * sizeof(XteeString));
        prefix->programs_ids = tmp_ptr;
        prefix->programs_ids[prefix->programs_n - 1] = XteeString_duplicate(
                        program->id);
        return XTEE_ADD_PROGRAM_0K;
}

int
xtee_prefix_reload_programs_list(XteePrefix *prefix)
{
        XteePrefixIdBlock ids_block;

        if (get_prefix_programs_ids(XteeString_to_XteeConstString(prefix->id),
                                &ids_block) != 0) {
                return -1;
        }

        for (size_t i = 0; i < prefix->programs_n; ++i) {
                XteeString_free(prefix->programs_ids[i]);
        }
        free(prefix->programs_ids);
        /* Do not free allocation as we move it to XteePrefix field. */
        prefix->programs_ids = ids_block.data;
        prefix->programs_n = ids_block.n;

        return 0;
}

enum XteePrefixArch
get_prefix_arch_from_str(const char *s)
{
        if (xtee_strcmp_bool(s, "win32")) {
                return XTEE_PREFIX_ARCH_WIN32;
        } else if (xtee_strcmp_bool(s, "win64")) {
                return XTEE_PREFIX_ARCH_WIN64;
        } else if (xtee_strcmp_bool(s, "wow64")) {
                return XTEE_PREFIX_ARCH_WOW64;
        }

        return XTEE_PREFIX_ARCH_WRONG;
}

void
xtee_prefix_set_WINEDLLOVERRIDES_env(const XteePrefix *prefix)
{
        if (prefix->disable_winemenubuilder) {
                setenv("WINEDLLOVERRIDES", "winemenubuilder.exe=", 1);
        }
        return;
}

static int
read_program_config_file(XteeProgram *restrict program,
                const char *restrict program_config_path,
                enum XteeGetProgramDataStatus *status)
{
        char var_name[XTEE_PREFIX_VAR_NAME_MAX_LEN];
        char var_value[XTEE_PREFIX_VAR_VALUE_MAX_LEN];
        XteeDate tmp_date;
        FILE *f;
        bool is_creation_year_set = false;
        bool is_creation_month_set = false;
        bool is_creation_day_set = false;
        bool is_creation_date_set = false;
        bool is_exe_path_set = false;
        int r;

        if (!(f = fopen(program_config_path, "r"))) {
                if (errno == EACCES) {
                        *status = XTEE_GET_PROGRAM_DATA_NOT_EXIST;
                } else {
                        *status = XTEE_GET_PROGRAM_DATA_READ_ERROR;
                }
                return -1;
        }

        while ((r = xtee_config_read_variable_line(var_name, var_value,
                                        sizeof(var_name), sizeof(var_value),
                                        f))
                        == 0) {
                if (feof(f)) {
                        break;
                }

                if (xtee_strcmp_bool(var_name, "creation_year")) {
                        if (is_creation_year_set) {
                                print_multiple_config_fields_instances(
                                                "creation_year");
                                continue;
                        }

                        if (strlen(var_value) > sizeof(tmp_date.year) - 1) {
                                print_warning("creation_year is too long, "
                                                "ignoring.");
                                continue;
                        }

                        if (!xtee_is_charstring_number(var_value)) {
                                print_warning("creation_year is not a "
                                                "number.");
                                continue;
                        }
                        strcpy(tmp_date.year, var_value);
                        is_creation_year_set = true;
                } else if (xtee_strcmp_bool(var_name, "creation_month")) {
                        if (is_creation_month_set) {
                                print_multiple_config_fields_instances(
                                                "creation_month");
                                continue;
                        }

                        if (strlen(var_value) > sizeof(tmp_date.month) - 1) {
                                print_warning("creation_month is too long, "
                                                "ignoring.");
                                continue;
                        }

                        if (!xtee_is_charstring_number(var_value)) {
                                print_warning("creation_month is not a "
                                                "number.");
                                continue;
                        }
                        strcpy(tmp_date.month, var_value);
                        is_creation_month_set = true;
                } else if (xtee_strcmp_bool(var_name, "creation_day")) {
                        if (is_creation_day_set) {
                                print_multiple_config_fields_instances(
                                                "creation_day");
                                continue;
                        }

                        if (strlen(var_value) > sizeof(tmp_date.day) - 1) {
                                print_warning("creation_day is too long, "
                                                "ignoring.");
                                continue;
                        }

                        if (!xtee_is_charstring_number(var_value)) {
                                print_warning("creation_day is not a number.");
                                continue;
                        }
                        strcpy(tmp_date.day, var_value);
                        is_creation_day_set = true;
                } else if (xtee_strcmp_bool(var_name, "exe_path")) {
                        if (is_exe_path_set) {
                                print_multiple_config_fields_instances(
                                                "exe_path");
                        } else {
                                program->exe_path =
                                        XteeString_create_from_charstring(
                                                        var_value);
                                is_exe_path_set = true;
                        }
                } else if (xtee_strcmp_bool(var_name, "cmd_line_arg")) {
                        xtee_program_add_argument(program, var_value);
                } else {
                        print_warning("Unknown var name in program "
                                        "configuration file.");
                }

        }
        fclose(f);

        if (!is_exe_path_set) {
                *status = XTEE_GET_PROGRAM_DATA_NO_EXE_PATH;
                return -1;
        }

        if (r != 0) {
                *status = XTEE_GET_PROGRAM_DATA_READ_ERROR;
                XteeString_free(program->id);
                return -1;
        }
        is_creation_date_set = (is_creation_year_set && is_creation_month_set
                        && is_creation_day_set);
        if (is_creation_date_set) {
                program->creation_date = tmp_date;
        } else {
                /* Set it to UNIX epoch */
                strcpy(program->creation_date.year, "1970");
                strcpy(program->creation_date.month, "01");
                strcpy(program->creation_date.day, "01");
        }
        *status = XTEE_GET_PROGRAM_DATA_OK;

        return 0;
}

XteeProgram *
xtee_prefix_get_program_data(XteePrefix *prefix, const char *program_id,
                enum XteeGetProgramDataStatus *status)
{
        const char *program_config_strs[] = { prefixes_data_dir_path.s,
                "/", prefix->id.s, "/programs/", program_id, NULL };
        char *program_config_path;
        XteeProgram *program_data;

        program_config_path = xtee_string_merge_char_strings_block(
                        program_config_strs);
        program_data = calloc_or_exit(1, sizeof(XteeProgram));
        if (read_program_config_file(program_data, program_config_path,
                                status) != 0) {
                free(program_config_path);
                free(program_data);
                return NULL;
        }
        free(program_config_path);
        program_data->id = XteeString_create_from_charstring(program_id);
        return program_data;
}

int
xtee_prefix_save_program(XteePrefix *prefix, XteeProgram *program)
{
        XteeString program_config_path;
        FILE *f;
        int r;

        program_config_path = xtee_prefix_get_program_config_path(prefix,
                        program);
        if(!(f = fopen(program_config_path.s, "wb"))) {
                return -1;
        }
        XteeString_free(program_config_path);
        r = write_program_config_file(program, f);
        fclose(f);
        return r;
}
