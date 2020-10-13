/*
 *      fm-job.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2012-2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define FM_DISABLE_SEAL

#include "fm-job.h"
#include "fm-marshal.h"
#include "glib-compat.h"
#include "fm-utils.h"

/**
 * SECTION:fm-job
 * @short_description: Base class of all kinds of asynchronous jobs.
 * @title: FmJob
 *
 * @include: libfm/fm.h
 *
 * The #FmJob can be used to create asynchronous jobs performing some
 * time-consuming tasks in another worker thread.
 * To run a #FmJob in another thread you simply call
 * fm_job_run_async(), and then the task will be done in another
 * worker thread. Later, when the job is finished, #FmJob::finished signal is
 * emitted. When the job is still running, it's possible to cancel it
 * from main thread by calling fm_job_cancel(). Then, #FmJob::cancelled signal
 * will be emitted before emitting #FmJob::finished signal. You can also run
 * the job in blocking fashion instead of running it asynchronously by
 * calling fm_job_run_sync().
 */

enum {
    FINISHED,
    ERROR,
    CANCELLED,
    ASK,
    N_SIGNALS
};

typedef struct _FmIdleCall
{
    FmJob* job;
    FmJobCallMainThreadFunc func;
    gpointer user_data;
    gpointer ret;
}FmIdleCall;

static void fm_job_finalize              (GObject *object);
/*
static gboolean fm_job_error_accumulator(GSignalInvocationHint *ihint, GValue *return_accu,
                                           const GValue *handler_return, gpointer data);
*/
static void on_cancellable_cancelled(GCancellable* cancellable, FmJob* job);

G_DEFINE_ABSTRACT_TYPE(FmJob, fm_job, G_TYPE_OBJECT);

static gboolean fm_job_real_run_async(FmJob* job);
static gboolean on_idle_cleanup(gpointer unused);
static void job_thread(FmJob* job, gpointer unused);

static guint idle_handler = 0;
static GSList* finished = NULL;
G_LOCK_DEFINE_STATIC(idle_handler);

static GThreadPool* thread_pool = NULL;
static guint n_jobs = 0;

static guint signals[N_SIGNALS];

static void fm_job_emit_finished(FmJob* job)
{
    g_signal_emit(job, signals[FINISHED], 0);
}

static void fm_job_emit_cancelled(FmJob* job)
{
    g_signal_emit(job, signals[CANCELLED], 0);
}

static void fm_job_dispose(GObject *object)
{
    FmJob *self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_JOB(object));

    self = (FmJob*)object;

    if(self->cancellable)
    {
        g_signal_handlers_disconnect_by_func(self->cancellable, on_cancellable_cancelled, self);
        g_object_unref(self->cancellable);
        self->cancellable = NULL;
#if GLIB_CHECK_VERSION(2, 32, 0)
        g_rec_mutex_clear(&self->stop);
#else
        g_static_rec_mutex_free(&self->stop);
#endif
    }

    G_OBJECT_CLASS(fm_job_parent_class)->dispose(object);
}

static void fm_job_class_init(FmJobClass *klass)
{
    GObjectClass *g_object_class;

    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->dispose = fm_job_dispose;
    g_object_class->finalize = fm_job_finalize;

    klass->run_async = fm_job_real_run_async;

    fm_job_parent_class = (GObjectClass*)g_type_class_peek(G_TYPE_OBJECT);

    /**
     * FmJob::finished:
     * @job: a job that emitted the signal
     *
     * The #FmJob::finished signal is emitted after the job is finished.
     * The signal is never emitted if the fm_job_run_XXX function
     * returned %FALSE, in that case the #FmJob::cancelled signal will be
     * emitted instead.
     *
     * Since: 0.1.0
     */
    signals[FINISHED] =
        g_signal_new( "finished",
                      G_TYPE_FROM_CLASS ( klass ),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET ( FmJobClass, finished ),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0 );

    /**
     * FmJob::error:
     * @job: a job that emitted the signal
     * @error: an error descriptor
     * @severity: #FmJobErrorSeverity of the error
     *
     * The #FmJob::error signal is emitted when errors happen. A case if
     * more than one handler is connected to this signal is ambiguous.
     *
     * Return value: #FmJobErrorAction that should be performed on that error.
     *
     * Since: 0.1.0
     */
    signals[ERROR] =
        g_signal_new( "error",
                      G_TYPE_FROM_CLASS ( klass ),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET ( FmJobClass, error ),
                      NULL /*fm_job_error_accumulator*/, NULL,
#if GLIB_CHECK_VERSION(2,26,0)
                      fm_marshal_UINT__BOXED_UINT,
                      G_TYPE_UINT, 2, G_TYPE_ERROR, G_TYPE_UINT );
#else
                      fm_marshal_INT__POINTER_INT,
                      G_TYPE_INT, 2, G_TYPE_POINTER, G_TYPE_INT );
