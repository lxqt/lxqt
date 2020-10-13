/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
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

#include "launcher.h"
#include "mainwindow.h"
#include "application.h"
#include <libfm-qt/core/filepath.h>

namespace PCManFM {

Launcher::Launcher(PCManFM::MainWindow* mainWindow):
    Fm::FileLauncher(),
    mainWindow_(mainWindow),
    openInNewTab_(false) {

    Application* app = static_cast<Application*>(qApp);
    setQuickExec(app->settings().quickExec());
}

Launcher::~Launcher() {

}

bool Launcher::openFolder(GAppLaunchContext* /*ctx*/, const Fm::FileInfoList& folderInfos, Fm::GErrorPtr& /*err*/) {
    auto fi = folderInfos[0];
    Application* app = static_cast<Application*>(qApp);
    MainWindow* mainWindow = mainWindow_;
    Fm::FilePath path = fi->path();
    if(!mainWindow) {
        mainWindow = new MainWindow(std::move(path));
        mainWindow->resize(app->settings().windowWidth(), app->settings().windowHeight());

        if(app->settings().windowMaximized()) {
            mainWindow->setWindowState(mainWindow->windowState() | Qt::WindowMaximized);
        }
    }
    else {
        if(openInNewTab_) {
            mainWindow->addTab(std::move(path));
        }
        else {
            mainWindow->chdir(std::move(path));
        }
    }

    for(size_t i = 1; i < folderInfos.size(); ++i) {
        fi = folderInfos[i];
        path = fi->path();
        mainWindow->addTab(std::move(path));
    }
    mainWindow->show();
    mainWindow->raise();
    mainWindow->activateWindow();
    openInNewTab_ = false;
    return true;
}

} //namespace PCManFM
