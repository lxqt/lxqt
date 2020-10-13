/*
 *      fm-folder-view.c
 *
 *      Copyright 2012-2015 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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
 * SECTION:fm-folder-view
 * @short_description: A folder view generic interface.
 * @title: FmFolderView
 *
 * @include: libfm/fm-gtk.h
 *
 * The #FmFolderView generic interface is used to implement folder views
 * common code including handling sorting change, and keyboard and mouse
 * buttons events.
 *
 * The #FmFolderView interface methods can attach context menu to widget
 * which does folder operations and consists of items:
 * |[
 * CreateNew -> NewFolder
 *              &lt;placeholder name='ph1'/&gt;
 *              ----------------
 *              NewBlank
 * ------------------------
 * &lt;placeholder name='CustomFileOps'/&gt;
 * ------------------------
 * Paste
 * Cut
 * Copy
 * Del
 * Remove
 * FileProp
 * ------------------------
 * SelAll
 * InvSel
 * ------------------------
 * Sort -> Asc
 *         Desc
 *         ----------------
 *         ByName
 *         ByMTime
 *         BySize
 *         ByType
 *         ----------------
 *         MingleDirs
 *         SortIgnoreCase
 *         &lt;placeholder name='CustomSortOps'/&gt;
 * ShowHidden
 * Rename
 * &lt;placeholder name='CustomFolderOps'/&gt;
 * ------------------------
 * &lt;placeholder name='CustomCommonOps'/&gt;
 * ------------------------
 * Prop
 * ]|
 * In created menu items 'Cut', 'Copy', 'Del', 'Remove', 'FileProp', and
 * 'Rename' are hidden.
 *
 * Widget can modity the menu replacing placeholders and hiding or
 * enabling existing items in it. Widget can do that in callback which
 * is supplied for call fm_folder_view_add_popup().
 *
 * If click was not on widget but on some item in it then not this
 * context menu but one with #FmFileMenu object will be opened instead.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "gtk-compat.h"

#include <glib/gi18n-lib.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>

#include "fm-folder-view.h"
#include "fm-file-properties.h"
#include "fm-clipboard.h"
#include "fm-gtk-file-launcher.h"
#include "fm-file-menu.h"
#include "fm-gtk-utils.h"
#include "fm-templates.h"
#include "fm-config.h"

static const char folder_popup_xml[] =
"<popup>"
  "<menu action='CreateNew'>"
    "<menuitem action='NewFolder'/>"
    /* placeholder for ~/Templates support */
    "<placeholder name='ph1'/>"
    "<separator/>"
    "<menuitem action='NewBlank'/>"
  "</menu>"
  "<separator/>"
  /* placeholder for custom file operations such as empty trash can */
  "<placeholder name='CustomFileOps'/>"
  "<separator/>"
  "<menuitem action='Paste'/>"
  "<menuitem action='Cut'/>"
  "<menuitem action='Copy'/>"
  "<menuitem action='Del'/>"
  "<menuitem action='Remove'/>"
  "<menuitem action='FileProp'/>"
  "<separator/>"
  "<menuitem action='SelAll'/>"
  "<menuitem action='InvSel'/>"
  "<separator/>"
  "<menu action='Sort'>"
    "<menuitem action='Asc'/>"
    "<menuitem action='Desc'/>"
    "<separator/>"
    "<menuitem action='ByName'/>"
    "<menuitem action='ByMTime'/>"
    "<menuitem action='BySize'/>"
    "<menuitem action='ByType'/>"
    "<separator/>"
    "<menuitem action='MingleDirs'/>"
    "<menuitem action='SortIgnoreCase'/>"
    /* placeholder for custom sort options */
    "<placeholder name='CustomSortOps'/>"
    /* "<separator/>"
    "<menuitem action='SortPerFolder'/>" */
  "</menu>"
  "<menuitem action='ShowHidden'/>"
  "<menuitem action='Rename'/>"
  /* placeholder for custom folder operations */
  "<placeholder name='CustomFolderOps'/>"
  "<separator/>"
  /* placeholder for custom application operations such as view mode changing */
  "<placeholder name='CustomCommonOps'/>"
  "<separator/>"
  "<menuitem action='Prop'/>"
"</popup>"
"<accelerator action='NewFolder2'/>"
"<accelerator action='NewFolder3'/>"
"<accelerator action='Copy2'/>"
"<accelerator action='Paste2'/>"
"<accelerator action='Del2'/>"
"<accelerator action='Remove2'/>"
"<accelerator action='FileProp2'/>"
"<accelerator action='FileProp3'/>";

static void on_create_new(GtkAction* act, FmFolderView* fv);
static void on_cut(GtkAction* act, FmFolderView* fv);
static void on_copy(GtkAction* act, FmFolderView* fv);
static void on_paste(GtkAction* act, FmFolderView* fv);
static void on_trash(GtkAction* act, FmFolderView* fv);
static void on_rm(GtkAction* act, FmFolderView* fv);
static void on_select_all(GtkAction* act, FmFolderView* fv);
static void on_invert_select(GtkAction* act, FmFolderView* fv);
static void on_rename(GtkAction* act, FmFolderView* fv);
static void on_prop(GtkAction* act, FmFolderView* fv);
static void on_file_prop(GtkAction* act, FmFolderView* fv);
static void on_menu(GtkAction* act, FmFolderView* fv);
static void on_file_menu(GtkAction* act, FmFolderView* fv);
static void on_show_hidden(GtkToggleAction* act, FmFolderView* fv);
static void on_mingle_dirs(GtkToggleAction* act, FmFolderView* fv);
static void on_ignore_case(GtkToggleAction* act, FmFolderView* fv);

static const GtkActionEntry folder_popup_actions[]=
{
    {"CreateNew", NULL, N_("Create _New..."), NULL, NULL, NULL},
    {"NewFolder", "folder", N_("Folder"), "<Ctrl><Shift>N", NULL, G_CALLBACK(on_create_new)},
    {"NewFolder2", NULL, NULL, "Insert", NULL, G_CALLBACK(on_create_new)},
    {"NewFolder3", NULL, NULL, "KP_Insert", NULL, G_CALLBACK(on_create_new)},
    {"NewBlank", NULL, N_("Empty File"), NULL, NULL, G_CALLBACK(on_create_new)},
    /* {"NewShortcut", "system-run", N_("Shortcut"), NULL, NULL, G_CALLBACK(on_create_new)}, */
    {"Cut", GTK_STOCK_CUT, NULL, "<Ctrl>X", NULL, G_CALLBACK(on_cut)},
    {"Copy", GTK_STOCK_COPY, NULL, "<Ctrl>C", NULL, G_CALLBACK(on_copy)},
    {"Copy2", NULL, NULL, "<Ctrl>Insert", NULL, G_CALLBACK(on_copy)},
    {"Paste", GTK_STOCK_PASTE, NULL, "<Ctrl>V", NULL, G_CALLBACK(on_paste)},
    {"Paste2", NULL, NULL, "<Shift>Insert", NULL, G_CALLBACK(on_paste)},
    {"Del", GTK_STOCK_DELETE, NULL, "Delete", NULL, G_CALLBACK(on_trash)},
    {"Del2", NULL, NULL, "KP_Delete", NULL, G_CALLBACK(on_trash)},
    {"Remove", GTK_STOCK_REMOVE, NULL, "<Shift>Delete", NULL, G_CALLBACK(on_rm)},
    {"Remove2", NULL, NULL, "<Shift>KP_Delete", NULL, G_CALLBACK(on_rm)},
    {"SelAll", GTK_STOCK_SELECT_ALL, NULL, "<Ctrl>A", NULL, G_CALLBACK(on_select_all)},
    {"InvSel", NULL, N_("_Invert Selection"), "<Ctrl>I", NULL, G_CALLBACK(on_invert_select)},
    {"Sort", NULL, N_("_Sort Files"), NULL, NULL, NULL},
    {"Rename", NULL, N_("_Rename Folder..."), NULL, NULL, G_CALLBACK(on_rename)},
    {"Prop", GTK_STOCK_PROPERTIES, N_("Folder Prop_erties"), "", NULL, G_CALLBACK(on_prop)},
    {"FileProp", GTK_STOCK_PROPERTIES, N_("Prop_erties"), "<Alt>Return", NULL, G_CALLBACK(on_file_prop)},
    {"FileProp2", NULL, NULL, "<Alt>KP_Enter", NULL, G_CALLBACK(on_file_prop)},
    {"FileProp3", NULL, NULL, "<Alt>ISO_Enter", NULL, G_CALLBACK(on_file_prop)}
};

