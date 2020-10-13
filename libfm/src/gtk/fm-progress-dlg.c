/*
 *      fm-progress-dlg.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2012-2015 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *      Copyright 2017 Nathan Osman <nathan@quickmediasolutions.com>
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
 * SECTION:fm-progress-dlg
 * @short_description: A dialog to show progress indicator for file operations.
 * @title: File progress dialog
 *
 * @include: libfm/fm-gtk.h
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-progress-dlg.h"
#include "fm-config.h"
#include "fm-gtk-utils.h"
#include "fm-utils.h"
#include <glib/gi18n-lib.h>

#define SHOW_DLG_DELAY  1000

enum
{
    RESPONSE_OVERWRITE = 1,
    RESPONSE_RENAME,
    RESPONSE_SKIP
};

struct _FmProgressDisplay
{
    GtkWindow* parent;
    GtkDialog* dlg;
    FmFileOpsJob* job;

    /* private */
    GtkImage* icon;
    GtkLabel* msg;
    GtkLabel* act;
    GtkLabel* src;
    GtkWidget* dest;
    GtkLabel* current;
    GtkProgressBar* progress;
    GtkLabel* data_transferred;
    GtkLabel* data_transferred_label;
    GtkLabel* remaining_time;
    GtkLabel *remaining_time_label;
    GtkWidget* error_pane;
    GtkTextView* error_msg;
    GtkTextBuffer* error_buf;
    GtkTextTag* bold_tag;
    GtkButton *suspend;
    GtkButton *cancel;

    FmFileOpOption default_opt;

    GString *str;
    const char *op_text;
    char* cur_file;
    char* old_cur_file;
    goffset data_transferred_size;
    goffset data_total_size;
    guint percent;

    guint delay_timeout;
    guint update_timeout;

    GTimer* timer;

    gboolean has_error : 1;
    gboolean suspended : 1;
};

static void ensure_dlg(FmProgressDisplay* data);
static void fm_progress_display_destroy(FmProgressDisplay* data);

static FmJobErrorAction on_error(FmFileOpsJob* job, GError* err, FmJobErrorSeverity severity, FmProgressDisplay* data)
{
    GtkTextIter it;
    if(err->domain == G_IO_ERROR)
    {
        if(err->code == G_IO_ERROR_CANCELLED)
            return FM_JOB_ABORT;
        else if(err->code == G_IO_ERROR_FAILED_HANDLED)
            return FM_JOB_CONTINUE;
    }

    if(data->timer)
        g_timer_stop(data->timer);

    data->has_error = TRUE;

    ensure_dlg(data);

/*
    FIXME: Need to mount volumes on demand here, too.
    if( err->domain == G_IO_ERROR )
    {
        if( err->code == G_IO_ERROR_NOT_MOUNTED && severity < FM_JOB_ERROR_CRITICAL )
            if(fm_mount_path(parent, dest_path))
                return FM_JOB_RETRY;
    }
*/

    gtk_text_buffer_get_end_iter(data->error_buf, &it);
    if(data->cur_file == NULL)
        g_warning("FmProgressDialog on_error: assertion `cur_file != NULL' failed");
    if(data->cur_file || data->old_cur_file)
    {
        gtk_text_buffer_insert_with_tags(data->error_buf, &it,
                                     data->cur_file ? data->cur_file : data->old_cur_file,
                                     -1, data->bold_tag, NULL);
        gtk_text_buffer_insert(data->error_buf, &it, _(": "), -1);
    }
    gtk_text_buffer_insert(data->error_buf, &it, err->message, -1);
    gtk_text_buffer_insert(data->error_buf, &it, "\n", 1);

    if(!gtk_widget_get_visible(data->error_pane))
        gtk_widget_show(data->error_pane);

    if(data->timer)
        g_timer_continue(data->timer);
    return FM_JOB_CONTINUE;
}

static gint on_ask(FmFileOpsJob* job, const char* question, char* const* options, FmProgressDisplay* data)
{
    ensure_dlg(data);
    return fm_askv(GTK_WINDOW(data->dlg), NULL, question, options);
}

