/*
 * Copyright (C) 2017  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QDebug>
#include "../core/folder.h"
#include "../foldermodel.h"
#include "../folderview.h"
#include "../cachedfoldermodel.h"
#include "../proxyfoldermodel.h"
#include "../pathedit.h"
#include "libfmqt.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    Fm::LibFmQt contex;
    QMainWindow win;

    Fm::FolderView folder_view;
    win.setCentralWidget(&folder_view);

    auto home = Fm::FilePath::homeDir();
    Fm::CachedFolderModel* model = Fm::CachedFolderModel::modelFromPath(home);
    auto proxy_model = new Fm::ProxyFolderModel();
    proxy_model->sort(Fm::FolderModel::ColumnFileName, Qt::AscendingOrder);
    proxy_model->setSourceModel(model);

    proxy_model->setThumbnailSize(64);
    proxy_model->setShowThumbnails(true);

    folder_view.setModel(proxy_model);

    QToolBar toolbar;
    win.addToolBar(Qt::TopToolBarArea, &toolbar);
    Fm::PathEdit edit;
    edit.setText(QString::fromUtf8(home.toString().get()));
    toolbar.addWidget(&edit);
    auto action = new QAction(QStringLiteral("Go"), nullptr);
    toolbar.addAction(action);
    QObject::connect(action, &QAction::triggered, [&]() {
        auto path = Fm::FilePath::fromPathStr(edit.text().toLocal8Bit().constData());
        auto new_model = Fm::CachedFolderModel::modelFromPath(path);
        proxy_model->setSourceModel(new_model);
    });

    win.show();
    return app.exec();
}
