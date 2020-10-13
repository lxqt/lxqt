#include "filedialoghelper.h"

#include "libfmqt.h"
#include "filedialog.h"

#include <QWindow>
#include <QDebug>
#include <QTimer>
#include <QSettings>
#include <QtGlobal>

#include <memory>

namespace Fm {

inline static const QString viewModeToString(Fm::FolderView::ViewMode value);
inline static Fm::FolderView::ViewMode viewModeFromString(const QString& str);

inline static const QString sortColumnToString(FolderModel::ColumnId value);
inline static FolderModel::ColumnId sortColumnFromString(const QString str);

inline static const QString sortOrderToString(Qt::SortOrder order);
inline static Qt::SortOrder sortOrderFromString(const QString str);

FileDialogHelper::FileDialogHelper() {
    // can only be used after libfm-qt initialization
    dlg_ = std::unique_ptr<Fm::FileDialog>(new Fm::FileDialog());
    connect(dlg_.get(), &Fm::FileDialog::accepted, [this]() {
        saveSettings();
        accept();
    });
    connect(dlg_.get(), &Fm::FileDialog::rejected, [this]() {
        saveSettings();
        reject();
    });

    connect(dlg_.get(), &Fm::FileDialog::fileSelected, this, &FileDialogHelper::fileSelected);
    connect(dlg_.get(), &Fm::FileDialog::filesSelected, this, &FileDialogHelper::filesSelected);
    connect(dlg_.get(), &Fm::FileDialog::currentChanged, this, &FileDialogHelper::currentChanged);
    connect(dlg_.get(), &Fm::FileDialog::directoryEntered, this, &FileDialogHelper::directoryEntered);
    connect(dlg_.get(), &Fm::FileDialog::filterSelected, this, &FileDialogHelper::filterSelected);
}

FileDialogHelper::~FileDialogHelper() {
}

void FileDialogHelper::exec() {
    dlg_->exec();
}

bool FileDialogHelper::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow* parent) {
    dlg_->setAttribute(Qt::WA_NativeWindow, true); // without this, sometimes windowHandle() will return nullptr

    dlg_->setWindowFlags(windowFlags);
    dlg_->setWindowModality(windowModality);

    // Reference: KDE implementation
    // https://github.com/KDE/plasma-integration/blob/master/src/platformtheme/kdeplatformfiledialoghelper.cpp
    dlg_->windowHandle()->setTransientParent(parent);

    applyOptions();

    loadSettings();
    // central positioning with respect to the parent window
    if(parent && parent->isVisible()) {
        dlg_->move(parent->x() + (parent->width() - dlg_->width()) / 2,
                   parent->y() + (parent->height() - dlg_->height()) / 2);
    }

    // NOTE: the timer here is required as a workaround borrowed from KDE. Without this, the dialog UI will be blocked.
    // QFileDialog calls our platform plugin to show our own native file dialog instead of showing its widget.
    // However, it still creates a hidden dialog internally, and then make it modal.
    // So user input from all other windows that are not the children of the QFileDialog widget will be blocked.
    // This includes our own dialog. After the return of this show() method, QFileDialog creates its own window and
    // then make it modal, which blocks our UI. The timer schedule a delayed popup of our file dialog, so we can
    // show again after QFileDialog and override the modal state. Then our UI can be unblocked.
    QTimer::singleShot(0, dlg_.get(), &QDialog::show);
    dlg_->setFocus();
    return true;
}

void FileDialogHelper::hide() {
    dlg_->hide();
}

bool FileDialogHelper::defaultNameFilterDisables() const {
    return false;
}

void FileDialogHelper::setDirectory(const QUrl& directory) {
    dlg_->setDirectory(directory);
}

QUrl FileDialogHelper::directory() const {
    return dlg_->directory();
}

void FileDialogHelper::selectFile(const QUrl& filename) {
    dlg_->selectFile(filename);
}

QList<QUrl> FileDialogHelper::selectedFiles() const {
    return dlg_->selectedFiles();
}

void FileDialogHelper::setFilter() {
    // FIXME: what's this?
    // The gtk+ 3 file dialog helper in Qt5 update options in this method.
    applyOptions();
}