static GtkToggleActionEntry folder_toggle_actions[]=
{
    {"ShowHidden", NULL, N_("Show _Hidden"), NULL, NULL, G_CALLBACK(on_show_hidden), FALSE},
    /* Note to translators: "Mingle..." means "Do not put folders before files" but make the translation as short as possible, please! */
    {"MingleDirs", NULL, N_("Mingle _Files and Folders"), NULL, NULL, G_CALLBACK(on_mingle_dirs), FALSE},
    {"SortIgnoreCase", NULL, N_("_Ignore Name Case"), NULL, NULL, G_CALLBACK(on_ignore_case), TRUE}
};

static const GtkRadioActionEntry folder_sort_type_actions[]=
{
    {"Asc", GTK_STOCK_SORT_ASCENDING, NULL, NULL, NULL, GTK_SORT_ASCENDING},
    {"Desc", GTK_STOCK_SORT_DESCENDING, NULL, NULL, NULL, GTK_SORT_DESCENDING},
};

static const GtkRadioActionEntry folder_sort_by_actions[]=
{
    {"ByName", NULL, N_("By _Name"), NULL, NULL, FM_FOLDER_MODEL_COL_NAME},
    {"ByMTime", NULL, N_("By _Modification Time"), NULL, NULL, FM_FOLDER_MODEL_COL_MTIME},
    {"BySize", NULL, N_("By _Size"), NULL, NULL, FM_FOLDER_MODEL_COL_SIZE},
    {"ByType", NULL, N_("By File _Type"), NULL, NULL, FM_FOLDER_MODEL_COL_DESC}
};

/* plugins for scheme */
typedef struct {
    FmPath *scheme; /* scheme path, NULL for scheme mask "*" */
    FmContextMenuSchemeAddonInit cb; /* callbacks */
} FmContextMenuSchemeExt;

static GList *extensions = NULL; /* elements are FmContextMenuSchemeExt */


G_DEFINE_INTERFACE(FmFolderView, fm_folder_view, GTK_TYPE_WIDGET);

enum
{
    CLICKED,
    SEL_CHANGED,
    SORT_CHANGED,
    FILTER_CHANGED,
    COLUMNS_CHANGED,
    //CHDIR,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

static GQuark ui_quark;
static GQuark popup_quark;
static GQuark templates_quark;

static void fm_folder_view_default_init(FmFolderViewInterface *iface)
{
    ui_quark = g_quark_from_static_string("popup-ui");
    popup_quark = g_quark_from_static_string("popup-menu");
    templates_quark = g_quark_from_static_string("templates-list");

    /* properties and signals */
    /**
     * FmFolderView::clicked:
     * @view: the widget that emitted the signal
     * @type: (#FmFolderViewClickType) type of click
     * @file: (#FmFileInfo *) file on which cursor is
     *
     * The #FmFolderView::clicked signal is emitted when user clicked
     * somewhere in the folder area. If click was on free folder area
     * then @file is %NULL.
     *
     * Since: 0.1.0
     */
    signals[CLICKED]=
        g_signal_new("clicked",
                     FM_TYPE_FOLDER_VIEW,
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderViewInterface, clicked),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__UINT_POINTER,
                     G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_POINTER);

    /**
     * FmFolderView::sel-changed:
     * @view: the widget that emitted the signal
     * @n_sel: number of files currently selected in the folder
     *
     * The #FmFolderView::sel-changed signal is emitted when
     * selection of the view got changed.
     *
     * Before 1.0.0 parameter was list of currently selected files.
     *
     * Since: 0.1.0
     */
    signals[SEL_CHANGED]=
        g_signal_new("sel-changed",
                     FM_TYPE_FOLDER_VIEW,
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderViewInterface, sel_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__INT,
                     G_TYPE_NONE, 1, G_TYPE_INT);

    /**
     * FmFolderView::sort-changed:
     * @view: the widget that emitted the signal
     *
     * The #FmFolderView::sort-changed signal is emitted when sorting
     * of the view got changed.
     *
     * Since: 0.1.10
     */
    signals[SORT_CHANGED]=
        g_signal_new("sort-changed",
                     FM_TYPE_FOLDER_VIEW,
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderViewInterface, sort_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    /**
     * FmFolderView::filter-changed:
     * @view: the widget that emitted the signal
     *
     * The #FmFolderView::filter-changed signal is emitted when filter
     * of the view model got changed. It's just bouncer for the same
     * signal of #FmFolderModel.
     *
     * Since: 1.0.2
     */
    signals[FILTER_CHANGED]=
        g_signal_new("filter-changed",
                     FM_TYPE_FOLDER_VIEW,
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderViewInterface, filter_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    /**
     * FmFolderView::columns-changed:
     * @view: the widget that emitted the signal
     *
     * The #FmFolderView::columns-changed signal is emitted when layout
     * of #FmFolderView instance is changed, i.e. some column is added,
     * deleted, or changed its size.
     *
     * Since: 1.2.0
     */
    signals[COLUMNS_CHANGED]=
        g_signal_new("columns-changed",
                     FM_TYPE_FOLDER_VIEW,
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderViewInterface, columns_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);
}

static void on_sort_col_changed(GtkTreeSortable* sortable, FmFolderView* fv)
{
    if(fm_folder_model_get_sort(FM_FOLDER_MODEL(sortable), NULL, NULL))
    {
        g_signal_emit(fv, signals[SORT_CHANGED], 0);
    }
}

static void on_filter_changed(FmFolderModel* model, FmFolderView* fv)
{
    g_signal_emit(fv, signals[FILTER_CHANGED], 0);
}

/**
 * fm_folder_view_set_selection_mode
 * @fv: a widget to apply
 * @mode: new mode of selection in @fv.
 *
 * Changes selection mode in @fv.
 *
 * Since: 0.1.0
 */
void fm_folder_view_set_selection_mode(FmFolderView* fv, GtkSelectionMode mode)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    FM_FOLDER_VIEW_GET_IFACE(fv)->set_sel_mode(fv, mode);
}

/**
 * fm_folder_view_get_selection_mode
 * @fv: a widget to inspect
 *
 * Retrieves current selection mode in @fv.
 *
 * Returns: current selection mode.
 *
 * Since: 0.1.0
 */
GtkSelectionMode fm_folder_view_get_selection_mode(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), GTK_SELECTION_NONE);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->get_sel_mode)(fv);
}

/**
 * fm_folder_view_sort
 * @fv: a widget to apply
 * @type: new mode of sorting (ascending or descending)
 * @by: criteria of sorting
 *
 * Changes sorting in the view. Invalid values for @type or @by are
 * ignored (will not change sorting).
 *
 * Since 1.0.2 values passed to this API aren't remembered in the @fv
 * object. If @fv has no model then this API has no effect.
 * After the model is removed from @fv (calling fm_folder_view_set_model()
 * with NULL) there is no possibility to recover last settings and any
 * model added to @fv later will get defaults: FM_FOLDER_MODEL_COL_DEFAULT
 * and FM_SORT_DEFAULT.
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.0.2: Use fm_folder_model_set_sort() instead.
 */
void fm_folder_view_sort(FmFolderView* fv, GtkSortType type, FmFolderModelCol by)
{
    FmFolderViewInterface* iface;
    FmFolderModel* model;
    FmSortMode mode;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    model = iface->get_model(fv);
    if(model)
    {
        if(type == GTK_SORT_ASCENDING || type == GTK_SORT_DESCENDING)
        {
            fm_folder_model_get_sort(model, NULL, &mode);
            mode &= ~FM_SORT_ORDER_MASK;
            mode |= (type == GTK_SORT_ASCENDING) ? FM_SORT_ASCENDING : FM_SORT_DESCENDING;
        }
        else
            mode = FM_SORT_DEFAULT;
        fm_folder_model_set_sort(model, by, mode);
    }
    /* model will generate signal to update config if changed */
}

/**
 * fm_folder_view_get_sort_type
 * @fv: a widget to inspect
 *
 * Retrieves current sorting type in @fv.
 *
 * Returns: mode of sorting (ascending or descending)
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.0.2: Use fm_folder_model_get_sort() instead.
 */
GtkSortType fm_folder_view_get_sort_type(FmFolderView* fv)
{
    FmFolderViewInterface* iface;
    FmFolderModel* model;
    FmSortMode mode;

    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), GTK_SORT_ASCENDING);

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    model = iface->get_model(fv);
    if(model == NULL || !fm_folder_model_get_sort(model, NULL, &mode))
        return GTK_SORT_ASCENDING;
    return FM_SORT_IS_ASCENDING(mode) ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING;
}

/**
 * fm_folder_view_get_sort_by
 * @fv: a widget to inspect
 *
 * Retrieves current criteria of sorting in @fv (e.g. by name).
 *
 * Returns: criteria of sorting.
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.0.2: Use fm_folder_model_get_sort() instead.
 */