static void on_filename_changed(GtkEditable* entry, GtkWidget* rename)
{
    const char* old_name = g_object_get_data(G_OBJECT(entry), "old_name");
    const char* new_name = gtk_entry_get_text(GTK_ENTRY(entry));
    gboolean can_rename = new_name && *new_name && g_strcmp0(old_name, new_name);
    gtk_widget_set_sensitive(rename, can_rename);
    if(can_rename)
    {
        GtkDialog* dlg = GTK_DIALOG(gtk_widget_get_toplevel(GTK_WIDGET(entry)));
        gtk_dialog_set_default_response(dlg, gtk_dialog_get_response_for_widget(dlg, rename));
    }
}

static gint on_ask_rename(FmFileOpsJob* job, FmFileInfo* src, FmFileInfo* dest, char** new_name, FmProgressDisplay* data)
{
    int res;
    GtkBuilder* builder;
    GtkDialog *dlg;
    GtkImage *src_icon, *dest_icon;
    GtkLabel *src_fi, *dest_fi;
    GtkEntry *filename;
    GtkToggleButton *apply_all;
    char* tmp;
    const char* disp_size;
    FmPath* path;
    FmIcon* icon;
    FmFileOpOption options;
    gboolean no_valid_dest;

    /* return default operation if the user has set it */
    if(data->default_opt)
        return data->default_opt;

    no_valid_dest = (fm_file_info_get_desc(dest) == NULL);

    builder = gtk_builder_new();
    path = fm_file_info_get_path(dest);
    icon = fm_file_info_get_icon(src);

    if(data->timer)
        g_timer_stop(data->timer);

    gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);
    ensure_dlg(data);

    gtk_builder_add_from_file(builder, PACKAGE_UI_DIR "/ask-rename.ui", NULL);
    dlg = GTK_DIALOG(gtk_builder_get_object(builder, "dlg"));
    src_icon = GTK_IMAGE(gtk_builder_get_object(builder, "src_icon"));
    src_fi = GTK_LABEL(gtk_builder_get_object(builder, "src_fi"));
    dest_icon = GTK_IMAGE(gtk_builder_get_object(builder, "dest_icon"));
    dest_fi = GTK_LABEL(gtk_builder_get_object(builder, "dest_fi"));
    filename = GTK_ENTRY(gtk_builder_get_object(builder, "filename"));
    apply_all = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "apply_all"));
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(data->dlg));

    gtk_image_set_from_gicon(src_icon, G_ICON(icon), GTK_ICON_SIZE_DIALOG);
    disp_size = fm_file_info_get_disp_size(src);
    if(disp_size)
    {
        tmp = g_strdup_printf(_("Type: %s\nSize: %s\nModified: %s"),
                              fm_file_info_get_desc(src), disp_size,
                              fm_file_info_get_disp_mtime(src));
    }
    else
    {
        tmp = g_strdup_printf(_("Type: %s\nModified: %s"),
                              fm_file_info_get_desc(src),
                              fm_file_info_get_disp_mtime(src));
    }

    gtk_label_set_text(src_fi, tmp);
    g_free(tmp);

    gtk_image_set_from_gicon(dest_icon, G_ICON(icon), GTK_ICON_SIZE_DIALOG);
    disp_size = fm_file_info_get_disp_size(dest);
    if(disp_size)
    {
        tmp = g_strdup_printf(_("Type: %s\nSize: %s\nModified: %s"),
                              fm_file_info_get_desc(dest), disp_size,
                              fm_file_info_get_disp_mtime(dest));
    }
    else if (no_valid_dest)
    {
        tmp = NULL;
        gtk_widget_destroy(GTK_WIDGET(dest_icon));
        gtk_widget_destroy(GTK_WIDGET(dest_fi));
        /* FIXME: change texts in dialog appropriately */
    }
    else
    {
        tmp = g_strdup_printf(_("Type: %s\nModified: %s"),
                              fm_file_info_get_desc(dest),
                              fm_file_info_get_disp_mtime(dest));
    }

    if (tmp)
        gtk_label_set_text(dest_fi, tmp);
    g_free(tmp);

    options = fm_file_ops_job_get_options(job);
    if (!(options & FM_FILE_OP_RENAME))
    {
        GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, "rename"));
        gtk_widget_destroy(widget);
    }
    if (!(options & FM_FILE_OP_OVERWRITE) || no_valid_dest)
    {
        GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, "overwrite"));
        gtk_widget_destroy(widget);
    }
    if (!(options & FM_FILE_OP_SKIP))
    {
        GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, "skip"));
        gtk_widget_destroy(widget);
    }

    tmp = g_filename_display_name(fm_path_get_basename(path));
    gtk_entry_set_text(filename, tmp);
    g_object_set_data_full(G_OBJECT(filename), "old_name", tmp, g_free);
    g_signal_connect(filename, "changed", G_CALLBACK(on_filename_changed), gtk_builder_get_object(builder, "rename"));

    g_object_unref(builder);

    res = gtk_dialog_run(dlg);
    switch(res)
    {
    case RESPONSE_RENAME:
        *new_name = g_strdup(gtk_entry_get_text(filename));
        res = FM_FILE_OP_RENAME;
        break;
    case RESPONSE_OVERWRITE:
        res = FM_FILE_OP_OVERWRITE;
        break;
    case RESPONSE_SKIP:
        res = FM_FILE_OP_SKIP;
        break;
    default:
        res = FM_FILE_OP_CANCEL;
    }

    if(gtk_toggle_button_get_active(apply_all))
    {
        if(res == RESPONSE_OVERWRITE || res == FM_FILE_OP_SKIP)
            data->default_opt = res;
    }

    gtk_widget_destroy(GTK_WIDGET(dlg));

    if(data->timer)
        g_timer_continue(data->timer);

    return res;
}

