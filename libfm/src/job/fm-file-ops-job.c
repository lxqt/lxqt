/*
 *      fm-file-ops-job.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2012-2016 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This file is a part of the Libfm library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:fm-file-ops-job
 * @short_description: Job to do something with files.
 * @title: FmFileOpsJob
 *
 * @include: libfm/fm.h
 *
 * The #FmFileOpsJob can be used to do some file operation such as move,
 * copy, delete, change file attributes, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>

#include "fm-file-ops-job.h"
#include "fm-file-ops-job-xfer.h"
#include "fm-file-ops-job-delete.h"
#include "fm-file-ops-job-change-attr.h"
#include "fm-marshal.h"
#include "fm-file-info-job.h"
#include "glib-compat.h"

enum
{
    PREPARED,
    CUR_FILE,
    PERCENT,
    ASK_RENAME,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

static void fm_file_ops_job_finalize              (GObject *object);

static gboolean fm_file_ops_job_run(FmJob* fm_job);
/* static void fm_file_ops_job_cancel(FmJob* job); */

/* funcs for io jobs */
static gboolean _fm_file_ops_job_link_run(FmFileOpsJob* job);


G_DEFINE_TYPE(FmFileOpsJob, fm_file_ops_job, FM_TYPE_JOB);

static void fm_file_ops_job_dispose(GObject *object)
{
    FmFileOpsJob *self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_FILE_OPS_JOB(object));

    self = (FmFileOpsJob*)object;

    if(self->srcs)
    {
        fm_path_list_unref(self->srcs);
        self->srcs = NULL;
    }
    if(self->dest)
    {
        fm_path_unref(self->dest);
        self->dest = NULL;
    }
    if(self->display_name)
    {
        g_free(self->display_name);
        self->display_name = NULL;
    }
    if(self->icon)
    {
        g_object_unref(self->icon);
        self->icon = NULL;
    }
    if(self->target)
    {
        g_free(self->target);
        self->target = NULL;
    }

    G_OBJECT_CLASS(fm_file_ops_job_parent_class)->dispose(object);
}

static void fm_file_ops_job_class_init(FmFileOpsJobClass *klass)
{
    GObjectClass *g_object_class;
    FmJobClass* job_class;
    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->dispose = fm_file_ops_job_dispose;
    g_object_class->finalize = fm_file_ops_job_finalize;

    job_class = FM_JOB_CLASS(klass);
    job_class->run = fm_file_ops_job_run;

    /**
     * FmFileOpsJob::prepared:
     * @job: a job object which emitted the signal
     *
     * The #FmFileOpsJob::prepared signal is emitted when preparation
     * of the file operation is done and @job is ready to start
     * copying/deleting...
     *
     * Since: 0.1.10
     */
    signals[PREPARED] =
        g_signal_new( "prepared",
                      G_TYPE_FROM_CLASS ( klass ),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET ( FmFileOpsJobClass, prepared ),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0 );

    /**
     * FmFileOpsJob::cur-file:
     * @job: a job object which emitted the signal
     * @file: (const char *) file which is processing
     *
     * The #FmFileOpsJob::cur-file signal is emitted when @job is about
     * to start operation on the @file.
     *
     * Since: 0.1.0
     */
    signals[CUR_FILE] =
        g_signal_new( "cur-file",
                      G_TYPE_FROM_CLASS ( klass ),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET ( FmFileOpsJobClass, cur_file ),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1, G_TYPE_POINTER );

    /**
     * FmFileOpsJob::percent:
     * @job: a job object which emitted the signal
     * @percent: current ratio of completed job size to full job size
     *
     * The #FmFileOpsJob::percent signal is emitted when one more file
     * operation is completed.
     *
     * Since: 0.1.0
     */
    signals[PERCENT] =
        g_signal_new( "percent",
                      G_TYPE_FROM_CLASS ( klass ),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET ( FmFileOpsJobClass, percent ),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__UINT,
                      G_TYPE_NONE, 1, G_TYPE_UINT );

    /**
     * FmFileOpsJob::ask-rename:
     * @job: a job object which emitted the signal
     * @src: (#FmFileInfo *) source file
     * @dest: (#FmFileInfo *) destination directory
     * @new_name: (char **) pointer to receive new name
     *
     * The #FmFileOpsJob::ask-rename signal is emitted when file operation
     * raises a conflict because file with the same name already exists
     * in the directory @dest. Signal handler should find a decision how
     * to resolve the situation. If there is more than one handler connected
     * to the signal then only one of them will receive it.
     * Implementations are expected to inspect supported options for the
     * decision calling fm_file_ops_job_get_options(). Behavior when
     * handler returns unsupported option is undefined.
     *
     * Return value: a #FmFileOpOption decision.
     *
     * Since: 0.1.0
     */
    signals[ASK_RENAME] =
        g_signal_new( "ask-rename",
                      G_TYPE_FROM_CLASS ( klass ),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET ( FmFileOpsJobClass, ask_rename ),
                      g_signal_accumulator_first_wins, NULL,
                      fm_marshal_INT__POINTER_POINTER_POINTER,
                      G_TYPE_INT, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER );

}