void FileDialogHelper::selectNameFilter(const QString& filter) {
    dlg_->selectNameFilter(filter);
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
QString FileDialogHelper::selectedMimeTypeFilter() const {
    return dlg_->selectedMimeTypeFilter();
}

void FileDialogHelper::selectMimeTypeFilter(const QString& filter) {
    dlg_->selectMimeTypeFilter(filter);
}
#endif

QString FileDialogHelper::selectedNameFilter() const {
    return dlg_->selectedNameFilter();
}

bool FileDialogHelper::isSupportedUrl(const QUrl& url) const {
    return dlg_->isSupportedUrl(url);
}

void FileDialogHelper::applyOptions() {
    auto& opt = options();

    // set title
    if(opt->windowTitle().isEmpty()) {
        dlg_->setWindowTitle(opt->acceptMode() == QFileDialogOptions::AcceptOpen ? tr("Open File")
                                                                                 : tr("Save File"));
    }
    else {
        dlg_->setWindowTitle(opt->windowTitle());
    }

    dlg_->setFilter(opt->filter());
    dlg_->setFileMode(QFileDialog::FileMode(opt->fileMode()));
    dlg_->setAcceptMode(QFileDialog::AcceptMode(opt->acceptMode())); // also sets a default label for accept button
    // bool useDefaultNameFilters() const;
    dlg_->setNameFilters(opt->nameFilters());
    if(!opt->mimeTypeFilters().empty()) {
        dlg_->setMimeTypeFilters(opt->mimeTypeFilters());
    }

    dlg_->setDefaultSuffix(opt->defaultSuffix());
    // QStringList history() const;

    // explicitly set labels
    for(int i = 0; i < QFileDialogOptions::DialogLabelCount; ++i) {
        auto label = static_cast<QFileDialogOptions::DialogLabel>(i);
        if(opt->isLabelExplicitlySet(label)) {
            dlg_->setLabelText(static_cast<QFileDialog::DialogLabel>(label), opt->labelText(label));
        }
    }

    auto url = opt->initialDirectory();
    if(url.isValid()) {
        dlg_->setDirectory(url);
    }


#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    auto filter = opt->initiallySelectedMimeTypeFilter();
    if(!filter.isEmpty()) {
        selectMimeTypeFilter(filter);
    }
    else {
        filter = opt->initiallySelectedNameFilter();
        if(!filter.isEmpty()) {
            selectNameFilter(opt->initiallySelectedNameFilter());
        }
    }
#else
    auto filter = opt->initiallySelectedNameFilter();
    if(!filter.isEmpty()) {
        selectNameFilter(filter);
    }
#endif

    auto selectedFiles = opt->initiallySelectedFiles();
    for(const auto& selectedFile: selectedFiles) {
        selectFile(selectedFile);
    }
    // QStringList supportedSchemes() const;
}

static const QString viewModeToString(Fm::FolderView::ViewMode value) {
    QString ret;
    switch(value) {
    case Fm::FolderView::DetailedListMode:
    default:
        ret = QLatin1String("Detailed");
        break;
    case Fm::FolderView::CompactMode:
        ret = QLatin1String("Compact");
        break;
    case Fm::FolderView::IconMode:
        ret = QLatin1String("Icon");
        break;
    case Fm::FolderView::ThumbnailMode:
        ret = QLatin1String("Thumbnail");
        break;
    }
    return ret;
}

Fm::FolderView::ViewMode viewModeFromString(const QString& str) {
    Fm::FolderView::ViewMode ret;
    if(str == QLatin1String("Detailed")) {
        ret = Fm::FolderView::DetailedListMode;
    }
    else if(str == QLatin1String("Compact")) {
        ret = Fm::FolderView::CompactMode;
    }
    else if(str == QLatin1String("Icon")) {
        ret = Fm::FolderView::IconMode;
    }
    else if(str == QLatin1String("Thumbnail")) {
        ret = Fm::FolderView::ThumbnailMode;
    }
    else {
        ret = Fm::FolderView::DetailedListMode;
    }
    return ret;
}

static Fm::FolderModel::ColumnId sortColumnFromString(const QString str) {
    Fm::FolderModel::ColumnId ret;
    if(str == QLatin1String("name")) {
        ret = Fm::FolderModel::ColumnFileName;
    }
    else if(str == QLatin1String("type")) {
        ret = Fm::FolderModel::ColumnFileType;
    }
    else if(str == QLatin1String("size")) {
        ret = Fm::FolderModel::ColumnFileSize;
    }
    else if(str == QLatin1String("mtime")) {
        ret = Fm::FolderModel::ColumnFileMTime;
    }
    else if(str == QLatin1String("dtime")) {
        ret = Fm::FolderModel::ColumnFileDTime;
    }
    else if(str == QLatin1String("owner")) {
        ret = Fm::FolderModel::ColumnFileOwner;
    }
    else if(str == QLatin1String("group")) {
        ret = Fm::FolderModel::ColumnFileGroup;
    }
    else {
        ret = Fm::FolderModel::ColumnFileName;
    }
    return ret;
}

static const QString sortColumnToString(Fm::FolderModel::ColumnId value) {
    QString ret;
    switch(value) {
    case Fm::FolderModel::ColumnFileName:
    default:
        ret = QLatin1String("name");
        break;
    case Fm::FolderModel::ColumnFileType:
        ret = QLatin1String("type");
        break;
    case Fm::FolderModel::ColumnFileSize:
        ret = QLatin1String("size");
        break;
    case Fm::FolderModel::ColumnFileMTime:
        ret = QLatin1String("mtime");
        break;
    case Fm::FolderModel::ColumnFileDTime:
        ret = QLatin1String("dtime");
        break;
    case Fm::FolderModel::ColumnFileOwner:
        ret = QLatin1String("owner");
        break;
    case Fm::FolderModel::ColumnFileGroup:
        ret = QLatin1String("group");
        break;
    }
    return ret;
}

static const QString sortOrderToString(Qt::SortOrder order) {
    return (order == Qt::DescendingOrder ? QLatin1String("descending") : QLatin1String("ascending"));
}

static Qt::SortOrder sortOrderFromString(const QString str) {
    return (str == QLatin1String("descending") ? Qt::DescendingOrder : Qt::AscendingOrder);
}

void FileDialogHelper::loadSettings() {
    QSettings settings(QSettings::UserScope, QStringLiteral("lxqt"), QStringLiteral("filedialog"));
    settings.beginGroup (QStringLiteral("Sizes"));
    dlg_->resize(settings.value(QStringLiteral("WindowSize"), QSize(700, 500)).toSize());
    dlg_->setSplitterPos(settings.value(QStringLiteral("SplitterPos"), 200).toInt());
    settings.endGroup();

   settings.beginGroup (QStringLiteral("View"));
   dlg_->setViewMode(viewModeFromString(settings.value(QStringLiteral("Mode"), QStringLiteral("Detailed")).toString()));
   dlg_->sort(sortColumnFromString(settings.value(QStringLiteral("SortColumn")).toString()), sortOrderFromString(settings.value(QStringLiteral("SortOrder")).toString()));
   dlg_->setSortFolderFirst(settings.value(QStringLiteral("SortFolderFirst"), true).toBool());
   dlg_->setSortHiddenLast(settings.value(QStringLiteral("SortHiddenLast"), false).toBool());
   dlg_->setSortCaseSensitive(settings.value(QStringLiteral("SortCaseSensitive"), false).toBool());
   dlg_->setShowHidden(settings.value(QStringLiteral("ShowHidden"), false).toBool());

   dlg_->setShowThumbnails(settings.value(QStringLiteral("ShowThumbnails"), true).toBool());
   dlg_->setNoItemTooltip(settings.value(QStringLiteral("NoItemTooltip"), false).toBool());

   dlg_->setBigIconSize(settings.value(QStringLiteral("BigIconSize"), 48).toInt());
   dlg_->setSmallIconSize(settings.value(QStringLiteral("SmallIconSize"), 24).toInt());
   dlg_->setThumbnailIconSize(settings.value(QStringLiteral("ThumbnailIconSize"), 128).toInt());
   settings.endGroup();

   settings.beginGroup(QStringLiteral("Places"));
   dlg_->setHiddenPlaces(settings.value(QStringLiteral("HiddenPlaces")).toStringList().toSet());
   settings.endGroup();
}

// This also prevents redundant writings whenever a file dialog is closed without a change in its settings.
void FileDialogHelper::saveSettings() {
    QSettings settings(QSettings::UserScope, QStringLiteral("lxqt"), QStringLiteral("filedialog"));
    settings.beginGroup (QStringLiteral("Sizes"));
    QSize windowSize = dlg_->size();
    if(settings.value(QStringLiteral("WindowSize")) != windowSize) { // no redundant write
        settings.setValue(QStringLiteral("WindowSize"), windowSize);
    }
    int splitterPos = dlg_->splitterPos();
    if(settings.value(QStringLiteral("SplitterPos")) != splitterPos) {
        settings.setValue(QStringLiteral("SplitterPos"), splitterPos);
    }
    settings.endGroup();

    settings.beginGroup (QStringLiteral("View"));
    QString mode = viewModeToString(dlg_->viewMode());
    if(settings.value(QStringLiteral("Mode")) != mode) {
        settings.setValue(QStringLiteral("Mode"), mode);
    }
    QString sortColumn = sortColumnToString(static_cast<Fm::FolderModel::ColumnId>(dlg_->sortColumn()));
    if(settings.value(QStringLiteral("SortColumn")) != sortColumn) {
        settings.setValue(QStringLiteral("SortColumn"), sortColumn);
    }
    QString sortOrder = sortOrderToString(dlg_->sortOrder());
    if(settings.value(QStringLiteral("SortOrder")) != sortOrder) {
        settings.setValue(QStringLiteral("SortOrder"), sortOrder);
    }
    bool sortFolderFirst = dlg_->sortFolderFirst();
    if(settings.value(QStringLiteral("SortFolderFirst")).toBool() != sortFolderFirst) {
        settings.setValue(QStringLiteral("SortFolderFirst"), sortFolderFirst);
    }
    bool sortHiddenLast = dlg_->sortHiddenLast();
    if(settings.value(QStringLiteral("SortHiddenLast")).toBool() != sortHiddenLast) {
        settings.setValue(QStringLiteral("SortHiddenLast"), sortHiddenLast);
    }
    bool sortCaseSensitive = dlg_->sortCaseSensitive();
    if(settings.value(QStringLiteral("SortCaseSensitive")).toBool() != sortCaseSensitive) {
        settings.setValue(QStringLiteral("SortCaseSensitive"), sortCaseSensitive);
    }
    bool showHidden = dlg_->showHidden();
    if(settings.value(QStringLiteral("ShowHidden")).toBool() != showHidden) {
        settings.setValue(QStringLiteral("ShowHidden"), showHidden);
    }

    bool showThumbnails = dlg_->showThumbnails();
    if(settings.value(QStringLiteral("ShowThumbnails")).toBool() != showThumbnails) {
        settings.setValue(QStringLiteral("ShowThumbnails"), showThumbnails);
    }
    bool noItemTooltip = dlg_->noItemTooltip();
    if(settings.value(QStringLiteral("NoItemTooltip")).toBool() != noItemTooltip) {
        settings.setValue(QStringLiteral("NoItemTooltip"), noItemTooltip);
    }

    int size = dlg_->bigIconSize();
    if(settings.value(QStringLiteral("BigIconSize")).toInt() != size) {
        settings.setValue(QStringLiteral("BigIconSize"), size);
    }
    size = dlg_->smallIconSize();
    if(settings.value(QStringLiteral("SmallIconSize")).toInt() != size) {
        settings.setValue(QStringLiteral("SmallIconSize"), size);
    }
    size = dlg_->thumbnailIconSize();
    if(settings.value(QStringLiteral("ThumbnailIconSize")).toInt() != size) {
        settings.setValue(QStringLiteral("ThumbnailIconSize"), size);
    }
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Places"));
    QSet<QString> hiddenPlaces = dlg_->getHiddenPlaces();
    if(hiddenPlaces.isEmpty()) { // don't save "@Invalid()"
        settings.remove(QStringLiteral("HiddenPlaces"));
    }
    else if(hiddenPlaces != settings.value(QStringLiteral("HiddenPlaces")).toStringList().toSet()) {
        QStringList sl = hiddenPlaces.toList();
        settings.setValue(QStringLiteral("HiddenPlaces"), sl);
    }
    settings.endGroup();
}

/*
FileDialogPlugin::FileDialogPlugin() {

}

QPlatformFileDialogHelper *FileDialogPlugin::createHelper() {
    return new FileDialogHelper();
}
*/

} // namespace Fm


QPlatformFileDialogHelper *createFileDialogHelper() {
    // When a process has this environment set, that means glib event loop integration is disabled.
    // In this case, libfm just won't work. So let's disable the file dialog helper and return nullptr.
    if(qgetenv("QT_NO_GLIB") == "1") {
        return nullptr;
    }

    static std::unique_ptr<Fm::LibFmQt> libfmQtContext_;
    if(!libfmQtContext_) {
        // initialize libfm-qt only once
        libfmQtContext_ = std::unique_ptr<Fm::LibFmQt>{new Fm::LibFmQt()};
    }
    return new Fm::FileDialogHelper{};
}
