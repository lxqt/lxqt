#include "filedialog.h"
#include "cachedfoldermodel.h"
#include "proxyfoldermodel.h"
#include "utilities.h"
#include "core/fileinfojob.h"
#include "ui_filedialog.h"
#include "filedialog_p.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QMimeType>
#include <QMimeDatabase>
#include <QMessageBox>
#include <QToolBar>
#include <QCompleter>
#include <QShortcut>
#include <QTimer>
#include <QDebug>

namespace Fm {


FileDialog::FileDialog(QWidget* parent, FilePath path) :
    QDialog(parent),
    ui{new Ui::FileDialog()},
    folderModel_{nullptr},
    proxyModel_{nullptr},
    folder_{nullptr},
    options_{QFileDialog::Options()},
    viewMode_{FolderView::DetailedListMode},
    fileMode_{QFileDialog::AnyFile},
    acceptMode_{QFileDialog::AcceptOpen},
    confirmOverwrite_{true},
    modelFilter_{this},
    noItemTooltip_{false} {

    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui->buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    // path bar
    connect(ui->location, &PathBar::chdir, [this](const FilePath &path) {
        setDirectoryPath(path);
    });
    // side pane
    ui->sidePane->setMode(Fm::SidePane::ModePlaces);
    connect(ui->sidePane, &SidePane::chdirRequested, [this](int /*type*/, const FilePath &path) {
        setDirectoryPath(path);
    });
    connect(ui->sidePane, &SidePane::hiddenPlaceSet, this, &FileDialog::onSettingHiddenPlace);

    // folder view
    proxyModel_ = new ProxyFolderModel(this);
    proxyModel_->sort(FolderModel::ColumnFileName, Qt::AscendingOrder);
    proxyModel_->setThumbnailSize(64);
    proxyModel_->setShowThumbnails(true);

    proxyModel_->addFilter(&modelFilter_);

    connect(ui->folderView, &FolderView::clicked, this, &FileDialog::onFileClicked);
    ui->folderView->setModel(proxyModel_);
    ui->folderView->setAutoSelectionDelay(0);
    // set the completer
    QCompleter* completer = new QCompleter(this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setModel(proxyModel_);
    ui->fileName->setCompleter(completer);
    connect(completer, static_cast<void(QCompleter::*)(const QString &)>(&QCompleter::activated), [this](const QString &text) {
        ui->folderView->selectionModel()->clearSelection();
        selectFilePath(directoryPath_.child(text.toLocal8Bit().constData()));
    });
    // select typed paths if it they exist
    connect(ui->fileName, &QLineEdit::textEdited, [this](const QString& /*text*/) {
        disconnect(ui->folderView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FileDialog::onSelectionChanged);
        ui->folderView->selectionModel()->clearSelection();
        QStringList parsedNames = parseNames();
        for(auto& name: parsedNames) {
            if(!defaultSuffix_.isEmpty() && name.indexOf(QLatin1Char('.'), 1) == -1) { // including ".X"
                name += QLatin1Char('.');
                name += defaultSuffix_;
            }
            selectFilePath(directoryPath_.child(name.toLocal8Bit().constData()));
        }
        updateAcceptButtonState();
        updateSaveButtonText(false);
        connect(ui->folderView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FileDialog::onSelectionChanged);
    });
    // update selection mode for the view
    updateSelectionMode();

    // file type
    connect(ui->fileTypeCombo, &QComboBox::currentTextChanged, [this](const QString& text) {
        selectNameFilter(text);
    });
    ui->fileTypeCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    ui->fileTypeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->fileTypeCombo->setCurrentIndex(0);

    QShortcut* shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_H), this);
    connect(shortcut, &QShortcut::activated, [this]() {
        proxyModel_->setShowHidden(!proxyModel_->showHidden());
    });