static void fm_file_ops_job_finalize(GObject *object)
{
    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_FILE_OPS_JOB(object));

    G_OBJECT_CLASS(fm_file_ops_job_parent_class)->finalize(object);
}


static void fm_file_ops_job_init(FmFileOpsJob *self)
{
    fm_job_init_cancellable(FM_JOB(self));

    /* for chown */
    self->uid = -1;
    self->gid = -1;
    self->set_hidden = -1;
}

/**
 * fm_file_ops_job_new
 * @type: type of file operation the new job will handle
 * @files: list of source files to perform operation
 *
 * Creates new #FmFileOpsJob which can be used in #FmJob API.
 *
 * Returns: a new #FmFileOpsJob object.
 *
 * Since: 0.1.0
 */
FmFileOpsJob *fm_file_ops_job_new(FmFileOpType type, FmPathList* files)
{
    FmFileOpsJob* job = (FmFileOpsJob*)g_object_new(FM_FILE_OPS_JOB_TYPE, NULL);
    job->srcs = fm_path_list_ref(files);
    job->type = type;
    return job;
}


static gboolean fm_file_ops_job_run(FmJob* fm_job)
{
    FmFileOpsJob* job = FM_FILE_OPS_JOB(fm_job);
    GError *err;
    switch(job->type)
    {
    case FM_FILE_OP_COPY:
        return _fm_file_ops_job_copy_run(job);
    case FM_FILE_OP_MOVE:
        return _fm_file_ops_job_move_run(job);
    case FM_FILE_OP_TRASH:
        return _fm_file_ops_job_trash_run(job);
    case FM_FILE_OP_UNTRASH:
        return _fm_file_ops_job_untrash_run(job);
    case FM_FILE_OP_DELETE:
        return _fm_file_ops_job_delete_run(job);
    case FM_FILE_OP_LINK:
        return _fm_file_ops_job_link_run(job);
    case FM_FILE_OP_CHANGE_ATTR:
        return _fm_file_ops_job_change_attr_run(job);
    case FM_FILE_OP_NONE: ;
    }
    err = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                              _("Operation not supported"));
    fm_job_emit_error(FM_JOB(job), err, FM_JOB_ERROR_CRITICAL);
    g_error_free(err);
    return FALSE;
}


/**
 * fm_file_ops_job_set_dest
 * @job: a job to set
 * @dest: destination path
 *
 * Sets destination path for operations FM_FILE_OP_MOVE, FM_FILE_OP_COPY,
 * or FM_FILE_OP_LINK.
 *
 * This API may be used only before @job is started.
 *
 * Since: 0.1.0
 */
void fm_file_ops_job_set_dest(FmFileOpsJob* job, FmPath* dest)
{
    job->dest = fm_path_ref(dest);
}

/**
 * fm_file_ops_job_get_dest
 * @job: a job to inspect
 *
 * Retrieves the destination path for operation. If type of operation
 * in not FM_FILE_OP_MOVE, FM_FILE_OP_COPY, or FM_FILE_OP_LINK then
 * result of this call is undefined. The returned value is owned by
 * @job and should be not freed by caller.
 *
 * Returns: (transfer none): the #FmPath which was set by previous call
 * to fm_file_ops_job_set_dest().
 *
 * Since: 0.1.0
 */
