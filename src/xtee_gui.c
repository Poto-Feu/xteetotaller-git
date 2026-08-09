#include "xtee_gui.h"
#include "xtee_error.h"
#include "xtee_file.h"
#include "xtee_gui_form.h"
#include "xtee_gui_new_prefix.h"
#include "xtee_gui_prefix_options.h"
#include "xtee_gui_program_options.h"
#include "xtee_state.h"
#include "xtee_system.h"
#include "xtee_utils.h"
#include "xtee_wine.h"
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
        Widget prefix_frame;
        Widget prefix_frame_title;
        Widget prefix_form;
        Widget prefix_btns_row;
        Widget reload_prefix_btn;
        Widget prefix_options_btn;
        Widget new_program_btn;
        Widget run_exec_in_prefix_btn;
        Widget winetricks_btn;
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
        XteeGuiNewProgramData new_program_data;
        XteeGuiPlaceholderDialog prefix_options_dialog;
        XtAppContext app_context;
        Widget toplevel;
        Widget main_win;
        Widget menubar;
        Widget file_menu;
        Widget edit_menu;
        Widget help_menu;
        Widget message_area_label;
} XteeGuiMainData;

XmRenderTable xtee_gui_render_table;

static struct XteeGuiMainData main_gui_data = { 0 };

static XteePushButtonData programs_btns_data[] = {
        { "start_program_btn", "Start program",
                &main_gui_data.work_area_struct.start_program_btn },
        { "program_options_btn", "Program options",
                &main_gui_data.work_area_struct.program_options_btn }
};

static int start_program_btn_i = 0;
static int program_options_btn_i = 1;

static XteePushButtonData prefix_btns_data[] = {
        { "reload_prefix_btn", "Reload prefix",
                &main_gui_data.work_area_struct.reload_prefix_btn },
        { "prefix_options_btn", "Prefix options",
                &main_gui_data.work_area_struct.prefix_options_btn },
        { "new_program_btn", "New program",
                &main_gui_data.work_area_struct.new_program_btn },
        { "run_exec_in_prefix_btn", "Run executable in prefix",
                &main_gui_data.work_area_struct.run_exec_in_prefix_btn },
        { "winetricks_btn", "Winetricks",
                &main_gui_data.work_area_struct.winetricks_btn },
};

static Widget run_exec_browse_dialog = NULL;
static Widget main_working_dialog = NULL;

static const size_t show_hidden_files_i = 0;

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
                        && type != XmDIALOG_WARNING) {
                type = XmDIALOG_MESSAGE;
        }

        dialog_title = XmStringCreateLocalized("Error");
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
xtee_gui_set_message_area_string(char *s)
{
        xtee_gui_set_motif_label_string(main_gui_data.message_area_label, s);
}

#define XTEE_UNFINISHED_APP_CONTENT \
"For testing purposes only. DO NOT DISTRIBUTE.\n\
You may encounter bugs or unimplemented features.\n\n\
Please note that xteetotaller is vulnerable to TOCTOU race conditions.\n\
Do not try to rename or modify configurations files during filesystem \
operations."

static
XTEE_XT_CALLBACK_FUNC(unfinished_app_dialog_unmap_cb)
{
        Widget dont_show_again;

        (void)client_data;
        (void)call_data;

        if ((dont_show_again = XtNameToWidget(w, "dont_show_again"))) {
                unsigned char is_set;

                XtVaGetValues(dont_show_again, XmNset, &is_set, NULL);
                if (is_set == XmSET) {
                        xtee_config_set_entry_value_and_write(
                                        "gui.show_unfinished_app_dialog",
                                        "false");
                        print_info("gui.show_unfinished_app_dialog set to "
                                        "false");
                } else if (is_set == XmUNSET) {
                        xtee_config_set_entry_value_and_write(
                                        "gui.show_unfinished_app_dialog",
                                        "true");
                        print_info("gui.show_unfinshed_app_dialog set to "
                                        "true");
                }
        }

        XtDestroyWidget(XtParent(w));
}