FmFolderModelCol fm_folder_view_get_sort_by(FmFolderView* fv)
{
    FmFolderViewInterface* iface;
    FmFolderModel* model;
    FmFolderModelCol by;

    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), FM_FOLDER_MODEL_COL_DEFAULT);

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    model = iface->get_model(fv);
    if(model == NULL || !fm_folder_model_get_sort(model, &by, NULL))
        return FM_FOLDER_MODEL_COL_DEFAULT;
    return by;
}

/**
 * fm_folder_view_set_show_hidden
 * @fv: a widget to apply
 * @show: new setting
 *
 * Changes whether hidden files in folder shown in @fv should be visible
 * or not.
 *
 * See also: fm_folder_view_get_show_hidden().
 *
 * Since: 0.1.0
 */
void fm_folder_view_set_show_hidden(FmFolderView* fv, gboolean show)
{
    FmFolderViewInterface* iface;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    if(iface->get_show_hidden(fv) != show)
    {
        FmFolderModel* model;
        iface->set_show_hidden(fv, show);
        model = iface->get_model(fv);
        if(G_LIKELY(model))
            fm_folder_model_set_show_hidden(model, show);
        /* "filter-changed" signal will be sent by model */
    }
}

/**
 * fm_folder_view_get_show_hidden
 * @fv: a widget to inspect
 *
 * Retrieves setting whether hidden files in folder shown in @fv should
 * be visible or not.
 *
 * See also: fm_folder_view_set_show_hidden().
 *
 * Returns: %TRUE if hidden files are visible.
 *
 * Since: 0.1.0
 */
gboolean fm_folder_view_get_show_hidden(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), FALSE);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->get_show_hidden)(fv);
}

/**
 * fm_folder_view_get_folder
 * @fv: a widget to inspect
 *
 * Retrieves the folder shown by @fv. Returned data are owned by @fv and
 * should not be freed by caller.
 *
 * Returns: (transfer none): the folder of view.
 *
 * Since: 1.0.0
 */
FmFolder* fm_folder_view_get_folder(FmFolderView* fv)
{
    FmFolderModel *model;

    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), NULL);

    model = FM_FOLDER_VIEW_GET_IFACE(fv)->get_model(fv);
    return model ? fm_folder_model_get_folder(model) : NULL;
}

/**
 * fm_folder_view_get_cwd
 * @fv: a widget to inspect
 *
 * Retrieves file path of the folder shown by @fv. Returned data are
 * owned by @fv and should not be freed by caller.
 *
 * Returns: (transfer none): file path of the folder.
 *
 * Since: 0.1.0
 */
FmPath* fm_folder_view_get_cwd(FmFolderView* fv)
{
    FmFolder* folder = fm_folder_view_get_folder(fv);

    return folder ? fm_folder_get_path(folder) : NULL;
}

/**
 * fm_folder_view_get_cwd_info
 * @fv: a widget to inspect
 *
 * Retrieves file info of the folder shown by @fv. Returned data are
 * owned by @fv and should not be freed by caller.
 *
 * Returns: (transfer none): file info descriptor of the folder.
 *
 * Since: 0.1.0
 */
FmFileInfo* fm_folder_view_get_cwd_info(FmFolderView* fv)
{
    FmFolder* folder = fm_folder_view_get_folder(fv);

    return folder ? fm_folder_get_info(folder) : NULL;
}

/**
 * fm_folder_view_get_model
 * @fv: a widget to inspect
 *
 * Retrieves the model used by @fv. Returned data are owned by @fv and
 * should not be freed by caller.
 *
 * Returns: (transfer none): the model of view.
 *
 * Since: 0.1.16
 */
FmFolderModel* fm_folder_view_get_model(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), NULL);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->get_model)(fv);
}

static void unset_model(FmFolderView* fv, FmFolderModel* model)
{
    g_signal_handlers_disconnect_by_func(model, on_sort_col_changed, fv);
    g_signal_handlers_disconnect_by_func(model, on_filter_changed, fv);
}

/**
 * fm_folder_view_set_model
 * @fv: a widget to apply
 * @model: (allow-none): new view model
 *
 * Changes model for the @fv.
 *
 * Since: 1.0.0
 */
void fm_folder_view_set_model(FmFolderView* fv, FmFolderModel* model)
{
    FmFolderViewInterface* iface;
    FmFolderModel* old_model;
    FmFolderModelCol by = FM_FOLDER_MODEL_COL_DEFAULT;
    FmSortMode mode = FM_SORT_ASCENDING;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    old_model = iface->get_model(fv);
    if(old_model)
    {
        fm_folder_model_get_sort(old_model, &by, &mode);
        unset_model(fv, old_model);
        /* https://bugs.launchpad.net/ubuntu/+source/pcmanfm/+bug/1071231:
           after changing the folder selection isn't reset */
        iface->unselect_all(fv);
    }
    /* FIXME: which setting to apply if this is first model? */
    iface->set_model(fv, model);
    if(model)
    {
        fm_folder_model_set_sort(model, by, mode);
        g_signal_connect(model, "sort-column-changed", G_CALLBACK(on_sort_col_changed), fv);
        g_signal_connect(model, "filter-changed", G_CALLBACK(on_filter_changed), fv);
    }
}

/**
 * fm_folder_view_get_n_selected_files
 * @fv: a widget to inspect
 *
 * Retrieves number of the currently selected files.
 *
 * Returns: number of files selected.
 *
 * Since: 1.0.1
 */
gint fm_folder_view_get_n_selected_files(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), 0);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->count_selected_files)(fv);
}

/**
 * fm_folder_view_dup_selected_files
 * @fv: a FmFolderView object
 *
 * Retrieves a list of
 * the currently selected files. The list should be freed after usage
 * with fm_file_info_list_unref&lpar;). If there are no files selected then
 * returns %NULL.
 *
 * Before 1.0.0 this API had name fm_folder_view_get_selected_files.
 *
 * Returns: (transfer full) (element-type FmFileInfo): list of selected file infos.
 *
 * Since: 0.1.0
 */
FmFileInfoList* fm_folder_view_dup_selected_files(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), NULL);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->dup_selected_files)(fv);
}

/**
 * fm_folder_view_dup_selected_file_paths
 * @fv: a FmFolderView object
 *
 * Retrieves a list of
 * the currently selected files. The list should be freed after usage
 * with fm_path_list_unref&lpar;). If there are no files selected then returns
 * %NULL.
 *
 * Before 1.0.0 this API had name fm_folder_view_get_selected_file_paths.
 *
 * Returns: (transfer full) (element-type FmPath): list of selected file paths.
 *
 * Since: 0.1.0
 */
FmPathList* fm_folder_view_dup_selected_file_paths(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), NULL);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->dup_selected_file_paths)(fv);
}

/**
 * fm_folder_view_select_all
 * @fv: a widget to apply
 *
 * Selects all files in folder.
 *
 * Since: 0.1.0
 */
void fm_folder_view_select_all(FmFolderView* fv)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    FM_FOLDER_VIEW_GET_IFACE(fv)->select_all(fv);
}

/**
 * fm_folder_view_unselect_all
 * @fv: a widget to apply
 *
 * Unselects all files in folder.
 *
 * Since: 1.0.1
 */
void fm_folder_view_unselect_all(FmFolderView* fv)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    FM_FOLDER_VIEW_GET_IFACE(fv)->unselect_all(fv);
}

/**
 * fm_folder_view_select_invert
 * @fv: a widget to apply
 *
 * Selects all unselected files in @fv but unselects all selected.
 *
 * Since: 0.1.0
 */
void fm_folder_view_select_invert(FmFolderView* fv)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    FM_FOLDER_VIEW_GET_IFACE(fv)->select_invert(fv);
}

/**
 * fm_folder_view_select_file_path
 * @fv: a widget to apply
 * @path: a file path to select
 *
 * Selects a file in the folder.
 *
 * Since: 0.1.0
 */
void fm_folder_view_select_file_path(FmFolderView* fv, FmPath* path)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    FM_FOLDER_VIEW_GET_IFACE(fv)->select_file_path(fv, path);
}

/**
 * fm_folder_view_select_file_paths
 * @fv: a widget to apply
 * @paths: list of files to select
 *
 * Selects few files in the folder.
 *
 * Since: 0.1.0
 */
void fm_folder_view_select_file_paths(FmFolderView* fv, FmPathList* paths)
{
    GList* l;
    FmFolderViewInterface* iface;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    for(l = fm_path_list_peek_head_link(paths);l; l=l->next)
    {
        FmPath* path = FM_PATH(l->data);
        iface->select_file_path(fv, path);
    }
}