FmPath* fm_file_ops_job_get_dest(FmFileOpsJob* job)
{
    return job->dest;
}

/**
 * fm_file_ops_job_set_chmod
 * @job: a job to set
 * @new_mode: which bits of file mode should be set
 * @new_mode_mask: which bits of file mode should be reset
 *
 * Sets that files for file operation FM_FILE_OP_CHANGE_ATTR should have
 * file mode changed according to @new_mode_mask and @new_mode: bits
 * that are present only in @new_mode_mask will be set to 0, and bits
 * that are present in both @new_mode_mask and @new_mode will be set to 1.
 *
 * This API may be used only before @job is started.
 *
 * Since: 0.1.0
 */
void fm_file_ops_job_set_chmod(FmFileOpsJob* job, mode_t new_mode, mode_t new_mode_mask)
{
    job->new_mode = new_mode;
    job->new_mode_mask = new_mode_mask;
}

/**
 * fm_file_ops_job_set_chown
 * @job: a job to set
 * @uid: user id to set as file owner
 * @gid: group id to set as file group
 *
 * Sets that files for file operation FM_FILE_OP_CHANGE_ATTR should have
 * owner or group changed. If @uid >= 0 then @job will try to change
 * owner of files. If @gid >= 0 then @job will try to change group of
 * files.
 *
 * This API may be used only before @job is started.
 *
 * Since: 0.1.0
 */
void fm_file_ops_job_set_chown(FmFileOpsJob* job, gint uid, gint gid)
{
    job->uid = uid;
    job->gid = gid;
}

/**
 * fm_file_ops_job_set_recursive
 * @job: a job to set
 * @recursive: recursion attribute to set
 *
 * Sets 'recursive' attribute for file operation according to @recursive.
 * If @recursive is %TRUE then file operation @job will try to do all
 * operations recursively.
 *
 * This API may be used only before @job is started.
 *
 * Since: 0.1.0
 */
void fm_file_ops_job_set_recursive(FmFileOpsJob* job, gboolean recursive)
{
    job->recursive = recursive;
}

static gpointer emit_cur_file(FmJob* job, gpointer cur_file)
{
    g_signal_emit(job, signals[CUR_FILE], 0, (const char*)cur_file);
    return NULL;
}

/**
 * fm_file_ops_job_emit_cur_file
 * @job: the job to emit signal
 * @cur_file: the data to emit
 *
 * Emits the #FmFileOpsJob::cur-file signal in main thread.
 *
 * This API is private to #FmFileOpsJob and should not be used outside
 * of libfm implementation.
 *
 * Since: 0.1.0
 */
void fm_file_ops_job_emit_cur_file(FmFileOpsJob* job, const char* cur_file)
{
    fm_job_call_main_thread(FM_JOB(job), emit_cur_file, (gpointer)cur_file);
}

static gpointer emit_percent(FmJob* job, gpointer percent)
{
    g_signal_emit(job, signals[PERCENT], 0, GPOINTER_TO_UINT(percent));
    return NULL;
}

/**
 * fm_file_ops_job_emit_percent
 * @job: the job to emit signal
 *
 * Emits the #FmFileOpsJob::percent signal in main thread.
 *
 * This API is private to #FmFileOpsJob and should not be used outside
 * of libfm implementation.
 *
 * Since: 0.1.0
 */
void fm_file_ops_job_emit_percent(FmFileOpsJob* job)
{
    guint percent;

    if (fm_job_is_cancelled(FM_JOB(job)))
        return;

    if(job->total > 0)
    {
        gdouble dpercent = (gdouble)(job->finished + job->current_file_finished) / job->total;
        percent = (guint)(dpercent * 100);
        if(percent > 100)
            percent = 100;
    }
    else
        percent = 100;

    if( percent > job->percent )
    {
        fm_job_call_main_thread(FM_JOB(job), emit_percent, GUINT_TO_POINTER(percent));
        job->percent = percent;
    }
}

static gpointer emit_prepared(FmJob* job, gpointer user_data)
{
    g_signal_emit(job, signals[PREPARED], 0);
    return NULL;
}