#endif

    /**
     * FmJob::cancelled:
     * @job: a job that emitted the signal
     *
     * The #FmJob::cancelled signal is emitted when the job is cancelled
     * or aborted due to critical errors.
     *
     * Since: 0.1.0
     */
    signals[CANCELLED] =
        g_signal_new( "cancelled",
                      G_TYPE_FROM_CLASS ( klass ),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET ( FmJobClass, cancelled ),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0 );

    /**
     * FmJob::ask:
     * @job: a job that emitted the signal
     * @question: (const gchar *) a question to ask user
     * @options: (gchar* const *) list of choices to ask user
     *
     * The #FmJob::ask signal is emitted when the job asks for some
     * user interactions. The user then will have a list of available
     * @options. If there is more than one handler connected to the
     * signal then only one of them will receive it.
     *
     * Return value: user's choice.
     *
     * Since: 0.1.0
     */
    signals[ASK] =
        g_signal_new( "ask",
                      G_TYPE_FROM_CLASS ( klass ),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET ( FmJobClass, ask ),
                      g_signal_accumulator_first_wins, NULL,
                      fm_marshal_INT__POINTER_POINTER,
                      G_TYPE_INT, 2, G_TYPE_POINTER, G_TYPE_POINTER );
}

static void fm_job_init(FmJob *self)
{
    /* create the thread pool if it doesn't exist. */
    if( G_UNLIKELY(!thread_pool) )
        thread_pool = g_thread_pool_new((GFunc)job_thread, NULL, -1, FALSE, NULL);
    ++n_jobs;
}

/**
 * fm_job_new
 *
 * Creates a new #FmJob object.
 *
 * Returns: a new #FmJob object.
 *
 * Since: 0.1.0
 */
FmJob* fm_job_new(void)
{
    return (FmJob*)g_object_new(FM_TYPE_JOB, NULL);
}


static void fm_job_finalize(GObject *object)
{
    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_JOB(object));

    if (G_OBJECT_CLASS(fm_job_parent_class)->finalize)
        (* G_OBJECT_CLASS(fm_job_parent_class)->finalize)(object);

    --n_jobs;
    if(0 == n_jobs)
    {
        g_thread_pool_free(thread_pool, TRUE, FALSE);
        thread_pool = NULL;
    }
}

static gboolean fm_job_real_run_async(FmJob* job)
{
    g_thread_pool_push(thread_pool, job, NULL);
    return TRUE;
}

/**
 * fm_job_run_async
 * @job: a job to run
 *
 * Starts the @job asyncronously creating new thread. If job starts
 * successfully then the #FmJob::finished signal will be emitted when
 * @job is either succeeded or was cancelled. If @job could not be
 * started then #FmJob::cancelled signal is emitted before return from
 * this function.
 *
 * Returns: %TRUE if job started successfully.
 *
 * Since: 0.1.0
 */
gboolean fm_job_run_async(FmJob* job)
{
    FmJobClass* klass = FM_JOB_CLASS(G_OBJECT_GET_CLASS(job));
    gboolean ret;
    job->running = TRUE;
    g_object_ref(job); /* acquire a ref, it will be unrefed by on_idle_cleanup() */
    ret = klass->run_async(job);
    if(G_UNLIKELY(!ret)) /* failed? */
    {
        fm_job_emit_cancelled(job);
        g_object_unref(job);
    }
    return ret;
}

