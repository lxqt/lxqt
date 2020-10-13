/*
 *      fm-simple-job.c
 *
 *      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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
 * SECTION:fm-simple-job
 * @short_description: Job to run a function asynchronously.
 * @title: FmSimpleJob
 *
 * @include: libfm/fm.h
 *
 * The #FmJob can be used to create asynchronous job which just run some
 * simple function with provided data.
 */

#include "fm-simple-job.h"

#define FM_TYPE_SIMPLE_JOB              (fm_simple_job_get_type())
#define FM_SIMPLE_JOB(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_TYPE_SIMPLE_JOB, FmSimpleJob))
#define FM_SIMPLE_JOB_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_TYPE_SIMPLE_JOB, FmSimpleJobClass))
#define FM_IS_SIMPLE_JOB(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_TYPE_SIMPLE_JOB))
#define FM_IS_SIMPLE_JOB_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_TYPE_SIMPLE_JOB))

typedef struct _FmSimpleJob         FmSimpleJob;
typedef struct _FmSimpleJobClass        FmSimpleJobClass;

struct _FmSimpleJob
{
    FmJob parent;
    FmSimpleJobFunc func;
    gpointer user_data;
    GDestroyNotify destroy_data;
};

struct _FmSimpleJobClass
{
    FmJobClass parent_class;
};

G_DEFINE_TYPE(FmSimpleJob, fm_simple_job, FM_TYPE_JOB);

static void fm_simple_job_finalize              (GObject *object);
static gboolean fm_simple_job_run(FmJob *job);


static void fm_simple_job_class_init(FmSimpleJobClass *klass)
{
    GObjectClass *g_object_class;
    FmJobClass* job_class = FM_JOB_CLASS(klass);
    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->finalize = fm_simple_job_finalize;

    job_class->run = fm_simple_job_run;
}


static void fm_simple_job_finalize(GObject *object)
{
    FmSimpleJob *self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_SIMPLE_JOB(object));

    self = (FmSimpleJob*)object;
    if(self->user_data && self->destroy_data)
        self->destroy_data(self->user_data);

    G_OBJECT_CLASS(fm_simple_job_parent_class)->finalize(object);
}


static void fm_simple_job_init(FmSimpleJob *self)
{

}

/**
 * fm_simple_job_new
 * @func: user function to run asynchronously
 * @user_data: user data provided for @func
 * @destroy_data: user function to free data after job finished
 *
 * Creates a new simple #FmJob for user task.
 *
 * Returns: (transfer full): a new #FmJob object.
 *
 * Since: 0.1.0
 */
FmJob*  fm_simple_job_new(FmSimpleJobFunc func, gpointer user_data, GDestroyNotify destroy_data)
{
    FmSimpleJob* job = (FmSimpleJob*)g_object_new(FM_TYPE_SIMPLE_JOB, NULL);
    job->func = func;
    job->user_data = user_data;
    job->destroy_data = destroy_data;
    return (FmJob*)job;
}

static gboolean fm_simple_job_run(FmJob *job)
{
    FmSimpleJob* sjob = FM_SIMPLE_JOB(job);
    return sjob->func ? sjob->func(job, sjob->user_data) : FALSE;
}
