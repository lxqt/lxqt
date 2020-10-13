/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef LXIMAGE_UPLOADDIALOG_H
#define LXIMAGE_UPLOADDIALOG_H

#include <QDialog>
#include <QFile>

#include "ui_uploaddialog.h"

namespace LxImage {

class Upload;

/**
 * @brief Dialog for uploading an image
 */
class UploadDialog : public QDialog
{
    Q_OBJECT

public:

    /**
     * @brief Create a dialog for uploading the specified file
     * @param parent widget parent
     * @param filename absolute path to file
     */
    UploadDialog(QWidget *parent, const QString &filename);

private Q_SLOTS:

    void on_actionButton_clicked();
    void on_copyButton_clicked();

private:

    void start();
    void updateUi();
    void showError(const QString &message);

    Ui::UploadDialog ui;

    enum {
        SelectProvider,
        UploadInProgress,
        Completed,
    } mState;

    QFile mFile;

    Upload *mUpload;
};

}

#endif // LXIMAGE_UPLOADDIALOG_H
