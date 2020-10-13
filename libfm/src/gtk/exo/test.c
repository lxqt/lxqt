#include <gtk/gtk.h>
#include "exo-icon-view.h"

int main(int argc, char** argv)
{
    gtk_init( &argc, &argv );

    GtkWidget* win = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    GtkListStore* list = gtk_list_store_new( 2, GDK_TYPE_PIXBUF, G_TYPE_STRING );
    ExoIconView* view = exo_icon_view_new_with_model( list );
    GtkTreeIter it;
    GdkPixbuf* pix = gtk_icon_theme_load_icon( gtk_icon_theme_get_default(), "folder", 48, 0, NULL );
    GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);

    gtk_list_store_insert_with_values( list, &it, 0, 0, pix, 1, "Test", -1 );
    gtk_list_store_insert_with_values( list, &it, 1, 0, pix, 1, "Test", -1 );
    gtk_list_store_insert_with_values( list, &it, 2, 0, pix, 1, "Test", -1 );
    gtk_list_store_insert_with_values( list, &it, 3, 0, pix, 1, "Test", -1 );
    g_object_unref( pix );

    exo_icon_view_set_text_column( view, 1 );
    exo_icon_view_set_pixbuf_column( view, 0 );
    exo_icon_view_set_orientation( view, GTK_ORIENTATION_HORIZONTAL );
    exo_icon_view_set_layout_mode( view, EXO_ICON_VIEW_LAYOUT_COLS );
    exo_icon_view_set_columns( view, -1 );
    exo_icon_view_set_item_width( view, 128 );
    exo_icon_view_set_single_click(view, TRUE);
    exo_icon_view_set_selection_mode( view, GTK_SELECTION_MULTIPLE );

    gtk_container_add( scroll, view );
    gtk_container_add( win, scroll );
    gtk_widget_show_all( win );
    gtk_main();
    gtk_widget_destroy( win );
    return 0;
}
