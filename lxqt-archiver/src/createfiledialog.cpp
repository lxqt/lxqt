#include "createfiledialog.h"
#include "archiver.h"
#include "ui_create.h"

#include <QBoxLayout>


CreateFileDialog::CreateFileDialog(QWidget *parent, Fm::FilePath path):
    Fm::FileDialog{parent, std::move(path)},
    ui_{new Ui::CreateArchiveExtraWidget{}} {

    setAcceptMode(QFileDialog::AcceptSave);
    setNameFilters(Archiver::supportedCreateNameFilters() << tr("All files (*)"));

    // extra options
    auto extraWidget = new QWidget{this};
    ui_->setupUi(extraWidget);
    auto boxLayout = qobject_cast<QBoxLayout*>(layout());
    if(boxLayout) {
        boxLayout->addWidget(extraWidget);
    }

    // default to 10 MB
    ui_->volumeSize->setValue(10);
}

CreateFileDialog::~CreateFileDialog() {
}

QString CreateFileDialog::password() const {
    return ui_->password->text();
}

bool CreateFileDialog::encryptFileList() const {
    return ui_->encryptFileList->isChecked();
}

bool CreateFileDialog::splitVolumes() const {
    return ui_->splitVolumes->isChecked();
}

unsigned int CreateFileDialog::volumeSize() const {
    return ui_->volumeSize->value();
}