/**
 * fm_job_run_sync
 * @job: a job to run
 *
 * Runs the @job in current thread in a blocking fashion. The job will
 * emit either #FmJob::cancelled signal if job was cancelled
 * or #FmJob::finished signal if it finished successfully.
 *
 * Returns: %TRUE if @job ran successfully.
 *
 * Since: 0.1.0
 */
/* run a job in current thread in a blocking fashion.  */
gboolean fm_job_run_sync(FmJob* job)
{
    FmJobClass* klass = FM_JOB_CLASS(G_OBJECT_GET_CLASS(job));
    gboolean ret;
    job->running = TRUE;
    ret = klass->run(job);
    job->running = FALSE;
    if(job->cancel)
        fm_job_emit_cancelled(job);
    else
        fm_job_emit_finished(job);
    return ret;
}

static void on_sync_job_finished(FmJob* job, GMainLoop* mainloop)
{
    g_main_loop_quit(mainloop);
    job->running = FALSE;
}

/**
 * fm_job_run_sync_with_mainloop
 * @job: a job to run
 *
 * Runs the @job in current thread in a blocking fashion and an additional
 * mainloop being created to prevent blocking of user interface. If @job
 * started successfully then #FmJob::finished signal is emitted when @job
 * is either succeeded or was cancelled.
 * Note: using this API from within GTK main loop will lead to deadlock
 * therefore if it is a GTK application then caller should unlock GDK
 * threads before calling this API and lock them back after return from
 * it. This statement is valid for any GTK application that uses locks.
 *
 * Returns: %TRUE if job started successfully.
 *
 * Since: 0.1.1
 */
gboolean fm_job_run_sync_with_mainloop(FmJob* job)
{
    GMainLoop* mainloop = g_main_loop_new(NULL, FALSE);
    gboolean ret;
    g_signal_connect(job, "finished", G_CALLBACK(on_sync_job_finished), mainloop);
    ret = fm_job_run_async(job);
    if(G_LIKELY(ret))
    {
        g_main_loop_run(mainloop);
    }
    g_signal_handlers_disconnect_by_func(job, on_sync_job_finished, mainloop);
    g_main_loop_unref(mainloop);
    return ret;
}

/* this is called from working thread */
static void job_thread(FmJob* job, gpointer unused)
{
    FmJobClass* klass = FM_JOB_CLASS(G_OBJECT_GET_CLASS(job));
    klass->run(job);

    /* let the main thread know that we're done, and free the job
     * in idle handler if neede. */
    fm_job_finish(job);
}

/**
 * fm_job_cancel
 * @job: a job to cancel
 *
 * Cancels the @job.
 *
 * Since: 0.1.0
 */
/* cancel the job */
void fm_job_cancel(FmJob* job)
{
    FmJobClass* klass = FM_JOB_CLASS(G_OBJECT_GET_CLASS(job));
    job->cancel = TRUE;
    if(job->cancellable)
        g_cancellable_cancel(job->cancellable);
    if(klass->cancel)
        klass->cancel(job);
}

static gboolean on_idle_call(gpointer input_data)
{
    FmIdleCall* data = (FmIdleCall*)input_data;
    data->ret = data->func(data->job, data->user_data);
    return FALSE;
}

/**
 * fm_job_call_main_thread
 * @job: the job that calls main thread
 * @func: callback to run from main thread
 * @user_data: user data for the callback
 *
 * Stops calling thread, waits main thread for idle, passes @user_data
 * to callback @func in main thread, gathers result of callback, and
 * returns it to caller.
 *
 * This APIs is private to #FmJob and should only be used in the
 * implementation of classes derived from #FmJob.
 *
 * This function should be called from working thread only.
 *
 * Returns: return value from running @func.
 *
 * Since: 0.1.0
 */
gpointer fm_job_call_main_thread(FmJob* job,
                                 FmJobCallMainThreadFunc func, gpointer user_data)
{
    FmIdleCall data;
    data.job = job;
    data.func = func;
    data.user_data = user_data;
    fm_run_in_default_main_context(on_idle_call, &data);
    return data.ret;
}

