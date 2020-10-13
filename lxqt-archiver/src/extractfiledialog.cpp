#include "extractfiledialog.h"
#include "ui_extract.h"

#include <QBoxLayout>


ExtractFileDialog::ExtractFileDialog(QWidget *parent, Fm::FilePath path):
    Fm::FileDialog{parent, std::move(path)},
    ui_{new Ui::ExtractArchiveExtraWidget{}} {

    setOptions(QFileDialog::ShowDirsOnly | QFileDialog::HideNameFilterDetails);
    setNameFilters(QStringList{} << tr("All files (*)"));
    setAcceptMode(QFileDialog::AcceptOpen);
    setFileMode(QFileDialog::Directory);

    // add extra options
    QWidget* extraWidget = new QWidget(this);
    ui_->setupUi(extraWidget);
    auto boxLayout = qobject_cast<QBoxLayout*>(layout());
    if(boxLayout) {
        boxLayout->addWidget(extraWidget);
    }
}

ExtractFileDialog::~ExtractFileDialog() {
}

bool ExtractFileDialog::skipOlder() const {
    return ui_->skipOlder->isChecked();
}

bool ExtractFileDialog::overwrite() const {
    return ui_->overwriteExisting->isChecked();
}

bool ExtractFileDialog::reCreateFolders() const {
    return ui_->reCreateFolders->isChecked();
}

bool ExtractFileDialog::extractAll() const {
    return ui_->extractAll->isChecked();
}

void ExtractFileDialog::setExtractAll(bool value){
    ui_->extractAll->setChecked(value);
}

void ExtractFileDialog::setExtractSelected(bool value){
    ui_->extractSelected->setChecked(value);
}

void ExtractFileDialog::setExtractAllEnabled(bool enabled)
{
    ui_->extractAll->setEnabled(enabled);
}

void ExtractFileDialog::setExtractSelectedEnabled(bool enabled) {
    ui_->extractSelected->setEnabled(enabled);
}
