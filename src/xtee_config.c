#include "xtee_config.h"
#include "xtee_error.h"
#include "xtee_utils.h"

#include <errno.h>

static struct XteeConfigurationVariable config_vars[] = {
        { XTEE_CONSTSTRING_STATIC_INIT("wine.default_version"),
                XTEE_CONSTSTRING_STATIC_INIT("System") },
        { XTEE_CONSTSTRING_STATIC_INIT("gui.show_unfinished_app_dialog"),
                XTEE_CONSTSTRING_STATIC_INIT("true") },
        { XTEE_CONSTSTRING_STATIC_INIT("gui.show_hidden_files"),
                XTEE_CONSTSTRING_STATIC_INIT("true") },
};

size_t config_vars_len = XTEE_ARRAY_LEN(config_vars);

static XteeString config_file_path = { NULL, 0 };
XteeConfigurationEntryList config_entries = { 0 };

/* Returned string is malloc'ed. Don't forget to free it! */
static XteeString
xtee_config_get_file_path(XteeConstString xtee_dir_path)
{
        const char conf_file_name[] = "config";
        XteeString file_path_xteestr;
        size_t conf_file_name_len = XTEE_STRINGLIT_LEN(conf_file_name);
        size_t xtee_dir_path_len;
        size_t file_path_len;
        char *file_path;

        xtee_dir_path_len = xtee_dir_path.n;
        file_path_len = xtee_dir_path_len + conf_file_name_len;
        file_path = malloc_or_exit(file_path_len + 1);
        memcpy(file_path, xtee_dir_path.s, xtee_dir_path_len);
        memcpy(file_path + xtee_dir_path_len, conf_file_name,
                        conf_file_name_len);
        file_path[file_path_len] = '\0';
        file_path_xteestr = move_string_to_XteeString(file_path);

        return file_path_xteestr;
}


static const char *config_file_format_error = "Configuration file formatting "
                "error.";
static const char *config_file_read_error = "Configuration file read error.";

static XteeConfigurationEntry *
xtee_config_get_entry(const char *name)
{
        size_t name_len;

        name_len = strlen(name);
        for (size_t i = 0; i < config_entries.n; ++i) {
                XteeConfigurationEntry *entry;
                size_t entry_name_len;

                entry = config_entries.entries + i;
                entry_name_len = strlen(entry->name);

                if (entry_name_len == name_len
                                && memcmp(entry->name, name, name_len) == 0) {
                        return entry;
                }
        }
        return NULL;
}

int
xtee_config_read_variable_line(char *name, char *value, size_t max_name_len,
                size_t max_value_len, FILE *file)
{
        int c;
        int is_end = 0, has_found_semi_colon = 0;

        /* Skip spaces if there are any */
        while (isspace(c = fgetc(file)));

        if(c == EOF) {
                if (feof(file)) {
                        /* End of file reached, just return. */
                        return 0;
                } else if(ferror(file)) {
                        print_error(config_file_read_error);
                        return -1;
                }
        }
        name[0] = (char)c;

        for (size_t i = 1; i < max_name_len - 1 && !is_end; ++i) {
                c = fgetc(file);
                if (c == '\n') {
                        print_error(config_file_format_error);
                } else if (c == EOF) {
                        print_error("Configuration variable read error.");
                        return -1;
                } else if (isspace(c)) {
                        name[i] = '\0';
                        is_end = 1;
                } else if (c == ':') {
                        has_found_semi_colon = 1;
                        name[i] = '\0';
                        is_end = 1;
                } else {
                        name[i] = (char)c;
                }
        }

        if (!is_end) {
                print_error("Configuration variable name too large.");
                return -1;
        }

        /* Reset is_end for next loop. */
        is_end = 0;

        /* Read until ':' character if not reached */
        if (!has_found_semi_colon) {
                while ((c = fgetc(file)) != ':') {
                        if (!(isspace(c))) {
                                print_error(config_file_format_error);
                                return -1;
                        }
                }
        }

        /* Skip space characters before variable value */
        while (isspace((c = fgetc(file))));

        if (c == EOF) {
                if (ferror(file)) {
                        print_error(config_file_read_error);
                } else {
                        print_error(config_file_format_error);
                }
                        return -1;
        }

        value[0] = (char)c;
        for (size_t i = 1; i < max_value_len - 1 && !is_end; ++i) {
                c = fgetc(file);
                if (c == '\n') {
                        value[i] = '\0';
                        is_end = 1;
                } else if (c == EOF) {
                        print_error("Configuration variable read error.");
                        return -1;
                } else {
                        value[i] = (char)c;
                }
        }

        if (!is_end) {
                print_error("Configuration variable value too large.");
                return -1;
        }

        return 0;
}

/* Returns a non-zero value on error.
 * If 0 is returned, check if EOF is reached. */
