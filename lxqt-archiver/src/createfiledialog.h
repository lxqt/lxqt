#ifndef CREATEFILEDIALOG_H
#define CREATEFILEDIALOG_H

#include <libfm-qt/filedialog.h>
#include <memory>

namespace Ui {
class CreateArchiveExtraWidget;
}

class CreateFileDialog : public Fm::FileDialog {
public:
    CreateFileDialog(QWidget *parent = 0, Fm::FilePath path = Fm::FilePath::homeDir());
    ~CreateFileDialog();

    QString password() const;

    bool encryptFileList() const;

    bool splitVolumes() const;

    unsigned int volumeSize() const;

private:
    std::unique_ptr<Ui::CreateArchiveExtraWidget> ui_;
};


#endif // CREATEFILEDIALOG_H
