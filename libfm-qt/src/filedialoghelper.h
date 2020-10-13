#ifndef FILEDIALOGHELPER_H
#define FILEDIALOGHELPER_H

#include "libfmqtglobals.h"
#include <qpa/qplatformdialoghelper.h> // this private header is subject to changes
#include <memory>

namespace Fm {

class FileDialog;

class LIBFM_QT_API FileDialogHelper : public QPlatformFileDialogHelper {
    Q_OBJECT

public:
    FileDialogHelper();

    ~FileDialogHelper() override;

    // QPlatformDialogHelper
    void exec() override;
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;

    // QPlatformFileDialogHelper
    bool defaultNameFilterDisables() const override;
    void setDirectory(const QUrl &directory) override;
    QUrl directory() const override;
    void selectFile(const QUrl &filename) override;
    QList<QUrl> selectedFiles() const override;
    void setFilter() override;
    void selectNameFilter(const QString &filter) override;
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    QString selectedMimeTypeFilter() const override;
    void selectMimeTypeFilter(const QString &filter) override;
#endif
    QString selectedNameFilter() const override;

    bool isSupportedUrl(const QUrl &url) const override;

private:
    void applyOptions();
    void loadSettings();
    void saveSettings();

private:
    std::unique_ptr<Fm::FileDialog> dlg_;
};

} // namespace Fm

// export a C API without C++ name mangling so others can dynamically load libfm-qt at runtime
// to call this API and get a new QPlatformFileDialogHelper object.

extern "C" {

// if the process calling this API fail to load libfm-qt, nullptr will be returned instead.
LIBFM_QT_API QPlatformFileDialogHelper* createFileDialogHelper();

}

#endif // FILEDIALOGHELPER_H
