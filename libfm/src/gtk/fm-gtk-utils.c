/*
 *      fm-gtk-utils.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2012-2016 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *      Copyright 2012 Vadim Ushakov <igeekless@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

/**
 * SECTION:fm-gtk-utils
 * @short_description: Different widgets and utilities that use GTK+
 * @title: Libfm-gtk utils
 *
 * @include: libfm/fm-gtk.h
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "gtk-compat.h"

#include <glib/gi18n-lib.h>
#include <gio/gdesktopappinfo.h>

#include "fm-gtk-utils.h"
#include "fm-file-ops-job.h"
#include "fm-progress-dlg.h"
#include "fm-path-entry.h"
#include "fm-app-chooser-dlg.h"
#include "fm-monitor.h"

#include "fm-config.h"

static GtkDialog*   _fm_get_user_input_dialog   (GtkWindow* parent, const char* title, const char* msg);
static gchar*       _fm_user_input_dialog_run   (GtkDialog* dlg, GtkEntry *entry, GtkWidget *extra);

/**
 * fm_show_error
 * @parent: a window to place dialog over it
 * @title: title for dialog window
 * @msg: message to present
 *
 * Presents error message to user and gives user no choices but close.
 *
 * Before 0.1.16 this call had different arguments.
 *
 * Since: 0.1.0
 */