static void on_finished(FmFileOpsJob* job, FmProgressDisplay* data)
{
    GtkWindow* parent = NULL;

    /* preserve pointers that fm_progress_display_destroy() will unreference
       as they may be requested by trash support below */
    if(data->parent)
        parent = g_object_ref(data->parent);
    g_object_ref(job);

    if(data->dlg)
    {
        /* errors happened */
        if(data->has_error)
        {
            gtk_widget_destroy(GTK_WIDGET(data->current));
            data->current = NULL;
            if (data->remaining_time_label)
            {
                gtk_widget_destroy(GTK_WIDGET(data->remaining_time_label));
                gtk_widget_destroy(GTK_WIDGET(data->remaining_time));
                data->remaining_time = NULL;
            }
            else
                gtk_label_set_text(data->remaining_time, "00:00:00");
            gtk_widget_hide(GTK_WIDGET(data->suspend));
            gtk_widget_hide(GTK_WIDGET(data->cancel));
            gtk_dialog_add_button(data->dlg, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

            gtk_image_set_from_stock(data->icon, GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);

            gtk_widget_show(GTK_WIDGET(data->icon));
            gtk_widget_show(GTK_WIDGET(data->msg));
            if(fm_job_is_cancelled(FM_JOB(job)))
            {
                gtk_label_set_markup(data->msg, _("<b>Errors occurred before file operation was stopped.</b>"));
                gtk_window_set_title(GTK_WINDOW(data->dlg),
                                     _("Cancelled"));
            }
            else
            {
                gtk_label_set_markup(data->msg, _("<b>The file operation was completed with errors.</b>"));
                gtk_window_set_title(GTK_WINDOW(data->dlg),
                                     _("Finished"));
            }
        }
        else
            fm_progress_display_destroy(data);
        g_debug("file operation is finished!");
    }
    else
        fm_progress_display_destroy(data);
    /* if it's not destroyed yet then it will be destroyed with dialog window */

    /* sepcial handling for trash
     * FIXME: need to refactor this to use a more elegant way later. */
    if(job->type == FM_FILE_OP_TRASH) /* FIXME: direct access to job struct! */
    {
        FmPathList* unsupported = (FmPathList*)g_object_get_data(G_OBJECT(job), "trash-unsupported");
        /* some files cannot be trashed because underlying filesystems don't support it. */
        g_object_unref(job);
        if(unsupported) /* delete them instead */
        {
            /* FIXME: parent window might be already destroyed! */
            if(fm_yes_no(parent, NULL,
                        _("Some files cannot be moved to trash can because "
                        "the underlying file systems don't support this operation.\n"
                        "Do you want to delete them instead?"), TRUE))
            {
                job = fm_file_ops_job_new(FM_FILE_OP_DELETE, unsupported);
                fm_file_ops_job_run_with_progress(parent, job);
                                                        /* it eats reference! */
            }
        }
    }
    else
        g_object_unref(job);
    if(parent)
        g_object_unref(parent);
}

static void on_cancelled(FmFileOpsJob* job, FmProgressDisplay* data)
{
    g_debug("file operation is cancelled!");
}

static void on_response(GtkDialog* dlg, gint id, FmProgressDisplay* data)
{
    /* cancel the job */
    if(id == GTK_RESPONSE_CANCEL)
    {
        fm_job_cancel(FM_JOB(data->job));
        if (data->suspended)
        {
            fm_job_resume(FM_JOB(data->job));
            data->suspended = FALSE;
        }
    }
    else if(id == GTK_RESPONSE_CLOSE || id == GTK_RESPONSE_DELETE_EVENT)
        fm_progress_display_destroy(data);
    else if (id == 1 && data->suspend)
    {
        if (data->suspended)
        {
            data->suspended = FALSE;
            fm_job_resume(FM_JOB(data->job));
            gtk_button_set_label(data->suspend, _("_Pause"));
            gtk_button_set_image(data->suspend,
                                 gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE,
                                                          GTK_ICON_SIZE_BUTTON));
        }
        else if (fm_job_pause(FM_JOB(data->job)))
        {
            data->suspended = TRUE;
            gtk_button_set_label(data->suspend, _("_Resume"));
            gtk_button_set_image(data->suspend,
                                 gtk_image_new_from_stock(GTK_STOCK_MEDIA_FORWARD,
                                                          GTK_ICON_SIZE_BUTTON));

        }
        else
            g_warning("FmJob failed to pause");
    }
}

