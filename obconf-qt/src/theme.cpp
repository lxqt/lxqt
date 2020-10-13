/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   theme.h for ObConf, the configuration tool for Openbox
   Copyright (c) 2003-2007   Dana Jansens
   Copyright (c) 2003        Tim Riley

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   See the COPYING file for a copy of the GNU General Public License.
*/

#include "maindialog.h"

#include <QStandardItem>
#include <QStandardItemModel>
#include <QItemSelection>
#include <QFileDialog>

#include "obconf-qt.h"
#include "tree.h"
#include "preview_update.h"
#include "archive.h"
#include "preview.h"

using namespace Obconf;

void MainDialog::theme_install(const char* path) {
  gchar* name;

  if((name = archive_install(path)))
    tree_set_string("theme/name", name);

  g_free(name);

  theme_load_all();
}

void MainDialog::theme_load_all() {
  gchar* name;
  gchar* p;
  GList* it, *next;
  gint i;
  QModelIndex currentItemIndex;

  name = tree_get_string("theme/name", "TheBear");
  if(themes) {
    g_list_foreach(themes, (GFunc)g_free, NULL);
    g_list_free(themes);
    themes = NULL;
  }

  p = g_build_filename(g_get_home_dir(), ".themes", NULL);
  add_theme_dir(p);
  g_free(p);
  {
    GSList* it;
    for(it = obt_paths_data_dirs(paths); it; it = g_slist_next(it)) {
      p = g_build_filename((char*)it->data, "themes", NULL);
      add_theme_dir(p);
      g_free(p);
    }
  }

  add_theme_dir(THEME_DIR);
  themes = g_list_sort(themes, (GCompareFunc) strcasecmp);
  
  themes_model->clear();

  /* return to regular scheduled programming */
  i = 0;
  for(it = themes; it; it = next) {
    next = g_list_next(it);
    /* remove duplicates */
    if(next && !strcmp((char*)it->data, (char*)next->data)) {
      g_free(it->data);
      themes = g_list_delete_link(themes, it);
      continue;
    }

    QStandardItem* item = new QStandardItem(QString::fromUtf8((char*)it->data));
    themes_model->appendRow(item);
    if(!strcmp(name, (char*)it->data)) {
      currentItemIndex = item->index();
      ui.theme_names->selectionModel()->select(currentItemIndex, QItemSelectionModel::Select);
    }
    ++i;
  }
  // FIXME: preview_update_all();
  g_free(name);
  if(currentItemIndex.isValid())
    ui.theme_names->scrollTo(currentItemIndex, QAbstractItemView::PositionAtCenter);
}

void MainDialog::add_theme_dir(const char* dirname) {
  GDir* dir;
  const gchar* n;

  if((dir = g_dir_open(dirname, 0, NULL))) {
    while((n = g_dir_read_name(dir))) {
      {
        gchar* full;
        full = g_build_filename(dirname, n, "openbox-3",
                                "themerc", NULL);

        if(!g_file_test(full, GFileTest(G_FILE_TEST_IS_REGULAR | G_FILE_TEST_IS_SYMLINK)))
          n = NULL;

        g_free(full);
      }

      if(n) {
        themes = g_list_append(themes, g_strdup(n));
      }
    }

    g_dir_close(dir);
  }
}

void MainDialog::theme_setup_tab() {
  QItemSelectionModel* selModel = ui.theme_names->selectionModel();
  connect(selModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          SLOT(onThemeNamesSelectionChanged(QItemSelection,QItemSelection)));

}

void MainDialog::onThemeNamesSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected) {
  QModelIndex sel = selected.indexes().first();
  if(sel.isValid()) {
    QStandardItem* item = themes_model->itemFromIndex(sel);
    QVariant val = item->data(Qt::DisplayRole);
    if(val.isValid()) {
      QString name = val.toString();
      tree_set_string("theme/name", name.toUtf8().constData());
    }
  }
}

void MainDialog::on_install_theme_clicked() {
  QString filename = QFileDialog::getOpenFileName(this,
                                                  tr("Choose an Openbox theme"),
                                                  QString(),
                                                  QStringLiteral("Openbox theme archives (*.obt);;"));
  if(!filename.isEmpty()) {
    theme_install(filename.toLocal8Bit().constData());
  }
}

void MainDialog::on_theme_archive_clicked() {
    QFileDialog* dialog = new QFileDialog();
    dialog->setFileMode(QFileDialog::Directory);
    QString filename=QLatin1String("");
    if(dialog->exec())
         filename= dialog->selectedFiles()[0];
  if(!filename.isEmpty()) {
    archive_create(filename.toLocal8Bit().constData());
  }
}