void fm_show_error(GtkWindow* parent, const char* title, const char* msg)
{
    GtkWidget* dlg = gtk_message_dialog_new(parent, 0,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK, "%s", msg);
    /* g_message("(!) %s", msg); */
    gtk_window_set_title(GTK_WINDOW(dlg), title ? title : _("Error"));
    /* #3606577: error window if parent is desktop is below other windows */
    gtk_window_set_keep_above(GTK_WINDOW(dlg), TRUE);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

/**
 * fm_yes_no
 * @parent: a window to place dialog over it
 * @title: title for dialog window
 * @question: the question to present to the user
 * @default_yes: the default answer
 *
 * Presents the question to user and gives user choices 'Yes' and 'No'.
 *
 * Before 0.1.16 this call had different arguments.
 *
 * Returns: %TRUE if user chose 'Yes'.
 *
 * Since: 0.1.0
 */
gboolean fm_yes_no(GtkWindow* parent, const char* title, const char* question, gboolean default_yes)
{
    int ret;
    GtkDialog* dlg = GTK_DIALOG(gtk_message_dialog_new_with_markup(parent, 0,
                                                        GTK_MESSAGE_QUESTION,
                                                        GTK_BUTTONS_YES_NO,
                                                        "%s", question));
    gtk_window_set_title(GTK_WINDOW(dlg), title ? title : _("Confirm"));
    gtk_dialog_set_default_response(dlg, default_yes ? GTK_RESPONSE_YES : GTK_RESPONSE_NO);
    /* #3300797: Delete prompt isn't on the first layer */
    gtk_window_set_keep_above(GTK_WINDOW(dlg), TRUE);
    ret = gtk_dialog_run(dlg);
    gtk_widget_destroy((GtkWidget*)dlg);
    return ret == GTK_RESPONSE_YES;
}

/**
 * fm_ok_cancel
 * @parent: a window to place dialog over it
 * @title: title for dialog window
 * @question: the question to show to the user
 * @default_ok: the default answer
 *
 * Presents the question to user and gives user choices 'OK' and 'Cancel'.
 *
 * Before 0.1.16 this call had different arguments.
 *
 * Returns: %TRUE if user chose 'OK'.
 *
 * Since: 0.1.0
 */
gboolean fm_ok_cancel(GtkWindow* parent, const char* title, const char* question, gboolean default_ok)
{
    int ret;
    GtkDialog* dlg = GTK_DIALOG(gtk_message_dialog_new_with_markup(parent, 0,
                                                        GTK_MESSAGE_QUESTION,
                                                        GTK_BUTTONS_OK_CANCEL,
                                                        "%s", question));
    gtk_window_set_title(GTK_WINDOW(dlg), title ? title : _("Confirm"));
    gtk_dialog_set_default_response(dlg, default_ok ? GTK_RESPONSE_OK : GTK_RESPONSE_CANCEL);
    ret = gtk_dialog_run(dlg);
    gtk_widget_destroy((GtkWidget*)dlg);
    return ret == GTK_RESPONSE_OK;
}

/**
 * fm_ask
 * @parent: toplevel parent widget
 * @title: title for the window with question
 * @question: the question to show to the user
 * @...: a NULL terminated list of button labels
 *
 * Ask the user a question with several options provided.
 *
 * Before 0.1.16 this call had different arguments.
 *
 * Return value: the index of selected button, or -1 if the dialog is closed.
 *
 * Since: 0.1.0
 */
int fm_ask(GtkWindow* parent, const char* title, const char* question, ...)
{
    int ret;
    va_list args;
    va_start (args, question);
    ret = fm_ask_valist(parent, title, question, args);
    va_end (args);
    return ret;
}

/**
 * fm_askv
 * @parent: toplevel parent widget
 * @title: title for the window with question
 * @question: the question to show to the user
 * @options: a NULL terminated list of button labels
 *
 * Ask the user a question with several options provided.
 *
 * Before 0.1.16 this call had different arguments.
 *
 * Return value: the index of selected button, or -1 if the dialog is closed.
 *
 * Since: 0.1.0
 */
int fm_askv(GtkWindow* parent, const char* title, const char* question, char* const* options)
{
    int ret;
    guint id = 1;
    GtkDialog* dlg = GTK_DIALOG(gtk_message_dialog_new_with_markup(parent, 0,
                                                        GTK_MESSAGE_QUESTION, 0,
                                                        "%s", question));
    gtk_window_set_title(GTK_WINDOW(dlg), title ? title : _("Question"));
    /* FIXME: need to handle defualt button and alternative button
     * order problems. */
    while(*options)
    {
        /* FIXME: handle button image and stock buttons */
        /*GtkWidget* btn =*/
        gtk_dialog_add_button(dlg, *options, id);
        ++options;
        ++id;
    }
    ret = gtk_dialog_run(dlg);
    if(ret >= 1)
        ret -= 1;
    else
        ret = -1;
    gtk_widget_destroy((GtkWidget*)dlg);
    return ret;
}

/**
 * fm_ask_valist
 * @parent: toplevel parent widget
 * @title: title for the window with question
 * @question: the question to show to the user
 * @options: va_arg list of button labels
 *
 * Ask the user a question with several options provided.
 *
 * Before 0.1.16 this call had different arguments.
 *
 * Return value: the index of selected button, or -1 if the dialog is closed.
 *
 * Since: 0.1.0
 */
int fm_ask_valist(GtkWindow* parent, const char* title, const char* question, va_list options)
{
    GArray* opts = g_array_sized_new(TRUE, TRUE, sizeof(char*), 6);
    gint ret;
    const char* opt = va_arg(options, const char*);
    while(opt)
    {
        g_array_append_val(opts, opt);
        opt = va_arg (options, const char *);
    }
    ret = fm_askv(parent, title, question, &opts->data);
    g_array_free(opts, TRUE);
    return ret;
}


/**
 * fm_get_user_input
 * @parent: a window to place dialog over it
 * @title: title for dialog window
 * @msg: the message to present to the user
 * @default_text: the default answer
 *
 * Presents the message to user and retrieves entered text.
 * Returned data should be freed with g_free() after usage.
 *
 * Returns: (transfer full): entered text.
 *
 * Since: 0.1.0
 */
gchar* fm_get_user_input(GtkWindow* parent, const char* title, const char* msg, const char* default_text)
{
    GtkDialog* dlg = _fm_get_user_input_dialog( parent, title, msg);
    GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_activates_default(entry, TRUE);

    if(default_text && default_text[0])
        gtk_entry_set_text(entry, default_text);

    return _fm_user_input_dialog_run(dlg, entry, NULL);
}

/**
 * fm_get_user_input_n
 * @parent: a window to place dialog over it
 * @title: title for dialog window
 * @msg: the message to present to the user
 * @default_text: the default answer
 * @n: which part of default answer should be selected
 * @extra: additional widgets to display (can be focusable)
 *
 * Presents the message to user and retrieves entered text.
 * In presented dialog the part of @default_text with length @n will be
 * preselected for edition (n < 0 means select all).
 * Returned data should be freed with g_free() after usage.
 *
 * Returns: (transfer full): entered text.
 *
 * Since: 1.2.0
 */
gchar* fm_get_user_input_n(GtkWindow* parent, const char* title, const char* msg,
                           const char* default_text, gint n, GtkWidget* extra)
{
    GtkDialog* dlg = _fm_get_user_input_dialog( parent, title, msg);
    GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_activates_default(entry, TRUE);

    if(default_text && default_text[0])
    {
        gtk_entry_set_text(entry, default_text);
        gtk_editable_select_region(GTK_EDITABLE(entry), 0, n);
    }

    return _fm_user_input_dialog_run(dlg, entry, extra);
}

/**
 * fm_get_user_input_path
 * @parent: a window to place dialog over it
 * @title: title for dialog window
 * @msg: the message to present to the user
 * @default_path: the default path
 *
 * Presents the message to user and retrieves entered path string.
 * Returned data should be freed with fm_path_unref() after usage.
 *
 * Returns: (transfer full): entered text.
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.2.0:
 */
FmPath* fm_get_user_input_path(GtkWindow* parent, const char* title, const char* msg, FmPath* default_path)
{

    GtkDialog* dlg = _fm_get_user_input_dialog( parent, title, msg);
    GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
    char *str, *path_str = NULL;
    FmPath* path;

    gtk_entry_set_activates_default(entry, TRUE);

    if(default_path)
    {
        path_str = fm_path_display_name(default_path, FALSE);
        gtk_entry_set_text(entry, path_str);
    }

    str = _fm_user_input_dialog_run(dlg, entry, NULL);
    path = fm_path_new_for_str(str);

    g_free(path_str);
    g_free(str);
    return path;
}


static gchar* fm_get_user_input_rename(GtkWindow* parent, const char* title, const char* msg, const char* default_text)
{
    GtkDialog* dlg = _fm_get_user_input_dialog( parent, title, msg);
    GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_activates_default(entry, TRUE);

    if(default_text && default_text[0])
    {
        gtk_entry_set_text(entry, default_text);
        /* only select filename part without extension name. */
        if(default_text[1])
        {
            /* FIXME: handle the special case for *.tar.gz or *.tar.bz2
             * We should exam the file extension with g_content_type_guess, and
             * find out a longest valid extension name.
             * For example, the extension name of foo.tar.gz is .tar.gz, not .gz. */
            const char* dot = g_utf8_strrchr(default_text, -1, '.');
            if(dot)
                gtk_editable_select_region(GTK_EDITABLE(entry), 0, g_utf8_pointer_to_offset(default_text, dot));
            else
                gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
            /*
            const char* dot = default_text;
            while( dot = g_utf8_strchr(dot + 1, -1, '.') )
            {
                gboolean uncertain;
                char* type = g_content_type_guess(dot-1, NULL, 0, &uncertain);
                if(!g_content_type_is_unknown(type))
                {
                    g_free(type);
                    gtk_editable_select_region(entry, 0, g_utf8_pointer_to_offset(default_text, dot));
                    break;
                }
                g_free(type);
            }
            */
        }
    }

    return _fm_user_input_dialog_run(dlg, entry, NULL);
}

static GtkDialog* _fm_get_user_input_dialog(GtkWindow* parent, const char* title, const char* msg)
{
    GtkDialog* dlg = GTK_DIALOG(gtk_dialog_new_with_buttons(title, parent,
#if GTK_CHECK_VERSION(3, 0, 0)
                                0,
#else
                                GTK_DIALOG_NO_SEPARATOR,
#endif
                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                GTK_STOCK_OK, GTK_RESPONSE_OK, NULL));
    GtkWidget* label = gtk_label_new(msg);
    GtkBox* box = (GtkBox*)gtk_dialog_get_content_area(dlg);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

    gtk_dialog_set_alternative_button_order(dlg, GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
    gtk_box_set_spacing(box, 6);
    gtk_box_pack_start(box, label, FALSE, TRUE, 6);

    gtk_container_set_border_width(GTK_CONTAINER(box), 12);
    gtk_container_set_border_width(GTK_CONTAINER(dlg), 5);
    gtk_dialog_set_default_response(dlg, GTK_RESPONSE_OK);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 480, -1);

    return dlg;
}

static gchar* _fm_user_input_dialog_run(GtkDialog* dlg, GtkEntry *entry,
                                        GtkWidget *extra)
{
    char* str = NULL;
    GtkBox *box = GTK_BOX(gtk_dialog_get_content_area(dlg));
    int sel_start, sel_end;
    gboolean has_sel;

    /* FIXME: this workaround is used to overcome bug of gtk+.
     * gtk+ seems to ignore select region and select all text for entry in dialog. */
    has_sel = gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &sel_start, &sel_end);
    gtk_box_pack_start(box, GTK_WIDGET(entry), FALSE, TRUE, extra ? 0 : 6);
    if(extra)
        gtk_box_pack_start(box, extra, FALSE, TRUE, 0);
    gtk_widget_show_all(GTK_WIDGET(dlg));

    if(has_sel)
        gtk_editable_select_region(GTK_EDITABLE(entry), sel_start, sel_end);

    while(gtk_dialog_run(dlg) == GTK_RESPONSE_OK)
    {
        const char* pstr = gtk_entry_get_text(entry);
        if( pstr && *pstr )
        {
            str = g_strdup(pstr);
            break;
        }
    }
    gtk_widget_destroy(GTK_WIDGET(dlg));
    return str;
}

