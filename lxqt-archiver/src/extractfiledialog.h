#ifndef EXTRACTFILEDIALOG_H
#define EXTRACTFILEDIALOG_H

#include <libfm-qt/filedialog.h>
#include <memory>

namespace Ui {
class ExtractArchiveExtraWidget;
}


class ExtractFileDialog : public Fm::FileDialog {
public:
    explicit ExtractFileDialog(QWidget *parent = 0, Fm::FilePath path = Fm::FilePath::homeDir());

    ~ExtractFileDialog();

    bool skipOlder() const;

    bool overwrite() const;

    bool reCreateFolders() const;

    bool extractAll() const;

    void setExtractAll(bool value);

    void setExtractSelected(bool value);

    void setExtractAllEnabled(bool enabled);

    void setExtractSelectedEnabled(bool enabled);

private:
    std::unique_ptr<Ui::ExtractArchiveExtraWidget> ui_;
};

#endif // EXTRACTFILEDIALOG_H