static gboolean on_update_dlg(gpointer user_data)
{
    FmProgressDisplay* data = (FmProgressDisplay*)user_data;
    gdouble elapsed;

    char* data_transferred_str;
    char trans_size_str[128];
    char total_size_str[128];

    if (g_source_is_destroyed(g_main_current_source()) || data->dlg == NULL)
        return FALSE;
    data->update_timeout = 0;
    /* the g_strdup very probably returns the same pointer that was g_free'd
       so we cannot just compare data->old_cur_file with data->cur_file */
    if(data->cur_file && data->current)
    {
        g_string_printf(data->str, "<i>%s %s</i>", data->op_text, data->cur_file);
        gtk_label_set_markup(data->current, data->str->str);
        gtk_widget_set_tooltip_text(GTK_WIDGET(data->current), data->cur_file);
        g_free(data->old_cur_file);
        data->old_cur_file = data->cur_file;
        data->cur_file = NULL;
    }
    g_string_printf(data->str, "%d %%", data->percent);
    gtk_progress_bar_set_fraction(data->progress, (gdouble)data->percent/100);
    gtk_progress_bar_set_text(data->progress, data->str->str);

    /* display the amount of data transferred */
    fm_file_size_to_str(trans_size_str, sizeof(trans_size_str),
        data->data_transferred_size, fm_config->si_unit);
    fm_file_size_to_str(total_size_str, sizeof(total_size_str),
        data->data_total_size, fm_config->si_unit);
    data_transferred_str = g_strdup_printf("%s / %s", trans_size_str, total_size_str);
    gtk_label_set_text(data->data_transferred, data_transferred_str);
    g_free(data_transferred_str);

    elapsed = g_timer_elapsed(data->timer, NULL);
    if(elapsed >= 0.5 && data->percent > 0)
    {
        gdouble remaining = elapsed * (100 - data->percent) / data->percent;
        if(data->remaining_time)
        {
            char time_str[32];
            guint secs = (guint)remaining;
            guint mins = 0;
            guint hrs = 0;
            if(secs >= 60)
            {
                mins = secs / 60;
                secs %= 60;
                if(mins >= 60)
                {
                    hrs = mins / 60;
                    mins %= 60;
                }
            }
            g_snprintf(time_str, 32, "%02d:%02d:%02d", hrs, mins, secs);
            gtk_label_set_text(data->remaining_time, time_str);
        }
    }
    return FALSE;
}

static void on_cur_file(FmFileOpsJob* job, const char* cur_file, FmProgressDisplay* data)
{
    g_free(data->cur_file);
    data->cur_file = g_strdup(cur_file);
    /* NOTE: Displaying currently processed file will slow down the
     * operation and waste CPU source due to showing the text with pango.
     * Consider showing current file every 0.5 second. */
    if(data->dlg && data->update_timeout == 0)
        data->update_timeout = gdk_threads_add_timeout(500, on_update_dlg, data);
}