static void on_update_img_preview( GtkFileChooser *chooser, GtkImage* img )
{
    char* file = gtk_file_chooser_get_preview_filename(chooser);
    GdkPixbuf* pix = NULL;
    if(file)
    {
        pix = gdk_pixbuf_new_from_file_at_scale( file, 128, 128, TRUE, NULL );
        g_free( file );
    }
    if(pix)
    {
        gtk_file_chooser_set_preview_widget_active(chooser, TRUE);
        gtk_image_set_from_pixbuf(img, pix);
        g_object_unref(pix);
    }
    else
    {
        gtk_image_clear(img);
        gtk_file_chooser_set_preview_widget_active(chooser, FALSE);
    }
}

static gulong fm_add_image_preview_to_file_chooser(GtkFileChooser* chooser)
{
    GtkWidget* img_preview = gtk_image_new();
    gtk_misc_set_alignment(GTK_MISC(img_preview), 0.5, 0.0);
    gtk_widget_set_size_request(img_preview, 128, 128);
    gtk_file_chooser_set_preview_widget(chooser, img_preview);
    return g_signal_connect(chooser, "update-preview", G_CALLBACK(on_update_img_preview), img_preview);
}

/**
 * fm_select_file
 * @parent: a window to place dialog over it
 * @title: title for dialog window
 * @default_folder: the starting folder path
 * @local_only: %TRUE if select only local paths
 * @show_preview: %TRUE to show file preview
 * @...: (element-type GtkFileFilter): optional filters
 *
 * Presents the message to user and lets him/her to select a file.
 * Returned data should be freed with fm_path_unref() after usage.
 *
 * Returns: (transfer full): selected file path or %NULL if dialog was closed.
 *
 * Since: 1.0.0
 */
