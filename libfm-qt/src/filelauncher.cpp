/*
 * Copyright (C) 2012 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#include "filelauncher.h"
#include "applaunchcontext.h"
#include <QMessageBox>
#include <QDebug>
#include "execfiledialog_p.h"
#include "appchooserdialog.h"
#include "utilities.h"

#include "core/fileinfojob.h"
#include "mountoperation.h"

namespace Fm {

FileLauncher::FileLauncher() {
}

FileLauncher::~FileLauncher() {
}


bool FileLauncher::launchFiles(QWidget* parent, const FileInfoList &file_infos) {
    GObjectPtr<FmAppLaunchContext> context{fm_app_launch_context_new_for_widget(parent), false};
    bool ret = BasicFileLauncher::launchFiles(file_infos, G_APP_LAUNCH_CONTEXT(context.get()));
    return ret;
}

bool FileLauncher::launchPaths(QWidget* parent, const FilePathList& paths) {
    GObjectPtr<FmAppLaunchContext> context{fm_app_launch_context_new_for_widget(parent), false};
    bool ret = BasicFileLauncher::launchPaths(paths, G_APP_LAUNCH_CONTEXT(context.get()));
    return ret;
}

int FileLauncher::ask(const char* /*msg*/, char* const* /*btn_labels*/, int /*default_btn*/) {
    /* FIXME: set default button properly */
    // return fm_askv(data->parent, nullptr, msg, btn_labels);
    return -1;
}

GAppInfoPtr FileLauncher::chooseApp(const FileInfoList& /*fileInfos*/, const char *mimeType, GErrorPtr& /*err*/) {
    AppChooserDialog dlg(nullptr);
    GAppInfoPtr app;
    if(mimeType) {
        dlg.setMimeType(Fm::MimeType::fromName(mimeType));
    }
    else {
        dlg.setCanSetDefault(false);
    }
    // FIXME: show error properly?
    if(execModelessDialog(&dlg) == QDialog::Accepted) {
        app = dlg.selectedApp();
    }
    return app;
}

bool FileLauncher::openFolder(GAppLaunchContext *ctx, const FileInfoList &folderInfos, GErrorPtr &err) {
    return BasicFileLauncher::openFolder(ctx, folderInfos, err);
}

bool FileLauncher::showError(GAppLaunchContext* /*ctx*/, const GErrorPtr &err, const FilePath &path, const FileInfoPtr &info) {
    if(err == nullptr) {
        return false;
    }
    /* ask for mount if trying to launch unmounted path */
    if(err->domain == G_IO_ERROR) {
        if(path && err->code == G_IO_ERROR_NOT_MOUNTED) {
            MountOperation* op = new MountOperation(true);
            op->setAutoDestroy(true);
            if(info && info->isMountable()) {
                // this is a mountable shortcut (such as computer:///xxxx.drive)
                op->mountMountable(path);
            }
            else {
                op->mountEnclosingVolume(path);
            }
            if(op->wait()) {
                // if the mount operation succeeds, we can ignore the error and continue
                return true;
            }
        }
        else if(err->code == G_IO_ERROR_FAILED_HANDLED) {
            return true;    /* don't show error message */
        }
    }
    QMessageBox dlg(QMessageBox::Critical, QObject::tr("Error"), QString::fromUtf8(err->message), QMessageBox::Ok);
    execModelessDialog(&dlg);
    return false;
}

BasicFileLauncher::ExecAction FileLauncher::askExecFile(const FileInfoPtr &file) {
    auto res = BasicFileLauncher::ExecAction::CANCEL;
    ExecFileDialog dlg(*file);
    if(execModelessDialog(&dlg) == QDialog::Accepted) {
        res = dlg.result();
    }
    return res;
}


} // namespace Fm
