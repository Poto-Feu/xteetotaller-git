#include "xtee_gui.h"
#include "xtee_error.h"
#include "xtee_file.h"
#include "xtee_gui_new_prefix.h"
#include "xtee_gui_new_program.h"
#include "xtee_gui_preferences.h"
#include "xtee_gui_prefix_options.h"
#include "xtee_gui_program_options.h"
#include "xtee_paths.h"
#include "xtee_state.h"
#include "xtee_system.h"
#include "xtee_utils.h"
#include "xteetotaller.h"

#include <Xm/AtomMgr.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/Protocols.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>
#include <Xm/ToggleBG.h>

#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define XTEE_GUI_START_WIDTH 750
#define XTEE_GUI_START_HEIGHT 525

typedef struct XteePushButtonData {
        const char *id;
        char *label;
        Widget *w;
} XteePushButtonData;

typedef struct XteeWorkAreaStruct {
        Widget parent_form;
        Widget programs_form;
        Widget programs_frame;
        Widget programs_frame_title;
        Widget programs_list;
        Widget programs_buttons;
        Widget start_program_btn;
        Widget program_options_btn;
        Widget remove_program_btn;
        Widget prefix_frame;
        Widget prefix_frame_title;
        Widget prefix_form;
        Widget prefix_info_label;
} XteeWorkAreaStruct;

typedef struct XteeGuiPlaceholderDialog {
        Widget dialog_form;
        Widget label;
        Widget ok_btn;
        Widget cancel_btn;
} XteeGuiPlaceholderDialog;

typedef struct XteeGuiNewProgramData {
        XteeGuiBasicFormDialog dialog;
} XteeGuiNewProgramData;

typedef struct XteeGuiMainData {
        XteeWorkAreaStruct work_area_struct;
        XteeGuiBasicFormDialog prefs_dialog;
        XteeGuiPlaceholderDialog prefix_options_dialog;
        XtAppContext app_context;
        Widget toplevel;
        Widget main_win;
        Widget menubar;
        Widget file_menu;
        Widget edit_menu;
        Widget prefix_menu;
        Widget wine_menu;
        Widget help_menu;
        Widget message_area_label;
} XteeGuiMainData;

typedef struct XteeGuiData {
        XteeGuiBasicFormDialog program_options_dialog;
        XteeGuiBasicFormDialog new_prefix_dialog;
        XteeGuiBasicFormDialog prefs_dialog;
        XteeGuiBasicFormDialog prefix_options_dialog;
        XteeGuiBasicFormDialog new_program_dialog;
} XteeGuiData;

XteeGuiData xtee_gui_data_struct = { 0 };

static const char *prefix_button_id = "button_2";
static const char *wine_button_id = "button_3";

XmRenderTable xtee_gui_render_table;

static struct XteeGuiMainData main_gui_data = { 0 };

static XteePushButtonData programs_btns_data[] = {
        { "start_program_btn", "Start program",
                &main_gui_data.work_area_struct.start_program_btn },
        { "program_options_btn", "Program options",
                &main_gui_data.work_area_struct.program_options_btn },
        { "remove_program_btn", "Remove program",
                &main_gui_data.work_area_struct.remove_program_btn },
};

static const int start_program_btn_i = 0;
static const int program_options_btn_i = 1;
static const int remove_program_btn_i = 2;

static Widget main_working_dialog = NULL;

char *
XmString_to_char_str(XmString *xmstr)
{
        return (char *)XmStringUnparse(*xmstr, XmFONTLIST_DEFAULT_TAG,
                        XmCHARSET_TEXT, XmCHARSET_TEXT,
                        NULL, 0, XmOUTPUT_ALL);
}

