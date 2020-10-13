/*
 *      test.c
 *      
 *      Copyright 2008 PCMan <pcman.tw@gmail.com>
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

#include <gtk/gtk.h>
#include "menu-cache.h"

static void on_menu_item( GtkMenuItem* mi, MenuCacheItem* item )
{
	g_debug( "Exec = %s", menu_cache_app_get_exec(MENU_CACHE_APP(item)) );
	g_debug( "Terminal = %d", menu_cache_app_get_use_terminal(MENU_CACHE_APP(item)) );
}

static GtkWidget* create_item( MenuCacheItem* item )
{
	GtkWidget* mi;
	if( menu_cache_item_get_type(item) == MENU_CACHE_TYPE_SEP )
		mi = gtk_separator_menu_item_new();
	else
	{
		GtkWidget* img;
		mi = gtk_image_menu_item_new_with_label( menu_cache_item_get_name(item) );
		gtk_widget_set_tooltip_text( mi, menu_cache_item_get_comment(item) );
		/*
		if( g_file_test( menu_cache_item_get_icon(item), G_FILE_TEST_IS_REGULAR ) )
		{
			img = gtk_image_new_from_file(menu_cache_item_get_icon(item));
		}
		else
		*/
			img = gtk_image_new_from_icon_name(menu_cache_item_get_icon(item), GTK_ICON_SIZE_MENU );
		gtk_image_menu_item_set_image( mi, img );
		if( menu_cache_item_get_type(item) == MENU_CACHE_TYPE_APP )
		{
			g_object_set_data_full( mi, "mcitem", menu_cache_item_ref(item), menu_cache_item_unref );
			g_signal_connect( mi, "activate", on_menu_item, item );
		}
	}
	gtk_widget_show( mi );
	return mi;
}

static GtkWidget* create_menu(MenuCacheDir* dir, GtkWidget* parent)
{
	GSList* l;
	GtkWidget* menu = gtk_menu_new();
	GtkWidget* mi;

	if( parent )
	{
		mi = create_item( dir );
		gtk_menu_item_set_submenu( mi, menu );
		gtk_menu_append( parent, mi );
		gtk_widget_show( mi );
//		printf( "comment:%s\n", menu_cache_item_get_comment(dir) );
	}

	for( l = menu_cache_dir_get_children(dir); l; l = l->next )
	{
		MenuCacheItem* item = MENU_CACHE_ITEM(l->data);

		if( menu_cache_item_get_type(item) == MENU_CACHE_TYPE_DIR )
			create_menu( item, menu );
		else
		{
			mi = create_item(item);
			gtk_widget_show( mi );
//			printf( "comment:%s\n", menu_cache_item_get_comment(item) );
//			printf( "icon:%s\n", menu_cache_item_get_icon(item) );
//			printf( "exec:%s\n", menu_cache_app_get_exec(item) );
			gtk_menu_append( menu, mi );
		}
	}
	return menu;
}

static void on_clicked( GtkButton* btn, GtkWidget* pop )
{
	gtk_menu_popup( pop, NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME );	
}

int main(int argc, char** argv)
{
	GtkWidget* win, *btn;
	gtk_init( &argc, &argv );

	MenuCache* menu_cache = menu_cache_new( argv[1] ? argv[1] : "/etc/xdg/menus/applications.menu", NULL, NULL );
	MenuCacheDir* menu = menu_cache_get_root_dir( menu_cache );
	GtkWidget* pop = create_menu( menu, NULL );

//	g_debug( "update: %d", menu_cache_is_updated( menu_cache ) );
	g_debug( "update: %d", menu_cache_file_is_updated( argv[1] ) );

	win = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title( win, "MenuCache Test" );
	btn = gtk_button_new_with_label( menu_cache_item_get_name(menu) );
	gtk_widget_set_tooltip_text( btn, menu_cache_item_get_comment(menu) );
	gtk_container_add( win, btn );
	g_signal_connect( btn, "clicked", G_CALLBACK(on_clicked), pop );
	g_signal_connect( win, "delete-event", G_CALLBACK(gtk_main_quit), NULL );

	gtk_widget_show_all( win );

	menu_cache_unref( menu );

	gtk_main();
	return 0;
}