/* TODO: support selecting multiple files */
FmPath* fm_select_file(GtkWindow* parent, 
                        const char* title, 
                        const char* default_folder,
                        gboolean local_only,
                        gboolean show_preview,
                        /* filter1, filter2, ..., NULL */ ...)
{
    FmPath* path;
    GtkFileChooser* chooser;
    GtkFileFilter* filter;
    gulong handler_id = 0;
    va_list args;

    chooser = (GtkFileChooser*)gtk_file_chooser_dialog_new(
                                        title, parent, GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        NULL);
    gtk_dialog_set_alternative_button_order(GTK_DIALOG(chooser),
                                        GTK_RESPONSE_CANCEL,
                                        GTK_RESPONSE_OK, NULL);
    if(local_only)
        gtk_file_chooser_set_local_only(chooser, TRUE);

    if(default_folder)
        gtk_file_chooser_set_current_folder(chooser, default_folder);

    va_start(args, show_preview);
    while((filter = va_arg(args, GtkFileFilter*)))
    {
        gtk_file_chooser_add_filter(chooser, filter);
    }
    va_end (args);

    if(show_preview)
        handler_id = fm_add_image_preview_to_file_chooser(chooser);

    if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK)
    {
        GFile* file = gtk_file_chooser_get_file(chooser);
        path = fm_path_new_for_gfile(file);
        g_object_unref(file);
    }
    else
        path = NULL;
    if(handler_id > 0)
        g_signal_handler_disconnect(chooser, handler_id);
    gtk_widget_destroy(GTK_WIDGET(chooser));
    return path;
}

/**
 * fm_select_folder
 * @parent: a window to place dialog over it
 * @title: title for dialog window
 *
 * Presents the message to user and lets him/her to select a folder.
 * Returned data should be freed with fm_path_unref() after usage.
 *
 * Before 0.1.16 this call had different arguments.
 *
 * Returns: (transfer full): selected folder path or %NULL if dialog was closed.
 *
 * Since: 0.1.0
 */
FmPath* fm_select_folder(GtkWindow* parent, const char* title)
{
    FmPath* path;
    GtkFileChooser* chooser;
    chooser = (GtkFileChooser*)gtk_file_chooser_dialog_new(
                                        title ? title : _("Select Folder"),
                                        parent, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        NULL);
    gtk_dialog_set_alternative_button_order(GTK_DIALOG(chooser),
                                        GTK_RESPONSE_CANCEL,
                                        GTK_RESPONSE_OK, NULL);
    if( gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK )
    {
        GFile* file = gtk_file_chooser_get_file(chooser);
        path = fm_path_new_for_gfile(file);
        g_object_unref(file);
    }
    else
        path = NULL;
    gtk_widget_destroy(GTK_WIDGET(chooser));
    return path;
}

typedef enum
{
    MOUNT_VOLUME,
    MOUNT_GFILE,
    UMOUNT_MOUNT,
    EJECT_MOUNT,
    EJECT_VOLUME
}MountAction;

struct MountData
{
    GMainLoop *loop;
    MountAction action;
    GError* err;
    gboolean ret;
};

static void on_mount_action_finished(GObject* src, GAsyncResult *res, gpointer user_data)
{
    struct MountData* data = user_data;

    switch(data->action)
    {
    case MOUNT_VOLUME:
        data->ret = g_volume_mount_finish(G_VOLUME(src), res, &data->err);
        break;
    case MOUNT_GFILE:
        data->ret = g_file_mount_enclosing_volume_finish(G_FILE(src), res, &data->err);
        break;
    case UMOUNT_MOUNT:
        data->ret = g_mount_unmount_with_operation_finish(G_MOUNT(src), res, &data->err);
        break;
    case EJECT_MOUNT:
        data->ret = g_mount_eject_with_operation_finish(G_MOUNT(src), res, &data->err);
        break;
    case EJECT_VOLUME:
        data->ret = g_volume_eject_with_operation_finish(G_VOLUME(src), res, &data->err);
        break;
    }
    g_main_loop_quit(data->loop);
}

static void prepare_unmount(GMount* mount)
{
    /* ensure that CWD is not on the mounted filesystem. */
    char* cwd_str = g_get_current_dir();
    GFile* cwd = g_file_new_for_path(cwd_str);
    GFile* root = g_mount_get_root(mount);
    g_free(cwd_str);
    /* FIXME: This cannot cover 100% cases since symlinks are not checked.
     * There may be other cases that cwd is actually under mount root
     * but checking prefix is not enough. We already did our best, though. */
    if(g_file_has_prefix(cwd, root))
        g_chdir("/");
    g_object_unref(cwd);
    g_object_unref(root);
}