    // setup toolbar buttons
    auto toolbar = new QToolBar(this);
    // back button
    backAction_ = toolbar->addAction(QIcon::fromTheme(QStringLiteral("go-previous")), tr("Go Back"));
    backAction_->setShortcut(QKeySequence(tr("Alt+Left", "Go Back")));
    connect(backAction_, &QAction::triggered, [this]() {
        history_.backward();
        setDirectoryPath(history_.currentPath(), FilePath(), false);
    });
    // forward button
    forwardAction_ = toolbar->addAction(QIcon::fromTheme(QStringLiteral("go-next")), tr("Go Forward"));
    forwardAction_->setShortcut(QKeySequence(tr("Alt+Right", "Go Forward")));
    connect(forwardAction_, &QAction::triggered, [this]() {
        history_.forward();
        setDirectoryPath(history_.currentPath(), FilePath(), false);
    });
    toolbar->addSeparator();
    // reload button
    auto reloadAction = toolbar->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), tr("Reload"));
    reloadAction->setShortcut(QKeySequence(tr("F5", "Reload")));
    connect(reloadAction, &QAction::triggered, [this]() {
        if(folder_ && folder_->isLoaded()) {
            QObject::disconnect(lambdaConnection_);
            auto selFiles = ui->folderView->selectedFiles();
            ui->folderView->selectionModel()->clear();
            // reselect files on reloading
            if(!selFiles.empty()
               && selFiles.size() <= 50) { // otherwise senseless and CPU-intensive
                lambdaConnection_ = QObject::connect(folder_.get(), &Fm::Folder::finishLoading, this, [this, selFiles]() {
                    selectFilesOnReload(selFiles);
                });
            }
            folder_->reload();
        }
    });
    // new folder button
    auto newFolderAction = toolbar->addAction(QIcon::fromTheme(QStringLiteral("folder-new")), tr("Create Folder"));
    connect(newFolderAction, &QAction::triggered, this, &FileDialog::onNewFolder);
    toolbar->addSeparator();

    // Options menu
    QMenu* menu = new QMenu(toolbar);

    // view actions
    auto viewModeGroup = new QActionGroup(this);
    // use generic icons for view actions only if theme icons don't exist
    iconViewAction_ = menu->addAction(QIcon::fromTheme(QLatin1String("view-list-icons"), style()->standardIcon(QStyle::SP_FileDialogContentsView)), tr("Icon View"));
    iconViewAction_->setCheckable(true);
    connect(iconViewAction_, &QAction::toggled, this, &FileDialog::onViewModeToggled);
    viewModeGroup->addAction(iconViewAction_);
    thumbnailViewAction_ = menu->addAction(QIcon::fromTheme(QLatin1String("view-preview"), style()->standardIcon(QStyle::SP_FileDialogInfoView)), tr("Thumbnail View"));
    thumbnailViewAction_->setCheckable(true);
    connect(thumbnailViewAction_, &QAction::toggled, this, &FileDialog::onViewModeToggled);
    viewModeGroup->addAction(thumbnailViewAction_);
    compactViewAction_ = menu->addAction(QIcon::fromTheme(QLatin1String("view-list-text"), style()->standardIcon(QStyle::SP_FileDialogListView)), tr("Compact View"));
    compactViewAction_->setCheckable(true);
    connect(compactViewAction_, &QAction::toggled, this, &FileDialog::onViewModeToggled);
    viewModeGroup->addAction(compactViewAction_);
    detailedViewAction_ = menu->addAction(QIcon::fromTheme(QLatin1String("view-list-details"), style()->standardIcon(QStyle::SP_FileDialogDetailedView)), tr("Detailed List View"));
    detailedViewAction_->setCheckable(true);
    connect(detailedViewAction_, &QAction::toggled, this, &FileDialog::onViewModeToggled);
    viewModeGroup->addAction(detailedViewAction_);

    menu->addSeparator();

    // icon size actions
    QMenu* sizeMenu = new QMenu(tr("Icon Sizes"), toolbar);
    menu->addMenu(sizeMenu);
    FileDialogIconSizeAction* bigSizeAction = new FileDialogIconSizeAction(tr("Size of big icons:"), toolbar);
    bigSizeAction->setBounds(32, 96);
    connect(bigSizeAction, &FileDialogIconSizeAction::editingFinished, [this, bigSizeAction]() {
        setBigIconSize(bigSizeAction->value());
    });
    sizeMenu->addAction(bigSizeAction);

    FileDialogIconSizeAction* smallSizeAction = new FileDialogIconSizeAction(tr("Size of small icons:"), toolbar);
    smallSizeAction->setBounds(16, 48);
    connect(smallSizeAction, &FileDialogIconSizeAction::editingFinished, [this, smallSizeAction]() {
        setSmallIconSize(smallSizeAction->value());
    });
    sizeMenu->addAction(smallSizeAction);

    FileDialogIconSizeAction* thumbnailSizeAction = new FileDialogIconSizeAction(tr("Size of thumbnails:"), toolbar);
    thumbnailSizeAction->setBounds(64, 256);
    connect(thumbnailSizeAction, &FileDialogIconSizeAction::editingFinished, [this, thumbnailSizeAction]() {
        setThumbnailIconSize(thumbnailSizeAction->value());
    });
    sizeMenu->addAction(thumbnailSizeAction);

    connect(sizeMenu, &QMenu::aboutToShow, [this, bigSizeAction, smallSizeAction, thumbnailSizeAction]() {
        bigSizeAction->setValue(bigIconSize());
        smallSizeAction->setValue(smallIconSize());
        thumbnailSizeAction->setValue(thumbnailIconSize());
    });

    menu->addSeparator();

    // thumbnail and tooltip actions
    auto thumbnailsAction = menu->addAction(tr("Show Thumbnails"));
    thumbnailsAction->setCheckable(true);
    connect(thumbnailsAction, &QAction::triggered, [this](bool checked) {
        setShowThumbnails(checked);
    });
    auto tooltipsAction = menu->addAction(tr("Show File Tooltips"));
    tooltipsAction->setCheckable(true);
    connect(tooltipsAction, &QAction::triggered, [this](bool checked) {
        setNoItemTooltip(!checked);
    });
    connect(menu, &QMenu::aboutToShow, [this, thumbnailsAction, tooltipsAction]() {
        thumbnailsAction->setChecked(proxyModel_ ? proxyModel_->showThumbnails() : true);
        tooltipsAction->setChecked(!noItemTooltip_);
    });

    // Options menu button
    auto optionsAction = toolbar->addAction(QIcon::fromTheme(QStringLiteral("preferences-system"), QIcon::fromTheme(QStringLiteral("document-properties"))), tr("Options"));
    optionsAction->setMenu(menu);
    // change the popup mode to instant (Qt sets it to MenuButtonPopup)
    if(QToolButton* optionsBtn = static_cast<QToolButton*>(toolbar->widgetForAction(optionsAction))) {
        optionsBtn->setPopupMode(QToolButton::InstantPopup);
    }

    ui->toolbarLayout->addWidget(toolbar);

    setViewMode(viewMode_);

    // set the default splitter position
    setSplitterPos(200);

    // browse to the directory
    if(path.isValid()) {
        setDirectoryPath(path);
    }
    else {
        goHome();
    }

    ui->fileName->installEventFilter(this);

    // focus the text entry on showing the dialog
    QTimer::singleShot(0, ui->fileName, SLOT(setFocus()));
}

FileDialog::~FileDialog() {
    freeFolder();
}

