#include "progressdialog.h"

#include "ui_progressdialog.h"
#include "archiver.h"
#include <QPushButton>

ProgressDialog::ProgressDialog(QWidget* parent) :
    QDialog(parent),
    ui_{new Ui::ProgressDialog{}},
    archiver_{nullptr} {

    ui_->setupUi(this);
    ui_->buttonBox_1->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui_->progressBar->setValue(0);
    ui_->progressBar->show();
    ui_->progressBar->setFormat(tr("%p %"));
}

ProgressDialog::~ProgressDialog() {
}

void ProgressDialog::setArchiver(Archiver* archiver) {
    if(archiver_) {
        archiver_->disconnect(this);
    }
    archiver_ = archiver;
    connect(archiver, &Archiver::progress, this, &ProgressDialog::onProgress);
    connect(archiver, &Archiver::message, this, &ProgressDialog::onMessage);
    connect(archiver, &Archiver::workingArchive, this, &ProgressDialog::onWorkingArchive);
}

void ProgressDialog::setOperation(const QString& operation) {
    ui_->operation->setText(operation);
}

void ProgressDialog::setMessage(const QString& msg) {
    ui_->message->setText(msg);
}

void ProgressDialog::reject() {
    QDialog::reject();
    if(archiver_) {
        archiver_->stopCurrentAction();
    }
}

void ProgressDialog::onProgress(double fraction) {
    //qDebug("progress: %lf", fraction);
    if(fraction < 0.0) {
        // negative progress indicates that progress is unknown
        ui_->progressBar->setRange(0, 0); // set it to undertermined state
    }
    else {
        ui_->progressBar->setRange(0, 100);
        ui_->progressBar->setValue(int(100 * fraction));
    }
}

void ProgressDialog::onFinished(FrAction action, ArchiverError error) {
}

void ProgressDialog::onMessage(QString msg) {
    //qDebug("progress: %s", msg.toUtf8().constData());
    ui_->message->setText(msg);
}

void ProgressDialog::onStoppableChanged(bool value) {

}

void ProgressDialog::onWorkingArchive(QString filename) {
    //qDebug("progress: %s", filename.toUtf8().constData());
    ui_->currentFile->setText(filename);
}