static gboolean fm_do_mount(GtkWindow* parent, GObject* obj, MountAction action, gboolean interactive)
{
    gboolean ret;
    struct MountData* data = g_new0(struct MountData, 1);
    /* bug #3615234: it seems GtkMountOperations is buggy and sometimes leaves
       parent window reference intact while destroys itself so it leads to
       severe memory corruption, therefore we pass here NULL as parent window
       to gtk_mount_operation_new() to not bind it to anything as a workaround */
    GMountOperation* op = interactive ? gtk_mount_operation_new(NULL) : NULL;
    GCancellable* cancellable = g_cancellable_new();

    data->loop = g_main_loop_new (NULL, TRUE);
    data->action = action;

    switch(data->action)
    {
    case MOUNT_VOLUME:
        g_volume_mount(G_VOLUME(obj), 0, op, cancellable, on_mount_action_finished, data);
        break;
    case MOUNT_GFILE:
        g_file_mount_enclosing_volume(G_FILE(obj), 0, op, cancellable, on_mount_action_finished, data);
        break;
    case UMOUNT_MOUNT:
        prepare_unmount(G_MOUNT(obj));
        g_mount_unmount_with_operation(G_MOUNT(obj), G_MOUNT_UNMOUNT_NONE, op, cancellable, on_mount_action_finished, data);
        break;
    case EJECT_MOUNT:
        prepare_unmount(G_MOUNT(obj));
        g_mount_eject_with_operation(G_MOUNT(obj), G_MOUNT_UNMOUNT_NONE, op, cancellable, on_mount_action_finished, data);
        break;
    case EJECT_VOLUME:
        {
            GMount* mnt = g_volume_get_mount(G_VOLUME(obj));
            if (mnt) /* it might be unmounted already */
            {
                prepare_unmount(mnt);
                g_object_unref(mnt);
            }
            g_volume_eject_with_operation(G_VOLUME(obj), G_MOUNT_UNMOUNT_NONE, op, cancellable, on_mount_action_finished, data);
        }
        break;
    }

    /* FIXME: create progress window with busy cursor */
    if (g_main_loop_is_running(data->loop))
    {
        GDK_THREADS_LEAVE();
        g_main_loop_run(data->loop);
        GDK_THREADS_ENTER();
    }

    g_main_loop_unref(data->loop);

    ret = data->ret;
    if(data->err)
    {
        if(interactive)
        {
            if(data->err->domain == G_IO_ERROR)
            {
                if(data->err->code == G_IO_ERROR_FAILED)
                {
                    /* Generate a more human-readable error message instead of using a gvfs one. */

                    /* The original error message is something like:
                     * Error unmounting: umount exited with exit code 1:
                     * helper failed with: umount: only root can unmount
                     * UUID=18cbf00c-e65f-445a-bccc-11964bdea05d from /media/sda4 */

                    /* Why they pass this back to us?
                     * This is not human-readable for the users at all. */

                    if(strstr(data->err->message, "only root can "))
                    {
                        g_debug("%s", data->err->message);
                        g_free(data->err->message);
                        data->err->message = g_strdup(_("Only system administrators have the permission to do this."));
                    }
                }
                else if(data->err->code == G_IO_ERROR_FAILED_HANDLED)
                    interactive = FALSE;
            }
            if(interactive)
                fm_show_error(parent, NULL, data->err->message);
        }
        g_error_free(data->err);
    }

    g_free(data);
    g_object_unref(cancellable);
    if(op)
        g_object_unref(op);
    return ret;
}

/**
 * fm_mount_path
 * @parent: a window to place dialog over it
 * @path: a path to the volume
 * @interactive: %TRUE to open dialog window
 *
 * Mounts a volume.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 0.1.0
 */
gboolean fm_mount_path(GtkWindow* parent, FmPath* path, gboolean interactive)
{
    GFile* gf = fm_path_to_gfile(path);
    gboolean ret = fm_do_mount(parent, G_OBJECT(gf), MOUNT_GFILE, interactive);
    g_object_unref(gf);
    return ret;
}

/**
 * fm_mount_volume
 * @parent: a window to place dialog over it
 * @vol: a volume to mount
 * @interactive: %TRUE to open dialog window
 *
 * Mounts a volume.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 0.1.0
 */
gboolean fm_mount_volume(GtkWindow* parent, GVolume* vol, gboolean interactive)
{
    return fm_do_mount(parent, G_OBJECT(vol), MOUNT_VOLUME, interactive);
}

/**
 * fm_unmount_mount
 * @parent: a window to place dialog over it
 * @mount: the mounted volume
 * @interactive: %TRUE to open dialog window
 *
 * Unmounts a volume.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 0.1.0
 */
gboolean fm_unmount_mount(GtkWindow* parent, GMount* mount, gboolean interactive)
{
    return fm_do_mount(parent, G_OBJECT(mount), UMOUNT_MOUNT, interactive);
}

