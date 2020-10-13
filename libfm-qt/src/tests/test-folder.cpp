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
#include <QDebug>
#include "../core/folder.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    auto home = Fm::FilePath::homeDir();
    auto folder = Fm::Folder::fromPath(home);

    QObject::connect(folder.get(), &Fm::Folder::startLoading, [=]() {
        qDebug("start loading");
    });
    QObject::connect(folder.get(), &Fm::Folder::finishLoading, [=]() {
        qDebug("finish loading");
    });

    QObject::connect(folder.get(), &Fm::Folder::filesAdded, [=](Fm::FileInfoList& files) {
        qDebug("files added");
        for(auto& item: files) {
            qDebug() << item->displayName();
        }
    });
    QObject::connect(folder.get(), &Fm::Folder::filesRemoved, [=](Fm::FileInfoList& files) {
        qDebug("files removed");
        for(auto& item: files) {
            qDebug() << item->displayName();
        }
    });
    QObject::connect(folder.get(), &Fm::Folder::filesChanged, [=](std::vector<Fm::FileInfoPair>& file_pairs) {
        qDebug("files changed");
        for(auto& pair: file_pairs) {
            auto& item = pair.second;
            qDebug() << item->displayName();
        }
    });

    for(auto& item: folder->files()) {
        qDebug() << item->displayName();
    }
    qDebug() << "here";

    return app.exec();
}
