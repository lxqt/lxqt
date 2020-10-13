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

#ifndef PCMANFM_LAUNCHER_H
#define PCMANFM_LAUNCHER_H

#include <libfm-qt/filelauncher.h>

namespace PCManFM {

class MainWindow;

class Launcher : public Fm::FileLauncher {
public:
    Launcher(MainWindow* mainWindow = nullptr);
    ~Launcher();

    bool hasMainWindow() const {
        return mainWindow_ != nullptr;
    }

    void openInNewTab() {
        openInNewTab_ = true;
    }

protected:
    bool openFolder(GAppLaunchContext* ctx, const Fm::FileInfoList& folderInfos, Fm::GErrorPtr& err) override;

private:
    MainWindow* mainWindow_;
    bool openInNewTab_;
};

}

#endif // PCMANFM_LAUNCHER_H