/**
 * fm_unmount_volume
 * @parent: a window to place dialog over it
 * @vol: the mounted volume
 * @interactive: %TRUE to open dialog window
 *
 * Unmounts a volume.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 0.1.0
 */
gboolean fm_unmount_volume(GtkWindow* parent, GVolume* vol, gboolean interactive)
{
    GMount* mount = g_volume_get_mount(vol);
    gboolean ret;
    if(!mount)
        return FALSE;
    ret = fm_do_mount(parent, G_OBJECT(vol), UMOUNT_MOUNT, interactive);
    g_object_unref(mount);
    return ret;
}

/**
 * fm_eject_mount
 * @parent: a window to place dialog over it
 * @mount: the mounted media
 * @interactive: %TRUE to open dialog window
 *
 * Ejects the media in @mount.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 0.1.0
 */
gboolean fm_eject_mount(GtkWindow* parent, GMount* mount, gboolean interactive)
{
    return fm_do_mount(parent, G_OBJECT(mount), EJECT_MOUNT, interactive);
}

/**
 * fm_eject_volume
 * @parent: a window to place dialog over it
 * @vol: the mounted media
 * @interactive: %TRUE to open dialog window
 *
 * Ejects the media in @vol.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 0.1.0
 */
gboolean fm_eject_volume(GtkWindow* parent, GVolume* vol, gboolean interactive)
{
    return fm_do_mount(parent, G_OBJECT(vol), EJECT_VOLUME, interactive);
}


/* File operations */
/* FIXME: only show the progress dialog if the job isn't finished
 * in 1 sec. */

/**
 * fm_copy_files
 * @parent: a window to place progress dialog over it
 * @files: list of files to copy
 * @dest_dir: target directory
 *
 * Copies files opening progress dialog if that operation takes some time.
 *
 * Before 0.1.15 this call had different arguments.
 *
 * Since: 0.1.0
 */
void fm_copy_files(GtkWindow* parent, FmPathList* files, FmPath* dest_dir)
{
    FmFileOpsJob* job = fm_file_ops_job_new(FM_FILE_OP_COPY, files);
    fm_file_ops_job_set_dest(job, dest_dir);
    fm_file_ops_job_run_with_progress(parent, job); /* it eats reference! */
}

/**
 * fm_move_files
 * @parent: a window to place progress dialog over it
 * @files: list of files to move
 * @dest_dir: directory where to move files to
 *
 * Moves files opening progress dialog if that operation takes some time.
 *
 * Before 0.1.15 this call had different arguments.
 *
 * Since: 0.1.0
 */
void fm_move_files(GtkWindow* parent, FmPathList* files, FmPath* dest_dir)
{
    FmFileOpsJob* job = fm_file_ops_job_new(FM_FILE_OP_MOVE, files);
    fm_file_ops_job_set_dest(job, dest_dir);
    fm_file_ops_job_run_with_progress(parent, job); /* it eats reference! */
}

/**
 * fm_link_files
 * @parent:   window to base progress dialog over it
 * @files:    list of files to make symbolic links to
 * @dest_dir: directory where symbolic links should be created
 *
 * Create symbolic links for some files in the target directory with
 * progress dialog.
 *
 * Since: 1.0.0
 */
void fm_link_files(GtkWindow* parent, FmPathList* files, FmPath* dest_dir)
{
    FmFileOpsJob* job = fm_file_ops_job_new(FM_FILE_OP_LINK, files);
    fm_file_ops_job_set_dest(job, dest_dir);
    fm_file_ops_job_run_with_progress(parent, job); /* it eats reference! */
}

/**
 * fm_trash_files
 * @parent: a window to place progress dialog over it
 * @files: list of files to move to trash
 *
 * Removes files into trash can opening progress dialog if that operation
 * takes some time.
 *
 * Before 0.1.15 this call had different arguments.
 *
 * Since: 0.1.0
 */
void fm_trash_files(GtkWindow* parent, FmPathList* files)
{
    FmFileOpsJob *job;
    char *msg, *name;
    int len;

    if (fm_config->confirm_trash)
    {
        len = fm_path_list_get_length(files);
        if (len == 1)
        {
            name = fm_path_display_basename(fm_path_list_peek_head(files));
            msg = g_strdup_printf(_("Do you want to move the file '%s' to trash can?"), name);
            g_free(name);
        }
        else
            msg = g_strdup_printf(dngettext(GETTEXT_PACKAGE,
                                            "Do you want to move the %d selected file to trash can?",
                                            "Do you want to move the %d selected files to trash can?",
                                            (gulong)len), len);
        if (!fm_yes_no(parent, NULL, msg, TRUE))
        {
            g_free(msg);
            return;
        }
        g_free(msg);
    }
    job = fm_file_ops_job_new(FM_FILE_OP_TRASH, files);
    fm_file_ops_job_run_with_progress(parent, job); /* it eats reference! */
}

/**
 * fm_untrash_files
 * @parent: a window to place progress dialog over it
 * @files: list of files to restore
 *
 * Restores files from trash can into original place opening progress
 * dialog if that operation takes some time.
 *
 * Before 0.1.15 this call had different arguments.
 *
 * Since: 0.1.11
 */