/**
 * fm_file_ops_job_emit_prepared
 * @job: the job to emit signal
 *
 * Emits the #FmFileOpsJob::prepared signal in main thread.
 *
 * This API is private to #FmFileOpsJob and should not be used outside
 * of libfm implementation.
 *
 * Since: 0.1.10
 */
void fm_file_ops_job_emit_prepared(FmFileOpsJob* job)
{
    fm_job_call_main_thread(FM_JOB(job), emit_prepared, NULL);
}

struct AskRename
{
    FmFileInfo* src_fi;
    FmFileInfo* dest_fi;
    char* new_name;
    FmFileOpOption ret;
};

static gpointer emit_ask_rename(FmJob* job, gpointer input_data)
{
#define data ((struct AskRename*)input_data)
    g_signal_emit(job, signals[ASK_RENAME], 0, data->src_fi, data->dest_fi, &data->new_name, &data->ret);
#undef data
    return NULL;
}

/**
 * fm_file_ops_job_ask_rename
 * @job: a job which asked
 * @src: source file descriptor
 * @src_inf: source file information
 * @dest: destination descriptor
 * @new_dest: pointer to get new destination
 *
 * Asks the user in main thread how to resolve conflict if file being
 * copied or moved already exists in destination directory. Ask is done
 * by emitting the #FmFileOpsJob::ask-rename signal.
 *
 * This API is private to #FmFileOpsJob and should not be used outside
 * of libfm implementation.
 *
 * Returns: a decision how to resolve conflict.
 *
 * Since: 0.1.0
 */
FmFileOpOption fm_file_ops_job_ask_rename(FmFileOpsJob* job, GFile* src, GFileInfo* src_inf, GFile* dest, GFile** new_dest)
{
    struct AskRename data;
    FmFileInfoJob* fijob;
    FmFileInfo *src_fi = NULL, *dest_fi = NULL;

    if (fm_job_is_cancelled(FM_JOB(job)))
        return 0;

    fijob = fm_file_info_job_new(NULL, 0);
    if( !src_inf )
        fm_file_info_job_add_gfile(fijob, src);
    else
        src_fi = fm_file_info_new_from_g_file_data(src, src_inf, NULL);
    fm_file_info_job_add_gfile(fijob, dest);

    fm_job_set_cancellable(FM_JOB(fijob), fm_job_get_cancellable(FM_JOB(job)));
    fm_job_run_sync(FM_JOB(fijob));

    if( fm_job_is_cancelled(FM_JOB(fijob)) )
    {
        if(src_fi)
            fm_file_info_unref(src_fi);
        g_object_unref(fijob);
        return 0;
    }

    if(!src_inf)
        src_fi = fm_file_info_list_pop_head(fijob->file_infos);
    dest_fi = fm_file_info_list_pop_head(fijob->file_infos);
    g_object_unref(fijob);
    if(!dest_fi) /* invalid destination directory! */
    {
        GError *err = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED,
                                          _("Cannot access destination file"));
        fm_job_emit_error(FM_JOB(job), err, FM_JOB_ERROR_CRITICAL);
        g_error_free(err);
        fm_file_info_unref(src_fi);
        return FM_FILE_OP_CANCEL;
    }

    data.ret = 0;
    data.src_fi = src_fi;
    data.dest_fi = dest_fi;
    data.new_name = NULL;
    fm_job_call_main_thread(FM_JOB(job), emit_ask_rename, (gpointer)&data);

    if(data.ret == FM_FILE_OP_RENAME)
    {
        if(data.new_name)
        {
            GFile* parent = g_file_get_parent(dest);
            *new_dest = g_file_get_child(parent, data.new_name);
            g_object_unref(parent);
            g_free(data.new_name);
        }
    }

    fm_file_info_unref(src_fi);
    fm_file_info_unref(dest_fi);

    return data.ret;
}

/* the same as fm_file_ops_job_ask_rename() but handles the case if the
   destination does not exist, i.e. error such as G_IO_ERROR_FILENAME_TOO_LONG */