static void
show_unfinished_app_dialog(Widget parent)
{
        Arg args[10];
        Widget dialog, dont_show_again;
        XmString dialog_title, dialog_text;
        unsigned int n = 0;

        dialog_title = XmStringCreateLocalized("Unfinished application");
        dialog_text = XmStringCreateLocalized(XTEE_UNFINISHED_APP_CONTENT);
        set_message_dialog_args(args, &n, dialog_title, dialog_text);
        dialog = XmCreateInformationDialog(parent, "unfinished_app_dialog",
                        args, n);
        XmStringFree(dialog_title);
        XmStringFree(dialog_text);

        dont_show_again = XmVaCreateToggleButtonGadget(dialog,
                        "dont_show_again",
                        XmNalignment, XmALIGNMENT_BEGINNING,
                        XmNspacing, 20,
                        NULL);
        xtee_gui_set_motif_label_string(dont_show_again,
                        "Don't show this again");
        XtManageChild(dont_show_again);

        xtee_gui_hide_dialog_cancel_button(dialog);
        xtee_gui_disable_dialog_help_button(dialog);
        XtAddCallback(dialog, XmNunmapCallback, unfinished_app_dialog_unmap_cb,
                        NULL);
        XtManageChild(dialog);
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
disable_prefix_btns(void)
{
        for (size_t i = 0; i < XTEE_ARRAY_LEN(prefix_btns_data); ++i) {
                XtSetSensitive(*(prefix_btns_data[i].w), False);
        }
}

static void
enable_prefix_btns(void)
{
        for (size_t i = 0; i < XTEE_ARRAY_LEN(prefix_btns_data); ++i) {
                XtSetSensitive(*(prefix_btns_data[i].w), True);
        }
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

static int
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
        disable_prefix_btns();
        disable_programs_btns();
        reset_prefix_info_label();
}

static void
refresh_prefix_gui(void)
{
        if (refresh_program_list() != 0) {
                return;
        }
        enable_prefix_btns();
        disable_programs_btns();
        update_prefix_info_label();
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

static
XTEE_XT_CALLBACK_FUNC(start_program_btn_cb)
{
        XteeEnvData env_data[1];
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
        enum XteeGetProgramDataStatus status;
        int list_positions_n;
        int r;
        Widget programs_list;

        (void)w;
        (void)call_data;
        (void)client_data;
        loaded_prefix = xtee_state_get_loaded_prefix();
        version_data = xtee_wine_get_version_data(
                        loaded_prefix->wine_version.s);
        programs_list = main_gui_data.work_area_struct.programs_list;

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
        xtee_free_program(program_data);
        free(program_data);
        env_data[0].name = "WINEPREFIX";
        env_data[0].value = loaded_prefix->wineprefix_dir.s;
        xtee_prefix_set_WINEDLLOVERRIDES_env(loaded_prefix);
        r = xtee_gui_fork_exec(version_data->wine_path.s,
                        args_ptr, args_n,
                        env_data, XTEE_ARRAY_LEN(env_data));
        free(args_ptr[2]);
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
        unsetenv("WINEDLLOVERRIDES");
        hide_main_working_dialog();
}

static
XTEE_XT_CALLBACK_FUNC(program_options_btn_cb)
{
        (void)w;
        (void)call_data;
        (void)client_data;
        xtee_gui_show_program_options_dialog(main_gui_data.main_win);
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
                        NULL);
        XtAddCallback(*programs_btns_data[program_options_btn_i].w,
                        XmNactivateCallback, program_options_btn_cb,
                        NULL);

        XtManageChild(wa_struct->programs_buttons);
        XtManageChild(wa_struct->programs_form);
        XtManageChild(wa_struct->programs_frame);
}


static
XTEE_XT_CALLBACK_FUNC(reload_prefix_btn_cb)
{
        XteePrefix *current_prefix;
        (void)w;
        (void)client_data;
        (void)call_data;

        current_prefix = xtee_state_get_loaded_prefix();
        reset_main_win_state();

        if (xtee_gui_load_prefix(current_prefix->id.s, main_gui_data.main_win)
                        != 0) {
                return;
        }
        xtee_gui_set_message_area_string("Prefix reloaded.");
}

static void
show_prefix_options_dialog(void)
{
        xtee_gui_show_prefix_options_dialog(main_gui_data.main_win);
}

static
XTEE_XT_CALLBACK_FUNC(prefix_options_btn_cb)
{
        (void)w;
        (void)client_data;
        (void)call_data;
        show_prefix_options_dialog();
}

#define XTEE_NEW_PROGRAM_ID_FIELD_I 0
#define XTEE_NEW_PROGRAM_EXEPATH_FIELD_I 1
static
XTEE_XT_CALLBACK_FUNC(new_program_ok_cb)
{
        const size_t id_field_i = XTEE_NEW_PROGRAM_ID_FIELD_I;
        const size_t exe_path_field_i = XTEE_NEW_PROGRAM_EXEPATH_FIELD_I;
        XteePrefix *current_prefix;
        XteeProgram program;
        XteeConstString exe_path_xteestr;
        XteeGuiNewProgramData *new_program_data;
        XteeGuiBasicFormDialog *dialog;
        XteeGuiFormField *id_field, *exe_path_field;
        char *id_str, *exe_path_str;
        enum XteeAddProgramStatus status;

        (void)w;
        (void)call_data;
        new_program_data = (XteeGuiNewProgramData *)client_data;
        dialog = &new_program_data->dialog;
        current_prefix = xtee_state_get_loaded_prefix();

        id_field = &dialog->fields[id_field_i];
        id_str = get_XteeGuiFormField_string_value(id_field);
        if (!id_str) {
                fatal_error("program id_str is NULL (alloc error?)");
        } else if (id_str[0] == '\0') {
                gui_print_error_log_and_dialog("Program id is empty.",
                                dialog->dialog_form);
                XtFree(id_str);
                return;
        } else if (strlen(id_str) > XTEE_PROGRAM_ID_MAX_LEN) {
                XtFree(id_str);
                gui_print_error_log_and_dialog("Program id is too long.",
                                dialog->dialog_form);
                return;
        }

        exe_path_field = &dialog->fields[exe_path_field_i];
        exe_path_str = get_XteeGuiFormField_string_value(exe_path_field);

        if (!exe_path_str) {
                fatal_error("exe_path_str is NULL (alloc error?)");
        } else if (exe_path_str[0] == '\0') {
                gui_print_error_log_and_dialog("Program executable path is "
                                "empty.",
                                dialog->dialog_form);
                XtFree(id_str);
                XtFree(exe_path_str);
                return;
        }

        exe_path_xteestr.s = exe_path_str;
        exe_path_xteestr.n = strlen(exe_path_str);
        if (!xtee_is_file_msi(exe_path_xteestr)) {
                enum XteeExecutableType exe_type;

                if (xtee_get_executable_type(&exe_type, exe_path_xteestr)
                                != 0) {
                        gui_print_error_log_and_dialog("Cannot read "
                                        "executable file.",
                                        dialog->dialog_form);
                        XtFree(id_str);
                        XtFree(exe_path_str);
                        return;
                } else if (exe_type == XTEE_EXECUTABLE_UNKNOWN) {
                        gui_print_error_log_and_dialog("Incorrect executable "
                                        "format.",
                                        dialog->dialog_form);
                        XtFree(id_str);
                        XtFree(exe_path_str);
                        return;
                }
        }

        xtee_init_program(&program, id_str, exe_path_str);
        status = add_program_to_prefix(current_prefix, &program);
        xtee_free_program(&program);
        XtFree(id_str);
        XtFree(exe_path_str);
        switch(status) {
                case XTEE_ADD_PROGRAM_ID_EXIST:
                        gui_print_error_log_and_dialog("Program id is already "
                                        "in use.",
                                        dialog->dialog_form);
                        return;
                case XTEE_ADD_PROGRAM_WRITE_ERROR:
                        gui_print_error_log_and_dialog("Cannot write program "
                                        "configuration.",
                                        dialog->dialog_form);
                        return;
                default:
                        xtee_gui_set_message_area_string(
                                        "New program created.");
                        XtUnmanageChild(dialog->dialog_form);
        }

        if (xtee_prefix_reload_programs_list(current_prefix) != 0) {
                gui_print_error_log_and_dialog("Cannot get prefix programs "
                                "list.", main_gui_data.main_win);
        }
        refresh_program_list();

}

static
XTEE_XT_CALLBACK_FUNC(new_program_cancel_cb)
{
        (void)w;
        (void)call_data;
        XtUnmanageChild((Widget)client_data);
}

static void
show_new_program_dialog(void)
{
        static bool is_dialog_ready;
        struct XteeGuiNewProgramData *new_program_data;

        new_program_data = &main_gui_data.new_program_data;

        if (!is_dialog_ready) {
                const size_t exe_path_field_i = 1;
                XmString tmp_str;
                XteeGuiLTPF *dir_form;
                XteeGuiBasicFormDialog *dialog;

                create_XteeGuiBasicFormDialog(&new_program_data->dialog,
                                main_gui_data.main_win,
                                "new_program_dialog",
                                "New Program");
                add_simple_form_field_to_XteeGuiBasicFormDialog(
                                &new_program_data->dialog,
                                XTEE_GUI_FORM_FIELD_LABELTEXT,
                                "program_id_form", "Id: ");
                add_simple_form_field_to_XteeGuiBasicFormDialog(
                                &new_program_data->dialog,
                                XTEE_GUI_FORM_FIELD_LTPF,
                                "program_exepath_form",
                                "Executable path: ");
                dialog = &new_program_data->dialog;
                dir_form = &dialog->fields[exe_path_field_i].data.ltpf;
                tmp_str = XmStringCreateLocalized("Program file selection");
                XtVaSetValues(dir_form->browse_dialog,
                                XmNfileTypeMask, XmFILE_REGULAR,
                                XmNdialogTitle, tmp_str,
                                NULL);
                XtAddCallback(new_program_data->dialog.ok_btn,
                                XmNactivateCallback,
                                new_program_ok_cb,
                                new_program_data);
                XtAddCallback(new_program_data->dialog.cancel_btn,
                                XmNactivateCallback,
                                new_program_cancel_cb,
                                new_program_data->dialog.dialog_form);
                XmStringFree(tmp_str);
                manage_XteeGuiBasicFormDialog_fields(
                                &new_program_data->dialog);
                is_dialog_ready = true;
        } else {
                reset_XteeGuiBasicFormDialog_fields(&new_program_data->dialog);
        }
        XtManageChild(new_program_data->dialog.dialog_form);
}

static
XTEE_XT_CALLBACK_FUNC(new_program_btn_cb)
{
        (void)w;
        (void)client_data;
        (void)call_data;
        show_new_program_dialog();
}

static int
fork_exec_run_executable(XteePrefix *prefix, XteeString path_str)
{
        XteeEnvData env_data[1];
        char *args[3];
        XteeWineVersionData *version_data;
        const size_t args_n = XTEE_ARRAY_LEN(args);
        int r;

        version_data = xtee_wine_get_version_data(prefix->wine_version.s);
        args[0] = "start";
        args[1] = "/unix";
        args[2] = path_str.s;
        env_data->name = "WINEPREFIX";
        env_data->value = prefix->wineprefix_dir.s;
        xtee_prefix_set_WINEDLLOVERRIDES_env(prefix);
        print_info("Running executable in Wine prefix.");
        r = xtee_gui_fork_exec(version_data->wine_path.s, args, args_n,
                        env_data, XTEE_ARRAY_LEN(env_data));
        unsetenv("WINEDLLOVERRIDES");
        return r;
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

static
XTEE_XT_CALLBACK_FUNC(run_exec_in_prefix_btn_cb)
{
        char *gui_show_hidden_files_val;

        (void)w;
        (void)client_data;
        (void)call_data;

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
XTEE_XT_CALLBACK_FUNC(winetricks_btn_cb)
{
        XteeEnvData env_data[3];
        XteePrefix *loaded_prefix;
        XteeWineVersionData *version_data;
        int r;

        (void)w;
        (void)client_data;
        (void)call_data;

        if (!xtee_system_is_winetricks_installed) {
                gui_print_error_log_and_dialog("Winetricks is not installed.",
                                main_gui_data.main_win);
                return;
        }

        loaded_prefix = xtee_state_get_loaded_prefix();
        version_data = xtee_wine_get_version_data(
                        loaded_prefix->wine_version.s);
        set_and_show_main_working_dialog("Running winetricks",
                        "Running winetricks");
        env_data[0].name = "WINE";
        env_data[0].value = version_data->wine_path.s;
        env_data[1].name = "WINESERVER";
        env_data[1].value = version_data->wineserver_path.s;
        env_data[2].name = "WINEPREFIX";
        env_data[2].value = loaded_prefix->wineprefix_dir.s;

        xtee_prefix_set_WINEDLLOVERRIDES_env(loaded_prefix);
        r = xtee_gui_fork_exec((char *)xtee_system_winetricks_path.s, NULL, 0,
                                env_data, XTEE_ARRAY_LEN(env_data));
        unsetenv("WINEDLLOVERRIDES");
        hide_main_working_dialog();
        if (r != 0) {
                gui_print_error_log_and_dialog("Cannot run winetricks.",
                                main_gui_data.main_win);
        }
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
        wa_struct->prefix_btns_row = XtVaCreateWidget(
                        "prefix_btns_row",
                        xmRowColumnWidgetClass,
                        wa_struct->prefix_form,
                        XmNorientation, XmHORIZONTAL,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNrightAttachment, XmATTACH_FORM,
                        NULL);
        for (size_t i = 0; i < XTEE_ARRAY_LEN(prefix_btns_data); ++i) {
                XmString btn_str;

                btn_str = XmStringCreateLocalized(prefix_btns_data[i].label);
                *(prefix_btns_data[i].w) = XtVaCreateManagedWidget(
                                prefix_btns_data[i].id,
                                xmPushButtonWidgetClass,
                                wa_struct->prefix_btns_row,
                                XmNlabelString, btn_str,
                                XmNmarginLeft, button_lr_margin,
                                XmNmarginRight, button_lr_margin,
                                NULL);
                XmStringFree(btn_str);
        }
        XtAddCallback(main_gui_data.work_area_struct.reload_prefix_btn,
                        XmNactivateCallback, reload_prefix_btn_cb, NULL);
        XtAddCallback(main_gui_data.work_area_struct.prefix_options_btn,
                        XmNactivateCallback, prefix_options_btn_cb, NULL);
        XtAddCallback(main_gui_data.work_area_struct.new_program_btn,
                        XmNactivateCallback, new_program_btn_cb, NULL);
        XtAddCallback(main_gui_data.work_area_struct.run_exec_in_prefix_btn,
                        XmNactivateCallback, run_exec_in_prefix_btn_cb, NULL);
        XtAddCallback(main_gui_data.work_area_struct.winetricks_btn,
                        XmNactivateCallback, winetricks_btn_cb, NULL);
        wa_struct->prefix_info_label = XtVaCreateManagedWidget(
                        "prefix_info_label", xmLabelGadgetClass,
                        wa_struct->prefix_form,
                        XmNalignment, XmALIGNMENT_BEGINNING,
                        XmNtopAttachment, XmATTACH_WIDGET,
                        XmNtopWidget, wa_struct->prefix_btns_row,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNbottomOffset, 5,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNleftOffset, button_lr_margin,
                        NULL);
        reset_prefix_info_label();

        XtManageChild(wa_struct->prefix_btns_row);
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
                show_new_prefix_dialog(main_gui_data.main_win);
        } else if (item_i == open_prefix_i) {
                show_open_prefix_dialog();
        } else if (item_i == exit_i) {
                exit(0);
        }
}

static
XTEE_XT_CALLBACK_FUNC(prefs_dialog_ok_cb)
{
        XteeGuiBasicFormDialog *prefs_dialog;
        XteeGuiFormField *show_hidden_files_field;
        Boolean show_hidden_files_bool;
        const char *show_hidden_files_str;

        prefs_dialog = (XteeGuiBasicFormDialog *)client_data;
        (void)w;
        (void)call_data;
        show_hidden_files_field = prefs_dialog->fields + show_hidden_files_i;
        show_hidden_files_bool = !!(XmToggleButtonGadgetGetState(
                        show_hidden_files_field->data.toggle_g));
        show_hidden_files_str = xtee_bool_to_string(show_hidden_files_bool);
        xtee_config_set_entry_value_and_write("gui.show_hidden_files",
                        show_hidden_files_str);
        xtee_gui_set_message_area_string("Configuration modified and saved.");
        XtUnmanageChild(prefs_dialog->dialog_form);
}

static
XTEE_XT_CALLBACK_FUNC(prefs_dialog_cancel_cb)
{
        XteeGuiBasicFormDialog *prefs_dialog;

        prefs_dialog = (XteeGuiBasicFormDialog *)client_data;
        (void)w;
        (void)call_data;
        XtUnmanageChild(prefs_dialog->dialog_form);
}

static void
gui_show_preferences(Widget parent)
{
        static bool is_dialog_ready = false;
        XteeGuiBasicFormDialog *prefs_dialog;
        XteeGuiFormField *show_hidden_files_field;
        const char *gui_show_hidden_files_val;

        prefs_dialog = &main_gui_data.prefs_dialog;

        if (!is_dialog_ready) {
                create_XteeGuiBasicFormDialog(prefs_dialog, parent,
                                "preferences_dialog",
                                "XTeetotaller preferences");
                add_simple_form_field_to_XteeGuiBasicFormDialog(
                                prefs_dialog,
                                XTEE_GUI_FORM_FIELD_TOGGLE,
                                "show_hidden_files_toggle",
                                "Show hidden files on File Selection "
                                "dialogs.");
                XtAddCallback(prefs_dialog->ok_btn, XmNactivateCallback,
                                prefs_dialog_ok_cb,
                                prefs_dialog);
                XtAddCallback(prefs_dialog->cancel_btn, XmNactivateCallback,
                                prefs_dialog_cancel_cb,
                                prefs_dialog);
                manage_XteeGuiBasicFormDialog_fields(prefs_dialog);
                is_dialog_ready = true;
        }
        gui_show_hidden_files_val = xtee_config_get_entry_value(
                        "gui.show_hidden_files");
        show_hidden_files_field = prefs_dialog->fields + show_hidden_files_i;
        if (!gui_show_hidden_files_val
                        || !xtee_string_to_bool(gui_show_hidden_files_val)) {
                XmToggleButtonGadgetSetState(
                                show_hidden_files_field->data.toggle_g,
                                False, False);
        } else {
                XmToggleButtonGadgetSetState(
                                show_hidden_files_field->data.toggle_g,
                                True, False);
        }
        XtManageChild(prefs_dialog->dialog_form);
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
                gui_show_preferences(main_gui_data.main_win);
        }
}

#define ABOUT_DIALOG_TEXT_CONTENT \
"XTeetotaller, a Wine prefixes and programs manager for UNIX and UNIX-like \
operating systems.\n\
\n\
Created by Adrien Saad.\n\n\
For testing purposes only. DO NOT DISTRIBUTE."

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
        XmString file_str, edit_str, help_str;
        XmString new_prefix_str;
        XmString open_prefix_str, exit_str;
        XmString preferences_str;
        XmString about_str;
        const char *help_button_id = "button_2";
        const int file_button_i = 0;
        const int edit_button_i = 1;
        const int help_button_i = 2;

        file_str = XmStringCreateLocalized("File");
        edit_str = XmStringCreateLocalized("Edit");
        help_str = XmStringCreateLocalized("Help");
        main_gui_data.menubar = XmVaCreateSimpleMenuBar(main_gui_data.main_win,
                        "menubar",
                        XmVaCASCADEBUTTON, file_str, 'F',
                        XmVaCASCADEBUTTON, edit_str, 'E',
                        XmVaCASCADEBUTTON, help_str, 'H',
                        NULL);
        XmStringFree(file_str);
        XmStringFree(edit_str);
        XmStringFree(help_str);

        if ((tmp_w = XtNameToWidget(main_gui_data.menubar, help_button_id))
                        != (Widget)0) {
                XtVaSetValues(main_gui_data.menubar, XmNmenuHelpWidget, tmp_w,
                                NULL);
        }

        new_prefix_str = XmStringCreateLocalized("New Prefix");
        open_prefix_str = XmStringCreateLocalized("Open prefix");
        exit_str = XmStringCreateLocalized("Exit");
        preferences_str = XmStringCreateLocalized("Preferences");
        about_str = XmStringCreateLocalized("About XTeetotaller");
        main_gui_data.file_menu = XmVaCreateSimplePulldownMenu(
                        main_gui_data.menubar, "file_menu", file_button_i,
                        file_menu_cb,
                        XmVaPUSHBUTTON, new_prefix_str, 'N', NULL, NULL,
                        XmVaPUSHBUTTON, open_prefix_str, 'O', NULL, NULL,
                        XmVaSEPARATOR,
                        XmVaPUSHBUTTON, exit_str, 'x', NULL, NULL,
                        NULL);
        main_gui_data.edit_menu = XmVaCreateSimplePulldownMenu(
                        main_gui_data.menubar, "edit_menu", edit_button_i,
                        edit_menu_cb,
                        XmVaPUSHBUTTON, preferences_str, 'e', NULL, NULL,
                        NULL);
        main_gui_data.help_menu = XmVaCreateSimplePulldownMenu(
                        main_gui_data.menubar, "help_menu", help_button_i,
                        help_menu_cb,
                        XmVaPUSHBUTTON, about_str, 'A', NULL, NULL,
                        NULL);
        XmStringFree(new_prefix_str);
        XmStringFree(open_prefix_str);
        XmStringFree(exit_str);
        XmStringFree(preferences_str);
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
                        XmNwidth, 800,
                        XmNheight, 600,
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
        disable_prefix_btns();
}