void fm_untrash_files(GtkWindow* parent, FmPathList* files)
{
    FmFileOpsJob* job = fm_file_ops_job_new(FM_FILE_OP_UNTRASH, files);
    fm_file_ops_job_run_with_progress(parent, job); /* it eats reference! */
}

static void fm_delete_files_internal(GtkWindow* parent, FmPathList* files)
{
    FmFileOpsJob* job = fm_file_ops_job_new(FM_FILE_OP_DELETE, files);
    fm_file_ops_job_run_with_progress(parent, job); /* it eats reference! */
}

/**
 * fm_delete_files
 * @parent: a window to place progress dialog over it
 * @files: list of files to delete
 *
 * Wipes out files opening progress dialog if that operation takes some time.
 *
 * Before 0.1.15 this call had different arguments.
 *
 * Since: 0.1.0
 */
void fm_delete_files(GtkWindow* parent, FmPathList* files)
{
    char *msg, *name;
    int len;

    if (fm_config->confirm_del)
    {
        len = fm_path_list_get_length(files);
        if (len == 1)
        {
            name = fm_path_display_basename(fm_path_list_peek_head(files));
            msg = g_strdup_printf(_("Do you want to delete the file '%s'?"), name);
            g_free(name);
        }
        else
            msg = g_strdup_printf(dngettext(GETTEXT_PACKAGE,
                                            "Do you want to delete the %d selected file?",
                                            "Do you want to delete the %d selected files?",
                                            (gulong)len), len);
        if (!fm_yes_no(parent, NULL, msg, TRUE))
        {
            g_free(msg);
            return;
        }
        g_free(msg);
    }
    fm_delete_files_internal(parent, files);
}

/**
 * fm_trash_or_delete_files
 * @parent: a window to place progress dialog over it
 * @files: list of files to delete
 *
 * Removes files into trash can if that operation is supported.
 * Otherwise erases them. If that operation takes some time then progress
 * dialog will be opened.
 *
 * Before 0.1.15 this call had different arguments.
 *
 * Since: 0.1.0
 */
void fm_trash_or_delete_files(GtkWindow* parent, FmPathList* files)
{
    if( !fm_path_list_is_empty(files) )
    {
        gboolean all_in_trash = TRUE;
        if(fm_config->use_trash)
        {
            GList* l = fm_path_list_peek_head_link(files);
            for(;l;l=l->next)
            {
                FmPath* path = FM_PATH(l->data);
                if(!fm_path_is_trash(path))
                    all_in_trash = FALSE;
            }
        }

        /* files already in trash:/// should only be deleted and cannot be trashed again. */
        if(fm_config->use_trash && !all_in_trash)
            fm_trash_files(parent, files);
        else
            fm_delete_files(parent, files);
    }
}

/**
 * fm_move_or_copy_files_to
 * @parent: a window to place progress dialog over it
 * @files: list of files
 * @is_move: %TRUE to move, %FALSE to copy
 *
 * Opens a dialog to choose destination directory. If it was not cancelled
 * by user then moves or copies @files into chosen directory with progress
 * dialog.
 *
 * Before 0.1.15 this call had different arguments.
 *
 * Since: 0.1.0
 */
void fm_move_or_copy_files_to(GtkWindow* parent, FmPathList* files, gboolean is_move)
{
    FmPath* dest = fm_select_folder(parent, NULL);
    if(dest)
    {
        if(is_move)
            fm_move_files(parent, files, dest);
        else
            fm_copy_files(parent, files, dest);
        fm_path_unref(dest);
    }
}


/**
 * fm_rename_file
 * @parent: a window to place dialog over it
 * @file: the file
 *
 * Opens a dialog to choose new name for @file. If it was not cancelled
 * by user then renames @file.
 *
 * Before 0.1.15 this call had different arguments.
 *
 * Since: 0.1.0
 */
void fm_rename_file(GtkWindow* parent, FmPath* file)
{
    gchar *old_name, *new_name;
    FmPathList *files;
    FmFileOpsJob *job;

    /* NOTE: it's better to use fm_file_info_get_edit_name() to get a name
       but we cannot get it from FmPath */
    old_name = fm_path_display_basename(file);
    new_name = fm_get_user_input_rename(parent, _("Rename File"),
                                        _("Please enter a new name:"),
                                        old_name);
    /* if file name wasn't changed then do nothing */
    if (new_name == NULL || strcmp(old_name, new_name) == 0)
    {
        g_free(old_name);
        g_free(new_name);
        return;
    }
    g_free(old_name);
    files = fm_path_list_new();
    fm_path_list_push_tail(files, file);
    job = fm_file_ops_job_new(FM_FILE_OP_CHANGE_ATTR, files);
    fm_file_ops_job_set_display_name(job, new_name);
    g_free(new_name);
    fm_path_list_unref(files);
    fm_file_ops_job_run_with_progress(parent, job); /* it eats reference! */
}

