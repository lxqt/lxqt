/*
 *      fm-list.h
 *      A generic list container supporting reference counting.
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2012 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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


#ifndef __FM_LIST_H__
#define __FM_LIST_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _FmList			FmList;
typedef struct _FmListFuncs		FmListFuncs;

struct _FmList
{
    /*< private >*/
    GQueue list;
    FmListFuncs* funcs;
    gint n_ref;
};

/**
 * FmListFuncs:
 * @item_ref: function to increase reference counter on item
 * @item_unref: function to decrease reference counter on item
 */
struct _FmListFuncs
{
    gpointer (*item_ref)(gpointer item);
    void (*item_unref)(gpointer item);
};

FmList* fm_list_new(FmListFuncs* funcs);

FmList* fm_list_ref(FmList* list);
void fm_list_unref(FmList* list);

#define FM_LIST(list)	((FmList*)list)

/* Since FmList is actually a GQueue with reference counting,
 * all APIs for GQueue should be usable */

void fm_list_clear(FmList* list);
#ifndef __GTK_DOC_IGNORE__
static inline gboolean fm_list_is_empty(FmList* list)
{
    return g_queue_is_empty((GQueue*)list);
}
static inline guint fm_list_get_length(FmList* list)
{
    return g_queue_get_length((GQueue*)list);
}

static inline void fm_list_reverse(FmList* list)
{
    g_queue_reverse((GQueue*)list);
}
static inline void fm_list_foreach(FmList* list, GFunc f, gpointer d)
{
    g_queue_foreach((GQueue*)list,f,d);
}
static inline GList* fm_list_find(FmList* list, gpointer d)
{
    return g_queue_find((GQueue*)list,d);
}
static inline GList* fm_list_find_custom(FmList* list, gconstpointer d, GCompareFunc f)
{
    return g_queue_find_custom((GQueue*)list,d,f);
}
static inline void fm_list_sort(FmList* list, GCompareDataFunc f, gpointer d)
{
    g_queue_sort((GQueue*)list,f,d);
}

static inline void fm_list_push_head(FmList* list, gpointer d)
{
    g_queue_push_head((GQueue*)list,(list)->funcs->item_ref(d));
}
static inline void fm_list_push_tail(FmList* list, gpointer d)
{
    g_queue_push_tail((GQueue*)list,(list)->funcs->item_ref(d));
}
static inline void fm_list_push_nth(FmList* list, gpointer d, guint n)
{
    g_queue_push_nth((GQueue*)list,(list)->funcs->item_ref(d),n);
}

static inline void fm_list_push_head_noref(FmList* list, gpointer d)
{
    g_queue_push_head((GQueue*)list,d);
}
static inline void fm_list_push_tail_noref(FmList* list, gpointer d)
{
    g_queue_push_tail((GQueue*)list,d);
}
static inline void fm_list_push_nth_noref(FmList* list, gpointer d, guint n)
{
    g_queue_push_nth((GQueue*)list,d,n);
}

static inline gpointer fm_list_pop_head(FmList* list)
{
    return g_queue_pop_head((GQueue*)list);
}
static inline gpointer fm_list_pop_tail(FmList* list)
{
    return g_queue_pop_tail((GQueue*)list);
}
static inline gpointer fm_list_pop_nth(FmList* list, guint n)
{
    return g_queue_pop_nth((GQueue*)list,n);
}

static inline gpointer fm_list_peek_head(FmList* list)
{
    return g_queue_peek_head((GQueue*)list);
}
static inline gpointer fm_list_peek_tail(FmList* list)
{
    return g_queue_peek_tail((GQueue*)list);
}
static inline gpointer fm_list_peek_nth(FmList* list, guint n)
{
    return g_queue_peek_nth((GQueue*)list,n);
}

static inline gint fm_list_index(FmList* list, gconstpointer d)
{
    return g_queue_index((GQueue*)list,d);
}
#endif /* __GTK_DOC_IGNORE__ */

void fm_list_remove(FmList* list, gpointer data);
void fm_list_remove_all(FmList* list, gpointer data);
#ifndef __GTK_DOC_IGNORE__
static inline void fm_list_insert_before(FmList* list, GList* s, gpointer d)
{
    g_queue_insert_before((GQueue*)list,s,(list)->funcs->item_ref(d));
}
static inline void fm_list_insert_after(FmList* list, GList* s, gpointer d)
{
    g_queue_insert_after((GQueue*)list,s,(list)->funcs->item_ref(d));
}
static inline void fm_list_insert_sorted(FmList* list, gpointer d, GCompareDataFunc f, gpointer u)
{
    g_queue_insert_sorted((GQueue*)list,(list)->funcs->item_ref(d),f,u);
}

static inline void fm_list_insert_before_noref(FmList* list, GList* s, gpointer d)
{
    g_queue_insert_before((GQueue*)list,s,d);
}
static inline void fm_list_insert_after_noref(FmList* list, GList* s, gpointer d)
{
    g_queue_insert_after((GQueue*)list,s,d);
}
static inline void fm_list_insert_sorted_noref(FmList* list, gpointer d, GCompareDataFunc f, gpointer u)
{
    g_queue_insert_sorted((GQueue*)list,d,f,u);
}

static inline void fm_list_push_head_link(FmList* list, GList* l_)
{
    g_queue_push_head_link((GQueue*)list,l_);
}
static inline void fm_list_push_tail_link(FmList* list, GList* l_)
{
    g_queue_push_tail_link((GQueue*)list,l_);
}
static inline void fm_list_push_nth_link(FmList* list, guint n, GList* l_)
{
    g_queue_push_nth_link((GQueue*)list,n,l_);
}

static inline GList* fm_list_pop_head_link(FmList* list)
{
    return g_queue_pop_head_link((GQueue*)list);
}
static inline GList* fm_list_pop_tail_link(FmList* list)
{
    return g_queue_pop_tail_link((GQueue*)list);
}
static inline GList* fm_list_pop_nth_link(FmList* list, guint n)
{
    return g_queue_pop_nth_link((GQueue*)list,n);
}

static inline GList* fm_list_peek_head_link(FmList* list)
{
    return g_queue_peek_head_link((GQueue*)list);
}
static inline GList* fm_list_peek_tail_link(FmList* list)
{
    return g_queue_peek_tail_link((GQueue*)list);
}
static inline GList* fm_list_peek_nth_link(FmList* list, guint n)
{
    return g_queue_peek_nth_link((GQueue*)list,n);
}

static inline gint fm_list_link_index(FmList* list, GList* l_)
{
    return g_queue_index((GQueue*)list,l_);
}
static inline void fm_list_unlink(FmList* list, GList* l_)
{
    g_queue_unlink((GQueue*)list,l_);
}
static inline void fm_list_delete_link_nounref(FmList* list, GList* l_)
{
    g_queue_delete_link((GQueue*)list,l_);
}
#endif /* __GTK_DOC_IGNORE__ */
void fm_list_delete_link(FmList* list, GList* l_);

G_END_DECLS

#endif /* __FM_LIST_H__ */