FmFileOpOption _fm_file_ops_job_ask_new_name(FmFileOpsJob* job, GFile* src,
                                             GFile* dest, GFile** new_dest,
                                             gboolean dest_exists)
{
    struct AskRename data;
    FmFileInfoJob* fijob;
    FmFileInfo *src_fi = NULL, *dest_fi = NULL;

    if (fm_job_is_cancelled(FM_JOB(job)))
        return 0;

    fijob = fm_file_info_job_new(NULL, 0);
    fm_file_info_job_add_gfile(fijob, src);
    if (dest_exists)
        fm_file_info_job_add_gfile(fijob, dest);

    fm_job_set_cancellable(FM_JOB(fijob), fm_job_get_cancellable(FM_JOB(job)));
    fm_job_run_sync(FM_JOB(fijob));

    if( fm_job_is_cancelled(FM_JOB(fijob)) )
    {
        if(src_fi)
            fm_file_info_unref(src_fi);
        g_object_unref(fijob);
        return 0;
    }

    src_fi = fm_file_info_list_pop_head(fijob->file_infos);
    if (dest_exists)
        dest_fi = fm_file_info_list_pop_head(fijob->file_infos);
    else if (src_fi)
    {
        FmPath *dpath = fm_path_new_for_gfile(dest);
        /* forge dest_fi with just display name */
        dest_fi = fm_file_info_new();
        fm_file_info_set_path(dest_fi, dpath);
        _fm_path_set_display_name(dpath, fm_file_info_get_disp_name(src_fi));
        fm_path_unref(dpath);
    }
    g_object_unref(fijob);
    if(!dest_fi) /* invalid destination directory! */
    {
        GError *err = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED,
                                          _("Cannot access destination file"));
        fm_job_emit_error(FM_JOB(job), err, FM_JOB_ERROR_CRITICAL);
        g_error_free(err);
        if (src_fi)
            fm_file_info_unref(src_fi);
        return FM_FILE_OP_CANCEL;
    }

    data.ret = 0;
    data.src_fi = src_fi;
    data.dest_fi = dest_fi;
    data.new_name = NULL;
    fm_job_call_main_thread(FM_JOB(job), emit_ask_rename, (gpointer)&data);

    if(data.ret == FM_FILE_OP_RENAME)
    {
        if(data.new_name)
        {
            GFile* parent = g_file_get_parent(dest);
            *new_dest = g_file_get_child(parent, data.new_name);
            g_object_unref(parent);
            g_free(data.new_name);
        }
    }

    fm_file_info_unref(src_fi);
    fm_file_info_unref(dest_fi);

    return data.ret;
}

