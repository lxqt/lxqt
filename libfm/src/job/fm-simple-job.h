/*
 *      fm-simple-job.h
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


#ifndef __FM_SIMPLE_JOB_H__
#define __FM_SIMPLE_JOB_H__

#include "fm-job.h"

G_BEGIN_DECLS

/**
 * FmSimpleJobFunc
 * @job: the job object
 * @user_data: user data provided on fm_simple_job_new() call
 *
 * The user function which will be ran asynchronously by #FmJob API.
 *
 * Return value: value to return from fm_job_run_sync().
 */
typedef gboolean (*FmSimpleJobFunc)(FmJob* job, gpointer user_data);

GType fm_simple_job_get_type(void);
FmJob* fm_simple_job_new(FmSimpleJobFunc func, gpointer user_data, GDestroyNotify destroy_data);

G_END_DECLS

#endif /* __FM_SIMPLE_JOB_H__ */