static
XTEE_XT_CALLBACK_FUNC(shell_close_callback)
{
        (void)w;
        (void)call_data;
        (void)client_data;
        print_info("Closing window shell");
        XtDestroyWidget(w);
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
        XtSetLanguageProc(NULL, NULL, NULL);
        main_gui_data.toplevel = XtVaOpenApplication(
                        &main_gui_data.app_context, "XTeetotaller", NULL, 0,
                        argc, argv, NULL,
                        sessionShellWidgetClass, NULL);
        set_window_close_callback(main_gui_data.toplevel);
        create_xtee_render_table();

        gui_create_main_application();
}

void
xtee_gui_run(void)
{
        const char *config_val;

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

        if(!(config_val = xtee_config_get_entry_value(
                                        "gui.show_unfinished_app_dialog"))
                        || !xtee_strcmp_bool(config_val, "false")) {
                show_unfinished_app_dialog(main_gui_data.main_win);
        } else {
                print_info("Not showing unfinished application dialog "
                                "per user setting");
        }
        XtAppMainLoop(main_gui_data.app_context);
}

void
xtee_gui_free(void)
{
        xtee_gui_new_prefix_free();
        xtee_gui_prefix_options_free();
        xtee_gui_program_options_free();
        free_XteeGuiBasicFormDialog(&main_gui_data.new_program_data.dialog);
        /*XtDestroyWidget(main_gui_data.toplevel);*/
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
                setenv(env_data[i].name, env_data[i].value, 1);
        }
        r = execv(pathname, exec_args);
        free(exec_args);
        for (size_t i = 0; i < env_data_n; ++i) {
                unsetenv(env_data[i].name);
        }
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