static void on_percent(FmFileOpsJob* job, guint percent, FmProgressDisplay* data)
{
    data->data_transferred_size = job->finished;
    data->data_total_size = job->total;
    data->percent = percent;
    if(data->dlg && data->update_timeout == 0)
        data->update_timeout = gdk_threads_add_timeout(500, on_update_dlg, data);
}

static void on_progress_dialog_destroy(gpointer user_data, GObject* dlg)
{
    FmProgressDisplay* data = (FmProgressDisplay*)user_data;

    data->dlg = NULL; /* it's destroying right now, don't destroy it again */
    g_object_unref(data->error_buf); /* these will be not unref if no dlg */
    g_object_unref(data->bold_tag);
    fm_progress_display_destroy(data);
}

static gboolean _on_show_dlg(gpointer user_data)
{
    FmProgressDisplay* data = (FmProgressDisplay*)user_data;
    GtkBuilder* builder;
    GtkLabel* to;
    GtkWidget *to_label;
    FmPath* dest;
    const char* title = NULL;
    GtkTextTagTable* tag_table;

    builder = gtk_builder_new();
    tag_table = gtk_text_tag_table_new();
    gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);
    gtk_builder_add_from_file(builder, PACKAGE_UI_DIR "/progress.ui", NULL);

    data->dlg = GTK_DIALOG(gtk_builder_get_object(builder, "dlg"));
    g_object_weak_ref(G_OBJECT(data->dlg), on_progress_dialog_destroy, data);

    g_signal_connect(data->dlg, "response", G_CALLBACK(on_response), data);
    /* FIXME: connect to "close" signal */

    to_label = (GtkWidget*)gtk_builder_get_object(builder, "to_label");
    to = GTK_LABEL(gtk_builder_get_object(builder, "dest"));
    data->icon = GTK_IMAGE(gtk_builder_get_object(builder, "icon"));
    data->msg = GTK_LABEL(gtk_builder_get_object(builder, "msg"));
    data->act = GTK_LABEL(gtk_builder_get_object(builder, "action"));
    data->src = GTK_LABEL(gtk_builder_get_object(builder, "src"));
    data->dest = (GtkWidget*)gtk_builder_get_object(builder, "dest");
    data->current = GTK_LABEL(gtk_builder_get_object(builder, "current"));
    data->progress = GTK_PROGRESS_BAR(gtk_builder_get_object(builder, "progress"));
    data->error_pane = (GtkWidget*)gtk_builder_get_object(builder, "error_pane");
    data->error_msg = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "error_msg"));
    data->data_transferred = GTK_LABEL(gtk_builder_get_object(builder, "data_transferred"));
    data->data_transferred_label = GTK_LABEL(gtk_builder_get_object(builder, "data_transferred_label"));
    data->remaining_time = GTK_LABEL(gtk_builder_get_object(builder, "remaining_time"));
    data->remaining_time_label = GTK_LABEL(gtk_builder_get_object(builder, "remaining_time_label"));
    data->cancel = GTK_BUTTON(gtk_builder_get_object(builder, "cancel"));
    data->suspend = GTK_BUTTON(gtk_dialog_add_button(data->dlg, _("_Pause"), 1));
    gtk_button_set_use_stock(data->suspend, FALSE);
    gtk_button_set_use_underline(data->suspend, TRUE);
    gtk_button_set_image(data->suspend,
                         gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE,
                                                  GTK_ICON_SIZE_BUTTON));
    gtk_dialog_set_alternative_button_order(data->dlg, 1, GTK_RESPONSE_CANCEL, -1);
    data->bold_tag = gtk_text_tag_new("bold");
    g_object_set(data->bold_tag, "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_tag_table_add(tag_table, data->bold_tag);
    data->error_buf = gtk_text_buffer_new(tag_table);
    g_object_unref(tag_table);
    gtk_text_view_set_buffer(data->error_msg, data->error_buf);

    gtk_widget_hide(GTK_WIDGET(data->icon));

    g_object_unref(builder);

    /* set the src label */
    /* FIXME: direct access to job struct! */
    if(data->job->srcs)
    {
        GList* l = fm_path_list_peek_head_link(data->job->srcs);
        int i;
        char* disp;
        FmPath* path;
        GString* str = g_string_sized_new(512);
        path = FM_PATH(l->data);
        disp = fm_path_display_basename(path);
        g_string_assign(str, disp);
        g_free(disp);
        for( i =1, l=l->next; i < 10 && l; l=l->next, ++i)
        {
            path = FM_PATH(l->data);
            g_string_append(str, _(", "));
            disp = fm_path_display_basename(path);
            g_string_append(str, disp);
            g_free(disp);
        }
        if(l)
            g_string_append(str, "...");
        gtk_label_set_text(data->src, str->str);
        gtk_widget_set_tooltip_text(GTK_WIDGET(data->src), str->str);
        g_string_free(str, TRUE);
    }

    /* FIXME: use accessor functions instead */
    /* FIXME: direct access to job struct! */
    switch(data->job->type)
    {
    /* we set title here if text is complex so may be different from
       the op_text when is translated to other languages */
    case FM_FILE_OP_MOVE:
        /* translators: it is part of "Moving files:" or "Moving xxx.txt" */
        data->op_text = _("Moving");
        break;
    case FM_FILE_OP_COPY:
        /* translators: it is part of "Copying files:" or "Copying xxx.txt" */
        data->op_text = _("Copying");
        break;
    case FM_FILE_OP_TRASH:
        /* translators: it is part of "Trashing files:" or "Trashing xxx.txt" */
        data->op_text = _("Trashing");
        break;
    case FM_FILE_OP_DELETE:
        /* translators: it is part of "Deleting files:" or "Deleting xxx.txt" */
        data->op_text = _("Deleting");
        break;
    case FM_FILE_OP_LINK:
        /* translators: it is part of "Creating link /path/xxx.txt" */
        data->op_text = _("Creating link");
        /* translators: 'In:' string is followed by destination folder path */
        gtk_label_set_markup(GTK_LABEL(to_label), _("<b>In:</b>"));
        title = _("Creating links to files");
        /* NOTE: on creating single symlink or shortcut all operation
           is done in single I/O therefore it should fit into 0.5s
           timeout and progress window will never be shown */
        break;
    case FM_FILE_OP_CHANGE_ATTR:
        /* translators: it is part of "Changing attributes of xxx.txt" */
        data->op_text = _("Changing attributes of");
        title = _("Changing attributes of files");
        /* NOTE: the same about single symlink is appliable here so
           there is no need to add never used string for translation */
        break;
    case FM_FILE_OP_UNTRASH:
        /* translators: it is part of "Restoring files:" or "Restoring xxx.txt" */
        data->op_text = _("Restoring");
        break;
    case FM_FILE_OP_NONE: ;
    }
    data->str = g_string_sized_new(64);
    if (title)
        gtk_window_set_title(GTK_WINDOW(data->dlg), title);
    else
    {
        /* note to translators: resulting string is such as "Deleting files" */
        g_string_printf(data->str, _("%s files"), data->op_text);
        gtk_window_set_title(GTK_WINDOW(data->dlg), data->str->str);
    }
    gtk_label_set_markup(data->msg, _("<b>File operation is in progress...</b>"));
    gtk_widget_show(GTK_WIDGET(data->msg));
    if (title) /* we already know the exact string */
        g_string_printf(data->str, "<b>%s:</b>", title);
    else if (fm_path_list_get_length(data->job->srcs) == 1)
        /* note to translators: resulting string is such as "Deleting file" */
        g_string_printf(data->str, _("<b>%s file:</b>"), data->op_text);
    else
        /* note to translators: resulting string is such as "Deleting files" */
        g_string_printf(data->str, _("<b>%s files:</b>"), data->op_text);
    gtk_label_set_markup(data->act, data->str->str);

    dest = fm_file_ops_job_get_dest(data->job);
    if(dest)
    {
        char* dest_str = fm_path_display_name(dest, TRUE);
        gtk_label_set_text(to, dest_str);
        gtk_widget_set_tooltip_text(GTK_WIDGET(data->dest), dest_str);
        g_free(dest_str);
    }
    else
    {
        gtk_widget_destroy(data->dest);
        gtk_widget_destroy(to_label);
    }

    gtk_window_set_transient_for(GTK_WINDOW(data->dlg), data->parent);
    gtk_window_present(GTK_WINDOW(data->dlg));

    data->delay_timeout = 0;
    return FALSE;
}