/**
 * fm_job_finish
 * @job: the job that was finished
 *
 * Schedules the finishing of job. Once this function is called the
 * @job becomes invalid for the caller and should be not used anymore.
 *
 * This APIs is private to #FmJob and should only be used in the
 * implementation of classes derived from #FmJob.
 *
 * This function should be called from working thread only.
 *
 * Since: 0.1.0
 */
void fm_job_finish(FmJob* job)
{
    G_LOCK(idle_handler);
    if(0 == idle_handler)
        idle_handler = g_idle_add(on_idle_cleanup, NULL);
    finished = g_slist_append(finished, job);
    job->running = FALSE;
    G_UNLOCK(idle_handler);
}

struct AskData
{
    const char* question;
    gchar* const *options;
};

static gpointer ask_in_main_thread(FmJob* job, gpointer input_data)
{
    gint ret;
#define data ((struct AskData*)input_data)
    g_signal_emit(job, signals[ASK], 0, data->question, data->options, &ret);
#undef data
    return GINT_TO_POINTER(ret);
}

/**
 * fm_job_ask
 * @job: the job that calls main thread
 * @question: the text to ask the user
 * @...: list of choices to give the user
 *
 * Asks the user for some interactions. The user will have a list of
 * available options and should make a choice.
 *
 * This APIs is private to #FmJob and should only be used in the
 * implementation of classes derived from #FmJob.
 *
 * This function should be called from working thread only.
 *
 * Returns: user's choice.
 *
 * Since: 0.1.0
 */
gint fm_job_ask(FmJob* job, const char* question, ...)
{
    gint ret;
    va_list args;
    va_start (args, question);
    ret = fm_job_ask_valist(job, question, args);
    va_end (args);
    return ret;
}

/**
 * fm_job_askv
 * @job: the job that calls main thread
 * @question: the text to ask the user
 * @options: list of choices to give the user
 *
 * Asks the user for some interactions. The user will have a list of
 * available options and should make a choice.
 *
 * This APIs is private to #FmJob and should only be used in the
 * implementation of classes derived from #FmJob.
 *
 * This function should be called from working thread only.
 *
 * Returns: user's choice.
 *
 * Since: 0.1.0
 */
gint fm_job_askv(FmJob* job, const char* question, gchar* const *options)
{
    struct AskData data;
    data.question = question;
    data.options = options;
    return GPOINTER_TO_INT(fm_job_call_main_thread(job, ask_in_main_thread, &data));
}

/**
 * fm_job_ask_valist
 * @job: the job that calls main thread
 * @question: the text to ask the user
 * @options: list of choices to give the user
 *
 * Asks the user for some interactions. The user will have a list of
 * available options and should make a choice.
 *
 * This APIs is private to #FmJob and should only be used in the
 * implementation of classes derived from #FmJob.
 *
 * This function should be called from working thread only.
 *
 * Returns: user's choice.
 *
 * Since: 0.1.0
 */
gint fm_job_ask_valist(FmJob* job, const char* question, va_list options)
{
    GArray* opts = g_array_sized_new(TRUE, TRUE, sizeof(char*), 6);
    gint ret;
    const char* opt = va_arg(options, const char*);
    while(opt)
    {
        g_array_append_val(opts, opt);
        opt = va_arg (options, const char *);
    }
    ret = fm_job_askv(job, question, &opts->data);
    g_array_free(opts, TRUE);
    return ret;
}


/* unref finished job objects in main thread on idle */
static gboolean on_idle_cleanup(gpointer unused)
{
    GSList* jobs;
    GSList* l;

    G_LOCK(idle_handler);
    jobs = finished;
    finished = NULL;
    idle_handler = 0;
    G_UNLOCK(idle_handler);

    for(l = jobs; l; l=l->next)
    {
        FmJob* job = FM_JOB(l->data);
        if(job->cancel)
            fm_job_emit_cancelled(job);
        fm_job_emit_finished(job);
        g_object_unref(job);
    }
    g_slist_free(jobs);
    return FALSE;
}