static void on_run_app_toggled(GtkToggleButton *button, gboolean *run)
{
    *run = gtk_toggle_button_get_active(button);
}

static void on_create_new(GtkAction* act, FmFolderView* fv)
{
    const char* name = gtk_action_get_name(act);
    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkUIManager *ui = g_object_get_qdata(G_OBJECT(fv), ui_quark);
    GList *templates = g_object_get_qdata(G_OBJECT(ui), templates_quark);
    FmTemplate *templ;
    FmMimeType *mime_type;
    const char *prompt, *name_template, *label;
    char *_prompt = NULL, *header, *basename;
    GFile *dir, *gf;
    GError *error = NULL;
    GtkWidget *run_button, *sub_button;
    gboolean new_folder = FALSE, run_app;
    gint n;

    g_return_if_fail(ui != NULL);
    if(strncmp(name, "NewFolder", 9) == 0)
    {
        templ = NULL;
        prompt = _("Enter a name for the newly created folder:");
        header = _("Creating New Folder");
        new_folder = TRUE;
    }
    else if(G_LIKELY(strncmp(name, "NewFile", 7) == 0))
    {
        n = atoi(&name[7]);
        if(n < 0 || (templ = g_list_nth_data(templates, n)) == NULL)
            return; /* invalid action name, is it possible? */
    }
    /* special option 'NewBlank' */
    else if(G_LIKELY(strcmp(name, "NewBlank") == 0))
    {
        templ = NULL;
        prompt = _("Enter a name for empty file:");
        header = _("Creating ...");
    }
    else /* invalid action name, is it possible? */
        return;
    if(templ == NULL) /* new folder or empty file */
    {
        name_template = _("New");
        n = -1;
        run_app = FALSE;
        run_button = NULL;
    }
    else
    {
        mime_type = fm_template_get_mime_type(templ);
        prompt = fm_template_get_prompt(templ);
        if(!prompt)
            prompt = _prompt = g_strdup_printf(_("Enter a name for the new %s:"),
                                               fm_mime_type_get_desc(mime_type));
        label = fm_template_get_label(templ);
        header = g_strdup_printf(_("Creating %s"), label ? label :
                                             fm_mime_type_get_desc(mime_type));
        name_template = fm_template_get_name(templ, &n);
        run_app = fm_config->template_run_app;
        sub_button = gtk_check_button_new_with_mnemonic(_("_Run default application on file after creation"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sub_button), run_app);
        g_signal_connect(sub_button, "toggled", G_CALLBACK(on_run_app_toggled), &run_app);
        run_button = gtk_alignment_new(0, 0, 1, 1);
        gtk_alignment_set_padding(GTK_ALIGNMENT(run_button), 0, 0, 16, 0);
        gtk_container_add(GTK_CONTAINER(run_button), sub_button);
    }
    basename = fm_get_user_input_n(GTK_WINDOW(win), header, prompt,
                                   name_template, n, run_button);
    if(templ)
    {
        g_free(_prompt);
        g_free(header);
    }
    if(!basename)
        return;
    if (new_folder)
        fm_folder_make_directory(fm_folder_view_get_folder(fv), basename, &error);
    else
    {
        dir = fm_path_to_gfile(fm_folder_view_get_cwd(fv));
        gf = g_file_get_child_for_display_name(dir, basename, &error);
        g_object_unref(dir);
        if (gf)
        {
            fm_template_create_file(templ, gf, &error, run_app);
            g_object_unref(gf);
        }
    }
    g_free(basename);
    if(error)
    {
        fm_show_error(GTK_WINDOW(win), NULL, error->message);
        g_error_free(error);
    }
}

#define FOCUS_IS_IN_FOLDER_VIEW(_focus_,_fv_) \
	_focus_ == NULL || _focus_ == _fv_ || gtk_widget_is_ancestor(_focus_, _fv_)

static void on_cut(GtkAction* act, FmFolderView* fv)
{
    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if we cut inside the view; for desktop focus will be NULL */
    if(FOCUS_IS_IN_FOLDER_VIEW(focus, GTK_WIDGET(fv)))
    {
        FmPathList* files = fm_folder_view_dup_selected_file_paths(fv);
        if(files)
        {
            fm_clipboard_cut_files(win, files);
            fm_path_list_unref(files);
        }
    }
    else if(GTK_IS_EDITABLE(focus) && /* fallback for editables */
            gtk_editable_get_selection_bounds((GtkEditable*)focus, NULL, NULL))
        gtk_editable_cut_clipboard((GtkEditable*)focus);
}

static void on_copy(GtkAction* act, FmFolderView* fv)
{
    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if we copy inside the view; for desktop focus will be NULL */
    if(FOCUS_IS_IN_FOLDER_VIEW(focus, GTK_WIDGET(fv)))
    {
        FmPathList* files = fm_folder_view_dup_selected_file_paths(fv);
        if(files)
        {
            fm_clipboard_copy_files(win, files);
            fm_path_list_unref(files);
        }
    }
    else if(GTK_IS_EDITABLE(focus) && /* fallback for editables */
            gtk_editable_get_selection_bounds((GtkEditable*)focus, NULL, NULL))
        gtk_editable_copy_clipboard((GtkEditable*)focus);
}

static void on_paste(GtkAction* act, FmFolderView* fv)
{
    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if we paste inside the view; for desktop focus will be NULL */
    if(FOCUS_IS_IN_FOLDER_VIEW(focus, GTK_WIDGET(fv)))
    {
        FmPath* path = fm_folder_view_get_cwd(fv);
        fm_clipboard_paste_files(GTK_WIDGET(fv), path);
    }
    else if(GTK_IS_EDITABLE(focus)) /* fallback for editables */
        gtk_editable_paste_clipboard((GtkEditable*)focus);
    else
        g_debug("paste on %s isn't supported by FmFolderView widget",
                G_OBJECT_TYPE_NAME(focus));
}

static void on_trash(GtkAction* act, FmFolderView* fv)
{
    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if user pressed 'Del' inside the view; for desktop focus will be NULL */
    if(FOCUS_IS_IN_FOLDER_VIEW(focus, GTK_WIDGET(fv)))
    {
        FmPathList* files = fm_folder_view_dup_selected_file_paths(fv);
        if(files)
        {
            fm_trash_or_delete_files(GTK_WINDOW(win), files);
            fm_path_list_unref(files);
        }
    }
    else if(GTK_IS_EDITABLE(focus)) /* fallback for editables */
    {
        if(!gtk_editable_get_selection_bounds((GtkEditable*)focus, NULL, NULL))
        {
            gint pos = gtk_editable_get_position((GtkEditable*)focus);
            /* if no text selected then delete character next to cursor */
            gtk_editable_select_region((GtkEditable*)focus, pos, pos + 1);
        }
        gtk_editable_delete_selection((GtkEditable*)focus);
    }
}

static void on_rm(GtkAction* act, FmFolderView* fv)
{
    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if user pressed 'Shift+Del' inside the view */
    if(FOCUS_IS_IN_FOLDER_VIEW(focus, GTK_WIDGET(fv)))
    {
        FmPathList* files = fm_folder_view_dup_selected_file_paths(fv);
        if(files)
        {
            fm_delete_files(GTK_WINDOW(win), files);
            fm_path_list_unref(files);
        }
    }
    /* for editables 'Shift+Del' means 'Cut' */
    else if(GTK_IS_EDITABLE(focus))
        gtk_editable_cut_clipboard((GtkEditable*)focus);
}

static void on_select_all(GtkAction* act, FmFolderView* fv)
{
    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if we are inside the view; for desktop focus will be NULL */
    if(FOCUS_IS_IN_FOLDER_VIEW(focus, GTK_WIDGET(fv)))
        fm_folder_view_select_all(fv);
    else if(GTK_IS_EDITABLE(focus)) /* fallback for editables */
        gtk_editable_select_region((GtkEditable*)focus, 0, -1);
}

static void on_invert_select(GtkAction* act, FmFolderView* fv)
{
    fm_folder_view_select_invert(fv);
}

static void on_rename(GtkAction* act, FmFolderView* fv)
{
    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);

    /* FIXME: is it OK to rename folder itself? */
    fm_rename_file(GTK_WINDOW(win), fm_folder_view_get_cwd(fv));
}

static void on_prop(GtkAction* act, FmFolderView* fv)
{
    FmFolder* folder = fm_folder_view_get_folder(fv);

    if(folder && fm_folder_is_valid(folder))
    {
        GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
        GtkWidget *win = gtk_menu_get_attach_widget(popup);
        FmFileInfo* fi = fm_folder_get_info(folder);
        FmFileInfoList* files = fm_file_info_list_new();
        fm_file_info_list_push_tail(files, fi);
        fm_show_file_properties(GTK_WINDOW(win), files);
        fm_file_info_list_unref(files);
    }
}