static gboolean on_show_dlg(gpointer user_data)
{
    if(g_source_is_destroyed(g_main_current_source()))
        return FALSE;
    return _on_show_dlg(user_data);
}

static void ensure_dlg(FmProgressDisplay* data)
{
    if(data->delay_timeout)
    {
        g_source_remove(data->delay_timeout);
        data->delay_timeout = 0;
    }
    GDK_THREADS_ENTER();
    if(!data->dlg)
        _on_show_dlg(data);
    GDK_THREADS_LEAVE();
}

static void on_prepared(FmFileOpsJob* job, FmProgressDisplay* data)
{
    data->timer = g_timer_new();
}

/**
 * fm_file_ops_job_run_with_progress
 * @parent: parent window to show dialog over it
 * @job: (transfer full): job descriptor to run
 *
 * Runs the file operation job with a progress dialog.
 * The returned data structure will be freed in idle handler automatically
 * when it's not needed anymore.
 *
 * NOTE: INCONSISTENCY: it takes a reference from job
 *
 * Before 0.1.15 this call had different arguments.
 *
 * Return value: (transfer none): progress data; not usable; caller should not free it either.
 *
 * Since: 0.1.0
 */
FmProgressDisplay* fm_file_ops_job_run_with_progress(GtkWindow* parent, FmFileOpsJob* job)
{
    FmProgressDisplay* data;

    g_return_val_if_fail(job != NULL, NULL);

    data = g_slice_new0(FmProgressDisplay);
    data->job = job;
    if(parent)
        data->parent = g_object_ref(parent);
    data->delay_timeout = gdk_threads_add_timeout(SHOW_DLG_DELAY, on_show_dlg, data);

    g_signal_connect(job, "ask", G_CALLBACK(on_ask), data);
    g_signal_connect(job, "ask-rename", G_CALLBACK(on_ask_rename), data);
    g_signal_connect(job, "error", G_CALLBACK(on_error), data);
    g_signal_connect(job, "prepared", G_CALLBACK(on_prepared), data);
    g_signal_connect(job, "cur-file", G_CALLBACK(on_cur_file), data);
    g_signal_connect(job, "percent", G_CALLBACK(on_percent), data);
    g_signal_connect(job, "finished", G_CALLBACK(on_finished), data);
    g_signal_connect(job, "cancelled", G_CALLBACK(on_cancelled), data);

    if (!fm_job_run_async(FM_JOB(job)))
    {
        fm_progress_display_destroy(data);
        return NULL;
    }

    return data;
}

