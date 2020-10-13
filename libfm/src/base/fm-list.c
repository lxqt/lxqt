/*
 *      fm-list.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
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

#include "fm-list.h"

FmList* fm_list_new(FmListFuncs* funcs)
{
	FmList* list = g_slice_new(FmList);
	list->funcs = funcs;
	g_queue_init(&list->list);
	list->n_ref = 1;
	return list;
}

FmList* fm_list_ref(FmList* list)
{
	g_atomic_int_inc(&list->n_ref);
	return list;
}

void fm_list_unref(FmList* list)
{
	if(g_atomic_int_dec_and_test(&list->n_ref))
	{
		g_queue_foreach((GQueue*)list, (GFunc)list->funcs->item_unref, NULL);
		g_queue_clear((GQueue*)list);
		g_slice_free(FmList, list);
	}
}

void fm_list_clear(FmList* list)
{
	g_queue_foreach((GQueue*)list, (GFunc)list->funcs->item_unref, NULL);
	g_queue_clear((GQueue*)list);
}

void fm_list_remove(FmList* list, gpointer data)
{
	GList* l = ((GQueue*)list)->head;
	for(;l; l=l->next)
	{
		if(l->data == data)
		{
			list->funcs->item_unref(data);
			break;
		}
	}
	if(l)
		g_queue_delete_link((GQueue*)data, l);
}

void fm_list_remove_all(FmList* list, gpointer data)
{
	/* FIXME: the performance can be better... */
	GList* l = ((GQueue*)list)->head;
	for(;l; l=l->next)
	{
		if(l->data == data)
			list->funcs->item_unref(data);
	}
	g_queue_remove_all((GQueue*)list, data);
}

void fm_list_delete_link(FmList *list, GList* l_)
{
	list->funcs->item_unref(l_->data);	
	g_queue_delete_link((GQueue*)list, l_);
}