/**
 * fm_job_init_cancellable
 * @job: the job to init
 *
 * Used by derived classes to implement #FmJobClass:run() using gio inside.
 * This API tries to initialize a #GCancellable object for use with gio and
 * should only be called once in the constructor of derived classes which
 * require the use of #GCancellable.
 *
 * This APIs is private to #FmJob and should only be used in the
 * implementation of classes derived from #FmJob.
 *
 * Since: 0.1.0
 */
void fm_job_init_cancellable(FmJob* job)
{
#if GLIB_CHECK_VERSION(2, 32, 0)
    g_rec_mutex_init(&job->stop);
#else
    g_static_rec_mutex_init(&job->stop);
#endif
    job->cancellable = g_cancellable_new();
    g_signal_connect(job->cancellable, "cancelled", G_CALLBACK(on_cancellable_cancelled), job);
}

/**
 * fm_job_get_cancellable
 * @job: the job to inspect
 *
 * Get an existing #GCancellable object from @job for use with gio in
 * another job by calling fm_job_set_cancellable().
 * This can be used when you wish to share a cancellable object
 * among different jobs.
 *
 * This APIs is private to #FmJob and should only be used in the
 * implementation of classes derived from #FmJob.
 *
 * Returns: a #GCancellable object if it was initialized for @job.
 *
 * Since: 0.1.9
 */
GCancellable* fm_job_get_cancellable(FmJob* job)
{
    return job->cancellable;
}

/**
 * fm_job_set_cancellable
 * @job: the job to set
 * @cancellable: (allow-none): a shared cancellable object
 *
 * Lets the job to use an existing @cancellable object.
 * This can be used when you wish to share a cancellable object
 * among different jobs.
 * This should only be called before the @job is launched.
 *
 * This APIs is private to #FmJob and should only be used in the
 * implementation of classes derived from #FmJob.
 *
 * Since: 0.1.0
 */
void fm_job_set_cancellable(FmJob* job, GCancellable* cancellable)
{
    if(G_UNLIKELY(job->cancellable))
    {
        g_signal_handlers_disconnect_by_func(job->cancellable, on_cancellable_cancelled, job);
        g_object_unref(job->cancellable);
    }
    if(G_LIKELY(cancellable))
    {
        job->cancellable = (GCancellable*)g_object_ref(cancellable);
        g_signal_connect(job->cancellable, "cancelled", G_CALLBACK(on_cancellable_cancelled), job);
    }
    else
        job->cancellable = NULL;
}

static void on_cancellable_cancelled(GCancellable* cancellable, FmJob* job)
{
    job->cancel = TRUE;
}

struct ErrData
{
    GError* err;
    FmJobErrorSeverity severity;
};

static gpointer error_in_main_thread(FmJob* job, gpointer input_data)
{
    guint ret;
#define data ((struct ErrData*)input_data)
    g_debug("FmJob error: %s", data->err->message);
    g_signal_emit(job, signals[ERROR], 0, data->err, (guint)data->severity, &ret);
#undef data
    return GUINT_TO_POINTER(ret);
}

/**
 * fm_job_emit_error
 * @job: a job that emitted the signal
 * @err: an error descriptor
 * @severity: severity of the error
 *
 * Emits an #FmJob::error signal in the main thread to notify it when an
 * error occurs.
 * The return value of this function is the return value returned by
 * the connected signal handlers.
 * If @severity is FM_JOB_ERROR_CRITICAL, the returned value is ignored and
 * fm_job_cancel() is called to abort the job. Otherwise, the signal
 * handler of this error can return FM_JOB_RETRY to ask for retrying the
 * failed operation, return FM_JOB_CONTINUE to ignore the error and
 * continue the remaining job, or return FM_JOB_ABORT to abort the job.
 * If FM_JOB_ABORT is returned by the signal handler, fm_job_cancel()
 * will be called in fm_job_emit_error().
 *
 * This APIs is private to #FmJob and should only be used in the
 * implementation of classes derived from #FmJob.
 *
 * This function should be called from working thread only.
 *
 * Returns: action that should be performed on that error.
 *
 * Since: 0.1.0
 */