static void fm_progress_display_destroy(FmProgressDisplay* data)
{
    g_signal_handlers_disconnect_by_func(data->job, on_cancelled, data);

    fm_job_cancel(FM_JOB(data->job));
    if (data->suspended)
        fm_job_resume(FM_JOB(data->job));

    g_signal_handlers_disconnect_by_func(data->job, on_ask, data);
    g_signal_handlers_disconnect_by_func(data->job, on_ask_rename, data);
    g_signal_handlers_disconnect_by_func(data->job, on_error, data);
    g_signal_handlers_disconnect_by_func(data->job, on_prepared, data);
    g_signal_handlers_disconnect_by_func(data->job, on_cur_file, data);
    g_signal_handlers_disconnect_by_func(data->job, on_percent, data);
    g_signal_handlers_disconnect_by_func(data->job, on_finished, data);

    g_object_unref(data->job);

    if(data->timer)
        g_timer_destroy(data->timer);

    if(data->parent)
        g_object_unref(data->parent);

    g_free(data->cur_file);
    g_free(data->old_cur_file);

    if(data->delay_timeout)
        g_source_remove(data->delay_timeout);

    if(data->update_timeout)
        g_source_remove(data->update_timeout);

    if(data->dlg)
    {
        g_object_weak_unref(G_OBJECT(data->dlg), on_progress_dialog_destroy, data);
        g_object_unref(data->error_buf);
        g_object_unref(data->bold_tag);
        gtk_widget_destroy(GTK_WIDGET(data->dlg));
    }

    if (data->str)
        g_string_free(data->str, TRUE);

    g_slice_free(FmProgressDisplay, data);
}