static void on_file_prop(GtkAction* act, FmFolderView* fv)
{
    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if it is inside the view; for desktop focus will be NULL */
    if(FOCUS_IS_IN_FOLDER_VIEW(focus, GTK_WIDGET(fv)))
    {
        FmFileInfoList* files = fm_folder_view_dup_selected_files(fv);
        if(files)
        {
            fm_show_file_properties(GTK_WINDOW(win), files);
            fm_file_info_list_unref(files);
        }
    }
}

static void popup_position_func(GtkMenu *menu, gint *x, gint *y,
                                gboolean *push_in, gpointer user_data)
{
    GtkWidget *widget = GTK_WIDGET(user_data);
    GdkWindow *parent_window;
    GdkScreen *screen;
    GtkAllocation a, ma;
    GdkRectangle mr;
    gint x2, y2, mon;
    gboolean rtl = (gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL);

    /* realize menu so we get actual size of it */
    gtk_widget_realize(GTK_WIDGET(menu));
    /* get all the relative coordinates */
    gtk_widget_get_allocation(widget, &a);
    screen = gtk_widget_get_screen(widget);
    gdk_window_get_device_position(gtk_widget_get_window(widget),
                                   gdk_device_manager_get_client_pointer(
                                        gdk_display_get_device_manager(
                                            gdk_screen_get_display(screen))),
                                   &x2, &y2, NULL);
    gtk_widget_get_allocation(GTK_WIDGET(menu), &ma);
    parent_window = gtk_widget_get_parent_window(widget);
    /* get absolute coordinate of parent window - we got coords relative to it */
    if (parent_window)
        gdk_window_get_origin(parent_window, x, y);
    else
    {
        /* desktop has no parent window so parent coords will be from geom */
        mon = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(widget));
        gdk_screen_get_monitor_geometry(screen, mon, &mr);
        *x = mr.x;
        *y = mr.y;
    }
    /* position menu inside widget */
    if(rtl) /* RTL */
        x2 = CLAMP(x2, a.x + 1, a.x + ma.width + a.width - 1);
    else /* LTR */
        x2 = CLAMP(x2, a.x + 1 - ma.width, a.x + a.width - 1);
    y2 = CLAMP(y2, a.y + 1 - ma.height, a.y + a.height - 1);
    /* calculate desired position for menu */
    *x += x2;
    *y += y2;
    /* get monitor geometry at the pointer: for desktop we already have it */
    if (parent_window)
    {
        mon = gdk_screen_get_monitor_at_point(screen, *x, *y);
        gdk_screen_get_monitor_geometry(screen, mon, &mr);
    }
    /* limit coordinates so menu will be not positioned outside of screen */
    if(rtl) /* RTL */
    {
        x2 = mr.x + mr.width;
        if (*x < mr.x + ma.width) /* out of monitor */
            *x = MIN(*x + ma.width, x2); /* place menu right to cursor */
        else
            *x = MIN(*x, x2);
    }
    else /* LTR */
    {
        if (*x + ma.width > mr.x + mr.width) /* out of monitor */
            *x = MAX(mr.x, *x - ma.width); /* place menu left to cursor */
        else
            *x = MAX(mr.x, *x); /* place menu right to cursor */
    }
    if (*y + ma.height > mr.y + mr.height) /* out of monitor */
        *y = MAX(mr.y, *y - ma.height); /* place menu above cursor */
    else
        *y = MAX(mr.y, *y); /* place menu below cursor */
}

static void on_menu(GtkAction* act, FmFolderView* fv)
{
    GtkUIManager *ui = g_object_get_qdata(G_OBJECT(fv), ui_quark);
    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    FmFolderViewInterface *iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    FmFolderModel *model;
    GtkActionGroup *act_grp;
    GList *templates;
    FmFileInfo *fi;
    gboolean show_hidden;
    FmSortMode mode;
    GtkSortType type = GTK_SORT_ASCENDING;
    FmFolderModelCol by;

    g_return_if_fail(ui != NULL);
    /* update actions */
    model = iface->get_model(fv);
    if(fm_folder_model_get_sort(model, &by, &mode))
        type = FM_SORT_IS_ASCENDING(mode) ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING;
    act = gtk_ui_manager_get_action(ui, "/popup/Sort/Asc");
    /* g_debug("set /popup/Sort/Asc: %u", type); */
    gtk_radio_action_set_current_value(GTK_RADIO_ACTION(act), type);
    act = gtk_ui_manager_get_action(ui, "/popup/Sort/ByName");
    if(by == FM_FOLDER_MODEL_COL_DEFAULT)
        by = FM_FOLDER_MODEL_COL_NAME;
    /* g_debug("set /popup/Sort/ByName: %u", by); */
    gtk_radio_action_set_current_value(GTK_RADIO_ACTION(act), by);
    act = gtk_ui_manager_get_action(ui, "/popup/Sort/SortIgnoreCase");
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act),
                                 (mode & FM_SORT_CASE_SENSITIVE) == 0);
    act = gtk_ui_manager_get_action(ui, "/popup/Sort/MingleDirs");
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act),
                                 (mode & FM_SORT_NO_FOLDER_FIRST) != 0);
    show_hidden = iface->get_show_hidden(fv);
    act = gtk_ui_manager_get_action(ui, "/popup/ShowHidden");
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), show_hidden);
    /* shadow 'Paste' if clipboard is empty and unshadow if not */
    act = gtk_ui_manager_get_action(ui, "/popup/Paste");
    fi = fm_folder_view_get_cwd_info(fv);
    if(fi && fm_file_info_is_writable_directory(fi))
        gtk_action_set_sensitive(act, fm_clipboard_have_files(GTK_WIDGET(fv)));
    else
        gtk_action_set_visible(act, FALSE);
    if(fi == NULL)
    {
        /* hide folder-oriented actions if there is no folder */
        act = gtk_ui_manager_get_action(ui, "/popup/SelAll");
        gtk_action_set_visible(act, FALSE);
        act = gtk_ui_manager_get_action(ui, "/popup/InvSel");
        gtk_action_set_visible(act, FALSE);
        act = gtk_ui_manager_get_action(ui, "/popup/Sort");
        gtk_action_set_visible(act, FALSE);
        act = gtk_ui_manager_get_action(ui, "/popup/Prop");
        gtk_action_set_visible(act, FALSE);
    }
    /* prepare templates list */
    templates = g_object_get_qdata(G_OBJECT(ui), templates_quark);
    /* FIXME: updating context menu is not lightweight here - we should
       remember all actions and ui we added, remove them, and add again.
       That will take some time, memory and may be error-prone as well.
       For simplicity we create it once here but if users will find
       any inconveniences this behavior should be changed later. */
    if(fi == NULL || !fm_file_info_is_writable_directory(fi))
    {
        act = gtk_ui_manager_get_action(ui, "/popup/CreateNew");
        gtk_action_set_visible(act, FALSE);
    }
    else if(!templates)
    {
        templates = fm_template_list_all(fm_config->only_user_templates);
        if(templates)
        {
            FmTemplate *templ;
            FmMimeType *mime_type;
            FmIcon *icon;
            const gchar *label;
            GList *l;
            GString *xml;
            GtkActionEntry actent;
            guint i;
            char act_name[16];

            l = gtk_ui_manager_get_action_groups(ui);
            act_grp = l->data; /* our action group if first one */
            actent.callback = G_CALLBACK(on_create_new);
            actent.accelerator = NULL;
            actent.tooltip = NULL;
            actent.name = act_name;
            actent.stock_id = NULL;
            xml = g_string_new("<popup><menu action='CreateNew'><placeholder name='ph1'>");
            for(l = templates, i = 0; l; l = l->next, i++)
            {
                templ = l->data;
                /* we support directories differently */
                if(fm_template_is_directory(templ))
                    continue;
                mime_type = fm_template_get_mime_type(templ);
                label = fm_template_get_label(templ);
                snprintf(act_name, sizeof(act_name), "NewFile%u", i);
                g_string_append_printf(xml, "<menuitem action='%s'/>", act_name);
                icon = fm_template_get_icon(templ);
                if(!icon)
                    icon = fm_mime_type_get_icon(mime_type);
                /* create and insert new action */
                actent.label = label ? label : fm_mime_type_get_desc(mime_type);
                gtk_action_group_add_actions(act_grp, &actent, 1, fv);
                if(icon)
                {
                    act = gtk_action_group_get_action(act_grp, act_name);
                    gtk_action_set_gicon(act, G_ICON(icon));
                }
            }
            g_string_append(xml, "</placeholder></menu></popup>");
            gtk_ui_manager_add_ui_from_string(ui, xml->str, -1, NULL);
            g_string_free(xml, TRUE);
        }
        g_object_set_qdata(G_OBJECT(ui), templates_quark, templates);
    }

    /* open popup */
    gtk_ui_manager_ensure_update(ui);
    gtk_menu_popup(popup, NULL, NULL, popup_position_func, fv, 3,
                   gtk_get_current_event_time());
}