FmJobErrorAction fm_job_emit_error(FmJob* job, GError* err, FmJobErrorSeverity severity)
{
    FmJobErrorAction ret;
    struct ErrData data;
    g_return_val_if_fail(err, FM_JOB_ABORT);
    data.err = err;
    data.severity = severity;
    ret = GPOINTER_TO_UINT(fm_job_call_main_thread(job, error_in_main_thread, &data));
    if(severity == FM_JOB_ERROR_CRITICAL || ret == FM_JOB_ABORT)
    {
        ret = FM_JOB_ABORT;
        fm_job_cancel(job);
    }

    /* If the job is already cancelled, retry is not allowed. */
    if(ret == FM_JOB_RETRY )
    {
        if(job->cancel || (err->domain == G_IO_ERROR && err->code == G_IO_ERROR_CANCELLED))
            ret = FM_JOB_CONTINUE;
    }

    return ret;
}

/* FIXME: need to re-think how to do this in a correct way. */
/*
gboolean fm_job_error_accumulator(GSignalInvocationHint *ihint, GValue *return_accu,
                                   const GValue *handler_return, gpointer data)
{
    int val = g_value_get_int(handler_return);
    g_debug("accumulate: %d, %d", g_value_get_int(return_accu), val);
    g_value_set_int(return_accu, val);
    return val != FM_JOB_CONTINUE;
}
*/

#if !GLIB_CHECK_VERSION(2, 32, 0)
#define g_rec_mutex_lock g_static_rec_mutex_lock
#define g_rec_mutex_unlock g_static_rec_mutex_unlock
#endif

/* lock the job but be sure lock is single */
static void _ensure_job_locked(FmJob *job)
{
    g_rec_mutex_lock(&job->stop);
    if (job->suspended)
        g_rec_mutex_unlock(&job->stop); /* drop extra lock */
}

/**
 * fm_job_is_cancelled
 * @job: the job to inspect
 *
 * Checks if the job is already cancelled.
 *
 * Returns: %TRUE if the job is already cancelled.
 *
 * Since: 0.1.9
 */
gboolean fm_job_is_cancelled(FmJob* job)
{
    g_rec_mutex_lock(&job->stop);
    /* wait on lock */
    g_rec_mutex_unlock(&job->stop);
    return job->cancel;
}

/**
 * fm_job_is_running
 * @job: the job to inspect
 *
 * Checks if the job is still running.
 *
 * Returns: %TRUE if the job is still running.
 *
 * Since: 0.1.9
 */
gboolean fm_job_is_running(FmJob* job)
{
    return job->running;
}

/**
 * fm_job_pause
 * @job: a job to apply
 *
 * Locks execution of job until next call to fm_job_resume(). This call
 * may be used from thread different from the thread where the job runs
 * in. This call may be done again (but have no extra effect) from the
 * same thread. Any other usage may lead to deadlock.
 *
 * Returns: %FALSE if job cannot be locked.
 *
 * Since: 1.2.0
 */
gboolean fm_job_pause(FmJob *job)
{
    g_return_val_if_fail(job != NULL && FM_IS_JOB(job), FALSE);
    if (!job->cancellable)
        return FALSE;
    _ensure_job_locked(job); /* acquire lock */
    job->suspended = TRUE; /* mark appropriately */
    return TRUE;
}

/**
 * fm_job_resume
 * @job: a job to apply
 *
 * Unlocks execution of @job that was made by previous call to
 * fm_job_pause(). This call may be used only from the same thread
 * where previous fm_job_pause() was made. Any other usage may lead to
 * deadlock.
 *
 * Since: 1.2.0
 */
void fm_job_resume(FmJob *job)
{
    g_return_if_fail(job != NULL && FM_IS_JOB(job));
    if (!job->cancellable)
        return;
    _ensure_job_locked(job); /* acquire lock... */
    job->suspended = FALSE;
    g_rec_mutex_unlock(&job->stop); /* ...and drop it */
}