static void _fm_set_file_hidden(GtkWindow *parent, FmPath *file, gboolean hidden)
{
    FmPathList *files;
    FmFileOpsJob *job;

    files = fm_path_list_new();
    fm_path_list_push_tail(files, file);
    job = fm_file_ops_job_new(FM_FILE_OP_CHANGE_ATTR, files);
    fm_file_ops_job_set_hidden(job, hidden);
    fm_path_list_unref(files);
    fm_file_ops_job_run_with_progress(parent, job); /* it eats reference! */
}

/**
 * fm_hide_file
 * @parent: a window to place progress dialog over it
 * @file: the file
 *
 * Sets file attribute "hidden" for @file.
 *
 * Since: 1.2.0
 */
void fm_hide_file(GtkWindow* parent, FmPath* file)
{
    _fm_set_file_hidden(parent, file, TRUE);
}

/**
 * fm_unhide_file
 * @parent: a window to place progress dialog over it
 * @file: the file
 *
 * Unsets file attribute "hidden" for @file.
 *
 * Since: 1.2.0
 */
void fm_unhide_file(GtkWindow* parent, FmPath* file)
{
    _fm_set_file_hidden(parent, file, FALSE);
}

/**
 * fm_empty_trash
 * @parent: a window to place dialog over it
 *
 * Asks user to confirm the emptying trash can and empties it if confirmed.
 *
 * Before 0.1.15 this call had different arguments.
 *
 * Since: 0.1.0
 */
void fm_empty_trash(GtkWindow* parent)
{
    if(fm_yes_no(parent, NULL, _("Are you sure you want to empty the trash can?"), TRUE))
    {
        FmPathList* paths = fm_path_list_new();
        fm_path_list_push_tail(paths, fm_path_get_trash());
        fm_delete_files_internal(parent, paths);
        fm_path_list_unref(paths);
    }
}

/**
 * fm_set_busy_cursor
 * @widget: a widget
 *
 * Sets cursor for @widget to "busy".
 *
 * See also: fm_unset_busy_cursor().
 *
 * Since: 1.0.0
 */
void fm_set_busy_cursor(GtkWidget* widget)
{
    if(gtk_widget_get_realized(widget))
    {
        GdkWindow* window = gtk_widget_get_window(widget);
        GdkCursor* cursor = gdk_cursor_new(GDK_WATCH);
        gdk_window_set_cursor(window, cursor);
    }
    else
    {
        /* FIXME: how to handle this case? */
        g_warning("fm_set_busy_cursor: widget is not realized");
    }
}

/**
 * fm_unset_busy_cursor
 * @widget: a widget
 *
 * Restores cursor for @widget to default.
 *
 * See also: fm_set_busy_cursor().
 *
 * Since: 1.0.0
 */
void fm_unset_busy_cursor(GtkWidget* widget)
{
    if(gtk_widget_get_realized(widget))
    {
        GdkWindow* window = gtk_widget_get_window(widget);
        gdk_window_set_cursor(window, NULL);
    }
}

static void assign_tooltip_from_action(GtkWidget* widget)
{
    GtkAction* action;
    const gchar * tooltip;

    action = gtk_activatable_get_related_action(GTK_ACTIVATABLE(widget));
    if (!action)
        return;

    if (!gtk_activatable_get_use_action_appearance(GTK_ACTIVATABLE(widget)))
        return;

    tooltip = gtk_action_get_tooltip(action);
    if (tooltip)
    {
        gtk_widget_set_tooltip_text(widget, tooltip);
        gtk_widget_set_has_tooltip(widget, TRUE);
    }
    else
    {
        gtk_widget_set_has_tooltip(widget, FALSE);
    }
}

static void assign_tooltips_from_actions(GtkWidget* widget)
{
    if(G_LIKELY(GTK_IS_MENU_ITEM(widget)))
    {
        if(GTK_IS_ACTIVATABLE(widget))
            assign_tooltip_from_action(widget);
        widget = gtk_menu_item_get_submenu((GtkMenuItem*)widget);
        if(widget)
            assign_tooltips_from_actions(widget);
    }
    else if (GTK_IS_CONTAINER(widget))
    {
        gtk_container_forall((GtkContainer*)widget,
                             (GtkCallback)assign_tooltips_from_actions, NULL);
    }
}

/**
 * fm_widget_menu_fix_tooltips
 * @menu: a #GtkMenu instance
 *
 * Fix on GTK bug: it does not assign tooltips of menu items from
 * appropriate #GtkAction objects. This API assigns them instead.
 *
 * Since: 1.2.0
 */
void fm_widget_menu_fix_tooltips(GtkMenu *menu)
{
    GtkWidget *parent;
    GtkSettings *settings;
    gboolean tooltips_enabled;

    g_return_if_fail(GTK_IS_MENU(menu));
    parent = gtk_menu_get_attach_widget(menu);
    settings = parent ? gtk_settings_get_for_screen(gtk_widget_get_screen(parent))
                      : gtk_settings_get_default();
    g_object_get(G_OBJECT(settings), "gtk-enable-tooltips", &tooltips_enabled, NULL);
    if(tooltips_enabled)
        assign_tooltips_from_actions(GTK_WIDGET(menu));
}