/* handle 'Menu' and 'Shift+F10' here */
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *evt, FmFolderView* fv)
{
    int modifier = (evt->state & gtk_accelerator_get_default_mod_mask());
    if((evt->keyval == GDK_KEY_Menu && !modifier) ||
       (evt->keyval == GDK_KEY_F10 && modifier == GDK_SHIFT_MASK))
    {
        on_file_menu(NULL, fv);
        return TRUE;
    }
    else if(evt->keyval == GDK_KEY_Menu && modifier == GDK_CONTROL_MASK)
    {
        on_menu(NULL, fv);
        return TRUE;
    }
    return FALSE;
}

static inline GtkMenu *_make_file_menu(FmFolderView* fv, GtkWindow *win,
                                       FmFolderViewUpdatePopup update_popup,
                                       FmLaunchFolderFunc open_folders,
                                       FmFileInfoList* files)
{
    FmFileMenu* menu;
    FmFileInfo *fi;
    GtkUIManager *ui;
    GtkAction *act;
    FmPath *scheme; /* not-NULL if on the same scheme */
    GList *l;
    GString *str;
    FmContextMenuSchemeExt *ext;
    gboolean single_file;

    menu = fm_file_menu_new_for_files(win, files, fm_folder_view_get_cwd(fv), TRUE);
    fm_file_menu_set_folder_func(menu, open_folders, win);

    /* TODO: add info message on selection if enabled in config */
    /* disable some items on R/O file system */
    fi = fm_folder_view_get_cwd_info(fv);
    if (fi == NULL || !fm_file_info_is_writable_directory(fi))
    {
        ui = fm_file_menu_get_ui(menu);
        act = gtk_ui_manager_get_action(ui, "/popup/Cut");
        gtk_action_set_visible(act, FALSE);
        act = gtk_ui_manager_get_action(ui, "/popup/Del");
        gtk_action_set_visible(act, FALSE);
        act = gtk_ui_manager_get_action(ui, "/popup/Rename");
        gtk_action_set_visible(act, FALSE);
        act = gtk_ui_manager_get_action(ui, "/popup/ph3/Extract");
        if (act)
            gtk_action_set_visible(act, FALSE);
    }
    /* merge some specific menu items */
    if(update_popup)
        update_popup(fv, win, fm_file_menu_get_ui(menu),
                     fm_file_menu_get_action_group(menu), files);
    /* check the scheme, reset to NULL if files are on different FS (mixed folder) */
    l = fm_file_info_list_peek_head_link(files);
    scheme = fm_path_get_scheme_path(fm_file_info_get_path(l->data));
    single_file = (l->next == NULL);
    while ((l = l->next))
        if (fm_path_get_scheme_path(fm_file_info_get_path(l->data)) != scheme)
        {
            scheme = NULL;
            break;
        }
    /* we are on single scheme, good, go for it */
    if (G_LIKELY(scheme))
    {
        /* run scheme-specific extensions ("*" runs on any) */
        str = g_string_sized_new(128);
        CHECK_MODULES();
        for (l = extensions; l; l = l->next)
        {
            ext = l->data;
            if (ext->scheme && ext->scheme != scheme)
                continue; /* not NULL nor matches */
            if (ext->cb.update_file_menu_for_scheme != NULL)
                ext->cb.update_file_menu_for_scheme(win, fm_file_menu_get_ui(menu),
                                                    str, fm_file_menu_get_action_group(menu),
                                                    menu, files, single_file);
        }
        if (str->len > 0)
            gtk_ui_manager_add_ui_from_string(fm_file_menu_get_ui(menu),
                                              str->str, str->len, NULL);
        g_string_free(str, TRUE);
    }
    gtk_ui_manager_ensure_update(fm_file_menu_get_ui(menu));
    return fm_file_menu_get_menu(menu);
}

static void on_file_menu(GtkAction* act, FmFolderView* fv)
{
    FmFolderViewInterface *iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    GtkMenu *popup;
    FmFileInfoList* files;
    GtkWindow *win;
    FmFolderViewUpdatePopup update_popup;
    FmLaunchFolderFunc open_folders;

    if(iface->count_selected_files(fv) > 0)
    {
        popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
        if(popup == NULL) /* no fm_folder_view_add_popup() was called before */
            return;
        files = iface->dup_selected_files(fv);
        win = GTK_WINDOW(gtk_menu_get_attach_widget(popup));
        iface->get_custom_menu_callbacks(fv, &update_popup, &open_folders);
        popup = _make_file_menu(fv, win, update_popup, open_folders, files);
        fm_file_info_list_unref(files);
        gtk_menu_popup(popup, NULL, NULL, popup_position_func, fv, 3,
                       gtk_get_current_event_time());
    }
}

static void on_show_hidden(GtkToggleAction* act, FmFolderView* fv)
{
    gboolean active = gtk_toggle_action_get_active(act);
    fm_folder_view_set_show_hidden(fv, active);
}

static void on_mingle_dirs(GtkToggleAction* act, FmFolderView* fv)
{
    gboolean active = gtk_toggle_action_get_active(act);
    FmFolderModel *model = fm_folder_view_get_model(fv);
    FmSortMode mode;

    if(model)
    {
        fm_folder_model_get_sort(model, NULL, &mode);
        mode &= ~FM_SORT_NO_FOLDER_FIRST;
        if (active)
            mode |= FM_SORT_NO_FOLDER_FIRST;
        fm_folder_model_set_sort(model, FM_FOLDER_MODEL_COL_DEFAULT, mode);
    }
}

static void on_ignore_case(GtkToggleAction* act, FmFolderView* fv)
{
    gboolean active = gtk_toggle_action_get_active(act);
    FmFolderModel *model = fm_folder_view_get_model(fv);
    FmSortMode mode;

    if(model)
    {
        fm_folder_model_get_sort(model, NULL, &mode);
        mode &= ~FM_SORT_CASE_SENSITIVE;
        if (!active)
            mode |= FM_SORT_CASE_SENSITIVE;
        fm_folder_model_set_sort(model, FM_FOLDER_MODEL_COL_DEFAULT, mode);
    }
}

static void on_change_by(GtkRadioAction* act, GtkRadioAction* cur, FmFolderView* fv)
{
    guint val = gtk_radio_action_get_current_value(cur);
    FmFolderModel *model = fm_folder_view_get_model(fv);

    /* g_debug("on_change_by"); */
    if(model)
        fm_folder_model_set_sort(model, val, FM_SORT_DEFAULT);
}

static void on_change_type(GtkRadioAction* act, GtkRadioAction* cur, FmFolderView* fv)
{
    guint val = gtk_radio_action_get_current_value(cur);
    FmFolderModel *model = fm_folder_view_get_model(fv);
    FmSortMode mode;

    /* g_debug("on_change_type"); */
    if(model)
    {
        fm_folder_model_get_sort(model, NULL, &mode);
        mode &= ~FM_SORT_ORDER_MASK;
        mode |= (val == GTK_SORT_ASCENDING) ? FM_SORT_ASCENDING : FM_SORT_DESCENDING;
        fm_folder_model_set_sort(model, FM_FOLDER_MODEL_COL_DEFAULT, mode);
    }
}

static void on_ui_destroy(gpointer ui_ptr)
{
    GtkUIManager* ui = (GtkUIManager*)ui_ptr;
    GtkMenu* popup = GTK_MENU(gtk_ui_manager_get_widget(ui, "/popup"));
    GtkWidget* win = gtk_menu_get_attach_widget(popup);
    GtkAccelGroup* accel_grp = gtk_ui_manager_get_accel_group(ui);
    GList *templates = g_object_get_qdata(G_OBJECT(ui), templates_quark);
    GSList *groups;

    if (win != NULL) /* it might be already destroyed */
    {
        g_object_weak_unref(G_OBJECT(win), (GWeakNotify)gtk_menu_detach, popup);
        groups = gtk_accel_groups_from_object(G_OBJECT(win));
        if(g_slist_find(groups, accel_grp) != NULL)
            gtk_window_remove_accel_group(GTK_WINDOW(win), accel_grp);
    }
    g_list_foreach(templates, (GFunc)g_object_unref, NULL);
    g_list_free(templates);
    g_object_set_qdata(G_OBJECT(ui), templates_quark, NULL);
    gtk_widget_destroy(GTK_WIDGET(popup));
    g_object_unref(ui);
}

