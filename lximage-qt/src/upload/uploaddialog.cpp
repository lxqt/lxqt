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

#include <QClipboard>
#include <QComboBox>
#include <QGuiApplication>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QVariant>

#include "imageshackprovider.h"
#include "imgbbprovider.h"
#include "imgurprovider.h"
#include "provider.h"
#include "upload.h"
#include "uploaddialog.h"

using namespace LxImage;

ImgurProvider gImgurProvider;
ImgBBProvider gImgBBProvider;
ImageShackProvider gImageShackProvider;

UploadDialog::UploadDialog(QWidget *parent, const QString &filename)
    : QDialog(parent),
      mState(SelectProvider),
      mFile(filename),
      mUpload(nullptr)
{
    ui.setupUi(this);

    // Populate the list of providers
    ui.providerComboBox->addItem(tr("Imgur"), QVariant::fromValue(&gImgurProvider));
    ui.providerComboBox->addItem(tr("ImgBB"), QVariant::fromValue(&gImgBBProvider));
    ui.providerComboBox->addItem(tr("ImageShack"), QVariant::fromValue(&gImageShackProvider));

    updateUi();
}

void UploadDialog::on_actionButton_clicked()
{
    switch (mState) {
    case SelectProvider:
        start();
        break;
    case UploadInProgress:
        mUpload->abort();
        break;
    case Completed:
        accept();
        break;
    }
}

void UploadDialog::on_copyButton_clicked()
{
    QGuiApplication::clipboard()->setText(ui.linkLineEdit->text());
}

void UploadDialog::start()
{
    // Attempt to open the file
    if (!mFile.open(QIODevice::ReadOnly)) {
        showError(mFile.errorString());
        return;
    }

    // Retrieve the selected provider
    Provider *provider = ui.providerComboBox->itemData(
        ui.providerComboBox->currentIndex()
    ).value<Provider*>();

    // Create the upload
    mUpload = provider->upload(&mFile);

    // Update the progress bar as the upload progresses
    connect(mUpload, &Upload::progress, ui.progressBar, &QProgressBar::setValue);

    // If the request completes, show the link to the user
    connect(mUpload, &Upload::completed, [this](const QString &url) {
        ui.linkLineEdit->setText(url);
        ui.linkLineEdit->selectAll();

        mState = Completed;
        updateUi();
    });

    // If the request fails, show an error
    connect(mUpload, &Upload::error, [this](const QString &message) {
        showError(message);
    });

    // Destroy the upload when it completes
    connect(mUpload, &Upload::finished, [this]() {
        mFile.close();
        mUpload->deleteLater();
        mUpload = nullptr;
    });

    mState = UploadInProgress;
    updateUi();
}

void UploadDialog::updateUi()
{
    // Show the appropriate control given the current state
    ui.providerComboBox->setVisible(mState == SelectProvider);
    ui.progressBar->setVisible(mState == UploadInProgress);
    ui.linkLineEdit->setVisible(mState == Completed);
    ui.copyButton->setVisible(mState == Completed);

    // Reset the progress bar to zero
    ui.progressBar->setValue(0);

    // Set the correct button text
    switch (mState) {
    case SelectProvider:
        ui.actionButton->setText(tr("Start"));
        break;
    case UploadInProgress:
        ui.actionButton->setText(tr("Stop"));
        break;
    case Completed:
        ui.actionButton->setText(tr("Close"));
        break;
    }
}

void UploadDialog::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Error"), message);

    mState = SelectProvider;
    updateUi();
}
