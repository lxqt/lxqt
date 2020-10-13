/*
 *      glib-compat.h
 *
 *      Copyright 2011 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef __GLIB_COMPAT_H__
#define __GLIB_COMPAT_H__
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/* GLib prior 2.24 have no such macro */
#ifndef G_DEFINE_INTERFACE
#   define G_DEFINE_INTERFACE(TN, t_n, T_P) \
static void     t_n##_default_init        (TN##Interface *klass); \
GType t_n##_get_type (void) \
{ \
  static volatile gsize g_define_type_id__volatile = 0; \
  if (g_once_init_enter (&g_define_type_id__volatile))  \
    { \
      GType g_define_type_id = \
        g_type_register_static_simple (G_TYPE_INTERFACE, \
                                       g_intern_static_string (#TN), \
                                       sizeof (TN##Interface), \
                                       (GClassInitFunc)t_n##_default_init, \
                                       0, \
                                       (GInstanceInitFunc)NULL, \
                                       (GTypeFlags) 0); \
      if (T_P) \
        g_type_interface_add_prerequisite (g_define_type_id, T_P); \
      g_once_init_leave (&g_define_type_id__volatile, g_define_type_id); \
    } \
  return g_define_type_id__volatile; \
} /* closes t_n##_get_type() */
#endif /* G_DEFINE_INTERFACE */

#if !GLIB_CHECK_VERSION(2, 28, 0)
gboolean
g_signal_accumulator_first_wins (GSignalInvocationHint *ihint,
                                 GValue                *return_accu,
                                 const GValue          *handler_return,
                                 gpointer               dummy);
#endif

#if !GLIB_CHECK_VERSION(2, 28, 0)

/* This API was added in glib 2.28 */

#define g_slist_free_full(slist, free_func)	\
{ \
g_slist_foreach(slist, (GFunc)free_func, NULL); \
g_slist_free(slist); \
}

#define g_list_free_full(list, free_func)	\
{ \
g_list_foreach(list, (GFunc)free_func, NULL); \
g_list_free(list); \
}

#endif

#if !GLIB_CHECK_VERSION(2, 34, 0)
/* This useful API was added in glib 2.34 */
static inline GSList *g_slist_copy_deep(GSList *list, GCopyFunc func, gpointer user_data)
{
    GSList *new_list = g_slist_copy(list), *l;
    for(l = new_list; l; l = l->next)
        l->data = func(l->data, user_data);
    return new_list;
}
#endif

G_END_DECLS

#endif