/**
 * fm_folder_view_add_popup
 * @fv: a widget to apply
 * @parent: parent window of @fv
 * @update_popup: function to extend popup menu for folder
 *
 * Adds popup menu to window @parent associated with widget @fv. This
 * includes hotkeys for popup menu items. Popup will be destroyed and
 * hotkeys will be removed from @parent when @fv is finalized or after
 * next call to fm_folder_view_add_popup() on the same @fv.
 *
 * Since plugins may change popup menu appearance in accordance with
 * the folder, implementaions are encouraged to use this API each time
 * the model is changed on the @fv.
 *
 * Returns: (transfer none): a new created widget.
 *
 * Since: 1.0.1
 */
GtkMenu* fm_folder_view_add_popup(FmFolderView* fv, GtkWindow* parent,
                                  FmFolderViewUpdatePopup update_popup)
{
    FmFolderViewInterface* iface;
    FmFolderModel* model;
    GtkUIManager* ui;
    GtkActionGroup* act_grp;
    GtkMenu* popup;
    GtkAction* act;
    GtkAccelGroup* accel_grp;
    FmPath *scheme;
    GList *l;
    FmContextMenuSchemeExt *ext;
    gboolean show_hidden;
    FmSortMode mode;
    GtkSortType type = (GtkSortType)-1;
    FmFolderModelCol by = (FmFolderModelCol)-1;

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    show_hidden = iface->get_show_hidden(fv);
    model = iface->get_model(fv);
    if(fm_folder_model_get_sort(model, &by, &mode))
        type = FM_SORT_IS_ASCENDING(mode) ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING;

    /* init popup from XML string */
    ui = gtk_ui_manager_new();
    act_grp = gtk_action_group_new("Folder");
    gtk_action_group_set_translation_domain(act_grp, GETTEXT_PACKAGE);
    gtk_action_group_add_actions(act_grp, folder_popup_actions,
                                 G_N_ELEMENTS(folder_popup_actions), fv);
    gtk_action_group_add_toggle_actions(act_grp, folder_toggle_actions,
                                        G_N_ELEMENTS(folder_toggle_actions), fv);
    gtk_action_group_add_radio_actions(act_grp, folder_sort_type_actions,
                                       G_N_ELEMENTS(folder_sort_type_actions),
                                       type, G_CALLBACK(on_change_type), fv);
    gtk_action_group_add_radio_actions(act_grp, folder_sort_by_actions,
                                       G_N_ELEMENTS(folder_sort_by_actions),
                                       by, G_CALLBACK(on_change_by), fv);
    gtk_ui_manager_insert_action_group(ui, act_grp, 0);
    gtk_ui_manager_add_ui_from_string(ui, folder_popup_xml, -1, NULL);
    act = gtk_ui_manager_get_action(ui, "/popup/ShowHidden");
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), show_hidden);
    act = gtk_ui_manager_get_action(ui, "/popup/Cut");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/Copy");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/Del");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/Remove");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/FileProp");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/Rename");
    gtk_action_set_visible(act, FALSE);
    if(update_popup)
        update_popup(fv, parent, ui, act_grp, NULL);
    scheme = fm_folder_view_get_cwd(fv);
    if (scheme)
    {
        scheme = fm_path_get_scheme_path(scheme);
        CHECK_MODULES();
        for (l = extensions; l; l = l->next)
        {
            ext = l->data;
            if (ext->scheme && ext->scheme != scheme)
                continue; /* not NULL nor matches */
            /* FIXME: can we update menu each time it's called so take in account
               the selection? May be it's possible but it will require full menu
               rebuild with all updates and accelerators so it will be slow */
            if (ext->cb.update_folder_menu != NULL)
                ext->cb.update_folder_menu(fv, parent, ui, act_grp, NULL);
        }
    }
    popup = GTK_MENU(gtk_ui_manager_get_widget(ui, "/popup"));
    accel_grp = gtk_ui_manager_get_accel_group(ui);
    gtk_window_add_accel_group(parent, accel_grp);
    gtk_menu_attach_to_widget(popup, GTK_WIDGET(parent), NULL);
    g_object_weak_ref(G_OBJECT(parent), (GWeakNotify)gtk_menu_detach, popup);
    g_object_unref(act_grp);
    g_object_set_qdata_full(G_OBJECT(fv), ui_quark, ui, on_ui_destroy);
    /* we bind popup to ui so don't handle qdata change here */
    g_object_set_qdata(G_OBJECT(fv), popup_quark, popup);
    /* special handling for 'Menu' key */
    g_signal_handlers_disconnect_by_func(fv, on_key_press, fv);
    g_signal_connect(G_OBJECT(fv), "key-press-event", G_CALLBACK(on_key_press), fv);
    return popup;
}

/**
 * fm_folder_view_bounce_action
 * @act: an action to execute
 * @fv: a widget to apply
 *
 * Executes the action with the same name as @act in popup menu of @fv.
 * The popup menu should be created with fm_folder_view_add_popup()
 * before this call.
 *
 * Implemented actions are:
 * - Cut       : cut files (or text from editable) into clipboard
 * - Copy      : copy files (or text from editable) into clipboard
 * - Paste     : paste files (or text from editable) from clipboard
 * - Del       : move files into trash bin (or delete text from editable)
 * - Remove    : delete files from filesystem (for editable does Cut)
 * - SelAll    : select all
 * - InvSel    : invert selection
 * - Rename    : rename the folder
 * - Prop      : folder properties dialog
 * - FileProp  : file properties dialog
 * - NewFolder : create new folder here
 * - NewBlank  : create an empty file here
 *
 * Actions 'Cut', 'Copy', 'Paste', 'Del', 'Remove', 'SelAll' do nothing
 * if current keyboard focus is neither on @fv nor on some #GtkEditable.
 *
 * See also: fm_folder_view_add_popup().
 *
 * Since: 1.0.1
 */
void fm_folder_view_bounce_action(GtkAction* act, FmFolderView* fv)
{
    const gchar *name;
    GtkUIManager *ui;
    GList *groups;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));
    g_return_if_fail(act != NULL);

    ui = g_object_get_qdata(G_OBJECT(fv), ui_quark);
    g_return_if_fail(ui != NULL && GTK_IS_UI_MANAGER(ui));

    groups = gtk_ui_manager_get_action_groups(ui);
    g_return_if_fail(groups != NULL);

    name = gtk_action_get_name(act);
    act = gtk_action_group_get_action((GtkActionGroup*)groups->data, name);
    if(act)
    {
        /* if we get here it means menu isn't shown but some action
           might be set insensitive by previous invocation of menu
           therefore we forcing its visibility to allow activation */
        gtk_action_set_sensitive(act, TRUE);
        gtk_action_activate(act);
    }
    else
        g_debug("requested action %s wasn't found in popup", name);
}

/**
 * fm_folder_view_set_active
 * @fv: the folder view widget to apply
 * @set: state of accelerators to be set
 *
 * If @set is %TRUE then activates accelerators on the @fv that were
 * created with fm_folder_view_add_popup() before. If @set is %FALSE
 * then deactivates accelerators on the @fv. This API is useful if the
 * application window contains more than one folder view so gestures
 * will be used only on active view. This API has no effect in no popup
 * menu was created with fm_folder_view_add_popup() before this call.
 *
 * See also: fm_folder_view_add_popup().
 *
 * Since: 1.0.1
 */
void fm_folder_view_set_active(FmFolderView* fv, gboolean set)
{
    GtkUIManager *ui;
    GtkMenu *popup;
    GtkWindow* win;
    GtkAccelGroup* accel_grp;
    GSList *groups;
    gboolean active;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    ui = g_object_get_qdata(G_OBJECT(fv), ui_quark);
    popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);

    g_return_if_fail(ui != NULL && GTK_IS_UI_MANAGER(ui));
    g_return_if_fail(popup != NULL && GTK_IS_MENU(popup));

    win = GTK_WINDOW(gtk_menu_get_attach_widget(popup));
    accel_grp = gtk_ui_manager_get_accel_group(ui);
    groups = gtk_accel_groups_from_object(G_OBJECT(win));
    active = (g_slist_find(groups, accel_grp) != NULL);

    if(set && !active)
        gtk_window_add_accel_group(win, accel_grp);
    else if(!set && active)
        gtk_window_remove_accel_group(win, accel_grp);
}