static gboolean _fm_file_ops_job_link_run(FmFileOpsJob* job)
{
    gboolean ret = TRUE;
    GFile *dest_dir;
    GList* l;
    FmJob* fmjob = FM_JOB(job);
    FmFolder *dest_folder;

    job->supported_options = FM_FILE_OP_RENAME | FM_FILE_OP_SKIP;
    dest_dir = fm_path_to_gfile(job->dest);

    /* cannot create links on non-native filesystems */
    if(!g_file_is_native(dest_dir))
    {
        GError *err = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED,
                                          _("Cannot create a link on non-native filesystem"));
        fm_job_emit_error(FM_JOB(job), err, FM_JOB_ERROR_CRITICAL);
        g_error_free(err);
        g_object_unref(dest_dir);
        return FALSE;
    }

    job->total = fm_path_list_get_length(job->srcs);
    g_debug("total files to link: %lu", (gulong)job->total);

    fm_file_ops_job_emit_prepared(job);

    dest_folder = fm_folder_find_by_path(job->dest);
    for(l = fm_path_list_peek_head_link(job->srcs);
        !fm_job_is_cancelled(fmjob) && l; l=l->next)
    {
        FmPath* path = FM_PATH(l->data);
        char* src = NULL;
        char *_basename = NULL;
        const char *basename = fm_path_get_basename(path);
        GFile* dest;
        GError* err = NULL;
        char* dname = NULL;

        /* if we drop URI query onto native filesystem, omit query part */
        if (!fm_path_is_native(path) && g_file_is_native(dest_dir))
            dname = strchr(basename, '?');
        /* if basename consist only from query then use first part of it */
        if (dname == basename)
        {
            basename++;
            dname = strchr(basename, '&');
        }
        if (dname)
        {
            _basename = g_strndup(basename, dname - basename);
            dname = strrchr(_basename, G_DIR_SEPARATOR);
            g_debug("cutting '%s' to '%s'",basename,dname?&dname[1]:_basename);
            if (dname)
                basename = &dname[1];
            else
                basename = _basename;
        }
        /* showing currently processed file. */
        dest = g_file_get_child(dest_dir, basename);
        dname = g_file_get_parse_name(dest);
        fm_file_ops_job_emit_cur_file(job, dname);
        g_free(dname);
        g_free(_basename);

_retry_link:
        if (fm_path_is_native(path))
        {
          if (src == NULL)
            src = fm_path_to_str(path);
          if(!g_file_make_symbolic_link(dest, src, fm_job_get_cancellable(fmjob), &err))
          {
_link_error:
            if(err)
            {
                if(err->domain == G_IO_ERROR && err->code == G_IO_ERROR_EXISTS)
                {
                    GFile *new_dest = NULL, *src_file;
                    FmFileOpOption opt;

                    src_file = fm_path_to_gfile(path);
                    opt = _fm_file_ops_job_ask_new_name(job, src_file, dest, &new_dest, TRUE);
                    g_object_unref(src_file);

                    g_error_free(err);
                    err = NULL;

                    switch(opt)
                    {
                    case FM_FILE_OP_RENAME:
                        if (new_dest) /* we got new dest */
                        {
                            g_object_unref(dest);
                            dest = new_dest;
                            goto _retry_link;
                        }
                        break;
                    case FM_FILE_OP_CANCEL:
                        fm_job_cancel(fmjob);
                        break;
                    case FM_FILE_OP_OVERWRITE:
                        /* we do not support overwrite */
                    case FM_FILE_OP_SKIP:
                        break;
                    case FM_FILE_OP_SKIP_ERROR: ; /* FIXME */
                    }
                    if (new_dest)
                        g_object_unref(new_dest);
                }
            }
            ret = FALSE;
          }
        }
        else /* create shortcut instead */
        {
            gsize out_len;
            /* unfortunately we cannot use g_file_replace_contents() here
               because we should handle the case if file already exists */
            GFileOutputStream *out = g_file_create(dest, G_FILE_CREATE_NONE,
                                                   fm_job_get_cancellable(fmjob),
                                                   &err);
            GFile *src_file;
            GFileInfo *inf;
            char *name = NULL, *iname = NULL;
            if (out == NULL)
                goto _link_error;
            src_file = fm_path_to_gfile(path);
            inf = g_file_query_info(src_file, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI","
                                              G_FILE_ATTRIBUTE_STANDARD_ICON","
                                              G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                    G_FILE_QUERY_INFO_NONE,
                                    fm_job_get_cancellable(fmjob), NULL);
            g_object_unref(src_file);
            if (inf)
            {
                GIcon *icon = g_file_info_get_icon(inf);

                /* set target icon if available */
                if (icon)
                {
                    iname = g_icon_to_string(icon);
                    if (iname && strncmp(iname, ". GThemedIcon ", 14) == 0)
                    {
                        char *tmp = strchr(&iname[14], ' ');
                        /* it is a themed icon, guess "right" name from it */
                        if (tmp)
                            *tmp = '\0';
                        tmp = g_strdup(&iname[14]);
                        g_free(iname);
                        iname = tmp;
                    }
                }
                /* FIXME: guess the icon if not available */
                src = g_strdup(g_file_info_get_attribute_string(inf, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI));
                name = g_strdup(g_file_info_get_display_name(inf));
                g_object_unref(inf);
            }
            if (src == NULL)
                src = fm_path_to_uri(path);
            if (name == NULL)
                name = fm_path_display_basename(path);
            dname = g_strdup_printf("[Desktop Entry]\n"
                                    "Type=Link\n"
                                    "Name=%s"
                                    "%s%s\n"
                                    "URL=%s\n", name,
                                    iname ? "\nIcon=" : "", iname ? iname : "",
                                    src);
            g_free(name);
            g_free(iname);
            if (!g_output_stream_write_all(G_OUTPUT_STREAM(out), dname,
                                           strlen(dname), &out_len,
                                           fm_job_get_cancellable(fmjob), &err) ||
                !g_output_stream_close(G_OUTPUT_STREAM(out),
                                       fm_job_get_cancellable(fmjob), &err))
            {
                g_object_unref(out);
                g_free(dname);
                goto _link_error;
            }
            g_object_unref(out);
            g_free(dname);
        }

        job->finished++;

        /* update progress */
        fm_file_ops_job_emit_percent(job);
        if (ret && dest_folder)
        {
            FmPath *dest_path = fm_path_new_for_gfile(dest);
            if (!_fm_folder_event_file_added(dest_folder, dest_path))
                fm_path_unref(dest_path);
        }

        g_free(src);
        g_object_unref(dest);
    }

    /* g_debug("finished: %llu, total: %llu", job->finished, job->total); */

    g_object_unref(dest_dir);
    if (dest_folder)
        g_object_unref(dest_folder);
    return ret;
}