bool FileDialog::eventFilter(QObject* watched, QEvent* event) {
    if (watched == ui->folderView->childView()->viewport() && event->type() == QEvent::ToolTip) {
        return true;
    }
    // Here we make Tab key switch the focus from the main view to the name entry
    // and BackTab do the reverse because QWidget::setTabOrder() cannot do that.
    if(event->type() == QEvent::KeyPress) {
        if(QKeyEvent *ke = static_cast<QKeyEvent*>(event)) {
            if(watched == ui->folderView->childView() && ui->folderView->childView()->hasFocus()
               && ke->key() == Qt::Key_Tab && ke->modifiers() == Qt::NoModifier) {
                ui->fileName->setFocus();
                // as in Qt -> QLineEdit::focusInEvent()
                if(!ui->fileName->hasSelectedText()) {
                    ui->fileName->selectAll();
                }
                return true;
            }
            if(watched == ui->fileName && ui->fileName->hasFocus()
               && ke->key() == Qt::Key_Backtab) {
                ui->folderView->childView()->setFocus();
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

int FileDialog::splitterPos() const {
    return ui->splitter->sizes().at(0);
}

void FileDialog::setSplitterPos(int pos) {
    QList<int> sizes;
    sizes.append(qMax(pos, 0));
    sizes.append(320);
    ui->splitter->setSizes(sizes);
}

int FileDialog::sortColumn() const {
    if(proxyModel_) {
        return proxyModel_->sortColumn();
    }
    return 0;
}

Qt::SortOrder FileDialog::sortOrder() const {
    if(proxyModel_) {
        return proxyModel_->sortOrder();
    }
    return Qt::AscendingOrder;
}

void FileDialog::sort(int col, Qt::SortOrder order) {
    if(proxyModel_) {
        proxyModel_->sort(col, order);
    }
}

bool FileDialog::sortFolderFirst() const {
    if(proxyModel_) {
        return proxyModel_->folderFirst();
    }
    return true;
}

void FileDialog::setSortFolderFirst(bool value) {
    if(proxyModel_) {
        proxyModel_->setFolderFirst(value);
    }
}

bool FileDialog::sortHiddenLast() const {
    if(proxyModel_) {
        return proxyModel_->hiddenLast();
    }
    return false;
}

void FileDialog::setSortHiddenLast(bool value) {
    if(proxyModel_) {
        proxyModel_->setHiddenLast(value);
    }
}

bool FileDialog::sortCaseSensitive() const {
    if(proxyModel_) {
        return proxyModel_->sortCaseSensitivity() == Qt::CaseSensitive ? true : false;
    }
    return false;
}

void FileDialog::setSortCaseSensitive(bool value) {
    if(proxyModel_) {
        proxyModel_->setSortCaseSensitivity(value ? Qt::CaseSensitive : Qt::CaseInsensitive);
    }
}

bool FileDialog::showHidden() const {
    if(proxyModel_) {
        return proxyModel_->showHidden();
    }
    return false;
}

void FileDialog::setShowHidden(bool showHidden) {
    if(proxyModel_) {
        proxyModel_->setShowHidden(showHidden);
    }
}

void FileDialog::setHiddenPlaces(const QSet<QString>& items) {
    ui->sidePane->restoreHiddenPlaces(items);
    hiddenPlaces_.clear();
    hiddenPlaces_ = items;
}

void FileDialog::onSettingHiddenPlace(const QString& str, bool hide) {
    if(hide) {
        hiddenPlaces_ << str;
    }
    else {
        hiddenPlaces_.remove(str);
    }
}

bool FileDialog::showThumbnails() const {
    if(proxyModel_) {
        return proxyModel_->showThumbnails();
    }
    return true;
}

void FileDialog::setShowThumbnails(bool show) {
    if(proxyModel_) {
        proxyModel_->setShowThumbnails(show);
    }
}

void FileDialog::setNoItemTooltip(bool noItemTooltip) {
    if(noItemTooltip_ == noItemTooltip) {
        return;
    }
    noItemTooltip_ = noItemTooltip;
    if(noItemTooltip_) {
        ui->folderView->childView()->viewport()->installEventFilter(this);
    }
    else {
        ui->folderView->childView()->viewport()->removeEventFilter(this);
    }
}

int FileDialog::bigIconSize() const {
    return ui->folderView->iconSize(FolderView::IconMode).width();
}

void FileDialog::setBigIconSize(int size) {
    if(bigIconSize() != size) {
        ui->folderView->setIconSize(FolderView::IconMode, QSize(size, size));
    }
}

int FileDialog::smallIconSize() const {
    return ui->folderView->iconSize(FolderView::DetailedListMode).width();
}

void FileDialog::setSmallIconSize(int size) {
    if(smallIconSize() != size) {
        ui->folderView->setIconSize(FolderView::DetailedListMode, QSize(size, size));
        ui->folderView->setIconSize(FolderView::CompactMode, QSize(size, size));
    }
}

int FileDialog::thumbnailIconSize() const {
    return ui->folderView->iconSize(FolderView::ThumbnailMode).width();
}

void FileDialog::setThumbnailIconSize(int size) {
    if(thumbnailIconSize() != size) {
        ui->folderView->setIconSize(FolderView::ThumbnailMode, QSize(size, size));
    }
}

// This should always be used instead of getting text directly from the entry.
QStringList FileDialog::parseNames() const {
    // parse the file names from the text entry
    QStringList parsedNames;
    auto fileNames = ui->fileName->text();
    if(!fileNames.isEmpty()) {
        /* check if there are multiple file names (containing "),
           considering the fact that inside quotes were escaped by \ */
        auto firstQuote = fileNames.indexOf(QLatin1Char('\"'));
        auto lastQuote = fileNames.lastIndexOf(QLatin1Char('\"'));
        if(firstQuote != -1 && lastQuote != -1
           && firstQuote != lastQuote
           && (firstQuote == 0 || fileNames.at(firstQuote - 1) != QLatin1Char('\\'))
           && fileNames.at(lastQuote - 1) != QLatin1Char('\\')) {
           // split the names
#if (QT_VERSION >= QT_VERSION_CHECK(5,12,0))
            QRegularExpression sep{QStringLiteral("\"\\s+\"")};  // separated with " "
#else
            QRegExp sep{QStringLiteral("\"\\s+\"")};  // separated with " "
#endif
            parsedNames = fileNames.mid(firstQuote + 1, lastQuote - firstQuote - 1).split(sep);
            parsedNames.replaceInStrings(QLatin1String("\\\""), QLatin1String("\""));
        }
        else {
            parsedNames << fileNames.replace(QLatin1String("\\\""), QLatin1String("\""));
        }
    }
    return parsedNames;
}

std::shared_ptr<const Fm::FileInfo> FileDialog::firstSelectedDir() const {
    std::shared_ptr<const Fm::FileInfo> selectedFolder = nullptr;
    auto list = ui->folderView->selectedFiles();
    for(auto it = list.cbegin(); it != list.cend(); ++it) {
        auto& item = *it;
        if(item->isDir()) {
            selectedFolder = item;
            break;
        }
    }
    return selectedFolder;
}

QString FileDialog::suffix(bool checkDefaultSuffix) const {
    if(checkDefaultSuffix && !defaultSuffix_.isEmpty()) {
        return defaultSuffix_;
    }
    // in the save mode, still try to make a suffix out of the currrent name filter
    if(acceptMode_ != QFileDialog::AcceptOpen) {
        auto left = currentNameFilter_.lastIndexOf(QLatin1Char('('));
        if(left != -1) {
            ++left;
            auto right = currentNameFilter_.indexOf(QLatin1Char(')'), left);
            if(right != -1) {
                QString nameFilter = currentNameFilter_.mid(left, right - left);
                QString suffix = nameFilter.simplified().split(QLatin1Char(' '), QString::SkipEmptyParts).at(0);
                left = suffix.indexOf(QLatin1Char('.')); // it can be like ".tar.xz"
                if(left != -1 && suffix.size() - left > 1) {
                    return suffix.right(suffix.size() - left - 1);
                }
            }
        }
    }
    return QString();
}

void FileDialog::accept() {
    // handle selected filenames
    selectedFiles_.clear();

    // if a folder is selected in file mode, chdir into it (as QFileDialog does)
    // by giving priority to the current index and, if it isn't a folder,
    // to the first selected folder
    if(fileMode_ != QFileDialog::Directory) {
        std::shared_ptr<const Fm::FileInfo> selectedFolder = nullptr;
        // check if the current index is a folder
        QItemSelectionModel* selModel = ui->folderView->selectionModel();
        QModelIndex cur = selModel->currentIndex();
        if(cur.isValid() && selModel->isSelected(cur)) {
            auto file = proxyModel_->fileInfoFromIndex(cur);
            if(file && file->isDir()) {
                selectedFolder = file;
            }
        }
        if(!selectedFolder) {
            selectedFolder = firstSelectedDir();
        }
        if(selectedFolder) {
            setDirectoryPath(selectedFolder->path());
            return;
        }
    }

    QStringList parsedNames = parseNames();
    if(parsedNames.isEmpty()) {
        // when selecting a dir and the name is not provided, just select current dir in the view
        if(fileMode_ == QFileDialog::Directory) {
            auto localPath = directoryPath_.localPath();
            if(localPath) {
                selectedFiles_.append(QUrl::fromLocalFile(QString::fromUtf8(localPath.get())));
            }
            else {
                selectedFiles_.append(directory());
            }
        }
        else {
            QMessageBox::critical(this, tr("Error"), tr("Please select a file"));
            return;
        }
    }
    else {
        QString _suffix;
        bool suffixFound = false;
        if(fileMode_ != QFileDialog::Directory) {
            // WARNING: proxyModel_->fileInfoFromPath() should not be used here
            // because the item may have been filtered out.
            if(folderModel_ == nullptr) {
                return;
            }
            auto firstName = parsedNames.at(0);
            if(firstName.indexOf(QLatin1Char('.'), 1) == -1) { // including ".X"
                _suffix = suffix();
                suffixFound = true;
            }
            // don't consider the suffix yet because a directory with the same name may exist
            auto childPath = directoryPath_.child(firstName.toLocal8Bit().constData());
            auto info = folderModel_->fileInfoFromPath(childPath);
            if(info && info->isDir()) {
                // if the typed name belongs to a (nonselected) directory, chdir into it
                setDirectoryPath(childPath);
                return;
            }
            if(!_suffix.isEmpty()) {
                firstName += QLatin1Char('.');
                firstName += _suffix;
                childPath = directoryPath_.child(firstName.toLocal8Bit().constData());
                info = folderModel_->fileInfoFromPath(childPath);
            }
            if(info) {
                if(info->isDir()) {
                    setDirectoryPath(childPath);
                    return;
                }
                // overwrite prompt (as in QFileDialog::accept)
                if(fileMode_ == QFileDialog::AnyFile
                   && acceptMode_ != QFileDialog::AcceptOpen
                   && confirmOverwrite_) {
                    if (QMessageBox::warning(this, windowTitle(),
                                            tr("%1 already exists.\nDo you want to replace it?")
                                            .arg(firstName),
                                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                            == QMessageBox::No) {
                        return;
                    }
                }
            }
        }

        // get full paths for the filenames and convert them to URLs
        for(auto& name: parsedNames) {
            // add extension as needed
            if(name.indexOf(QLatin1Char('.'), 1) == -1) {
                if(!suffixFound) {
                    _suffix = suffix();
                    suffixFound = true;
                }
                if(!_suffix.isEmpty()) {
                    name += QLatin1Char('.');
                    name += _suffix;
                }
            }
            auto fullPath = directoryPath_.child(name.toLocal8Bit().constData());
            auto localPath = fullPath.localPath();
            /* add the local path if it exists; otherwise, add the uri */
            if(localPath) {
                selectedFiles_.append(QUrl::fromLocalFile(QString::fromUtf8(localPath.get())));
            }
            else {
                selectedFiles_.append(QUrl::fromEncoded(fullPath.uri().get()));
            }
        }
    }

    // check existence of the selected files and if their types are correct
    // async operation, call doAccept() in the callback.
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(false);

    auto pathList = pathListFromQUrls(selectedFiles_);
    auto job = new FileInfoJob(pathList);
    job->setAutoDelete(true);
    connect(job, &Job::finished, this, &FileDialog::onFileInfoJobFinished);
    job->runAsync();
}

void FileDialog::reject() {
    QDialog::reject();
}

void FileDialog::setDirectory(const QUrl &directory) {
    auto path = Fm::FilePath::fromUri(directory.toEncoded().constData());
    setDirectoryPath(path);
}

// interface for QPlatformFileDialogHelper

void FileDialog::freeFolder() {
    if(folder_) {
        QObject::disconnect(lambdaConnection_); // lambdaConnection_ can be invalid
        disconnect(folder_.get(), nullptr, this, nullptr);
        folder_ = nullptr;
    }
}

void FileDialog::goHome() {
    setDirectoryPath(FilePath::homeDir());
}

void FileDialog::setDirectoryPath(FilePath directory, FilePath selectedPath, bool addHistory) {
    if(!directory.isValid()) {
        updateAcceptButtonState(); // FIXME: is this needed?
        return;
    }

    if(directoryPath_ != directory) {
        if(folder_) {
            if(folderModel_) {
                proxyModel_->setSourceModel(nullptr);
                folderModel_->unref(); // unref the cached model
                folderModel_ = nullptr;
            }
            freeFolder();
        }

        directoryPath_ = std::move(directory);

        ui->location->setPath(directoryPath_);
        ui->sidePane->chdir(directoryPath_);
        if(addHistory) {
            history_.add(directoryPath_);
        }
        backAction_->setEnabled(history_.canBackward());
        forwardAction_->setEnabled(history_.canForward());

        folder_ = Fm::Folder::fromPath(directoryPath_);
        folderModel_ = CachedFolderModel::modelFromFolder(folder_);
        proxyModel_->setSourceModel(folderModel_);

        // no lambda in these connections for easy disconnection
        connect(folder_.get(), &Fm::Folder::removed, this, &FileDialog::goHome);
        connect(folder_.get(), &Fm::Folder::unmount, this, &FileDialog::goHome);

        QUrl uri = QUrl::fromEncoded(directory.uri().get());
        Q_EMIT directoryEntered(uri);
    }

    // select the path if valid
    if(selectedPath.isValid() && folder_) {
        if(folder_->isLoaded()) {
            selectFilePathWithDelay(selectedPath);
        }
        else {
            lambdaConnection_ = QObject::connect(folder_.get(), &Fm::Folder::finishLoading, this, [this, selectedPath]() {
                selectFilePathWithDelay(selectedPath);
            });
        }
    }
    else {
        updateAcceptButtonState();
        updateSaveButtonText(false);
    }
}

void FileDialog::selectFilePath(const FilePath &path) {
    auto idx = proxyModel_->indexFromPath(path);
    if(!idx.isValid()) {
        return;
    }

    // FIXME: add a method to Fm::FolderView to select files

    // FIXME: need to add this for detailed list
    QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::Select;
    if(viewMode_ == FolderView::DetailedListMode) {
        flags |= QItemSelectionModel::Rows;
    }
    QItemSelectionModel* selModel = ui->folderView->selectionModel();
    selModel->select(idx, flags);
    selModel->setCurrentIndex(idx, QItemSelectionModel::Current);
    QTimer::singleShot(0, this, [this, path]() { // idx should no be captured because dir may change
        auto idx = proxyModel_->indexFromPath(path);
        if(idx.isValid()) {
            ui->folderView->childView()->scrollTo(idx, QAbstractItemView::PositionAtCenter);
        }
    });
}

void FileDialog::selectFilePathWithDelay(const FilePath &path) {
    QTimer::singleShot(0, this, [this, path]() {
        if(acceptMode_ == QFileDialog::AcceptSave) {
            // with a save dialog, always put the base name in line-edit, regardless of selection
            ui->fileName->setText(QString::fromUtf8(path.baseName().get()));
        }
        // update "accept" button because there might be no selection later
        updateAcceptButtonState();
        updateSaveButtonText(false);
        // try to select path
        selectFilePath(path);
    });
}

void FileDialog::selectFilesOnReload(const Fm::FileInfoList& infos) {
    QObject::disconnect(lambdaConnection_);
    QTimer::singleShot(0, this, [this, infos]() {
        for(auto& fileInfo: infos) {
            selectFilePath(fileInfo->path());
        }
    });
}

void FileDialog::onCurrentRowChanged(const QModelIndex &current, const QModelIndex& /*previous*/) {
    // emit currentChanged signal
    QUrl currentUrl;
    if(current.isValid()) {
        // emit changed siangl for newly selected items
        auto fi = proxyModel_->fileInfoFromIndex(current);
        if(fi) {
            currentUrl = QUrl::fromEncoded(fi->path().uri().get());
        }
    }
    Q_EMIT currentChanged(currentUrl);
}

void FileDialog::onSelectionChanged(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/) {
    auto selFiles = ui->folderView->selectedFiles();
    if(selFiles.empty()) {
        updateAcceptButtonState();
        updateSaveButtonText(false);
        return;
    }
    bool multiple(selFiles.size() > 1);
    bool hasDir(false);
    QString fileNames;
    for(auto& fileInfo: selFiles) {
        if(fileMode_ == QFileDialog::Directory) {
            // if we want to select dir, ignore selected files
            if(!fileInfo->isDir()) {
                continue;
            }
        }
        else if(fileInfo->isDir()) {
            // if we want to select files, ignore selected dirs
            hasDir = true;
            continue;
        }

        auto baseName = fileInfo->path().baseName();
        if(multiple) {
            // support multiple selection
            if(!fileNames.isEmpty()) {
                fileNames += QLatin1Char(' ');
            }
            fileNames += QLatin1Char('\"');
            // escape inside quotes with \ to distinguish between them
            // and the quotes used for separating file names from each other
            QString name(QString::fromUtf8(baseName.get()));
            fileNames += name.replace(QLatin1String("\""), QLatin1String("\\\""));
            fileNames += QLatin1Char('\"');
        }
        else {
            // support single selection only
            QString name(QString::fromUtf8(baseName.get()));
            fileNames = name.replace(QLatin1String("\""), QLatin1String("\\\""));
            break;
        }
    }
    // put the selection list in the text entry
    if(!fileNames.isEmpty()) {
        ui->fileName->setText(fileNames);
    }
    updateSaveButtonText(hasDir);
    updateAcceptButtonState();
}

void FileDialog::onFileClicked(int type, const std::shared_ptr<const FileInfo> &file) {
    bool canAccept = false;
    if(file && type == FolderView::ActivatedClick) {
        if(file->isDir()) {
            if(fileMode_ == QFileDialog::Directory) {
                ui->fileName->clear();
            }
            // chdir into the activated dir
            if(file->isMountable() && !file->target().empty()) {
                // a mounted mountable directory (like computer:///root.link)
                setDirectoryPath(FilePath::fromPathStr(file->target().c_str()));
            }
            else {
                setDirectoryPath(file->path());
            }
        }
        else if(fileMode_ != QFileDialog::Directory) {
            // select file(s) and a file item is activated
            canAccept = true;
        }
    }

    if(canAccept) {
        selectFilePath(file->path());
        accept();
    }
}

void FileDialog::onNewFolder() {
    createFileOrFolder(CreateNewFolder, directoryPath_, nullptr, this);
}

void FileDialog::onViewModeToggled(bool active) {
    if(active) {
        auto action = static_cast<QAction*>(sender());
        FolderView::ViewMode newMode;
        if(action == iconViewAction_) {
            newMode = FolderView::IconMode;
        }
        else if(action == thumbnailViewAction_) {
            newMode = FolderView::ThumbnailMode;
        }
        else if(action == compactViewAction_) {
            newMode = FolderView::CompactMode;
        }
        else if(action == detailedViewAction_) {
            newMode = FolderView::DetailedListMode;
        }
        else {
            return;
        }
        setViewMode(newMode);
    }
}

void FileDialog::updateSelectionMode() {
    // enable multiple selection?
    ui->folderView->childView()->setSelectionMode(fileMode_ == QFileDialog::ExistingFiles ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);
}

void FileDialog::doAccept() {

    Q_EMIT filesSelected(selectedFiles_);

    if(selectedFiles_.size() == 1) {
        Q_EMIT fileSelected(selectedFiles_[0]);
    }

    QDialog::accept();
}

void FileDialog::onFileInfoJobFinished() {
    auto job = static_cast<FileInfoJob*>(sender());
    if(job->isCancelled()) {
        selectedFiles_.clear();
        reject();
    }
    else {
        QString error;
        // check if the files exist and their types are correct
        auto paths = job->paths();
        auto files = job->files();
        for(size_t i = 0; i < paths.size(); ++i) {
            const auto& path = paths[i];
            if(i >= files.size() || files[i]->path() != path) {
                // the file path is not found and does not have file info
                if(fileMode_ != QFileDialog::AnyFile) {
                    // if we do not allow non-existent file, this is an error.
                    error = tr("Path \"%1\" does not exist").arg(QString::fromUtf8(path.displayName().get()));
                    break;
                }
                ++i; // skip the file
                continue;
            }

            // FIXME: currently, if a path is not found, FmFileInfoJob does not return its file info object.
            // This is bad API design. We may return nullptr for the failed file info query instead.
            const auto& file = files[i];
            // check if the file type is correct
            if(fileMode_ == QFileDialog::Directory) {
                if(!file->isDir()) {
                    // we're selecting dirs, but the selected file path does not point to a dir
                    error = tr("\"%1\" is not a directory").arg(QString::fromUtf8(path.displayName().get()));
                    break;
                }
            }
            else if(file->isDir() || file->isShortcut()) {
                // we're selecting files, but the selected file path refers to a dir or shortcut (such as computer:///)
                error = tr("\"%1\" is not a file").arg(QString::fromUtf8(path.displayName().get()));;
                break;
            }
        }

        if(error.isEmpty()) {
            // no error!
            doAccept();
        }
        else {
            QMessageBox::critical(this, tr("Error"), error);
            selectedFiles_.clear();
        }
    }
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(true);
}

QUrl FileDialog::directory() const {
    QUrl url{QString::fromUtf8(directoryPath_.uri().get())};
    return url;
}

void FileDialog::selectFile(const QUrl& filename) {
    auto urlStr = filename.toEncoded();
    auto path = FilePath::fromUri(urlStr.constData());
    auto parent = path.parent();
    // chdir into file's parent if needed and select the file
    setDirectoryPath(parent, path);
}

QList<QUrl> FileDialog::selectedFiles() {
    return selectedFiles_;
}

void FileDialog::selectNameFilter(const QString& filter) {
    if(filter != currentNameFilter_) {
        currentNameFilter_ = filter;
        ui->fileTypeCombo->setCurrentText(filter);

        // in save mode, change file name extension if it exists
        if(acceptMode_ != QFileDialog::AcceptOpen) {
            auto name = ui->fileName->text();
            if(!name.isEmpty()) {
                auto dot = name.indexOf(QLatin1Char('.'), 1); // not ".X"
                if(dot != -1) {
                    auto _suffix = suffix(false);
                    if(!_suffix.isEmpty()) {
                        name = name.left(dot + 1);
                        name += _suffix;
                        ui->fileName->setText(name);
                    }
                }
            }
        }

        modelFilter_.update();
        proxyModel_->invalidate();
        Q_EMIT filterSelected(filter);
    }
}

void FileDialog::selectMimeTypeFilter(const QString &filter) {
    auto idx = mimeTypeFilters_.indexOf(filter);
    if(idx != -1) {
        ui->fileTypeCombo->setCurrentIndex(idx);
    }
}

QString FileDialog::selectedMimeTypeFilter() const {
    QString filter;
    auto idx = mimeTypeFilters_.indexOf(filter);
    if(idx >= 0 && idx < mimeTypeFilters_.size()) {
        filter = mimeTypeFilters_[idx];
    }
    return filter;
}

bool FileDialog::isSupportedUrl(const QUrl& url) {
    auto scheme = url.scheme().toLocal8Bit();
    // FIXME: this is not reliable due to the bug of gvfs.
    return Fm::isUriSchemeSupported(scheme.constData());
}


// options

void FileDialog::setFilter(QDir::Filters filters) {
    filters_ = filters;
    // TODO:
}

void FileDialog::setViewMode(FolderView::ViewMode mode) {
    viewMode_ = mode;

    // Since setModel() is called by FolderView::setViewMode(), the selectionModel will be replaced by one
    // created by the view. So, we need to deal with selection changes again after setting the view mode.
    disconnect(ui->folderView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &FileDialog::onCurrentRowChanged);
    disconnect(ui->folderView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FileDialog::onSelectionChanged);

    ui->folderView->setViewMode(mode);
    switch(mode) {
    case FolderView::IconMode:
        iconViewAction_->setChecked(true);
        break;
    case FolderView::ThumbnailMode:
        thumbnailViewAction_->setChecked(true);
        break;
    case FolderView::CompactMode:
        compactViewAction_->setChecked(true);
        break;
    case FolderView::DetailedListMode:
        detailedViewAction_->setChecked(true);
        break;
    default:
        break;
    }
    // selection changes
    connect(ui->folderView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &FileDialog::onCurrentRowChanged);
    connect(ui->folderView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FileDialog::onSelectionChanged);
    // update selection mode for the view
    updateSelectionMode();

    // FolderView::setViewMode() may delete the view to switch between list and tree.
    // So, the event filter should be re-installed.
    ui->folderView->childView()->removeEventFilter(this);
    ui->folderView->childView()->installEventFilter(this);
    if(noItemTooltip_) {
        ui->folderView->childView()->viewport()->removeEventFilter(this);
        ui->folderView->childView()->viewport()->installEventFilter(this);
    }
}


void FileDialog::setFileMode(QFileDialog::FileMode mode) {
    if(mode == QFileDialog::DirectoryOnly) {
        // directly only is deprecated and not allowed.
        mode = QFileDialog::Directory;
    }
    fileMode_ = mode;

    // enable multiple selection?
    updateSelectionMode();
}


void FileDialog::setAcceptMode(QFileDialog::AcceptMode mode) {
    acceptMode_ = mode;
    // set "open/save" label if it isn't set explicitly
    if(isLabelExplicitlySet(QFileDialog::Accept)) {
        return;
    }
    if(acceptMode_ == QFileDialog::AcceptOpen) {
        setLabelTextControl(QFileDialog::Accept, tr("&Open"));
    }
    else if(acceptMode_ == QFileDialog::AcceptSave) {
        setLabelTextControl(QFileDialog::Accept, tr("&Save"));
    }
}

void FileDialog::setNameFilters(const QStringList& filters) {
    if(filters.isEmpty()) {
        // default filename pattern
        nameFilters_ = (QStringList() << tr("All Files (*)"));
    }
    else {
        nameFilters_ = filters;
    }
    ui->fileTypeCombo->clear();
    ui->fileTypeCombo->addItems(nameFilters_);
}

void FileDialog::setMimeTypeFilters(const QStringList& filters) {
    mimeTypeFilters_ = filters;

    QStringList nameFilters;
    QMimeDatabase db;
    for(const auto& filter: filters) {
        auto mimeType = db.mimeTypeForName(filter);
        auto nameFilter = mimeType.comment();
        if(!mimeType.suffixes().empty()) {
            nameFilter += QLatin1String(" (");
            const auto suffixes = mimeType.suffixes();
            for(const auto& suffix: suffixes) {
                nameFilter += QLatin1String("*.");
                nameFilter += suffix;
                nameFilter += QLatin1Char(' ');
            }
            nameFilter[nameFilter.length() - 1] = ')';
        }
        nameFilters << nameFilter;
    }
    setNameFilters(nameFilters);
}

void FileDialog::setLabelTextControl(QFileDialog::DialogLabel label, const QString& text) {
    switch(label) {
    case QFileDialog::LookIn:
        ui->lookInLabel->setText(text);
        break;
    case QFileDialog::FileName:
        ui->fileNameLabel->setText(text);
        break;
    case QFileDialog::FileType:
        ui->fileTypeLabel->setText(text);
        break;
    case QFileDialog::Accept:
        ui->buttonBox_1->button(QDialogButtonBox::Ok)->setText(text);
        break;
    case QFileDialog::Reject:
        ui->buttonBox_2->button(QDialogButtonBox::Cancel)->setText(text);
        break;
    default:
        break;
    }
}

void FileDialog::setLabelText(QFileDialog::DialogLabel label, const QString& text) {
    setLabelExplicitly(label, text);
    setLabelTextControl(label, text);
}

QString FileDialog::labelText(QFileDialog::DialogLabel label) const {
    QString text;
    switch(label) {
    case QFileDialog::LookIn:
        text = ui->lookInLabel->text();
        break;
    case QFileDialog::FileName:
        text = ui->fileNameLabel->text();
        break;
    case QFileDialog::FileType:
        text = ui->fileTypeLabel->text();
        break;
    case QFileDialog::Accept:
        ui->buttonBox_1->button(QDialogButtonBox::Ok)->text();
        break;
    case QFileDialog::Reject:
        ui->buttonBox_2->button(QDialogButtonBox::Cancel)->text();
        break;
    default:
        break;
    }
    return text;
}

void FileDialog::updateSaveButtonText(bool saveOnFolder) {
    if(fileMode_ != QFileDialog::Directory
       && acceptMode_ == QFileDialog::AcceptSave) {
        // change save button to open button when there is a dir with the save name,
        // otherwise restore it to a save button again
        if(!saveOnFolder) {
            QStringList parsedNames = parseNames();
            if(!parsedNames.isEmpty()) {
                auto info = proxyModel_->fileInfoFromPath(directoryPath_.child(parsedNames.at(0).toLocal8Bit().constData()));
                if(info && info->isDir()) {
                    saveOnFolder = true;
                }
            }
        }
        if(saveOnFolder) {
            setLabelTextControl(QFileDialog::Accept, tr("&Open"));
        }
        else {
            // restore save button text appropriately
            if(isLabelExplicitlySet(QFileDialog::Accept)) {
                setLabelTextControl(QFileDialog::Accept, explicitLabels_[QFileDialog::Accept]);
            }
            else {
                setLabelTextControl(QFileDialog::Accept, tr("&Save"));
            }
        }
    }
}

void FileDialog::updateAcceptButtonState() {
    bool enable(false);
    if(fileMode_ != QFileDialog::Directory) {
        if(acceptMode_ == QFileDialog::AcceptOpen)
        {
            if(firstSelectedDir()) {
                // enable "open" button if a dir is selected
                enable = true;
            }
            else {
              // enable "open" button when there is a file whose name is listed
              QStringList parsedNames = parseNames();
              for(auto& name: parsedNames) {
                  if(proxyModel_->indexFromPath(directoryPath_.child(name.toLocal8Bit().constData())).isValid()) {
                    enable = true;
                    break;
                  }
              }
            }
        }
        else if(acceptMode_ == QFileDialog::AcceptSave) {
            // enable "save" button when there is a name or a dir selection
            if(!ui->fileName->text().isEmpty()) {
                enable = true;
            }
            else if(firstSelectedDir()) {
                enable = true;
            }
        }
    }
    else if(fileMode_ == QFileDialog::Directory
            && acceptMode_ != QFileDialog::AcceptSave) {
        QStringList parsedNames = parseNames();
        if(parsedNames.isEmpty()) {
            // in the dir mode, the current dir will be opened
            // if no dir is selected and the name list is empty
            enable = true;
        }
        else {
            for(auto& name: parsedNames) {
                auto info = proxyModel_->fileInfoFromPath(directoryPath_.child(name.toLocal8Bit().constData()));
                if(info && info->isDir()) {
                    // the name of a dir is listed
                    enable = true;
                    break;
                }
            }
        }
    }
    else {
        enable = true;
    }
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

bool FileDialog::FileDialogFilter::filterAcceptsRow(const ProxyFolderModel* /*model*/, const std::shared_ptr<const FileInfo> &info) const {
    if(dlg_->fileMode_ == QFileDialog::Directory) {
        // we only want to select directories
        if(!info->isDir()) { // not a dir
            // NOTE: here we ignore dlg_->options_& QFileDialog::ShowDirsOnly option.
            return false;
        }
    }
    else {
        // we want to select files, so all directories can be shown regardless of their names
        if(info->isDir()) {
            return true;
        }
    }

    bool nameMatched = false;
    auto& name = info->displayName();
    for(const auto& pattern: patterns_) {
#if (QT_VERSION >= QT_VERSION_CHECK(5,12,0))
        if(name.indexOf(pattern) == 0) {
#else
        if(pattern.exactMatch(name)) {
#endif
            nameMatched = true;
            break;
        }
    }
    return nameMatched;
}

void FileDialog::FileDialogFilter::update() {
    // update filename patterns
    patterns_.clear();
    QString nameFilter = dlg_->currentNameFilter_;
    // if the filter contains (...), get the part inside the last pair of parentheses
    // because "NAME (DESCRIPTION) (*.X *.Y)" is also possible
    auto left = nameFilter.lastIndexOf(QLatin1Char('('));
    if(left != -1) {
        ++left;
        auto right = nameFilter.indexOf(QLatin1Char(')'), left);
        if(right == -1) {
            right = nameFilter.length();
        }
        nameFilter = nameFilter.mid(left, right - left);
    }
    // parse the "*.ext1 *.ext2 *.ext3 ..." list into QRegularExpression objects
    auto globs = nameFilter.simplified().split(QLatin1Char(' '));
    for(const auto& glob: globs) {
#if (QT_VERSION >= QT_VERSION_CHECK(5,12,0))
        patterns_.emplace_back(QRegularExpression(QStringLiteral("\\A(?:")
                                                    + QRegularExpression::wildcardToRegularExpression(glob)
                                                    + QStringLiteral(")\\z"), QRegularExpression::CaseInsensitiveOption));
#else
        patterns_.emplace_back(QRegExp(glob, Qt::CaseInsensitive, QRegExp::Wildcard));
#endif
    }
}

} // namespace Fm