static int
xtee_config_read_file_value(char *var_name, char *var_value,
                FILE *config_file)
{
        if (xtee_config_read_variable_line(var_name, var_value,
                                XTEE_VAR_NAME_MAX_LEN,
                                XTEE_VAR_VALUE_MAX_LEN,
                                config_file) != 0) {
                return -1;
        }

        if (feof(config_file)) {
                return 0;
        }

        if (xtee_config_set_entry_value(var_name, var_value) != 0) {
                print_warning(var_name);
                print_warning("Unknown configuration variable.");
        }

        return 0;
}

static int
xtee_config_read_file(void)
{
        FILE *config_file;
        char var_name[256], var_value[500];
        int is_end = 0;
        int rtrn_val = -1;

        if (!(config_file = fopen(config_file_path.s, "r"))) {
                if(errno == ENOENT) {
                        print_info("Configuration file does not exist.");
                        return 0;
                } else {
                        print_error(config_file_read_error);
                        return -1;
                }
        }

        while (!is_end) {
                if (xtee_config_read_file_value(var_name, var_value,
                                        config_file) != 0) {
                        goto error_rtrn;
                }

                if (feof(config_file)) {
                        is_end = 1;
                }
        }

        rtrn_val = 0;
error_rtrn:
        fclose(config_file);
        return rtrn_val;
}

int
xtee_write_config_entry(FILE *f, const XteeConfigurationEntry entry)
{
        const char *write_error_str = "Cannot write configuration entry.";
        size_t write_len;

        write_len = strlen(entry.name);
        if (fwrite(entry.name, 1, write_len, f) != write_len) {
                print_error(write_error_str);
                return -1;
        }

        if (fwrite(": ", 1, 2, f) != 2) {
                print_error(write_error_str);
                return -1;
        }

        write_len = strlen(entry.val);
        if (fwrite(entry.val, 1, write_len, f) != write_len
                        || fputc('\n', f) != '\n') {
                print_error(write_error_str);
                return -1;
        }
        return 0;
}

static int
xtee_config_write_file(void)
{
        FILE *config_file;
        size_t i;

        if (!(config_file = fopen(config_file_path.s, "wb"))) {
                print_error("Cannot write configuration file.");
                return -1;
        }

        for (i = 0; i < config_entries.n; ++i) {
                if (xtee_write_config_entry(config_file,
                                        config_entries.entries[i]) != 0) {
                        fclose(config_file);
                        return -1;
                }
        }
        fclose(config_file);
        return 0;
}

int
xtee_config_init(XteeConstString xtee_dir_path)
{
        size_t i;

        config_file_path = xtee_config_get_file_path(xtee_dir_path);
        config_entries.n = XTEE_ARRAY_LEN(config_vars);
        config_entries.entries = malloc_or_exit(
                        config_entries.n * sizeof(XteeConfigurationEntry));

        for (i = 0; i < config_entries.n; ++i) {
                config_entries.entries[i].name = config_vars[i].name.s;
                config_entries.entries[i].val = NULL;
        }

        if (xtee_config_read_file() != 0) {
                return -1;
        }

        for (i = 0; i < config_entries.n; ++i) {
                if (!config_entries.entries[i].val) {
                        char *new_val;

                        new_val = xtee_strdup_or_exit(
                                        config_vars[i].default_val.s);
                        config_entries.entries[i].val = new_val;
                }
        }

        if (xtee_config_write_file() != 0) {
                return -1;
        }
        return 0;
}

void
xtee_config_free(void)
{
        for (size_t i = 0; i < config_entries.n; ++i) {
                free(config_entries.entries[i].val);
        }
        free_XteeString(config_file_path);
        free(config_entries.entries);
}

char *
xtee_config_get_entry_value(const char *name)
{
        XteeConfigurationEntry *entry;

        if (!(entry = xtee_config_get_entry(name))) {
                return NULL;
        }

        return entry->val;
}

int
xtee_config_set_entry_value(const char *restrict name,
                const char *restrict val)
{
        XteeConfigurationEntry *entry;
        char *val_buf;
        size_t val_len;

        val_len = strlen(val);
        if (!(entry = xtee_config_get_entry(name))) {
                return -1;
        }
        val_buf = malloc_or_exit(val_len + 1);
        memcpy(val_buf, val, val_len);
        val_buf[val_len] = '\0';

        free(entry->val);
        entry->val = val_buf;
        return 0;
}

void
xtee_config_set_entry_value_and_write(const char *restrict name,
                const char *restrict val)
{
        int r;

        r = xtee_config_set_entry_value(name, val);
        if (r != 0) {
                print_error("Cannot find configuration entry.");
        } else if((xtee_config_write_file()) != 0) {
                /* Error message is already printed */
                exit(-1);
        }
}