/**
 * fm_file_ops_job_set_display_name
 * @job: a job to set
 * @name: new display name
 *
 * Sets that file for file operation FM_FILE_OP_CHANGE_ATTR should have
 * display name changed according to @name. The job will fail if it will
 * be started for more than one file or if file doesn't support display
 * name change.
 *
 * This API may be used only before @job is started.
 *
 * Since: 1.2.0
 */
void fm_file_ops_job_set_display_name(FmFileOpsJob *job, const char *name)
{
    g_return_if_fail(FM_IS_FILE_OPS_JOB(job));
    g_free(job->display_name);
    job->display_name = g_strdup(name);
}

/**
 * fm_file_ops_job_set_icon
 * @job: a job to set
 * @icon: new icon
 *
 * Sets that files for file operation FM_FILE_OP_CHANGE_ATTR should have
 * associated icon changed according to @icon. Error will be generated
 * if some of the files doesn't support icon change.
 *
 * This API may be used only before @job is started.
 *
 * Since: 1.2.0
 */
void fm_file_ops_job_set_icon(FmFileOpsJob *job, GIcon *icon)
{
    g_return_if_fail(FM_IS_FILE_OPS_JOB(job));
    if (G_UNLIKELY(job->icon))
        g_free(job->icon);
    job->icon = NULL;
    if (G_LIKELY(icon))
        job->icon = g_object_ref(icon);
}

/**
 * fm_file_ops_job_set_hidden
 * @job: a job to set
 * @hidden: new hidden attribute
 *
 * Sets that files for file operation FM_FILE_OP_CHANGE_ATTR should have
 * 'hidden' attribute changed according to value @hidden. Error will be
 * generated if some of the files doesn't support such change.
 *
 * This API may be used only before @job is started.
 *
 * Since: 1.2.0
 */
void fm_file_ops_job_set_hidden(FmFileOpsJob *job, gboolean hidden)
{
    g_return_if_fail(FM_IS_FILE_OPS_JOB(job));
    job->set_hidden = hidden ? 1 : 0;
}

/**
 * fm_file_ops_job_set_target
 * @job: a job to set
 * @url: new URL for shortcut
 *
 * Sets that shortcut file for file operation FM_FILE_OP_CHANGE_ATTR
 * should have its target URL changed according to @url. The job will
 * fail if it will be started for more than one file or if file doesn't
 * support target change.
 *
 * This API may be used only before @job is started.
 *
 * Since: 1.2.0
 */
void fm_file_ops_job_set_target(FmFileOpsJob *job, const char *url)
{
    g_return_if_fail(FM_IS_FILE_OPS_JOB(job));
    g_free(job->target);
    job->target = g_strdup(url);
}

/**
 * fm_file_ops_job_get_options
 * @job: a job to set
 *
 * Retrieves bitmask set of options that are supported as return of the
 * #FmFileOpsJob::ask-rename signal handler.
 *
 * Returns: list of options.
 *
 * Since: 1.2.0
 */
FmFileOpOption fm_file_ops_job_get_options(FmFileOpsJob* job)
{
    g_return_val_if_fail(FM_IS_FILE_OPS_JOB(job), 0);
    return job->supported_options;
}