static void
set_and_show_main_working_dialog(char *title, char *msg)
{
        XmString title_str, msg_str;
        if (!main_working_dialog) {
                Arg args[5];
                Cardinal n = 0;

                XtSetArg(args[n], XmNdialogStyle,
                                XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
                main_working_dialog = XmCreateWorkingDialog(
                                main_gui_data.main_win, "main_working_dialog",
                                args, n);
                XtUnmanageChild(XtNameToWidget(main_working_dialog, "OK"));
                xtee_gui_hide_dialog_cancel_button(main_working_dialog);
                xtee_gui_hide_dialog_help_button(main_working_dialog);
        }
        title_str = XmStringCreateLocalized(title);
        msg_str = XmStringCreateLocalized(msg);
        XtVaSetValues(main_working_dialog,
                        XmNdialogTitle, title_str,
                        XmNmessageString, msg_str,
                        NULL);
        XmStringFree(title_str);
        XmStringFree(msg_str);
        XtManageChild(main_working_dialog);
}

static void
hide_main_working_dialog(void)
{
        XtUnmanageChild(main_working_dialog);
}

void
xtee_gui_set_motif_label_string(Widget w, char *s)
{
        XmString xmstr;

        xmstr = XmStringCreateLocalized(s);
        XtVaSetValues(w, XmNlabelString, xmstr, NULL);
        XmStringFree(xmstr);
}

static
XTEE_XT_CALLBACK_FUNC(destroy_dialog_cb)
{
        (void)client_data;
        (void)call_data;
        XtDestroyWidget(XtParent(w));
}

/* args array must have a minimum size of 10 just to be safe. */
static void
set_message_dialog_args(Arg args[10], unsigned int *n,
                XmString dialog_title, XmString dialog_text)
{
        *n = 0;
        XtSetArg(args[*n], XmNdialogTitle, dialog_title); (*n)++;
        XtSetArg(args[*n], XmNmessageString, dialog_text); (*n)++;
        XtSetArg(args[*n], XmNdialogStyle,
                        XmDIALOG_PRIMARY_APPLICATION_MODAL); (*n)++;
        XtSetArg(args[*n], XmNnoResize, True); ;
        XtSetArg(args[*n], XmNresizePolicy, XmRESIZE_NONE); (*n)++;
        XtSetArg(args[*n], XmNdefaultButton, XmDIALOG_OK_BUTTON); (*n)++;
}

/* functions name prefixed with "gui_" to distinguish them from xtee_error
 * functions */
static void
gui_print_message_dialog(char *msg, Widget parent, unsigned char type)
{
        Arg args[10];
        Widget dialog;
        XmString dialog_title, dialog_text;
        unsigned int n = 0;

        if (type != XmDIALOG_MESSAGE
                        && type != XmDIALOG_ERROR
                        && type != XmDIALOG_WARNING
                        && type != XmDIALOG_INFORMATION) {
                type = XmDIALOG_MESSAGE;
        }

        switch (type) {
                case XmDIALOG_ERROR:
                        dialog_title = XmStringCreateLocalized("Error");
                        break;
                case XmDIALOG_WARNING:
                        dialog_title = XmStringCreateLocalized("Warning");
                        break;
                case XmDIALOG_INFORMATION:
                        dialog_title = XmStringCreateLocalized("Information");
                        break;
                default:
                        dialog_title = XmStringCreateLocalized("Message");
                        break;
        }
        dialog_text = XmStringCreateLocalized(msg);
        set_message_dialog_args(args, &n, dialog_title, dialog_text);
        switch (type) {
                case XmDIALOG_ERROR:
                        dialog = XmCreateErrorDialog(parent,
                                        "error_dialog", args, n);
                        break;
                case XmDIALOG_WARNING:
                        dialog = XmCreateWarningDialog(parent,
                                        "warning_dialog", args, n);
                        break;
                case XmDIALOG_INFORMATION:
                        dialog = XmCreateInformationDialog(parent,
                                        "information_dialog", args, n);
                        break;
                default:
                        dialog = XmCreateMessageDialog(parent,
                                        "message_dialog", args, n);
                        break;
        }
        XmStringFree(dialog_title);
        XmStringFree(dialog_text);
        xtee_gui_hide_dialog_cancel_button(dialog);
        xtee_gui_disable_dialog_help_button(dialog);
        XtAddCallback(dialog, XmNunmapCallback, destroy_dialog_cb, NULL);
        XtManageChild(dialog);
}

void
gui_print_error_dialog(char *msg, Widget parent)
{
        gui_print_message_dialog(msg, parent, XmDIALOG_ERROR);
}

void
gui_print_error_log_and_dialog(char *msg, Widget parent)
{
        print_error(msg);
        gui_print_error_dialog(msg, parent);
}

void
gui_print_warning_dialog(char *msg, Widget parent)
{
        gui_print_message_dialog(msg, parent, XmDIALOG_WARNING);
}

void
gui_print_info_dialog(char *msg, Widget parent)
{
        gui_print_message_dialog(msg, parent, XmDIALOG_INFORMATION);
}

void
xtee_gui_set_message_area_string(char *s)
{
        xtee_gui_set_motif_label_string(main_gui_data.message_area_label, s);
}

static void
disable_programs_btns(void)
{
        for (size_t i = 0; i < XTEE_ARRAY_LEN(programs_btns_data); ++i) {
                XtSetSensitive(*(programs_btns_data[i].w), False);
        }
}

static void
enable_programs_btns(void)
{
        for (size_t i = 0; i < XTEE_ARRAY_LEN(programs_btns_data); ++i) {
                XtSetSensitive(*(programs_btns_data[i].w), True);
        }
}

static void
reset_prefix_info_label(void)
{
        XteeWorkAreaStruct *wa_struct;
        char *no_prefix_opened_str = "No prefix opened.";

        wa_struct = &main_gui_data.work_area_struct;

        xtee_gui_set_motif_label_string(wa_struct->prefix_info_label,
                        no_prefix_opened_str);
}

static void
update_prefix_info_label(void)
{
        char id_field_str[] = "Id: ";
        char wineprefix_dir_field_str[] = "Wine prefix directory: ";
        char wine_version_field_str[] = "Wine version: ";
        char arch_field_str[] = "Architecture: ";
        char date_field_str[] = "Date created: ";
        const char *arch_str;
        char *source_str;
        XteePrefix *current_prefix;

        current_prefix = xtee_state_get_loaded_prefix();
        arch_str = xtee_prefix_get_arch_string(current_prefix->arch);
        source_str = malloc(sizeof(id_field_str) + current_prefix->id.n
                        + sizeof(wineprefix_dir_field_str)
                        + current_prefix->wineprefix_dir.n
                        + sizeof(wine_version_field_str)
                        + current_prefix->wine_version.n
                        + sizeof(arch_field_str)
                        + strlen(arch_str)
                        + sizeof(date_field_str)
                        + sizeof(current_prefix->creation_date.year)
                        + sizeof(current_prefix->creation_date.month)
                        + sizeof(current_prefix->creation_date.day) + 1);
        source_str[0] = '\0';
        strcat(source_str, id_field_str);
        strcat(source_str, current_prefix->id.s);
        strcat(source_str, "\n");
        strcat(source_str, wineprefix_dir_field_str);
        strcat(source_str, current_prefix->wineprefix_dir.s);
        strcat(source_str, "\n");
        strcat(source_str, wine_version_field_str);
        strcat(source_str, current_prefix->wine_version.s);
        strcat(source_str, "\n");
        strcat(source_str, arch_field_str);
        strcat(source_str, arch_str);
        strcat(source_str, "\n");
        strcat(source_str, date_field_str);
        strcat(source_str, current_prefix->creation_date.year);
        strcat(source_str, "-");
        strcat(source_str, current_prefix->creation_date.month);
        strcat(source_str, "-");
        strcat(source_str, current_prefix->creation_date.day);

        xtee_gui_set_motif_label_string(
                        main_gui_data.work_area_struct.prefix_info_label,
                        source_str);
        free(source_str);
}

int
refresh_program_list(void)
{
        XmStringTable str_list;
        XteePrefix *current_prefix;
        Widget programs_list;

        if (!xtee_state_is_prefix_loaded) {
                return 0;
        }

        programs_list = main_gui_data.work_area_struct.programs_list;
        current_prefix = xtee_state_get_loaded_prefix();
        XmListDeleteAllItems(programs_list);
        if (!current_prefix->programs_n) {
                return 0;
        }

        str_list = malloc_or_exit(
                        current_prefix->programs_n * sizeof(XmString));
        for (size_t i = 0; i < current_prefix->programs_n; ++i) {
                str_list[i] = XmStringCreateLocalized(
                                current_prefix->programs_ids[i].s);
        }
        XtVaSetValues(programs_list,
                        XmNitemCount, (int)current_prefix->programs_n,
                        XmNitems, str_list,
                        NULL);
        for (size_t i = 0; i < current_prefix->programs_n; ++i) {
                XmStringFree(str_list[i]);
        }
        free(str_list);

        return 0;
}

static void
reset_main_win_state(void)
{
        Widget tmp_w;

        /* Disable Prefix and Wine menu */
        tmp_w = XtNameToWidget(main_gui_data.menubar, prefix_button_id);
        XtSetSensitive(tmp_w, False);
        tmp_w = XtNameToWidget(main_gui_data.menubar, wine_button_id);
        XtSetSensitive(tmp_w, False);
        disable_programs_btns();
        reset_prefix_info_label();
}

static void
refresh_prefix_gui(void)
{
        Widget tmp_w;

        /* Enable Prefix and Wine menu */
        tmp_w = XtNameToWidget(main_gui_data.menubar, prefix_button_id);
        XtSetSensitive(tmp_w, True);
        tmp_w = XtNameToWidget(main_gui_data.menubar, wine_button_id);
        XtSetSensitive(tmp_w, True);

        disable_programs_btns();
        update_prefix_info_label();
        if (refresh_program_list() != 0) {
                return;
        }
}

static
XTEE_XT_CALLBACK_FUNC(programs_list_cb)
{
        XmListCallbackStruct *cbs;

        (void)w;
        (void)client_data;
        cbs = (XmListCallbackStruct *)call_data;
        switch (cbs->reason) {
                case XmCR_SINGLE_SELECT:
                        enable_programs_btns();
                        break;
                default:
                        break;
        }
}

/* If program is NULL, then environment variables related to programs will not
 * be set.
 * This is mainly for use with wine utilities such as winecfg. */
static XteeEnvDataHandler
xtee_gui_set_env_data_handler(const XteePrefix *prefix, XteeProgram *program,
                bool is_winetricks)
{
        XteeEnvDataHandler env_data_handler = { 0 };

        xtee_env_data_add_var_to_handler(&env_data_handler, "WINEPREFIX",
                        prefix->wineprefix_dir.s);
        if (is_winetricks) {
                XteeWineVersionData *wine_version;

                wine_version =
                        xtee_state_get_loaded_prefix_wine_version_data();
                xtee_env_data_add_var_to_handler(&env_data_handler,
                                "WINE",
                                wine_version->wine_path.s);
                xtee_env_data_add_var_to_handler(&env_data_handler,
                                "WINESERVER",
                                wine_version->wineserver_path.s);
        }

        env_data_handler.winedlloverrides_str =
                xtee_prefix_get_WINEDLLOVERRIDES_env_value(prefix);
        if (env_data_handler.winedlloverrides_str
                        && *env_data_handler.winedlloverrides_str != '\0') {
                xtee_env_data_add_var_to_handler(&env_data_handler,
                                "WINEDLLOVERRIDES",
                                env_data_handler.winedlloverrides_str);
        }

        /* Set X11 DISPLAY if requested by user in program options */
        if (program && program->is_x11_display_set) {
                env_data_handler.x11_display_str = xtee_strdup_or_exit(
                                program->x11_display_str.s);
                xtee_env_data_add_var_to_handler(&env_data_handler,
                                "DISPLAY",
                                env_data_handler.x11_display_str);
        }

        /* Set WINE_D3D_CONFIG if needed */
        if (program && program->is_damavand_on) {
                xtee_env_data_add_var_to_handler(&env_data_handler,
                                "WINE_D3D_CONFIG",
                                "renderer=vulkan");
        }

        return env_data_handler;
}

static
XTEE_XT_CALLBACK_FUNC(start_program_btn_cb)
{
        XteeEnvDataHandler env_data_handler;
        XteeString program_str;
        char *args_regular[3];
        char **args_multiple = NULL;
        char **args_ptr;
        XteePrefix *loaded_prefix;
        XteeProgram *program_data;
        XteeWineVersionData *version_data;
        const char *info_strs[3];
        unsigned int *list_positions;
        size_t args_n;
        Widget programs_list;
        enum XteeGetProgramDataStatus status;
        int list_positions_n;
        int r;

        (void)w;
        (void)call_data;
        programs_list = (Widget)client_data;
        loaded_prefix = xtee_state_get_loaded_prefix();
        version_data = xtee_wine_get_version_data(
                        loaded_prefix->wine_version.s);

        XtVaGetValues(programs_list,
                        XmNselectedPositionCount, &list_positions_n,
                        XmNselectedPositions, &list_positions,
                        NULL);
        if (xtee_gui_get_selected_program_string(&program_str) != 0) {
                fatal_error("Cannot get selected program string.");
        }
        program_data = xtee_prefix_get_program_data(loaded_prefix,
                        program_str.s, &status);
        if (!program_data) {
                xtee_gui_print_get_program_data_error(status);
                return;
        }
        set_and_show_main_working_dialog("Running program",
                        "Running program");
        if (program_data->args_list.n != 0) {
                args_n = program_data->args_list.n + 3;
                args_multiple = malloc_or_exit(args_n * sizeof(char *));
                args_ptr = args_multiple;
                for (size_t i = 0; i < args_n - 3; ++i) {
                        args_multiple[i + 3] = xtee_strdup_or_exit(
                                        program_data->args_list.data[i].s);
                }
        } else {
                args_ptr = args_regular;
                args_n = XTEE_ARRAY_LEN(args_regular);
        }
        args_ptr[0] = "start";
        args_ptr[1] = "/unix";
        args_ptr[2] = xtee_strdup_or_exit(program_data->exe_path.s);
        info_strs[0] = "Running ";
        info_strs[1] = args_ptr[2];
        info_strs[2] = NULL;
        print_info_strs(info_strs);
        env_data_handler = xtee_gui_set_env_data_handler(loaded_prefix,
                        program_data, false);
        xtee_free_program(program_data);
        free(program_data);
        r = xtee_gui_fork_exec(version_data->wine_path.s,
                        args_ptr, args_n,
                        env_data_handler.block, env_data_handler.n);
        free(args_ptr[2]);
        xtee_env_data_free_handler(&env_data_handler);

        if (args_multiple) {
                for (size_t i = 0; i < args_n - 3; ++i) {
                        free(args_multiple[i + 3]);
                }
                free(args_multiple);
        }
        if (r != 0) {
                gui_print_error_log_and_dialog( "Error while running "
                                "program in Wine prefix.\n"
                                "This can also happen if you use the Cancel "
                                "button on an installer.",
                                main_gui_data.main_win);
        }
        hide_main_working_dialog();
}

static
XTEE_XT_CALLBACK_FUNC(program_options_btn_cb)
{
        (void)w;
        (void)call_data;
        (void)client_data;
        xtee_gui_show_program_options_dialog(main_gui_data.main_win,
                        &xtee_gui_data_struct.program_options_dialog);
}

static
XTEE_XT_CALLBACK_FUNC(remove_program_yes_cb)
{
        const char *charstrs_block[6];
        const char *info_strs[3];
        XteeConstString data_path;
        XteeString program_name;
        char *full_program_path;
        XteePrefix *loaded_prefix;

        (void)w;
        (void)call_data;
        (void)client_data;

        if (xtee_gui_get_selected_program_string(&program_name) != 0) {
                gui_print_error_dialog("No program selected, cannot remove "
                                "program entry", xtee_gui_get_main_win());
        }
        loaded_prefix = xtee_state_get_loaded_prefix();
        data_path = get_xteetotaller_dir_path();
        charstrs_block[0] = data_path.s;
        charstrs_block[1] = "prefixes/";
        charstrs_block[2] = loaded_prefix->id.s;
        charstrs_block[3] = "/programs/";
        charstrs_block[4] = program_name.s;
        charstrs_block[5] = NULL;
        full_program_path = xtee_string_merge_char_strings_block(
                        charstrs_block);
        info_strs[0] = "Deleting program: ";
        info_strs[1] = full_program_path;
        info_strs[2] = NULL;
        print_info_strs(info_strs);
        /* We use unlink since we are deleting only files.
         * This means that even if there is a bug, no directory will be
         * removed. */
        if (unlink(full_program_path) != 0) {
                gui_print_error_log_and_dialog("unlink failed, cannot remove "
                                "program entry",
                                xtee_gui_get_main_win());
                xtee_gui_set_message_area_string("Program entry removal "
                                "failed.");
        } else {
                xtee_prefix_reload_programs_list(
                                xtee_state_get_loaded_prefix());
                refresh_program_list();
                disable_programs_btns();
                gui_print_info_dialog("Program entry sucessfully removed.",
                                xtee_gui_get_main_win());
                xtee_gui_set_message_area_string("Program entry removed.");
        }
        free(full_program_path);
}

static
XTEE_XT_CALLBACK_FUNC(remove_program_btn_cb)
{
        static Widget remove_program_confirm_dialog = NULL;
        (void)w;
        (void)call_data;
        (void)client_data;

        if (!remove_program_confirm_dialog) {
                Arg args[10];
                Cardinal i = 0;
                XmString dialog_title, dialog_text;
                XmString yes_string, no_string;

                dialog_title = XmStringCreateLocalized("Remove program");
                dialog_text = XmStringCreateLocalized("Do you want to "
                                "remove this program entry?\n"
                                "Please note that the actual executable "
                                "file will be untouched.");
                set_message_dialog_args(args, &i, dialog_title,
                                dialog_text);
                remove_program_confirm_dialog = XmCreateQuestionDialog(
                                xtee_gui_get_main_win(),
                                "remove_program_confirm_dialog", args, i);
                XmStringFree(dialog_title);
                XmStringFree(dialog_text);
                xtee_gui_hide_dialog_help_button(
                                remove_program_confirm_dialog);
                yes_string = XmStringCreateLocalized("Yes");
                no_string = XmStringCreateLocalized("No");
                /* No need for Cancel callback as the dialog is already
                 * unmanaged by default. */
                XtVaSetValues(remove_program_confirm_dialog,
                                XmNokLabelString, yes_string,
                                XmNcancelLabelString, no_string,
                                NULL);
                XmStringFree(yes_string);
                XmStringFree(no_string);
                XtAddCallback(remove_program_confirm_dialog, XmNokCallback,
                                remove_program_yes_cb, NULL);
        }
        XtManageChild(remove_program_confirm_dialog);
}

static void
create_gui_programs_frame(XteeWorkAreaStruct *wa_struct, Widget parent)
{
        Arg args[5];
        Cardinal n = 0;
        Widget programs_list_parent;
        XmString tmp_str;

        wa_struct->programs_frame = XmVaCreateFrame(
                        parent, "programs_frame",
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNtopOffset, 15,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNleftOffset, 20,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNrightOffset, 20,
                        NULL);
        tmp_str = XmStringCreateLocalized("Programs");
        wa_struct->programs_frame_title = XtVaCreateManagedWidget(
                        "programs_frame_title",
                        xmLabelGadgetClass,
                        wa_struct->programs_frame,
                        XmNframeChildType, XmFRAME_TITLE_CHILD,
                        XmNlabelString, tmp_str,
                        NULL);
        XmStringFree(tmp_str);
        wa_struct->programs_form = XmVaCreateForm(
                        wa_struct->programs_frame, "programs_form",
                        XmNfractionBase, 6,
                        NULL);

        XtSetArg(args[n], XmNselectionPolicy, XmSINGLE_SELECT); ++n;
        XtSetArg(args[n], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE); ++n;
        /* XmCreateScrolledList returns the List widget.
         * Use XtParent to access the ScrolledWindow. */
        wa_struct->programs_list = XmCreateScrolledList(
                        wa_struct->programs_form,
                        "programs_list", args, n);
        /* Don't set XmNsingleSelectionCallback with XtVaSetValues as it
         * causes a SEGFAULT.
         * Use XtAddCallback instead. */
        XtAddCallback(wa_struct->programs_list,
                        XmNsingleSelectionCallback, programs_list_cb,
                        NULL);

        programs_list_parent = XtParent(wa_struct->programs_list);
        XtVaSetValues(programs_list_parent,
                        XmNheight, 200,
                        XmNleftAttachment, XmATTACH_POSITION,
                        XmNleftPosition, 0,
                        XmNleftOffset, 15,
                        XmNrightAttachment, XmATTACH_POSITION,
                        XmNrightPosition, 4,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNbottomWidget, wa_struct->programs_form,
                        XmNbottomOffset, 10,
                        NULL);
        XtManageChild(wa_struct->programs_list);

        wa_struct->programs_buttons = XmVaCreateRowColumn(
                        wa_struct->programs_form, "programs_buttons",
                        XmNorientation, XmVERTICAL,
                        XmNisAligned, False,
                        XmNentryAlignment, XmALIGNMENT_CENTER,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, programs_list_parent,
                        XmNleftOffset, 10,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNrightOffset, 20,
                        XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNtopWidget, programs_list_parent,
                        XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNbottomWidget, programs_list_parent,
                        NULL);
        for (size_t i = 0; i < XTEE_ARRAY_LEN(programs_btns_data); ++i) {
                /* Reuse tmp_str as its contents has already been freed */
                tmp_str = XmStringCreateLocalized(programs_btns_data[i].label);
                *(programs_btns_data[i].w) = XtVaCreateManagedWidget(
                                programs_btns_data[i].id,
                                xmPushButtonWidgetClass,
                                wa_struct->programs_buttons,
                                XmNlabelString, tmp_str,
                                NULL);
                XmStringFree(tmp_str);
        }
        XtAddCallback(*programs_btns_data[start_program_btn_i].w,
                        XmNactivateCallback, start_program_btn_cb,
                        main_gui_data.work_area_struct.programs_list);
        XtAddCallback(*programs_btns_data[program_options_btn_i].w,
                        XmNactivateCallback, program_options_btn_cb,
                        NULL);
        XtAddCallback(*programs_btns_data[remove_program_btn_i].w,
                        XmNactivateCallback, remove_program_btn_cb,
                        NULL);
        XtManageChild(wa_struct->programs_buttons);
        XtManageChild(wa_struct->programs_form);
        XtManageChild(wa_struct->programs_frame);
}

static int
fork_exec_run_executable(XteePrefix *prefix, XteeString path_str)
{
        XteeEnvDataHandler env_data_handler;
        char *args[3];
        XteeWineVersionData *version_data;
        const size_t args_n = XTEE_ARRAY_LEN(args);
        int r;

        version_data = xtee_wine_get_version_data(prefix->wine_version.s);
        args[0] = "start";
        args[1] = "/unix";
        args[2] = path_str.s;
        env_data_handler = xtee_gui_set_env_data_handler(prefix,
                        NULL, false);
        print_info("Running executable in Wine prefix.");
        r = xtee_gui_fork_exec(version_data->wine_path.s, args, args_n,
                        env_data_handler.block, env_data_handler.n);
        xtee_env_data_free_handler(&env_data_handler);
        return r;
}

static void
create_gui_prefix_frame(XteeWorkAreaStruct *wa_struct, Widget parent)
{
        unsigned int button_lr_margin = 5;
        XmString tmp_str;

        wa_struct->prefix_frame = XmVaCreateFrame(parent,
                        "prefix_frame",
                        XmNtopAttachment, XmATTACH_WIDGET,
                        XmNtopWidget, wa_struct->programs_frame,
                        XmNtopOffset, 15,
                        XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNleftWidget, wa_struct->programs_frame,
                        XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNrightWidget, wa_struct->programs_frame,
                        NULL);
        tmp_str = XmStringCreateLocalized("Prefix");
        wa_struct->prefix_frame_title = XmVaCreateManagedLabelGadget(
                        wa_struct->prefix_frame,
                        "prefix_frame_title",
                        XmNframeChildType, XmFRAME_TITLE_CHILD,
                        XmNlabelString, tmp_str,
                        NULL);
        XmStringFree(tmp_str);

        wa_struct->prefix_form = XtCreateWidget("prefix_frame_form",
                        xmFormWidgetClass,
                        wa_struct->prefix_frame,
                        NULL, 0);
        wa_struct->prefix_info_label = XtVaCreateManagedWidget(
                        "prefix_info_label", xmLabelGadgetClass,
                        wa_struct->prefix_form,
                        XmNalignment, XmALIGNMENT_BEGINNING,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNbottomOffset, 5,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNleftOffset, button_lr_margin,
                        NULL);
        reset_prefix_info_label();

        XtManageChild(wa_struct->prefix_form);
        XtManageChild(wa_struct->prefix_frame);
}

static void
create_gui_work_area(Widget main_win)
{
        XteeWorkAreaStruct *work_area_struct;

        work_area_struct = &main_gui_data.work_area_struct;
        work_area_struct->parent_form = XmVaCreateForm(main_win,
                        "work_area_form", NULL);
        create_gui_programs_frame(work_area_struct,
                        work_area_struct->parent_form);
        create_gui_prefix_frame(work_area_struct,
                        work_area_struct->parent_form);
        XtManageChild(work_area_struct->parent_form);
}

int
xtee_gui_load_prefix(const char *prefix_id, Widget parent)
{
        XteePrefix tmp_prefix;
        XteePrefix *current_prefix;
        enum XteeLoadPrefixStatus load_prefix_status;

        current_prefix = xtee_state_get_loaded_prefix();
        init_XteePrefix(&tmp_prefix);
        load_prefix_status = xtee_prefix_load(&tmp_prefix, prefix_id);
        switch (load_prefix_status) {
                case XTEE_LOAD_PREFIX_OK:
                        /* Do nothing */
                        break;
                case XTEE_LOAD_PREFIX_NO_CONFIG:
                        gui_print_error_log_and_dialog("No config file "
                                        "found.", parent);
                        return -1;
                case XTEE_LOAD_PREFIX_CONFIG_ACCESS_ERR:
                case XTEE_LOAD_PREFIX_CONFIG_READ_ERR:
                        gui_print_error_log_and_dialog("Cannot read config "
                                        "file.", parent);
                        return -1;
                case XTEE_LOAD_PREFIX_CONFIG_MISSING_PREFIXDIR:
                        gui_print_error_log_and_dialog("Prefix directory "
                                        "variable not set in config file.",
                                        parent);
                        return -1;
                case XTEE_LOAD_PREFIX_DIR_NOT_EXIST:
                        gui_print_error_log_and_dialog("Wineprefix directory "
                                        "does not exist.",
                                        parent);
                        return -1;
                case XTEE_LOAD_PREFIX_PROGRAMS_READ_ERR:
                        gui_print_error_log_and_dialog("Prefix programs "
                                        "cannot be read.",
                                        parent);
                        return -1;
                case XTEE_LOAD_PREFIX_WRONG_ARCH:
                        gui_print_error_log_and_dialog("Incorrect prefix "
                                        "architecture.",
                                        parent);
                        return -1;
                case XTEE_LOAD_PREFIX_WINE_VERSION_NOTEXIST:
                        gui_print_error_log_and_dialog("Prefix Wine version "
                                        "is not installed.",
                                        parent);
                        return -1;
                case XTEE_LOAD_PREFIX_WINE_VERSION_INCOMP_ARCH:
                        gui_print_error_log_and_dialog("Prefix arch is "
                                        "incompatible with selected Wine "
                                        "version.",
                                        parent);
                        return -1;
                case XTEE_LOAD_PREFIX_WINE_ARCH_SYSTEM_NOMATCH:
                        gui_print_error_log_and_dialog("Prefix architecture "
                                        "is not compatible with 32-bits "
                                        "systems.",
                                        parent);
                        return -1;
                default:
                        gui_print_error_log_and_dialog("An error occured "
                                        "while trying to open a prefix.",
                                        parent);
                        return -1;
        }

        xtee_state_set_loaded_prefix(&tmp_prefix);
        refresh_prefix_gui();
        /* Save prefix config file to set the default config values if missing
         * from the original configuration file. */
        if (xtee_prefix_save_config(current_prefix) != 0) {
                gui_print_warning_dialog("Cannot save prefix config "
                                "file.", main_gui_data.main_win);
        } else {
                xtee_gui_set_message_area_string("Prefix loaded and saved.");
                print_info("Prefix config file saved.");
        }

        return 0;

}

static
XTEE_XT_CALLBACK_FUNC(open_prefix_dialog_cb)
{
        XteePrefix *current_prefix;
        XmSelectionBoxCallbackStruct *cbs;
        char *prefix_id;
        int r;

        (void)w;
        (void)client_data;

        current_prefix = xtee_state_get_loaded_prefix();
        cbs = (XmSelectionBoxCallbackStruct *)call_data;
        switch (cbs->reason) {
                case XmCR_OK:
                        print_info("Prefix selected");
                        break;
                case XmCR_NO_MATCH:
                        gui_print_error_log_and_dialog("Prefix name not valid",
                                        w);
                        return;
                case XmCR_CANCEL:
                        XtUnmanageChild(w);
                        return;
                default:
                        gui_print_error_dialog("Something wrong happened",
                                        w);
                        return;
        }

        prefix_id = XmString_to_char_str(&cbs->value);
        r = xtee_gui_load_prefix(prefix_id, w);
        XtFree(prefix_id);
        if (r != 0) {
                return;
        }

        if (current_prefix->arch == XTEE_PREFIX_ARCH_WRONG) {
                gui_print_warning_dialog("Prefix Wine architecture is "
                                "incorrect.\n"
                                "Programs will not start until the prefix "
                                "configuration has a suitable Wine "
                                "architecture.", main_gui_data.main_win);
        }
        refresh_prefix_gui();
        XtUnmanageChild(w);
}

static void
show_open_prefix_dialog(void)
{
        static Widget dialog = NULL;
        XteePrefixIdBlock id_block;
        XmString *prefixes_strs;

        if (!dialog) {
                Arg args[10];
                unsigned int n = 0;
                XmString dialog_title;
                XmString selection_label;

                dialog_title = XmStringCreateLocalized("Open prefix");
                selection_label = XmStringCreateLocalized("Prefixes");
                XtSetArg(args[n], XmNdialogTitle, dialog_title); n++;
                XtSetArg(args[n], XmNlistLabelString, selection_label); n++;
                XtSetArg(args[n], XmNdialogStyle,
                                XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
                XtSetArg(args[n], XmNautoUnmanage, False); n++;
                XtSetArg(args[n], XmNmustMatch, True); n++;
                dialog = XmCreateSelectionDialog(
                                main_gui_data.main_win, "open_prefix_dialog",
                                args, n);
                XmStringFree(dialog_title);
                XmStringFree(selection_label);
                XtAddCallback(dialog, XmNokCallback, open_prefix_dialog_cb,
                                NULL);
                XtAddCallback(dialog, XmNcancelCallback, open_prefix_dialog_cb,
                                NULL);
                XtAddCallback(dialog, XmNnoMatchCallback,
                                open_prefix_dialog_cb, NULL);
                xtee_gui_disable_dialog_help_button(dialog);
                /* Hide Apply button */
                XtUnmanageChild(XtNameToWidget(dialog, "Apply"));
        }

        if (get_prefixes_ids(&id_block) != 0) {
                gui_print_error_dialog("Cannot read prefixes ids and names.",
                                dialog);
                return;
        }

        prefixes_strs = malloc_or_exit(id_block.n * sizeof(XmString));
        for (size_t i = 0; i < id_block.n; ++i) {
                prefixes_strs[i] = XmStringCreateLocalized(
                                id_block.data[i].s);
        }
        XtVaSetValues(dialog,
                        XmNlistItems, prefixes_strs,
                        XmNlistItemCount, id_block.n,
                        NULL);

        for (size_t i = 0; i < id_block.n; ++i) {
                XmStringFree(prefixes_strs[i]);
        }
        free(prefixes_strs);
        free_XteePrefixIdBlock(&id_block);
        XtManageChild(dialog);
}


/* Callbacks may contain pointers to int conversion.
 * This is documented in the Motif manual.
 * Warnings can thus be safely ignored. */
static
XTEE_XT_CALLBACK_FUNC(file_menu_cb)
{
        const int new_prefix_i = 0;
        const int open_prefix_i = 1;
        const int exit_i = 2;
        int item_i;

        (void)w;
        (void)call_data;

#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        item_i = (int)client_data;
#pragma GCC diagnostic pop
        if (item_i == new_prefix_i) {
                xtee_gui_show_new_prefix_dialog(
                                &xtee_gui_data_struct.new_prefix_dialog);
        } else if (item_i == open_prefix_i) {
                show_open_prefix_dialog();
        } else if (item_i == exit_i) {
                exit(0);
        }
}

static
XTEE_XT_CALLBACK_FUNC(edit_menu_cb)
{
        const int preferences_item_i = 0;
        int item_i;

        (void)w;
        (void)call_data;

#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        item_i = (int)client_data;
#pragma GCC diagnostic pop
        if (item_i == preferences_item_i) {
                xtee_gui_preferences_show_dialog(
                                &xtee_gui_data_struct.prefs_dialog);
        }
}

#define ABOUT_DIALOG_TEXT_CONTENT \
"XTeetotaller, Alpha version.\n\
A Wine prefixes and programs manager for UNIX and UNIX-like \
operating systems.\n\
\n\
Copyright 2026 Adrien Saad. All Rights Reserved.\n\n"

static Widget
create_about_dialog(void)
{
        Widget about_dialog;
        Arg args[10];
        XmString dialog_title, dialog_text;
        unsigned int n = 0;

        dialog_title = XmStringCreateLocalized("About XTeetotaller");
        dialog_text = XmStringCreateLocalized(ABOUT_DIALOG_TEXT_CONTENT);

        /* Don't make this dialog modal as it doesn't interact with the rest of
         * the program. */
        XtSetArg(args[n], XmNdialogTitle, dialog_title); n++;
        XtSetArg(args[n], XmNmessageString, dialog_text); n++;
        XtSetArg(args[n], XmNnoResize, True); n++;
        XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
        XtSetArg(args[n], XmNdefaultButton, XmDIALOG_OK_BUTTON); n++;
        about_dialog = XmCreateInformationDialog(main_gui_data.main_win,
                        "about_dialog", args, n);
        XmStringFree(dialog_title);
        XmStringFree(dialog_text);

        xtee_gui_hide_dialog_cancel_button(about_dialog);
        xtee_gui_disable_dialog_help_button(about_dialog);

        return about_dialog;
}

static
XTEE_XT_CALLBACK_FUNC(run_exec_browse_dialog_btns_cb)
{
        XmFileSelectionBoxCallbackStruct *cbs;
        XteeString val_xteestr;
        char *val_str;
        XteePrefix *loaded_prefix;
        XteeWineVersionData *version_data;
        enum XteeExecutableType exe_type;
        int r;
        bool is_file_msi = false;

        (void)client_data;
        cbs = (XmFileSelectionBoxCallbackStruct *)call_data;
        if (cbs->reason == XmCR_CANCEL) {
                XtUnmanageChild(w);
                return;
        } else if (cbs->reason != XmCR_OK) {
                return;
        }
        loaded_prefix = xtee_state_get_loaded_prefix();
        version_data = xtee_wine_get_version_data(
                        loaded_prefix->wine_version.s);
        if (!version_data) {
                gui_print_error_log_and_dialog(
                                "Prefix wine version does not exist.",
                                main_gui_data.main_win);
                return;
        }
        val_str = XmString_to_char_str(&cbs->value);
        if (!val_str) {
                XtUnmanageChild(w);
                return;
        }
        XtUnmanageChild(w);
        val_xteestr.s = val_str;
        val_xteestr.n = strlen(val_str);
        is_file_msi = xtee_is_file_msi(
                        XteeString_to_XteeConstString(val_xteestr));
        if (xtee_get_executable_type(&exe_type,
                                XteeString_to_XteeConstString(val_xteestr))
                        != 0
                        && !is_file_msi) {
                gui_print_error_log_and_dialog(
                                "Executable type is incorrect.",
                                main_gui_data.main_win);
                free(val_str);
                return;
        }
        if (exe_type == XTEE_EXECUTABLE_PE32PLUS
                        && loaded_prefix->arch == XTEE_PREFIX_ARCH_WIN32) {
                gui_print_error_log_and_dialog(
                                "PE32+ programs cannot be run in Win32 prefix",
                                main_gui_data.main_win);
                free(val_str);
                return;
        } else if(exe_type != XTEE_EXECUTABLE_PE32
                        && exe_type != XTEE_EXECUTABLE_PE32PLUS
                        && !is_file_msi) {
                gui_print_error_log_and_dialog(
                                "Executable is not compatible with Wine "
                                "prefix.",
                                main_gui_data.main_win);
                free(val_str);
                return;
        }

        set_and_show_main_working_dialog("Running program",
                        "Program is running...");
        r = fork_exec_run_executable(loaded_prefix, val_xteestr);
        free(val_str);
        hide_main_working_dialog();
        if (r != 0) {
                gui_print_error_log_and_dialog(
                                "Error while running executable in Wine "
                                "prefix.\n"
                                "This can also happen if you use the Cancel "
                                "button on an installer.",
                                main_gui_data.main_win);
        } else {
                xtee_gui_set_message_area_string("Program run successfully.");
        }
}

static void
xtee_gui_run_executable_in_prefix(void)
{
        static Widget run_exec_browse_dialog = NULL;
        char *gui_show_hidden_files_val;

        if (!run_exec_browse_dialog) {
                Arg args[10];
                Cardinal n = 0;
                XmString dialog_title;

                dialog_title = XmStringCreateLocalized("Run executable...");
                XtSetArg(args[n], XmNpathMode, XmPATH_MODE_RELATIVE); n++;
                XtSetArg(args[n], XmNdialogTitle, dialog_title); n++;
                XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
                XtSetArg(args[n], XmNwidth, XTEE_FILESELECTIONBOX_WIDTH); n++;
                XtSetArg(args[n], XmNheight,
                                XTEE_FILESELECTIONBOX_HEIGHT); n++;
                XtSetArg(args[n], XmNdialogStyle,
                                XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
                run_exec_browse_dialog = XmCreateFileSelectionDialog(
                                main_gui_data.main_win,
                                "run_exec_browse_dialog", args, n);
                XmStringFree(dialog_title);
                XtAddCallback(run_exec_browse_dialog, XmNokCallback,
                                run_exec_browse_dialog_btns_cb,
                                NULL);
                XtAddCallback(run_exec_browse_dialog, XmNcancelCallback,
                                run_exec_browse_dialog_btns_cb,
                                NULL);
        }
        gui_show_hidden_files_val = xtee_config_get_entry_value(
                        "gui.show_hidden_files");
        if (!gui_show_hidden_files_val
                        || !xtee_string_to_bool(
                                gui_show_hidden_files_val)) {
                XtVaSetValues(run_exec_browse_dialog, XmNfileFilterStyle,
                                XmFILTER_HIDDEN_FILES,
                                NULL);
        } else {
                XtVaSetValues(run_exec_browse_dialog, XmNfileFilterStyle,
                                XmFILTER_NONE,
                                NULL);
        }
        XtManageChild(run_exec_browse_dialog);
}

static
XTEE_XT_CALLBACK_FUNC(prefix_menu_cb)
{
        const int reload_prefix_i = 0;
        const int prefix_options_i = 1;
        const int new_program_i = 2;
        const int run_executable_i = 3;
        int item_i;

        (void)w;
        (void)call_data;
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        item_i = (int)client_data;
#pragma GCC diagnostic pop

        if (item_i == reload_prefix_i) {
                XteePrefix *current_prefix;
                (void)w;
                (void)client_data;
                (void)call_data;

                current_prefix = xtee_state_get_loaded_prefix();
                reset_main_win_state();

                if (xtee_gui_load_prefix(current_prefix->id.s,
                                        main_gui_data.main_win) != 0) {
                        return;
                }
                xtee_gui_set_message_area_string("Prefix reloaded.");
        } else if (item_i == prefix_options_i) {
                xtee_gui_show_prefix_options_dialog(main_gui_data.main_win,
                                &xtee_gui_data_struct.prefix_options_dialog);
        } else if (item_i == new_program_i) {
                xtee_gui_new_program_show_dialog(
                                &xtee_gui_data_struct.new_program_dialog);
        } else if (item_i == run_executable_i) {
                xtee_gui_run_executable_in_prefix();
        }
}

static void
xtee_gui_run_wine_utility(char *tool_exe_name,
                char *working_dialog_title,
                char *working_dialog_text)
{
        char *exec_args[2];
        XteeEnvDataHandler env_data_handler;
        XteeWineVersionData *wine_version;
        XteePrefix *loaded_prefix;
        int r;
        size_t args_n = XTEE_ARRAY_LEN(exec_args);

        loaded_prefix = xtee_state_get_loaded_prefix();
        wine_version = xtee_wine_get_version_data(
                        loaded_prefix->wine_version.s);
        set_and_show_main_working_dialog(working_dialog_title,
                        working_dialog_text);
        exec_args[0] = "start";
        exec_args[1] = tool_exe_name;
        env_data_handler = xtee_gui_set_env_data_handler(loaded_prefix,
                        NULL, false);
        r = xtee_gui_fork_exec(wine_version->wine_path.s, exec_args, args_n,
                        env_data_handler.block, env_data_handler.n);
        xtee_env_data_free_handler(&env_data_handler);
        hide_main_working_dialog();
        if (r != 0) {
                gui_print_error_log_and_dialog("Cannot run wine utility.",
                                main_gui_data.main_win);
        }
}

static void
xtee_gui_run_winetricks(void)
{
        XteeEnvDataHandler env_data_handler;
        XteePrefix *loaded_prefix;
        int r;

        if (!xtee_system_is_winetricks_installed) {
                gui_print_error_log_and_dialog("Winetricks is not installed.",
                                main_gui_data.main_win);
                return;
        }

        loaded_prefix = xtee_state_get_loaded_prefix();
        set_and_show_main_working_dialog("Running winetricks",
                        "Running winetricks");
        env_data_handler = xtee_gui_set_env_data_handler(loaded_prefix,
                        NULL, true);

        r = xtee_gui_fork_exec((char *)xtee_system_winetricks_path.s, NULL, 0,
                                env_data_handler.block,
                                env_data_handler.n);
        xtee_env_data_free_handler(&env_data_handler);
        hide_main_working_dialog();
        if (r != 0) {
                gui_print_error_log_and_dialog("Cannot run winetricks.",
                                main_gui_data.main_win);
        }
}

XTEE_XT_CALLBACK_FUNC(kill_wine_confirm_dialog_ok_cb)
{
        XteeEnvDataHandler env_data_handler;
        XteePrefix *loaded_prefix;
        XteeWineVersionData *wine_version;
        char *args[] = { "-k" };
        int r;

        (void)w;
        (void)call_data;
        (void)client_data;
        loaded_prefix = xtee_state_get_loaded_prefix();
        wine_version = xtee_state_get_loaded_prefix_wine_version_data();
        env_data_handler = xtee_gui_set_env_data_handler(loaded_prefix,
                        NULL, false);

        print_info("Running wineserver -k");
        set_and_show_main_working_dialog("Killing wine processes",
                        "Killing wine processes.");
        r = xtee_gui_fork_exec((char *)wine_version->wineserver_path.s,
                        args, XTEE_ARRAY_LEN(args),
                        env_data_handler.block, env_data_handler.n);
        xtee_env_data_free_handler(&env_data_handler);
        hide_main_working_dialog();
        if (r != 0) {
                gui_print_error_log_and_dialog("Cannot kill wine processes.\n"
                                "Check if any wine process is running.",
                                main_gui_data.main_win);
                xtee_gui_set_message_area_string("wineserver -k failed");
        } else {
                xtee_gui_set_message_area_string("Wine processes killed");
        }
}

static void
show_kill_wine_confirm_dialog(void)
{
        static Widget confirm_dialog = NULL;

        if (!confirm_dialog) {
                Arg args[10];
                Cardinal i = 0;
                XmString dialog_title, dialog_text;
                XmString yes_string, no_string;

                dialog_title = XmStringCreateLocalized("Kill wine processes");
                dialog_text = XmStringCreateLocalized("Do you want to kill "
                                "prefix wine processes?");
                set_message_dialog_args(args, &i, dialog_title,
                                dialog_text);
                confirm_dialog = XmCreateQuestionDialog(
                                xtee_gui_get_main_win(),
                                "kill_wine_confirm_dialog", args, i);
                XmStringFree(dialog_title);
                XmStringFree(dialog_text);
                xtee_gui_hide_dialog_help_button(confirm_dialog);
                yes_string = XmStringCreateLocalized("Yes");
                no_string = XmStringCreateLocalized("No");
                XtVaSetValues(confirm_dialog,
                                XmNokLabelString, yes_string,
                                XmNcancelLabelString, no_string,
                                NULL);
                XmStringFree(yes_string);
                XmStringFree(no_string);
                XtAddCallback(confirm_dialog, XmNokCallback,
                                kill_wine_confirm_dialog_ok_cb, NULL);
        }
        XtManageChild(confirm_dialog);
}

static
XTEE_XT_CALLBACK_FUNC(wine_menu_cb)
{
        char *wine_menu_entries[][3] = {
                { "winecfg", "Running winecfg", "Running winecfg" },
                { "uninstaller", "Running uninstaller",
                        "Running uninstaller" },
                { "explorer", "Running explorer", "Running explorer" },
                { "regedit", "Running regedit", "Running regedit" },
                { "winefile", "Running winefile", "Running winefile" },
        };
        const int winetricks_i = (int)XTEE_ARRAY_LEN(wine_menu_entries);
        const int kill_wine_i = (int)XTEE_ARRAY_LEN(wine_menu_entries) + 1;
        int item_i;

        (void)w;
        (void)call_data;
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        item_i = (int)client_data;
#pragma GCC diagnostic pop

        if (item_i < (int)XTEE_ARRAY_LEN(wine_menu_entries)) {
                xtee_gui_run_wine_utility(wine_menu_entries[item_i][0],
                                wine_menu_entries[item_i][1],
                                wine_menu_entries[item_i][2]);
        } else if (item_i == winetricks_i) {
                xtee_gui_run_winetricks();
        } else if (item_i == kill_wine_i) {
                show_kill_wine_confirm_dialog();
        }
}

static
XTEE_XT_CALLBACK_FUNC(help_menu_cb)
{
        static Widget about_dialog = NULL;
        const int about_item_i = 0;
        int item_i;

        (void)w;
        (void)call_data;

#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        item_i = (int)client_data;
#pragma GCC diagnostic pop
        if (item_i == about_item_i) {
                if (!about_dialog) {
                        about_dialog = create_about_dialog();
                }
                XtManageChild(about_dialog);
        }
}

static void
gui_create_menubar(void)
{
        Widget tmp_w;
        XmString file_str, edit_str, prefix_str, wine_str, help_str;
        XmString new_prefix_str, open_prefix_str, exit_str;
        XmString new_prefix_acc, open_prefix_acc, exit_acc;
        XmString preferences_str;
        XmString reload_prefix_str, prefix_options_str, new_program_str,
                 run_executable_str;
        XmString reload_prefix_acc;
        XmString winecfg_str, uninstaller_str, explorer_str, regedit_str,
                 winefile_str, winetricks_str, kill_wine_str;
        XmString kill_wine_acc;
        XmString about_str;
        const char *help_button_id = "button_4";
        const int file_button_i = 0;
        const int edit_button_i = 1;
        const int prefix_button_i = 2;
        const int wine_button_i = 3;
        const int help_button_i = 4;

        file_str = XmStringCreateLocalized("File");
        edit_str = XmStringCreateLocalized("Edit");
        prefix_str = XmStringCreateLocalized("Prefix");
        wine_str = XmStringCreateLocalized("Wine");
        help_str = XmStringCreateLocalized("Help");
        main_gui_data.menubar = XmVaCreateSimpleMenuBar(main_gui_data.main_win,
                        "menubar",
                        XmVaCASCADEBUTTON, file_str, 'F',
                        XmVaCASCADEBUTTON, edit_str, 'E',
                        XmVaCASCADEBUTTON, prefix_str, 'P',
                        XmVaCASCADEBUTTON, wine_str, 'W',
                        XmVaCASCADEBUTTON, help_str, 'H',
                        NULL);
        XmStringFree(file_str);
        XmStringFree(edit_str);
        XmStringFree(prefix_str);
        XmStringFree(wine_str);
        XmStringFree(help_str);

        /* Set Help menu */
        tmp_w = XtNameToWidget(main_gui_data.menubar, help_button_id);
        XtVaSetValues(main_gui_data.menubar, XmNmenuHelpWidget, tmp_w, NULL);
        /* Disable Prefix menu at launch as no prefix is loaded. */
        tmp_w = XtNameToWidget(main_gui_data.menubar, prefix_button_id);
        XtSetSensitive(tmp_w, False);
        tmp_w = XtNameToWidget(main_gui_data.menubar, wine_button_id);
        XtSetSensitive(tmp_w, False);

        new_prefix_str = XmStringCreateLocalized("New Prefix");
        open_prefix_str = XmStringCreateLocalized("Open prefix");
        exit_str = XmStringCreateLocalized("Exit");
        new_prefix_acc = XmStringCreateLocalized("Ctrl+N");
        open_prefix_acc = XmStringCreateLocalized("Ctrl+O");
        exit_acc = XmStringCreateLocalized("Ctrl+Q");
        main_gui_data.file_menu = XmVaCreateSimplePulldownMenu(
                        main_gui_data.menubar, "file_menu", file_button_i,
                        file_menu_cb,
                        XmVaPUSHBUTTON, new_prefix_str, 'N', "Ctrl<Key>n",
                        new_prefix_acc,
                        XmVaPUSHBUTTON, open_prefix_str, 'O', "Ctrl<Key>o",
                        open_prefix_acc,
                        XmVaSEPARATOR,
                        XmVaPUSHBUTTON, exit_str, 'x', "Ctrl<Key>q", exit_acc,
                        NULL);
        XmStringFree(new_prefix_str);
        XmStringFree(open_prefix_str);
        XmStringFree(exit_str);
        XmStringFree(new_prefix_acc);
        XmStringFree(open_prefix_acc);
        XmStringFree(exit_acc);
        preferences_str = XmStringCreateLocalized("Preferences");
        main_gui_data.edit_menu = XmVaCreateSimplePulldownMenu(
                        main_gui_data.menubar, "edit_menu", edit_button_i,
                        edit_menu_cb,
                        XmVaPUSHBUTTON, preferences_str, 'e', NULL, NULL,
                        NULL);
        XmStringFree(preferences_str);
        reload_prefix_str = XmStringCreateLocalized("Reload prefix");
        prefix_options_str = XmStringCreateLocalized("Options");
        new_program_str = XmStringCreateLocalized("New program");
        run_executable_str = XmStringCreateLocalized(
                        "Run executable in prefix");
        reload_prefix_acc = XmStringCreateLocalized("F5");
        main_gui_data.prefix_menu = XmVaCreateSimplePulldownMenu(
                        main_gui_data.menubar, "prefix_menu", prefix_button_i,
                        prefix_menu_cb,
                        XmVaPUSHBUTTON, reload_prefix_str, 'R',
                        "<Key>F5", reload_prefix_acc,
                        XmVaPUSHBUTTON, prefix_options_str, 'O', NULL, NULL,
                        XmVaSEPARATOR,
                        XmVaPUSHBUTTON, new_program_str, 'N', NULL, NULL,
                        XmVaSEPARATOR,
                        XmVaPUSHBUTTON, run_executable_str, 'e', NULL, NULL,
                        NULL);
        XmStringFree(reload_prefix_str);
        XmStringFree(prefix_options_str);
        XmStringFree(new_program_str);
        XmStringFree(run_executable_str);
        XmStringFree(reload_prefix_acc);
        winecfg_str = XmStringCreateLocalized("winecfg");
        uninstaller_str = XmStringCreateLocalized("uninstaller");
        explorer_str = XmStringCreateLocalized("explorer");
        regedit_str = XmStringCreateLocalized("regedit");
        winefile_str = XmStringCreateLocalized("winefile");
        winetricks_str = XmStringCreateLocalized("winetricks");
        kill_wine_str = XmStringCreateLocalized("Kill wine processes");
        kill_wine_acc = XmStringCreateLocalized("Ctrl+Shift+K");
        main_gui_data.wine_menu = XmVaCreateSimplePulldownMenu(
                        main_gui_data.menubar, "wine_menu", wine_button_i,
                        wine_menu_cb,
                        XmVaPUSHBUTTON, winecfg_str, 'c', NULL, NULL,
                        XmVaPUSHBUTTON, uninstaller_str, 'u', NULL, NULL,
                        XmVaPUSHBUTTON, explorer_str, 'x', NULL, NULL,
                        XmVaPUSHBUTTON, regedit_str, 'r', NULL, NULL,
                        XmVaPUSHBUTTON, winefile_str, 'f', NULL, NULL,
                        XmVaSEPARATOR,
                        XmVaPUSHBUTTON, winetricks_str, 't', NULL, NULL,
                        XmVaSEPARATOR,
                        XmVaPUSHBUTTON, kill_wine_str, 'K',
                        "Ctrl Shift <Key>K", kill_wine_acc,
                        NULL);
        XmStringFree(winecfg_str);
        XmStringFree(uninstaller_str);
        XmStringFree(explorer_str);
        XmStringFree(regedit_str);
        XmStringFree(winefile_str);
        XmStringFree(winetricks_str);
        XmStringFree(kill_wine_acc);
        about_str = XmStringCreateLocalized("About XTeetotaller");
        main_gui_data.help_menu = XmVaCreateSimplePulldownMenu(
                        main_gui_data.menubar, "help_menu", help_button_i,
                        help_menu_cb,
                        XmVaPUSHBUTTON, about_str, 'A', NULL, NULL,
                        NULL);
        XmStringFree(about_str);

        XtManageChild(main_gui_data.menubar);
}

static void
create_gui_message_area(Widget main_win)
{
        main_gui_data.message_area_label = XmVaCreateLabel(main_win,
                        "message_area_label",
                        XmNalignment, XmALIGNMENT_BEGINNING,
                        XmNmarginLeft, XTEE_FORM_LR_MARGIN,
                        XmNmarginBottom, 15,
                        NULL);
        xtee_gui_set_motif_label_string(main_gui_data.message_area_label,
                        "Ready.");
        XtManageChild(main_gui_data.message_area_label);
}

static void
gui_create_main_application(void)
{
        XteeWorkAreaStruct *wa_struct;

        wa_struct = &main_gui_data.work_area_struct;
        main_gui_data.main_win = XmCreateMainWindow(main_gui_data.toplevel,
                        "main_win", NULL, 0);
        XtVaSetValues(main_gui_data.main_win,
                        XmNwidth, XTEE_GUI_START_WIDTH,
                        XmNheight, XTEE_GUI_START_HEIGHT,
                        NULL);
        gui_create_menubar();
        create_gui_work_area(main_gui_data.main_win);
        create_gui_message_area(main_gui_data.main_win);
        XtVaSetValues(main_gui_data.main_win,
                        XmNmenuBar, main_gui_data.menubar,
                        XmNworkWindow, wa_struct->parent_form,
                        XmNmessageWindow, main_gui_data.message_area_label,
                        NULL);
        XtManageChild(main_gui_data.main_win);
        disable_programs_btns();
}

static
XTEE_XT_CALLBACK_FUNC(shell_close_callback)
{
        (void)w;
        (void)call_data;
        (void)client_data;
        print_info("Closing window shell");
        xtee_gui_free();
        xtee_clean();
        exit(0);
}

static void
set_window_close_callback(Widget shell)
{
        Display *dpy;
        Atom prop, prot;

        if (!shell) {
                fatal_error("Motif shell is NULL");
        }

        if (!(dpy = XtDisplay(shell))) {
                print_error("Cannot get X11 Display from window shell.");
                return;
        }
        if (!(prop = XmInternAtom(dpy, "WM_PROTOCOLS", False))) {
                print_error("Cannot get WM_PROTOCOLS atom");
                return;
        }
        if (!(prot = XmInternAtom(dpy, "WM_DELETE_WINDOW", True))) {
                print_error("Cannot get WM_DELETE_WINDOW atom");
                return;
        }
        XmAddProtocols(shell, prop, &prot, 1);
        XmAddProtocolCallback(shell, prop, prot, shell_close_callback, NULL);
}

static void
create_xtee_render_table(void)
{
        char *xlfd_str = "-misc-fixed-medium-r-*-*-13-*-*-*-*-80-iso10646-*";
        Arg args[10];
        Cardinal n;
        XmRendition rendition;

        n = 0;
        XtSetArg(args[n], XmNfontName, xlfd_str); n++;
        XtSetArg(args[n], XmNfontType, XmFONT_IS_FONT); n++;
        XtSetArg(args[n], XmNloadModel, XmLOAD_IMMEDIATE); n++;
        rendition = XmRenditionCreate(main_gui_data.toplevel,
                        "text_field_tag", args, n);
        xtee_gui_render_table = XmRenderTableAddRenditions(NULL, &rendition, 1,
                        XmMERGE_NEW);
        XmRenditionFree(rendition);
}

void
xtee_gui_init(int *argc, char *argv[])
{
        Screen *screen;
        int screen_width, screen_height;
        Dimension x_pos, y_pos;

        XtSetLanguageProc(NULL, NULL, NULL);
        main_gui_data.toplevel = XtVaOpenApplication(
                        &main_gui_data.app_context, "XTeetotaller", NULL, 0,
                        argc, argv, NULL,
                        sessionShellWidgetClass, NULL);
        set_window_close_callback(main_gui_data.toplevel);
        create_xtee_render_table();

        screen = XtScreen(main_gui_data.toplevel);
        screen_width = WidthOfScreen(screen);
        screen_height = HeightOfScreen(screen);
        x_pos = (Dimension)((screen_width - XTEE_GUI_START_WIDTH) / 2);
        y_pos = (Dimension)((screen_height - XTEE_GUI_START_HEIGHT) / 2);
        XtVaSetValues(main_gui_data.toplevel,
                        XmNwidth, XTEE_GUI_START_WIDTH,
                        XmNheight, XTEE_GUI_START_HEIGHT,
                        XmNx, x_pos,
                        XmNy, y_pos,
                        NULL);

        gui_create_main_application();
}

void
xtee_gui_run(void)
{
        XtRealizeWidget(main_gui_data.toplevel);

        if (!xtee_system_is_wine_installed) {
                gui_print_warning_dialog(
                                "Wine is not installed.\n"
                                "This means that prefixes with Wine version "
                                "set to System will not work.",
                                main_gui_data.main_win);
        }

        if(!xtee_system_is_winetricks_installed) {
                gui_print_error_log_and_dialog(
                                "winetricks is not installed.\n"
                                "Some essential features will not work.",
                                main_gui_data.main_win);
        }

        XtAppMainLoop(main_gui_data.app_context);
}

static void
xtee_gui_free_data_struct(XteeGuiData *xtee_gui_data)
{
        free_XteeGuiBasicFormDialog(&xtee_gui_data->program_options_dialog);
        free_XteeGuiBasicFormDialog(&xtee_gui_data->new_prefix_dialog);
        free_XteeGuiBasicFormDialog(&xtee_gui_data->prefs_dialog);
        free_XteeGuiBasicFormDialog(&xtee_gui_data->prefix_options_dialog);
        free_XteeGuiBasicFormDialog(&xtee_gui_data->new_program_dialog);
}

void
xtee_gui_free(void)
{
        xtee_gui_free_data_struct(&xtee_gui_data_struct);
        XtDestroyWidget(main_gui_data.toplevel);
        XmRenderTableFree(xtee_gui_render_table);
}

Widget
xtee_gui_get_main_win(void)
{
        return main_gui_data.main_win;
}

static int
xtee_gui_exec(char *pathname, char **args, size_t args_n,
                const XteeEnvData *env_data, size_t env_data_n)
{
        char **exec_args;
        int r;

        exec_args = malloc_or_exit((args_n + 2) * sizeof(const char *));;
        exec_args[0] = pathname;
        exec_args[args_n +  1] = NULL;
        if (args_n != 0) {
                memcpy(exec_args + 1, args, args_n * sizeof(const char *));
        }

        for (size_t i = 0; i < env_data_n; ++i) {
                const char *env_info_strs[5];

                env_info_strs[0] = "Setting ";
                env_info_strs[1] = env_data[i].name;
                env_info_strs[2] = " environment variable to ";
                env_info_strs[3] = env_data[i].value;
                env_info_strs[4] = NULL;
                print_info_strs(env_info_strs);
                setenv(env_data[i].name, env_data[i].value, 1);
        }
        r = execv(pathname, exec_args);
        free(exec_args);
        return r;
}

void
xtee_gui_handle_busy_events(void)
{
        Display *dpy;
        long event_mask;
        XEvent event;

        event_mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
                | PointerMotionMask | KeyPressMask;
        dpy = xtee_gui_get_display();
        XFlush(dpy);
        xtee_gui_update_display();
        while (XCheckMaskEvent(dpy, event_mask, &event)) {
                /* Ignore events. */
        }
}

int
xtee_gui_fork_exec(char *pathname, char **args, size_t args_n,
                const XteeEnvData *env_data, size_t env_data_n)
{
        pid_t pid;
        int rv;

        switch (pid = fork()) {
                case -1:
                        return -1;
                case 0:
                        exit(xtee_gui_exec(pathname, args, args_n, env_data,
                                                env_data_n));
                default:
                        print_info("Waiting for child process to end");
                        while (true) {
                                struct timespec req = { 0,
                                        XTEE_BUSY_NANOSLEEP };
                                int wait_rtrn;

                                xtee_gui_handle_busy_events();
                                wait_rtrn = waitpid(-1, &rv, 0 | WNOHANG);
                                if(wait_rtrn == -1) {
                                        return -1;
                                } else if(wait_rtrn != 0) {
                                        break;
                                }
                                nanosleep(&req, NULL);

                        }
                        if ((WEXITSTATUS(rv)) != 0) {
                                return -1;
                        }
                        break;
        }
        return 0;
}

int
xtee_gui_get_selected_program_string(XteeString *str)
{
        XteePrefix *loaded_prefix;
        unsigned int *list_positions;
        int list_positions_n;
        Widget programs_list;

        loaded_prefix = xtee_state_get_loaded_prefix();
        programs_list = main_gui_data.work_area_struct.programs_list;
        XtVaGetValues(programs_list,
                        XmNselectedPositionCount, &list_positions_n,
                        XmNselectedPositions, &list_positions,
                        NULL);
        if (list_positions_n > 1) {
                fatal_error("Multiple selections in programs list.");
        } else if (list_positions_n == 0) {
                return -1;
        }
        *str = loaded_prefix->programs_ids[*list_positions - 1];
        return 0;
}

void
xtee_gui_print_get_program_data_error(enum XteeGetProgramDataStatus status)
{
        switch (status) {
                case XTEE_GET_PROGRAM_DATA_NOT_EXIST:
                        gui_print_error_log_and_dialog(
                                        "Executable does not exist.",
                                        main_gui_data.main_win);
                        break;
                case XTEE_GET_PROGRAM_DATA_NO_EXE_PATH:
                        gui_print_error_log_and_dialog(
                                        "Missing executable path in "
                                        "program configuration file.",
                                        main_gui_data.main_win);
                        break;
                case XTEE_GET_PROGRAM_DATA_READ_ERROR:
                        gui_print_error_log_and_dialog(
                                        "Program configuration file "
                                        "reading error.",
                                        main_gui_data.main_win);
                        break;
                default:
                        gui_print_error_log_and_dialog(
                                        "Cannot start program",
                                        main_gui_data.main_win);
                        break;
        }
}