/**
 * fm_folder_view_item_clicked
 * @fv: the folder view widget
 * @path: (allow-none): path to current pointed item
 * @type: what click was received
 *
 * Handles left click and right click in folder area. If some item was
 * left-clicked then fm_folder_view_item_clicked() tries to launch it.
 * If some item was right-clicked then opens file menu (applying the
 * update_popup returned by get_custom_menu_callbacks interface function
 * before opening it if it's not %NULL). If it was right-click on empty
 * space of folder view (so @path is %NULL) then opens folder popup
 * menu that was created by fm_folder_view_add_popup(). After that
 * emits the #FmFolderView::clicked signal.
 *
 * If open_folders callback from interface function get_custom_menu_callbacks
 * is %NULL then assume it was old API call so click will be not handled
 * by this function and signal handler will handle it instead. Otherwise
 * the user_data for it will be #GtkWindow the menu is attached to.
 *
 * This API is internal for #FmFolderView and should be used only in
 * class implementations.
 *
 * Since: 1.0.1
 */
void fm_folder_view_item_clicked(FmFolderView* fv, GtkTreePath* path,
                                 FmFolderViewClickType type)
{
    FmFolderViewInterface* iface;
    GtkTreeModel* model;
    FmFileInfo* fi;
    FmFileInfoList *files;
    GtkMenu *popup;
    GtkWindow *win;
    FmFolderViewUpdatePopup update_popup;
    FmLaunchFolderFunc open_folders;
    GtkTreeIter it;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    if(path)
    {
        model = GTK_TREE_MODEL(iface->get_model(fv));
        if(gtk_tree_model_get_iter(model, &it, path))
            gtk_tree_model_get(model, &it, FM_FOLDER_MODEL_COL_INFO, &fi, -1);
    }
    else
        fi = NULL;
    popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    if(popup == NULL) /* no fm_folder_view_add_popup() was called before */
        goto send_signal;
    win = GTK_WINDOW(gtk_menu_get_attach_widget(popup));
    /* handle left and rigth clicks */
    iface->get_custom_menu_callbacks(fv, &update_popup, &open_folders);
    /* if open_folders is NULL then it's old API call so don't handle */
    if(open_folders) switch(type)
    {
    case FM_FV_ACTIVATED: /* file(s) activated */
        files = iface->dup_selected_files(fv);
        if (files == NULL)
        {
            if (fi == NULL) /* oops, nothing to activate */
                goto send_signal;
            files = fm_file_info_list_new();
            fm_file_info_list_push_tail(files, fi);
        }
        fm_launch_files_simple(win, NULL, fm_file_info_list_peek_head_link(files),
                               open_folders, win);
        fm_file_info_list_unref(files);
        break;
    case FM_FV_CONTEXT_MENU:
        if(fi && iface->count_selected_files(fv) > 0)
                 /* workaround on ExoTreeView bug */
        {
            files = iface->dup_selected_files(fv);
            popup = _make_file_menu(fv, win, update_popup, open_folders, files);
            fm_file_info_list_unref(files);
            gtk_menu_popup(popup, NULL, NULL, popup_position_func, fv, 3,
                           gtk_get_current_event_time());
        }
        else /* no files are selected. Show context menu of current folder. */
            on_menu(NULL, fv);
        break;
    default: ;
    }
send_signal:
    /* send signal */
    g_signal_emit(fv, signals[CLICKED], 0, type, fi);
}

/**
 * fm_folder_view_sel_changed
 * @obj: some object; unused
 * @fv: the folder view widget to apply
 *
 * Emits the #FmFolderView::sel-changed signal.
 *
 * This API is internal for #FmFolderView and should be used only in
 * class implementations.
 *
 * Since: 1.0.1
 */
void fm_folder_view_sel_changed(GObject* obj, FmFolderView* fv)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    /* if someone is connected to our "sel-changed" signal. */
    if(g_signal_has_handler_pending(fv, signals[SEL_CHANGED], 0, TRUE))
    {
        FmFolderViewInterface* iface = FM_FOLDER_VIEW_GET_IFACE(fv);
        gint files = iface->count_selected_files(fv);

        /* emit a selection changed notification to the world. */
        g_signal_emit(fv, signals[SEL_CHANGED], 0, files);
    }
}

#if 0
/**
 * fm_folder_view_chdir
 * @fv: the folder view widget to apply
 *
 * Emits the #FmFolderView::chdir signal.
 *
 * This API is internal for #FmFolderView and should be used only in
 * class implementations.
 *
 * Since: 1.0.2
 */
void fm_folder_view_chdir(FmFolderView* fv, FmPath* path)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    g_signal_emit(fv, signals[CHDIR], 0, path);
}
#endif

/**
 * fm_folder_view_set_columns
 * @fv: the folder view widget to apply
 * @cols: (element-type FmFolderViewColumnInfo): new list of column infos
 *
 * Changes composition (rendering) of folder view @fv in accordance to
 * new list of column infos.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 1.0.2
 */
gboolean fm_folder_view_set_columns(FmFolderView* fv, const GSList* cols)
{
    FmFolderViewInterface* iface;

    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), FALSE);

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);

    if(iface->set_columns)
        return iface->set_columns(fv, cols);
    return FALSE;
}

/**
 * fm_folder_view_get_columns
 * @fv: the folder view widget to query
 *
 * Retrieves current composition of @fv as list of column infos. Returned
 * list should be freed with g_slist_free() after usage.
 *
 * Returns: (transfer container) (element-type FmFolderViewColumnInfo): list of column infos.
 *
 * Since: 1.0.2
 */
GSList* fm_folder_view_get_columns(FmFolderView* fv)
{
    FmFolderViewInterface* iface;

    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), NULL);

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);

    if(iface->get_columns)
        return iface->get_columns(fv);
    return NULL;
}

/**
 * fm_folder_view_columns_changed
 * @fv: the folder view widget to apply
 *
 * Emits the #FmFolderView::columns-changed signal.
 *
 * This API is internal for #FmFolderView and should be used only in
 * class implementations.
 *
 * Since: 1.2.0
 */
void fm_folder_view_columns_changed(FmFolderView* fv)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    g_signal_emit(fv, signals[COLUMNS_CHANGED], 0);
}

/* modules support */
FM_MODULE_DEFINE_TYPE(gtk_menu_scheme, FmContextMenuSchemeAddonInit, 1)

static gboolean fm_module_callback_gtk_menu_scheme(const char *name, gpointer init, int ver)
{
    FmContextMenuSchemeExt *ext = g_slice_new(FmContextMenuSchemeExt);
    FmContextMenuSchemeAddonInit *cb = init;
    char *scheme_str;
    FmPath *path;

    /* not checking version, it's only 1 for now */
    if (strcmp(name, "*") == 0)
        ext->scheme = NULL;
    else if (strcmp(name, "menu") == 0) /* special support */
        ext->scheme = fm_path_new_for_uri("menu://applications/");
    else if (strchr(name, '/') != NULL)
    {
        path = fm_path_new_for_uri(name);
        ext->scheme = fm_path_ref(fm_path_get_scheme_path(path));
        fm_path_unref(path);
    }
    else
    {
        scheme_str = g_strdup_printf("%s://", name);
        path = fm_path_new_for_uri(scheme_str);
        ext->scheme = fm_path_ref(fm_path_get_scheme_path(path));
        g_free(scheme_str);
        fm_path_unref(path);
    }
    ext->cb = *cb;
    if (cb->init != NULL)
        cb->init();
    extensions = g_list_append(extensions, ext);
    return TRUE;
}

void _fm_folder_view_init(void)
{
    fm_module_register_gtk_menu_scheme();
}

void _fm_folder_view_finalize(void)
{
    GList *list, *l;
    FmContextMenuSchemeExt *ext;

    list = extensions;
    extensions = NULL;
    for (l = list; l; l = l->next)
    {
        ext = l->data;
        if (ext->cb.finalize != NULL)
            ext->cb.finalize();
        if (ext->scheme != NULL)
            fm_path_unref(ext->scheme);
        g_slice_free(FmContextMenuSchemeExt, ext);
    }
    g_list_free(list);
    fm_module_unregister_type("gtk_menu_scheme");
}

/**
 * fm_folder_view_scroll_to_path
 * @fv: the folder view widget to query
 * @path: the item to scroll
 * @focus: %TRUE to set cursor focus on item
 *
 * Scrolls the view to get item defined by @path closely to center of the
 * view window. If @focus is %TRUE then also keyboard focus will be set
 * to the @path.
 *
 * Since: 1.2.0
 */
void fm_folder_view_scroll_to_path(FmFolderView* fv, FmPath *path, gboolean focus)
{
    FmFolderViewInterface* iface;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv) && path != NULL);

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);

    if (iface->scroll_to_path)
        iface->scroll_to_path(fv, path, focus);
}
