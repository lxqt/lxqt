/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "desktopwindow.h"
#include <QWidget>
#include <QPainter>
#include <QImage>
#include <QImageReader>
#include <QFile>
#include <QPixmap>
#include <QPalette>

#include <QLayout>
#include <QDebug>
#include <QTimer>
#include <QTime>
#include <QSettings>
#include <QStringBuilder>
#include <QDir>
#include <QShortcut>
#include <QDropEvent>
#include <QMimeData>
#include <QPaintEvent>
#include <QStandardPaths>
#include <QClipboard>
#include <QWindow>
#include <QRandomGenerator>

#include "./application.h"
#include "mainwindow.h"
#include <libfm-qt/foldermenu.h>
#include <libfm-qt/filemenu.h>
#include <libfm-qt/folderitemdelegate.h>
#include <libfm-qt/cachedfoldermodel.h>
#include <libfm-qt/folderview_p.h>
#include <libfm-qt/fileoperation.h>
#include <libfm-qt/filepropsdialog.h>
#include <libfm-qt/utilities.h>
#include <libfm-qt/core/fileinfo.h>
#include "xdgdir.h"
#include "bulkrename.h"
#include "desktopentrydialog.h"

#include <QX11Info>
#include <QScreen>
#include <xcb/xcb.h>
#include <X11/Xlib.h>

#define WORK_AREA_MARGIN 12 // margin of the work area
#define MIN_SLIDE_INTERVAL 5*60000 // 5 min
#define MAX_SLIDE_INTERVAL (24*60+55)*60000 // 24 h and 55 min

namespace PCManFM {

DesktopWindow::DesktopWindow(int screenNum):
    View(Fm::FolderView::IconMode),
    proxyModel_(nullptr),
    model_(nullptr),
    wallpaperMode_(WallpaperNone),
    slideShowInterval_(0),
    wallpaperTimer_(nullptr),
    wallpaperRandomize_(false),
    fileLauncher_(nullptr),
    desktopHideItems_(false),
    screenNum_(screenNum),
    relayoutTimer_(nullptr),
    selectionTimer_(nullptr),
    trashUpdateTimer_(nullptr),
    trashMonitor_(nullptr) {

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);
    setAttribute(Qt::WA_DeleteOnClose);

    // set our custom file launcher
    View::setFileLauncher(&fileLauncher_);

    listView_ = static_cast<Fm::FolderViewListView*>(childView());
    listView_->setMovement(QListView::Snap);
    listView_->setResizeMode(QListView::Adjust);
    listView_->setFlow(QListView::TopToBottom);
    listView_->setDropIndicatorShown(false); // we draw the drop indicator ourself

    // This is to workaround Qt bug 54384 which affects Qt >= 5.6
    // https://bugreports.qt.io/browse/QTBUG-54384
    // Setting a QPixmap larger then the screen resolution to desktop's QPalette won't work.
    // So we make the viewport transparent by preventing its backround from being filled automatically.
    // Then we paint desktop's background ourselves by using its paint event handling method.
    listView_->viewport()->setAutoFillBackground(false);

    Settings& settings = static_cast<Application* >(qApp)->settings();

    // NOTE: When XRandR is in use, the all screens are actually combined to form a
    // large virtual desktop and only one DesktopWindow needs to be created and screenNum is -1.
    // In some older multihead setups, such as xinerama, every physical screen
    // is treated as a separate desktop so many instances of DesktopWindow may be created.
    // In this case we only want to show desktop icons on the primary screen.
    if((screenNum_ == 0 || qApp->primaryScreen()->virtualSiblings().size() > 1)) {
        loadItemPositions();

        setShadowHidden(settings.shadowHidden());

        auto desktopPath = Fm::FilePath::fromLocalPath(XdgDir::readDesktopDir().toStdString().c_str());
        model_ = new Fm::FolderModel();
        model_->setFolder(Fm::Folder::fromPath(desktopPath));
        model_->setShowFullName(false); // always show display name on Desktop
        folder_ = model_->folder();
        connect(folder_.get(), &Fm::Folder::startLoading, this, &DesktopWindow::onFolderStartLoading);
        connect(folder_.get(), &Fm::Folder::finishLoading, this, &DesktopWindow::onFolderFinishLoading);

        proxyModel_ = new Fm::ProxyFolderModel();
        proxyModel_->setSourceModel(model_);
        proxyModel_->setShowThumbnails(settings.showThumbnails());
        proxyModel_->sort(settings.desktopSortColumn(), settings.desktopSortOrder());
        proxyModel_->setFolderFirst(settings.desktopSortFolderFirst());
        proxyModel_->setHiddenLast(settings.desktopSortHiddenLast());
        setModel(proxyModel_);

        connect(proxyModel_, &Fm::ProxyFolderModel::rowsInserted, this, &DesktopWindow::onRowsInserted);
        connect(proxyModel_, &Fm::ProxyFolderModel::rowsAboutToBeRemoved, this, &DesktopWindow::onRowsAboutToBeRemoved);
        connect(proxyModel_, &Fm::ProxyFolderModel::layoutChanged, this, &DesktopWindow::onLayoutChanged);
        connect(proxyModel_, &Fm::ProxyFolderModel::sortFilterChanged, this, &DesktopWindow::onModelSortFilterChanged);
    }

    // remove frame
    listView_->setFrameShape(QFrame::NoFrame);
    // inhibit scrollbars FIXME: this should be optional in the future
    listView_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    listView_->installEventFilter(this);
    listView_->viewport()->installEventFilter(this);

    // setup shortcuts
    QShortcut* shortcut;
    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_X), this); // cut
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onCutActivated);

    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_C), this); // copy
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onCopyActivated);

    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C), this); // copy full path
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onCopyFullPathActivated);

    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_V), this); // paste
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onPasteActivated);

    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this); // select all
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::selectAll);

    shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this); // delete
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onDeleteActivated);

    shortcut = new QShortcut(QKeySequence(Qt::Key_F2), this); // rename
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onRenameActivated);

    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F2), this); // bulk rename
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onBulkRenameActivated);

    shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Return), this); // properties
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onFilePropertiesActivated);
    shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Enter), this); // properties
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onFilePropertiesActivated);

    shortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete), this); // force delete
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onDeleteActivated);
}

DesktopWindow::~DesktopWindow() {
    if(trashMonitor_) {
        g_signal_handlers_disconnect_by_func(trashMonitor_, (gpointer)G_CALLBACK(onTrashChanged), this);
        g_object_unref(trashMonitor_);
        trashMonitor_ = nullptr;
    }

    listView_->viewport()->removeEventFilter(this);
    listView_->removeEventFilter(this);

    disconnect(folder_.get(), nullptr, this, nullptr);

    if(relayoutTimer_) {
        relayoutTimer_->stop();
        delete relayoutTimer_;
    }

    if(wallpaperTimer_) {
        wallpaperTimer_->stop();
        delete wallpaperTimer_;
    }

    if(proxyModel_) {
        delete proxyModel_;
    }

    if(model_) {
        disconnect(model_, &Fm::FolderModel::filesAdded, this, &DesktopWindow::onFilesAdded);
        delete model_;
    }
}

void DesktopWindow::updateShortcutsFromSettings(Settings& settings) {
    // Shortcuts should be deleted only when the user removes them
    // in the Preferences dialog, not when the desktop is created.
    static bool firstCall = true;

    const QStringList ds = settings.desktopShortcuts();
    Fm::FilePathList paths;
    // Trash
    if(ds.contains(QLatin1String("Trash")) && settings.useTrash()) {
        createTrash();
    }
    else {
        if(trashUpdateTimer_) {
            trashUpdateTimer_->stop();
            delete trashUpdateTimer_;
            trashUpdateTimer_ = nullptr;
        }
        if(trashMonitor_) {
            g_signal_handlers_disconnect_by_func(trashMonitor_, (gpointer)G_CALLBACK(onTrashChanged), this);
            g_object_unref(trashMonitor_);
            trashMonitor_ = nullptr;
        }
        if(!firstCall) {
            QString trash = XdgDir::readDesktopDir() + QLatin1String("/trash-can.desktop");
            if(QFile::exists(trash)) {
                paths.push_back(Fm::FilePath::fromLocalPath(trash.toStdString().c_str()));
            }
        }
    }
    // Home
    if(ds.contains(QLatin1String("Home"))) {
        createHomeShortcut();
    }
    else if(!firstCall) {
        QString home = XdgDir::readDesktopDir() + QLatin1String("/user-home.desktop");
        if(QFile::exists(home)) {
            paths.push_back(Fm::FilePath::fromLocalPath(home.toStdString().c_str()));
        }
    }
    // Computer
    if(ds.contains(QLatin1String("Computer"))) {
        createComputerShortcut();
    }
    else if(!firstCall) {
        QString computer = XdgDir::readDesktopDir() + QLatin1String("/computer.desktop");
        if(QFile::exists(computer)) {
            paths.push_back(Fm::FilePath::fromLocalPath(computer.toStdString().c_str()));
        }
    }
    // Network
    if(ds.contains(QLatin1String("Network"))) {
        createNetworkShortcut();
    }
    else if(!firstCall) {
        QString network = XdgDir::readDesktopDir() + QLatin1String("/network.desktop");
        if(QFile::exists(network)) {
            paths.push_back(Fm::FilePath::fromLocalPath(network.toStdString().c_str()));
        }
    }

    // WARNING: QFile::remove() is not compatible with libfm-qt and should not be used.
    if(!paths.empty()) {
        Fm::FileOperation::deleteFiles(paths, false);
    }

    firstCall = false; // desktop is created
}

void DesktopWindow::createTrashShortcut(int items) {
    GKeyFile* kf = g_key_file_new();
    g_key_file_set_string(kf, "Desktop Entry", "Type", "Application");
    g_key_file_set_string(kf, "Desktop Entry", "Exec", "pcmanfm-qt trash:///");
    // icon
    const char* icon_name = items > 0 ? "user-trash-full" : "user-trash";
    g_key_file_set_string(kf, "Desktop Entry", "Icon", icon_name);
    // name
    QString name;
    if(items > 0) {
        if (items == 1) {
            name = tr("Trash (One item)");
        }
        else {
            name = tr("Trash (%Ln items)", "", items);
        }
    }
    else {
        name = tr("Trash (Empty)");
    }
    g_key_file_set_string(kf, "Desktop Entry", "Name", name.toStdString().c_str());

    auto path = Fm::FilePath::fromLocalPath(XdgDir::readDesktopDir().toStdString().c_str()).localPath();
    auto trash_can = Fm::CStrPtr{g_build_filename(path.get(), "trash-can.desktop", nullptr)};
    g_key_file_save_to_file(kf, trash_can.get(), nullptr);
    g_key_file_free(kf);
}

void DesktopWindow::createHomeShortcut() {
    GKeyFile* kf = g_key_file_new();
    g_key_file_set_string(kf, "Desktop Entry", "Type", "Application");
    g_key_file_set_string(kf, "Desktop Entry", "Exec", Fm::CStrPtr(g_strconcat("pcmanfm-qt ", Fm::FilePath::homeDir().toString().get(), nullptr)).get());
    g_key_file_set_string(kf, "Desktop Entry", "Icon", "user-home");
    g_key_file_set_string(kf, "Desktop Entry", "Name", g_get_user_name());

    auto path = Fm::FilePath::fromLocalPath(XdgDir::readDesktopDir().toStdString().c_str()).localPath();
    auto trash_can = Fm::CStrPtr{g_build_filename(path.get(), "user-home.desktop", nullptr)};
    g_key_file_save_to_file(kf, trash_can.get(), nullptr);
    g_key_file_free(kf);
}

void DesktopWindow::createComputerShortcut() {
    GKeyFile* kf = g_key_file_new();
    g_key_file_set_string(kf, "Desktop Entry", "Type", "Application");
    g_key_file_set_string(kf, "Desktop Entry", "Exec", "pcmanfm-qt computer:///");
    g_key_file_set_string(kf, "Desktop Entry", "Icon", "computer");
    const QString name = tr("Computer");
    g_key_file_set_string(kf, "Desktop Entry", "Name", name.toStdString().c_str());

    auto path = Fm::FilePath::fromLocalPath(XdgDir::readDesktopDir().toStdString().c_str()).localPath();
    auto trash_can = Fm::CStrPtr{g_build_filename(path.get(), "computer.desktop", nullptr)};
    g_key_file_save_to_file(kf, trash_can.get(), nullptr);
    g_key_file_free(kf);
}

void DesktopWindow::createNetworkShortcut() {
    GKeyFile* kf = g_key_file_new();
    g_key_file_set_string(kf, "Desktop Entry", "Type", "Application");
    g_key_file_set_string(kf, "Desktop Entry", "Exec", "pcmanfm-qt network:///");
    g_key_file_set_string(kf, "Desktop Entry", "Icon", "folder-network");
    const QString name = tr("Network");
    g_key_file_set_string(kf, "Desktop Entry", "Name", name.toStdString().c_str());

    auto path = Fm::FilePath::fromLocalPath(XdgDir::readDesktopDir().toStdString().c_str()).localPath();
    auto trash_can = Fm::CStrPtr{g_build_filename(path.get(), "network.desktop", nullptr)};
    g_key_file_save_to_file(kf, trash_can.get(), nullptr);
    g_key_file_free(kf);
}

void DesktopWindow::createTrash() {
    if(trashMonitor_) {
        return;
    }
    Fm::FilePath trashPath = Fm::FilePath::fromUri("trash:///");
    // check if trash is supported by the current vfs
    // if gvfs is not installed, this can be unavailable.
    if(!g_file_query_exists(trashPath.gfile().get(), nullptr)) {
        trashMonitor_ = nullptr;
        return;
    }

    trashMonitor_ = g_file_monitor_directory(trashPath.gfile().get(), G_FILE_MONITOR_NONE, nullptr, nullptr);
    if(trashMonitor_) {
        if(trashUpdateTimer_ == nullptr) {
            trashUpdateTimer_ = new QTimer(this);
            trashUpdateTimer_->setSingleShot(true);
            connect(trashUpdateTimer_, &QTimer::timeout, this, &DesktopWindow::updateTrashIcon);
        }
        updateTrashIcon();
        g_signal_connect(trashMonitor_, "changed", G_CALLBACK(onTrashChanged), this);
    }
}

// static
void DesktopWindow::onTrashChanged(GFileMonitor* /*monitor*/, GFile* /*gf*/, GFile* /*other*/, GFileMonitorEvent /*evt*/, DesktopWindow* pThis) {
    if(pThis->trashUpdateTimer_ != nullptr && !pThis->trashUpdateTimer_->isActive()) {
        pThis->trashUpdateTimer_->start(250); // don't update trash very fast
    }
}

void DesktopWindow::updateTrashIcon() {
    struct UpdateTrashData {
        QPointer<DesktopWindow> desktop;
        Fm::FilePath trashPath;
        UpdateTrashData(DesktopWindow* _desktop) : desktop(_desktop) {
            trashPath = Fm::FilePath::fromUri("trash:///");
        }
    };

    UpdateTrashData* data = new UpdateTrashData(this);
    g_file_query_info_async(data->trashPath.gfile().get(), G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT, G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, nullptr,
    [](GObject * /*source_object*/, GAsyncResult * res, gpointer user_data) {
        // the callback lambda function is called when the asyn query operation is finished
        UpdateTrashData* data = reinterpret_cast<UpdateTrashData*>(user_data);
        DesktopWindow* _this = data->desktop.data();
        if(_this != nullptr) {
            Fm::GFileInfoPtr inf{g_file_query_info_finish(data->trashPath.gfile().get(), res, nullptr), false};
            if(inf) {
                guint32 n = g_file_info_get_attribute_uint32(inf.get(), G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT);
                _this->createTrashShortcut(static_cast<int>(n));
            }
        }
        delete data; // free the data used for this async operation.
    }, data);
}

bool DesktopWindow::isTrashCan(std::shared_ptr<const Fm::FileInfo> file) const {
    bool ret(false);
    if(file && file->isDesktopEntry() && trashMonitor_) {
        const QString fileName = QString::fromStdString(file->name());
        const char* execStr = fileName == QLatin1String("trash-can.desktop")
                                ? "pcmanfm-qt trash:///" : nullptr;
        if(execStr) {
            GKeyFile* kf = g_key_file_new();
            if(g_key_file_load_from_file(kf, file->path().toString().get(), G_KEY_FILE_NONE, nullptr)) {
                Fm::CStrPtr str{g_key_file_get_string(kf, "Desktop Entry", "Exec", nullptr)};
                if(str && strcmp(str.get(), execStr) == 0) {
                    ret = true;
                }
            }
            g_key_file_free(kf);
        }
    }
    return ret;
}

void DesktopWindow::setBackground(const QColor& color) {
    bgColor_ = color;
}

void DesktopWindow::setForeground(const QColor& color) {
    QPalette p = listView_->palette();
    p.setBrush(QPalette::Text, color);
    listView_->setPalette(p);
    fgColor_ = color;
}

void DesktopWindow::setShadow(const QColor& color) {
    shadowColor_ = color;
    auto delegate = static_cast<Fm::FolderItemDelegate*>(listView_->itemDelegateForColumn(Fm::FolderModel::ColumnFileName));
    delegate->setShadowColor(color);
}

void DesktopWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // resize wall paper if needed
    if(isVisible() && wallpaperMode_ != WallpaperNone && wallpaperMode_ != WallpaperTile) {
        updateWallpaper();
        update();
    }
    queueRelayout(100); // Qt use a 100 msec delay for relayout internally so we use it, too.
}

void DesktopWindow::setDesktopFolder() {
    if(folder_) {
        // free the previous model and folder
        if(model_) {
            disconnect(model_, &Fm::FolderModel::filesAdded, this, &DesktopWindow::onFilesAdded);
            proxyModel_->setSourceModel(nullptr);
            delete model_;
            model_ = nullptr;
        }
        disconnect(folder_.get(), nullptr, this, nullptr);
        folder_ = nullptr;
    }

    auto path = Fm::FilePath::fromLocalPath(XdgDir::readDesktopDir().toStdString().c_str());
    model_ = new Fm::FolderModel();
    model_->setFolder(Fm::Folder::fromPath(path));
    model_->setShowFullName(false);
    folder_ = model_->folder();
    connect(folder_.get(), &Fm::Folder::startLoading, this, &DesktopWindow::onFolderStartLoading);
    connect(folder_.get(), &Fm::Folder::finishLoading, this, &DesktopWindow::onFolderFinishLoading);
    proxyModel_->setSourceModel(model_);
    if(folder_->isLoaded()) {
        onFolderStartLoading();
        onFolderFinishLoading();
    }
    else {
        onFolderStartLoading();
    }
}

void DesktopWindow::setWallpaperFile(QString filename) {
    wallpaperFile_ = filename;
}

void DesktopWindow::setWallpaperMode(WallpaperMode mode) {
    wallpaperMode_ = mode;
}

void DesktopWindow::setLastSlide(QString filename) {
    lastSlide_ = filename;
}

void DesktopWindow::setWallpaperDir(QString dirname) {
    wallpaperDir_ = dirname;
}

void DesktopWindow::setSlideShowInterval(int interval) {
    slideShowInterval_ = interval;
}

void DesktopWindow::setWallpaperRandomize(bool randomize) {
    wallpaperRandomize_ = randomize;
}

QImage DesktopWindow::getWallpaperImage() const {
    QImage image(wallpaperFile_);
    if(!image.isNull()) {
        Settings& settings = static_cast<Application* >(qApp)->settings();
        if(settings.transformWallpaper()
        && (wallpaperFile_.endsWith(QLatin1String(".jpg"), Qt::CaseInsensitive)
            || wallpaperFile_.endsWith(QLatin1String(".jpeg"), Qt::CaseInsensitive))) {
            // auto-transform jpeg images based on their EXIF data
            QImageReader reader(wallpaperFile_);
            QImageIOHandler::Transformations tr = reader.transformation();
            QMatrix m;
            // mirroring
            if(tr & QImageIOHandler::TransformationMirror) {
                m.scale(-1, 1);
            }
            if(tr & QImageIOHandler::TransformationFlip) {
                m.scale(1, -1);
            }
            // rotation
            if(tr & QImageIOHandler::TransformationRotate90) {
                m.rotate(90);
            }
            if(!m.isIdentity()) {
                image = image.transformed(m);
            }
        }
    }
    return image;
}

QImage DesktopWindow::loadWallpaperFile(QSize requiredSize) {
    // NOTE: for ease of programming, we only use the cache for the primary screen.
    bool useCache = (screenNum_ == -1 || screenNum_ == 0);
    QFile info;
    QString cacheFileName;
    if(useCache) {
        // see if we have a scaled version cached on disk
        cacheFileName = QString::fromLocal8Bit(qgetenv("XDG_CACHE_HOME"));
        if(cacheFileName.isEmpty()) {
            cacheFileName = QDir::homePath() + QLatin1String("/.cache");
        }
        Application* app = static_cast<Application*>(qApp);
        cacheFileName += QLatin1String("/pcmanfm-qt/") + app->profileName();
        QDir().mkpath(cacheFileName); // ensure that the cache dir exists
        cacheFileName += QLatin1String("/wallpaper.cache");

        // read info file
        QString origin;
        info.setFileName(cacheFileName + QStringLiteral(".info"));
        if(info.open(QIODevice::ReadOnly)) {
            // FIXME: we need to compare mtime to see if the cache is out of date
            origin = QString::fromLocal8Bit(info.readLine());
            info.close();
            if(!origin.isEmpty()) {
                // try to see if we can get the size of the cached image.
                QImageReader reader(cacheFileName);
                reader.setAutoDetectImageFormat(true);
                QSize cachedSize = reader.size();
                qDebug() << "size of cached file" << cachedSize << ", requiredSize:" << requiredSize;
                if(cachedSize.isValid()) {
                    if(cachedSize == requiredSize) { // see if the cached wallpaper has the size we want
                        QImage image = reader.read(); // return the loaded image
                        qDebug() << "origin" << origin;
                        if(origin == wallpaperFile_) {
                            return image;
                        }
                    }
                }
            }
        }
        qDebug() << "no cached wallpaper. generate a new one!";
    }

    // we don't have a cached scaled image, load the original file
    QImage image = getWallpaperImage();
    qDebug() << "size of original image" << image.size();
    if(image.isNull() || image.size() == requiredSize) { // if the original size is what we want
        return image;
    }

    // scale the original image
    QImage scaled = image.scaled(requiredSize.width(), requiredSize.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    // FIXME: should we save the scaled image if its size is larger than the original image?

    if(useCache) {
        // write the path of the original image to the .info file
        if(info.open(QIODevice::WriteOnly)) {
            info.write(wallpaperFile_.toLocal8Bit());
            info.close();

            // write the scaled cache image to disk
            const char* format; // we keep jpg format for *.jpg files, and use png format for others.
            if(wallpaperFile_.endsWith(QLatin1String(".jpg"), Qt::CaseInsensitive) || wallpaperFile_.endsWith(QLatin1String(".jpeg"), Qt::CaseInsensitive)) {
                format = "JPG";
            }
            else {
                format = "PNG";
            }
            scaled.save(cacheFileName, format);
        }
        qDebug() << "wallpaper cached saved to " << cacheFileName;
        // FIXME: we might delay the write of the cached image?
    }
    return scaled;
}

// really generate the background pixmap according to current settings and apply it.
void DesktopWindow::updateWallpaper() {
    if(wallpaperMode_ != WallpaperNone) {  // use wallpaper
        QPixmap pixmap;
        QImage image;
        Settings& settings = static_cast<Application* >(qApp)->settings();
        auto screen = getDesktopScreen();
        bool perScreenWallpaper(screen != nullptr && screen->virtualSiblings().size() > 1 && settings.perScreenWallpaper());
        if(wallpaperMode_ == WallpaperTile) { // use the original size
            image = getWallpaperImage();
            if(!image.isNull()) {
                // Note: We can't use the QPainter::drawTiledPixmap(), because it doesn't tile
                // correctly for background pixmaps bigger than the current screen size.
                const QSize s = size();
                pixmap = QPixmap{s};
                QPainter painter{&pixmap};
                for (int x = 0; x < s.width(); x += image.width()) {
                    for (int y = 0; y < s.height(); y += image.height()) {
                        painter.drawImage(x, y, image);
                    }
                }
            }
        }
        else if(wallpaperMode_ == WallpaperStretch) {
            if(perScreenWallpaper) {
                const QSize s = size();
                pixmap = QPixmap{s};
                QPainter painter{&pixmap};
                pixmap.fill(bgColor_);
                image = getWallpaperImage();
                if(!image.isNull()) {
                    QImage scaled;
                    const auto screens = screen->virtualSiblings();
                    for(const auto& scr : screens) {
                        scaled = image.scaled(scr->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                        painter.drawImage(scr->geometry().x(), scr->geometry().y(), scaled);
                    }
                }
            }
            else {
                image = loadWallpaperFile(size());
                pixmap = QPixmap::fromImage(image);
            }
        }
        else { // WallpaperCenter || WallpaperFit
            if(perScreenWallpaper) {
                const QSize s = size();
                pixmap = QPixmap{s};
                QPainter painter{&pixmap};
                pixmap.fill(bgColor_);
                image = getWallpaperImage();
                if(!image.isNull()) {
                    QImage scaled;
                    int x, y;
                    const auto screens = screen->virtualSiblings();
                    if(wallpaperMode_ == WallpaperCenter) {
                        for(const auto& scr : screens) {
                            // get the gap between image and screen to avoid overlapping and displacement
                            int x_gap = (image.width() - scr->geometry().width()) / 2;
                            int y_gap = (image.height() - scr->geometry().height()) / 2;
                            scaled = image.copy(qMax(x_gap, 0), qMax(y_gap, 0), scr->geometry().width(), scr->geometry().height());
                            x = scr->geometry().x() + qMax(0, -x_gap);
                            y = scr->geometry().y() + qMax(0, - y_gap);
                            painter.drawImage(x, y, scaled);
                        }
                    }
                    else if((wallpaperMode_ == WallpaperFit || wallpaperMode_ == WallpaperZoom)
                            && image.width() > 0 && image.height() > 0) {
                        for(const auto& scr : screens) {
                            // get the screen-to-image ratio to calculate the scale factors
                            const qreal w_ratio = static_cast<qreal>(scr->geometry().width()) / image.width();
                            const qreal h_ratio = static_cast<qreal>(scr->geometry().height()) / image.height();
                            if(w_ratio <= h_ratio) {
                                if(wallpaperMode_ == WallpaperFit) {
                                    // fit horizontally
                                    scaled = image.scaledToWidth(scr->geometry().width(), Qt::SmoothTransformation);
                                    x = scr->geometry().x();
                                    y = scr->geometry().y() + (scr->geometry().height() - scaled.height()) / 2;
                                }
                                else { // zoom
                                    // fit vertically
                                    scaled = image.scaledToHeight(scr->geometry().height(), Qt::SmoothTransformation);
                                    // crop to avoid overlapping
                                    int x_gap = (scaled.width() - scr->geometry().width()) / 2;
                                    scaled = scaled.copy(x_gap, 0, scr->geometry().width(), scaled.height());
                                    x = scr->geometry().x();
                                    y = scr->geometry().y();
                                }
                            }
                            else  { // w_ratio > h_ratio
                                if(wallpaperMode_ == WallpaperFit) {
                                    // fit vertically
                                    scaled = image.scaledToHeight(scr->geometry().height(), Qt::SmoothTransformation);
                                    x = scr->geometry().x() + (scr->geometry().width() - scaled.width()) / 2;
                                    y = scr->geometry().y();
                                }
                                else { // zoom
                                    // fit horizonatally
                                    scaled = image.scaledToWidth(scr->geometry().width(), Qt::SmoothTransformation);
                                    // crop to avoid overlapping
                                    int y_gap = (scaled.height() - scr->geometry().height()) / 2;
                                    scaled = scaled.copy(0, y_gap, scaled.width(), scr->geometry().height());
                                    x = scr->geometry().x();
                                    y = scr->geometry().y();
                                }
                            }
                            painter.drawImage(x, y, scaled);
                        }
                    }
                }
            }
            else {
                if(wallpaperMode_ == WallpaperCenter) {
                    image = getWallpaperImage(); // load original image
                }
                else if(wallpaperMode_ == WallpaperFit || wallpaperMode_ == WallpaperZoom) {
                    // calculate the desired size
                    QImageReader reader(wallpaperFile_);
                    QSize origSize = reader.size(); // get the size of the original file
                    if(reader.transformation() & QImageIOHandler::TransformationRotate90) {
                        if(settings.transformWallpaper()
                        && (wallpaperFile_.endsWith(QLatin1String(".jpg"), Qt::CaseInsensitive)
                            || wallpaperFile_.endsWith(QLatin1String(".jpeg"), Qt::CaseInsensitive))) {
                            origSize.transpose();
                        }
                    }
                    if(origSize.isValid()) {
                        QSize desiredSize = origSize;
                        Qt::AspectRatioMode mode = (wallpaperMode_ == WallpaperFit ? Qt::KeepAspectRatio : Qt::KeepAspectRatioByExpanding);
                        desiredSize.scale(width(), height(), mode);
                        image = loadWallpaperFile(desiredSize); // load the scaled image
                    }
                }
                if(!image.isNull()) {
                    pixmap = QPixmap(size());
                    QPainter painter(&pixmap);
                    pixmap.fill(bgColor_);
                    int x = (width() - image.width()) / 2;
                    int y = (height() - image.height()) / 2;
                    painter.drawImage(x, y, image);
                }
            }
        }
        wallpaperPixmap_ = pixmap;
    }
}

bool DesktopWindow::pickWallpaper() {
    if(slideShowInterval_ <= 0
       || !QFileInfo(wallpaperDir_).isDir()) {
        return false;
    }

    QList<QByteArray> formats = QImageReader::supportedImageFormats();
    QStringList formatsFilters;
    for (const QByteArray& format: formats)
        formatsFilters << QStringLiteral("*.") + QString::fromUtf8(format);
    QDir folder(wallpaperDir_);
    QStringList files = folder.entryList(formatsFilters,
                                         QDir::Files | QDir::NoDotAndDotDot,
                                         QDir::Name);
    if(!files.isEmpty()) {
       QString dir = wallpaperDir_ + QLatin1Char('/');
       if(!wallpaperRandomize_) {
           if(!lastSlide_.startsWith(dir)) { // not in the directory
               wallpaperFile_ = dir + files.first();
           }
           else {
               QString ls = lastSlide_.remove(0, dir.size());
               if(ls.isEmpty() // invalid
                  || ls.contains(QLatin1Char('/'))) { // in a subdirectory or invalid
                   wallpaperFile_ = dir + files.first();
               }
               else {
                   int index = files.indexOf(ls);
                   if(index == -1) { // removed or invalid
                       wallpaperFile_ = dir + files.first();
                   }
                   else {
                       wallpaperFile_ = dir + (index + 1 < files.size()
                                               ? files.at(index + 1)
                                               : files.first());
                   }
               }
           }
       }
       else {
           if(files.size() > 1) {
               if(lastSlide_.startsWith(dir)) {
                   QString ls = lastSlide_.remove(0, dir.size());
                   if(!ls.isEmpty() && !ls.contains(QLatin1Char('/')))
                       files.removeOne(ls); // choose from other images
               }
               // this is needed for the randomness, especially when choosing the first wallpaper

               // Conversion notes:
               // The random generated value can be safely converted to an int because it's max
               // value is also an int (files.size() - 1).
               // files.size() - 1 (the highest random value) is always => 1
               // files.at(0) -> valid, files.at(size) -> invalid
               int randomValue = static_cast<int>(QRandomGenerator::global()->bounded(static_cast<quint32>(files.size())));
               wallpaperFile_ = dir + files.at(randomValue);
           }
           else {
               wallpaperFile_ = dir + files.first();
           }
       }

       if (lastSlide_ != wallpaperFile_) {
           lastSlide_ = wallpaperFile_;
           Settings& settings = static_cast<Application*>(qApp)->settings();
           settings.setLastSlide(lastSlide_);
           return true;
       }
    }

  return false;
}

void DesktopWindow::nextWallpaper() {
    if(pickWallpaper()) {
        updateWallpaper();
        update();
    }
}

void DesktopWindow::updateFromSettings(Settings& settings, bool changeSlide) {
    setDesktopFolder();
    setWallpaperFile(settings.wallpaper());
    setWallpaperMode(settings.wallpaperMode());
    setLastSlide(settings.lastSlide());
    QString wallpaperDir = settings.wallpaperDir();
    if(wallpaperDir_ != wallpaperDir) {
      changeSlide = true; // another wallpapaer directory; change slide!
    }
    setWallpaperDir(wallpaperDir);
    int interval = settings.slideShowInterval();
    if(interval > 0 && (interval < MIN_SLIDE_INTERVAL || interval > MAX_SLIDE_INTERVAL)) {
        interval = qBound(MIN_SLIDE_INTERVAL, interval, MAX_SLIDE_INTERVAL);
        settings.setSlideShowInterval(interval);
    }
    setSlideShowInterval(interval);
    setWallpaperRandomize(settings.wallpaperRandomize());
    setFont(settings.desktopFont());
    setIconSize(Fm::FolderView::IconMode, QSize(settings.desktopIconSize(), settings.desktopIconSize()));
    setMargins(settings.desktopCellMargins());
    updateShortcutsFromSettings(settings);
    // setIconSize and setMargins may trigger relayout of items by QListView, so we need to do the layout again.
    queueRelayout();
    setForeground(settings.desktopFgColor());
    setBackground(settings.desktopBgColor());
    setShadow(settings.desktopShadowColor());
    desktopHideItems_ = settings.desktopHideItems();
    if(desktopHideItems_) {
        // hide all items by hiding the list view and also
        // prevent the current item from being changed by arrow keys
        listView_->clearFocus();
        listView_->setVisible(false);
    }

    if(slideShowInterval_ > 0
       && QFileInfo(wallpaperDir_).isDir()) {
        if(!wallpaperTimer_) {
            changeSlide = true; // slideshow activated; change slide!
            wallpaperTimer_ = new QTimer();
            connect(wallpaperTimer_, &QTimer::timeout, this, &DesktopWindow::nextWallpaper);
        }
        else {
            wallpaperTimer_->stop(); // restart the timer after updating wallpaper
        }
        if(changeSlide) {
            pickWallpaper();
        }
        else if(QFile::exists(lastSlide_)) {
            /* show the last slide if it still exists,
               otherwise show the wallpaper until timeout */
            wallpaperFile_ = lastSlide_;
        }
    }
    else if(wallpaperTimer_) {
        wallpaperTimer_->stop();
        delete wallpaperTimer_;
        wallpaperTimer_ = nullptr;
    }

    updateWallpaper();
    update();

    if(wallpaperTimer_) {
        wallpaperTimer_->start(slideShowInterval_);
    }
}

void DesktopWindow::onFileClicked(int type, const std::shared_ptr<const Fm::FileInfo>& fileInfo) {
    if(desktopHideItems_) { // only a context menu with desktop actions
        if(type == Fm::FolderView::ActivatedClick) {
            return;
        }
        QMenu* menu = new QMenu(this);
        addDesktopActions(menu);
        menu->exec(QCursor::pos());
        delete menu;
    }
    else {
        // special right-click menus for our desktop shortcuts
        if(fileInfo && fileInfo->isDesktopEntry() && type == Fm::FolderView::ContextMenuClick) {
            Settings& settings = static_cast<Application* >(qApp)->settings();
            const QStringList ds = settings.desktopShortcuts();
            if(!ds.isEmpty()) {
                const QString fileName = QString::fromStdString(fileInfo->name());
                if((fileName == QLatin1String("trash-can.desktop") && ds.contains(QLatin1String("Trash")))
                   || (fileName == QLatin1String("user-home.desktop") && ds.contains(QLatin1String("Home")))
                   || (fileName == QLatin1String("computer.desktop") && ds.contains(QLatin1String("Computer")))
                   || (fileName == QLatin1String("network.desktop") && ds.contains(QLatin1String("Network")))) {
                    QMenu* menu = new QMenu(this);
                    // "Open" action for all
                    QAction* action = menu->addAction(tr("Open"));
                    connect(action, &QAction::triggered, this, [this, fileInfo] {
                        onFileClicked(Fm::FolderView::ActivatedClick, fileInfo);
                    });
                    // "Stick" action for all
                    action = menu->addAction(tr("Stic&k to Current Position"));
                    action->setCheckable(true);
                    action->setChecked(customItemPos_.find(fileInfo->name()) != customItemPos_.cend());
                    connect(action, &QAction::toggled, this, &DesktopWindow::onStickToCurrentPos);
                    // "Empty Trash" action for Trash shortcut
                    if(fileName == QLatin1String("trash-can.desktop")) {
                        menu->addSeparator();
                        action = menu->addAction(tr("Empty Trash"));
                        // disable the item is Trash is empty
                        GKeyFile* kf = g_key_file_new();
                        if(g_key_file_load_from_file(kf, fileInfo->path().toString().get(), G_KEY_FILE_NONE, nullptr)) {
                            Fm::CStrPtr str{g_key_file_get_string(kf, "Desktop Entry", "Icon", nullptr)};
                            if(str && strcmp(str.get(), "user-trash") == 0) {
                                action->setEnabled(false);
                            }
                        }
                        g_key_file_free(kf);
                        // empty Trash on clicking the item
                        connect(action, &QAction::triggered, this, [this, &settings] {
                            Fm::FilePathList files;
                            files.push_back(Fm::FilePath::fromUri("trash:///"));
                            Fm::FileOperation::deleteFiles(std::move(files), settings.confirmDelete(), this);
                        });
                    }
                    menu->exec(QCursor::pos());
                    delete menu;
                    return;
                }
            }
        }
        View::onFileClicked(type, fileInfo);
    }
}

void DesktopWindow::prepareFileMenu(Fm::FileMenu* menu) {
    // qDebug("DesktopWindow::prepareFileMenu");
    PCManFM::View::prepareFileMenu(menu);
    QAction* action = new QAction(tr("Stic&k to Current Position"), menu);
    action->setCheckable(true);
    menu->insertSeparator(menu->separator2());
    menu->insertAction(menu->separator2(), action);

    bool checked(true);
    auto files = menu->files();
    for(const auto& file : files) {
        if(customItemPos_.find(file->name()) == customItemPos_.cend()) {
            checked = false;
            break;
        }
    }
    action->setChecked(checked);
    connect(action, &QAction::toggled, this, &DesktopWindow::onStickToCurrentPos);
}

void DesktopWindow::prepareFolderMenu(Fm::FolderMenu* menu) {
    PCManFM::View::prepareFolderMenu(menu);
    // remove file properties action
    menu->removeAction(menu->propertiesAction());
    // add desktop actions instead
    addDesktopActions(menu);
}

void DesktopWindow::addDesktopActions(QMenu* menu) {
    QAction* action = menu->addAction(tr("Hide Desktop Items"));
    action->setCheckable(true);
    action->setChecked(desktopHideItems_);
    menu->addSeparator();
    connect(action, &QAction::triggered, this, &DesktopWindow::toggleDesktop);
    if(!desktopHideItems_) {
        action = menu->addAction(tr("Create Launcher"));
        connect(action, &QAction::triggered, this, &DesktopWindow::onCreatingShortcut);
    }
    action = menu->addAction(tr("Desktop Preferences"));
    connect(action, &QAction::triggered, this, &DesktopWindow::onDesktopPreferences);
}

void DesktopWindow::toggleDesktop() {
    desktopHideItems_ = !desktopHideItems_;
    Settings& settings = static_cast<Application*>(qApp)->settings();
    settings.setDesktopHideItems(desktopHideItems_);
    listView_->setVisible(!desktopHideItems_);
    // a relayout is needed on showing the items for the first time
    // because the positions aren't updated while the view is hidden
    if(!desktopHideItems_) {
        listView_->setFocus(); // refocus the view
        queueRelayout();
    }
    else {
        listView_->clearFocus(); // prevent the current item from being changed by arrow keys
        setCursor(Qt::ArrowCursor); // ensure arrow cursor on an empty desktop
    }
}

void DesktopWindow::selectAll() {
    if(!desktopHideItems_) {
        FolderView::selectAll();
    }
}

void DesktopWindow::onCreatingShortcut() {
    DesktopEntryDialog* dlg = new DesktopEntryDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &DesktopEntryDialog::desktopEntryCreated, [this] (const QString& name) {
        filesToTrust_ << name;
    });
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void DesktopWindow::onDesktopPreferences() {
    static_cast<Application* >(qApp)->desktopPrefrences(QString());
}

void DesktopWindow::onRowsInserted(const QModelIndex& parent, int start, int end) {
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    // disable view updates temporarily and delay relayout to prevent items from shaking
    listView_->setUpdatesEnabled(false);
    queueRelayout(100);
}

void DesktopWindow::onRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    if(!customItemPos_.empty()) {
        // also delete stored custom item positions for the items currently being removed.
        // Here we can't rely on ProxyFolderModel::fileInfoFromIndex() because, although rows
        // aren't removed yet, files are already removed.
        bool changed = false;
        QString desktopDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        desktopDir += QLatin1Char('/');
        for(auto it = customItemPos_.cbegin(); it != customItemPos_.cend();) {
            auto& name = it->first;
            if(!QFile::exists(desktopDir + QString::fromStdString(name))) {
                it = customItemPos_.erase(it);
                changed = true;
            }
            else {
                ++it;
            }
        }
        if(changed) {
            saveItemPositions();
        }
    }
    listView_->setUpdatesEnabled(false);
    queueRelayout(100);
}

void DesktopWindow::onLayoutChanged() {
    queueRelayout();
}

void DesktopWindow::onModelSortFilterChanged() {
    Settings& settings = static_cast<Application*>(qApp)->settings();
    settings.setDesktopSortColumn(static_cast<Fm::FolderModel::ColumnId>(proxyModel_->sortColumn()));
    settings.setDesktopSortOrder(proxyModel_->sortOrder());
    settings.setDesktopSortFolderFirst(proxyModel_->folderFirst());
    settings.setDesktopSortHiddenLast(proxyModel_->hiddenLast());
}

void DesktopWindow::onFolderStartLoading() { // desktop may be reloaded
    if(model_) {
        disconnect(model_, &Fm::FolderModel::filesAdded, this, &DesktopWindow::onFilesAdded);
    }
}

void DesktopWindow::onFolderFinishLoading() {
    QTimer::singleShot(10, [this]() { // Qt delays the UI update (as in TabPage::onFolderFinishLoading)
        if(model_) {
            connect(model_, &Fm::FolderModel::filesAdded, this, &DesktopWindow::onFilesAdded);
        }
    });
}

void DesktopWindow::onFilesAdded(const Fm::FileInfoList files) {
    if(static_cast<Application*>(qApp)->settings().selectNewFiles()) {
        if(!selectionTimer_) {
            selectFiles(files, false);
            selectionTimer_ = new QTimer (this);
            selectionTimer_->setSingleShot(true);
            selectionTimer_->start(200);
        }
        else {
            selectFiles(files, selectionTimer_->isActive());
            selectionTimer_->start(200);
        }
    }
}

void DesktopWindow::removeBottomGap() {
    auto screen = getDesktopScreen();
    if(screen == nullptr) {
        return;
    }
    /************************************************************
     NOTE: Desktop is an area bounded from below while icons snap
     to its grid srarting from above. Therefore, we try to adjust
     the vertical cell margin to prevent relatively large gaps
     from taking shape at the desktop bottom.
     ************************************************************/
    auto delegate = static_cast<Fm::FolderItemDelegate*>(listView_->itemDelegateForColumn(0));
    auto itemSize = delegate->itemSize();
    //qDebug() << "delegate:" << delegate->itemSize();
    QSize cellMargins = getMargins();
    int workAreaHeight = screen->availableVirtualGeometry().height()
                         - 2 * WORK_AREA_MARGIN;
    int cellHeight = itemSize.height() + listView_->spacing();
    int iconNumber = workAreaHeight / cellHeight;
    int bottomGap = workAreaHeight % cellHeight;
    /*******************************************
     First try to make room for an extra icon...
     *******************************************/
    // If one pixel is subtracted from the vertical margin, cellHeight
    // will decrease by 2 while bottomGap will increase by 2*iconNumber.
    // So, we can add an icon to the bottom once this inequality holds:
    // bottomGap + 2*n*iconNumber >= cellHeight - 2*n
    // From here, we get our "subtrahend":
    qreal exactNumber = (static_cast<qreal>(cellHeight) - static_cast<qreal>(bottomGap))
                        / (2 * static_cast<qreal>(iconNumber) + static_cast<qreal>(2));
    int subtrahend = (int)exactNumber + ((int)exactNumber == exactNumber ? 0 : 1);
    Settings& settings = static_cast<Application*>(qApp)->settings();
    int minCellHeight = settings.desktopCellMargins().height();
    if(subtrahend > 0
            && cellMargins.height() - subtrahend >= minCellHeight) {
        cellMargins -= QSize(0, subtrahend);
    }
    /***************************************************
     ... but if that can't be done, try to spread icons!
     ***************************************************/
    else {
        cellMargins += QSize(0, (bottomGap / iconNumber) / 2);
    }
    // set the new margins (if they're changed)
    delegate->setMargins(cellMargins);
    setMargins(cellMargins);
    // in case the text shadow is reset to (0,0,0,0)
    setShadow(settings.desktopShadowColor());
}

void DesktopWindow::paintBackground(QPaintEvent* event) {
    // This is to workaround Qt bug 54384 which affects Qt >= 5.6
    // https://bugreports.qt.io/browse/QTBUG-54384
    QPainter painter(this);
    if(wallpaperMode_ == WallpaperNone || wallpaperPixmap_.isNull()) {
        painter.fillRect(event->rect(), QBrush(bgColor_));
    }
    else {
        painter.drawPixmap(event->rect(), wallpaperPixmap_, event->rect());
    }
}

void DesktopWindow::trustOurDesktopShortcut(std::shared_ptr<const Fm::FileInfo> file) {
    if(file->isTrustable()) {
        return;
    }
    Settings& settings = static_cast<Application*>(qApp)->settings();
    const QStringList ds = settings.desktopShortcuts();
    if(ds.isEmpty()) {
        return;
    }
    const QString fileName = QString::fromStdString(file->name());
    auto homeExec = Fm::CStrPtr(g_strconcat("pcmanfm-qt ", Fm::FilePath::homeDir().toString().get(), nullptr));
    const char* execStr = fileName == QLatin1String("trash-can.desktop") && ds.contains(QLatin1String("Trash")) ? "pcmanfm-qt trash:///" :
                          fileName == QLatin1String("user-home.desktop") && ds.contains(QLatin1String("Home")) ? homeExec.get() :
                          fileName == QLatin1String("computer.desktop") && ds.contains(QLatin1String("Computer")) ? "pcmanfm-qt computer:///" :
                          fileName == QLatin1String("network.desktop") && ds.contains(QLatin1String("Network")) ? "pcmanfm-qt network:///" : nullptr;
    if(execStr) {
        GKeyFile* kf = g_key_file_new();
        if(g_key_file_load_from_file(kf, file->path().toString().get(), G_KEY_FILE_NONE, nullptr)) {
            Fm::CStrPtr str{g_key_file_get_string(kf, "Desktop Entry", "Exec", nullptr)};
            if(str && strcmp(str.get(), execStr) == 0) {
                file->setTrustable(true);
            }
        }
        g_key_file_free(kf);
    }
}

// QListView does item layout in a very inflexible way, so let's do our custom layout again.
// FIXME: this is very inefficient, but due to the design flaw of QListView, this is currently the only workaround.
void DesktopWindow::relayoutItems() {
    auto screen = getDesktopScreen();
    if(screen == nullptr) {
        return;
    }
    loadItemPositions(); // something may have changed
    // qDebug("relayoutItems()");
    if(relayoutTimer_) {
        // this slot might be called from the timer, so we cannot delete it directly here.
        relayoutTimer_->deleteLater();
        relayoutTimer_ = nullptr;
    }

    int row = 0;
    int rowCount = proxyModel_->rowCount();

    auto delegate = static_cast<Fm::FolderItemDelegate*>(listView_->itemDelegateForColumn(0));
    auto itemSize = delegate->itemSize();

    QRect workArea = screen->availableVirtualGeometry();
    workArea.adjust(WORK_AREA_MARGIN, WORK_AREA_MARGIN, -WORK_AREA_MARGIN, -WORK_AREA_MARGIN);
    // qDebug() << "workArea" << screenNum_ <<  workArea;
    // FIXME: we use an internal class declared in a private header here, which is pretty bad.
    QPoint pos = workArea.topLeft();
    for(; row < rowCount; ++row) {
        QModelIndex index = proxyModel_->index(row, 0);
        int itemWidth = delegate->sizeHint(listView_->getViewOptions(), index).width();
        auto file = proxyModel_->fileInfoFromIndex(index);
        auto name = file->name();
        if(filesToTrust_.contains(QString::fromStdString(name))) {
            file->setTrustable(true);
            filesToTrust_.removeAll(QString::fromStdString(name));
        }
        else if(file->isDesktopEntry()) {
            trustOurDesktopShortcut(file);
         }
        auto find_it = customItemPos_.find(name);
        if(find_it != customItemPos_.cend()) { // the item has a custom position
            QPoint customPos = find_it->second;
            // center the contents vertically
            listView_->setPositionForIndex(customPos + QPoint((itemSize.width() - itemWidth) / 2, 0), index);
            // qDebug() << "set custom pos:" << name << row << index << customPos;
            continue;
        }
        // check if the current pos is already occupied by a custom item
        bool used = false;
        for(auto it = customItemPos_.cbegin(); it != customItemPos_.cend(); ++it) {
            QPoint customPos = it->second;
            if(QRect(customPos, itemSize).contains(pos)) {
                used = true;
                break;
            }
        }
        if(used) { // go to next pos
            --row;
        }
        else {
            // center the contents vertically
            listView_->setPositionForIndex(pos + QPoint((itemSize.width() - itemWidth) / 2, 0), index);
            // qDebug() << "set pos" << name << row << index << pos;
        }
        // move to next cell in the column
        pos.setY(pos.y() + itemSize.height() + listView_->spacing());
        if(pos.y() + itemSize.height() > workArea.bottom() + 1) {
            // if the next position may exceed the bottom of work area, go to the top of next column
            pos.setX(pos.x() + itemSize.width() + listView_->spacing());
            pos.setY(workArea.top());
        }
    }

    if(!listView_->updatesEnabled()) {
        listView_->setUpdatesEnabled(true);
    }
}

void DesktopWindow::loadItemPositions() {
    auto screen = getDesktopScreen();
    if(screen == nullptr) {
        return;
    }
    // load custom item positions
    customItemPos_.clear();
    Settings& settings = static_cast<Application*>(qApp)->settings();
    QString configFile = QStringLiteral("%1/desktop-items-%2.conf").arg(settings.profileDir(settings.profileName())).arg(screenNum_);
    QSettings file(configFile, QSettings::IniFormat);

    auto delegate = static_cast<Fm::FolderItemDelegate*>(listView_->itemDelegateForColumn(0));
    auto grid = delegate->itemSize();
    QRect workArea = screen->availableVirtualGeometry();
    workArea.adjust(WORK_AREA_MARGIN, WORK_AREA_MARGIN, -WORK_AREA_MARGIN, -WORK_AREA_MARGIN);
    QString desktopDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    desktopDir += QLatin1Char('/');
    std::vector<QPoint> usedPos;
    for(auto& item: customItemPos_) {
        usedPos.push_back(item.second);
    }

    // FIXME: this is inefficient
    const auto names = file.childGroups();
    for(const QString& name : names) {
        if(!QFile::exists(desktopDir + name)) {
            // the file may have been removed from outside LXQT
            continue;
        }
        file.beginGroup(name);
        QVariant var = file.value(QStringLiteral("pos"));
        if(var.isValid()) {
            QPoint customPos = var.toPoint();
            if(customPos.x() >= workArea.x() && customPos.y() >= workArea.y()
                    && customPos.x() + grid.width() <= workArea.right() + 1
                    && customPos.y() + grid.height() <= workArea.bottom() + 1) {
                // correct positions that are't aligned to the grid
                alignToGrid(customPos, workArea.topLeft(), grid, listView_->spacing());
                // FIXME: this is very inefficient
                while(std::find(usedPos.cbegin(), usedPos.cend(), customPos) != usedPos.cend()) {
                    customPos.setY(customPos.y() + grid.height() + listView_->spacing());
                    if(customPos.y() + grid.height() > workArea.bottom() + 1) {
                        customPos.setX(customPos.x() + grid.width() + listView_->spacing());
                        customPos.setY(workArea.top());
                    }
                }
                customItemPos_[name.toStdString()] = customPos;
                usedPos.push_back(customPos);
            }
        }
        file.endGroup();
    }
}

void DesktopWindow::saveItemPositions() {
    Settings& settings = static_cast<Application*>(qApp)->settings();
    // store custom item positions
    QString configFile = QStringLiteral("%1/desktop-items-%2.conf").arg(settings.profileDir(settings.profileName())).arg(screenNum_);
    // FIXME: using QSettings here is inefficient and it's not friendly to UTF-8.
    QSettings file(configFile, QSettings::IniFormat);
    file.clear(); // remove all existing entries

    // FIXME: we have to remove dead entries not associated to any files?
    for(auto it = customItemPos_.cbegin(); it != customItemPos_.cend(); ++it) {
        auto& name = it->first;
        auto& pos = it->second;
        file.beginGroup(QString::fromStdString(name));
        file.setValue(QStringLiteral("pos"), pos);
        file.endGroup();
    }
}

void DesktopWindow::onStickToCurrentPos(bool toggled) {
    QModelIndexList indexes = listView_->selectionModel()->selectedIndexes();
    if(!indexes.isEmpty()) {
        bool relayout(false);
        QModelIndexList::const_iterator it;
        for(it = indexes.constBegin(); it != indexes.constEnd(); ++it) {
            auto file = proxyModel_->fileInfoFromIndex(*it);
            auto name = file->name();
            if(toggled) { // remember the current custom position
                QRect itemRect = listView_->rectForIndex(*it);
                customItemPos_[name] = itemRect.topLeft();
            }
            else { // cancel custom position and perform relayout
                auto item = customItemPos_.find(name);
                if(item != customItemPos_.end()) {
                    customItemPos_.erase(item);
                    relayout = true;
                }
            }
        }
        saveItemPositions();
        if(relayout) {
            relayoutItems();
        }
    }
}

void DesktopWindow::queueRelayout(int delay) {
    // qDebug() << "queueRelayout";
    removeBottomGap();
    if(!relayoutTimer_) {
        relayoutTimer_ = new QTimer();
        relayoutTimer_->setSingleShot(true);
        connect(relayoutTimer_, &QTimer::timeout, this, &DesktopWindow::relayoutItems);
        relayoutTimer_->start(delay);
    }
}

// slots for file operations

void DesktopWindow::onCutActivated() {
    if(desktopHideItems_) {
        return;
    }
    auto paths = selectedFilePaths();
    if(!paths.empty()) {
        Fm::cutFilesToClipboard(paths);
    }
}

void DesktopWindow::onCopyActivated() {
    if(desktopHideItems_) {
        return;
    }
    auto paths = selectedFilePaths();
    if(!paths.empty()) {
        Fm::copyFilesToClipboard(paths);
    }
}

void DesktopWindow::onCopyFullPathActivated() {
    if(desktopHideItems_) {
        return;
    }
    auto paths = selectedFilePaths();
    if(paths.size() == 1) {
        QApplication::clipboard()->setText(QString::fromUtf8(paths.front().toString().get()), QClipboard::Clipboard);
    }
}

void DesktopWindow::onPasteActivated() {
    if(desktopHideItems_) {
        return;
    }
    Fm::pasteFilesFromClipboard(path());
}

void DesktopWindow::onDeleteActivated() {
    if(desktopHideItems_) {
        return;
    }
    auto paths = selectedFilePaths();
    if(!paths.empty()) {
        Settings& settings = static_cast<Application*>(qApp)->settings();
        bool shiftPressed = (qApp->keyboardModifiers() & Qt::ShiftModifier ? true : false);
        if(settings.useTrash() && !shiftPressed) {
            Fm::FileOperation::trashFiles(paths, settings.confirmTrash(), this);
        }
        else {
            Fm::FileOperation::deleteFiles(paths, settings.confirmDelete(), this);
        }
    }
}

void DesktopWindow::onRenameActivated() {
    if(desktopHideItems_) {
        return;
    }
    // do inline renaming if only one item is selected,
    // otherwise use the renaming dialog
    if(selectedIndexes().size() == 1) {
        QModelIndex cur = listView_->currentIndex();
        if (cur.isValid()) {
            listView_->edit(cur);
            return;
        }
    }
    auto files = selectedFiles();
    if(!files.empty()) {
        for(auto& info: files) {
            if(!Fm::renameFile(info, nullptr)) {
                break;
            }
        }
     }
}

void DesktopWindow::onBulkRenameActivated() {
    if(desktopHideItems_) {
        return;
    }
    BulkRenamer(selectedFiles(), this);
}

void DesktopWindow::onFilePropertiesActivated() {
    if(desktopHideItems_) {
        return;
    }
    auto files = selectedFiles();
    if(!files.empty()) {
        Fm::FilePropsDialog::showForFiles(std::move(files));
    }
}

bool DesktopWindow::event(QEvent* event) {
    switch(event->type()) {
    case QEvent::WinIdChange: {
        //qDebug() << "winid change:" << effectiveWinId();
        if(effectiveWinId() == 0) {
            break;
        }
        // set freedesktop.org EWMH hints properly
        if(QX11Info::isPlatformX11() && QX11Info::connection()) {
            xcb_connection_t* con = QX11Info::connection();
            const char* atom_name = "_NET_WM_WINDOW_TYPE_DESKTOP";
            xcb_atom_t atom = xcb_intern_atom_reply(con, xcb_intern_atom(con, 0, strlen(atom_name), atom_name), nullptr)->atom;
            const char* prop_atom_name = "_NET_WM_WINDOW_TYPE";
            xcb_atom_t prop_atom = xcb_intern_atom_reply(con, xcb_intern_atom(con, 0, strlen(prop_atom_name), prop_atom_name), nullptr)->atom;
            xcb_atom_t XA_ATOM = 4;
            xcb_change_property(con, XCB_PROP_MODE_REPLACE, effectiveWinId(), prop_atom, XA_ATOM, 32, 1, &atom);
        }
        break;
    }
#undef FontChange // FontChange is defined in the headers of XLib and clashes with Qt, let's undefine it.
    case QEvent::StyleChange:
    case QEvent::FontChange:
        queueRelayout();
        break;

    default:
        break;
    }

    return QWidget::event(event);
}

#undef FontChange // this seems to be defined in Xlib headers as a macro, undef it!

bool DesktopWindow::eventFilter(QObject* watched, QEvent* event) {
    if(watched == listView_) {
        switch(event->type()) {
        case QEvent::StyleChange:
        case QEvent::FontChange:
            if(model_) {
                queueRelayout();
            }
            break;
        default:
            break;
        }
    }
    else if(watched == listView_->viewport()) {
        switch(event->type()) {
        case QEvent::Paint:
            // NOTE: The drop indicator isn't drawn/updated automatically, perhaps,
            // because we paint desktop ourself. So, we draw it here.
            paintDropIndicator();
            break;
        case QEvent::Wheel:
            // removal of scrollbars is not enough to prevent scrolling
            return true;
        default:
            break;
        }
    }
    return Fm::FolderView::eventFilter(watched, event);
}

void DesktopWindow::childDragMoveEvent(QDragMoveEvent* e) { // see DesktopWindow::eventFilter for an explanation
    auto screen = getDesktopScreen();
    if(screen == nullptr) {
        return;
    }
    auto delegate = static_cast<Fm::FolderItemDelegate*>(listView_->itemDelegateForColumn(0));
    auto grid = delegate->itemSize();
    QRect workArea = screen->availableVirtualGeometry();
    workArea.adjust(WORK_AREA_MARGIN, WORK_AREA_MARGIN, -WORK_AREA_MARGIN, -WORK_AREA_MARGIN);
    bool isTrash;
    QRect oldDropRect = dropRect_;
    dropRect_ = QRect();
    QModelIndex dropIndex = indexForPos(&isTrash, e->pos(), workArea, grid);
    if(dropIndex.isValid()) {
        bool dragOnSelf = false;
        if(e->source() == listView_ && e->keyboardModifiers() == Qt::NoModifier) { // drag source is desktop
            QModelIndex curIndx = listView_->currentIndex();
            if(curIndx.isValid() && curIndx == dropIndex) {
                dragOnSelf = true;
            }
        }
        if(!dragOnSelf) {
            if(isTrash) {
                dropRect_ = listView_->rectForIndex(dropIndex);
            }
            else if(dropIndex.model()) {
                QVariant data = dropIndex.model()->data(dropIndex, Fm::FolderModel::Role::FileInfoRole);
                auto info = data.value<std::shared_ptr<const Fm::FileInfo>>();
                if(info && info->isDir()) {
                    dropRect_ = listView_->rectForIndex(dropIndex);
                }
            }
        }
    }
    if(oldDropRect != dropRect_) {
        listView_->viewport()->update();
    }
}

void DesktopWindow::paintDropIndicator()
{
    if(!dropRect_.isNull()) {
        QPainter painter(listView_->viewport());
        QStyleOption opt;
        opt.init(listView_->viewport());
        opt.rect = dropRect_;
        style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &opt, &painter, listView_);
    }
}

void DesktopWindow::childDropEvent(QDropEvent* e) {
    auto screen = getDesktopScreen();
    if(screen == nullptr) {
        return;
    }
    auto delegate = static_cast<Fm::FolderItemDelegate*>(listView_->itemDelegateForColumn(0));
    auto grid = delegate->itemSize();
    QRect workArea = screen->availableVirtualGeometry();
    workArea.adjust(WORK_AREA_MARGIN, WORK_AREA_MARGIN, -WORK_AREA_MARGIN, -WORK_AREA_MARGIN);

    const QMimeData* mimeData = e->mimeData();
    bool moveItem = false;
    QModelIndex curIndx = listView_->currentIndex();
    if(e->source() == listView_ && e->keyboardModifiers() == Qt::NoModifier) {
        // drag source is our list view, and no other modifier keys are pressed
        // => we're dragging desktop items
        if(mimeData->hasFormat(QStringLiteral("application/x-qabstractitemmodeldatalist"))) {
            bool isTrash;
            QModelIndex dropIndex = indexForPos(&isTrash, e->pos(), workArea, grid);
            if(dropIndex.isValid() // drop on an item
               && curIndx.isValid() && curIndx != dropIndex) { // not a drop on self
                if(auto file = proxyModel_->fileInfoFromIndex(dropIndex)) {
                    if(!file->isDir()) { // drop on a non-directory file
                        // if the files are dropped on our Trash shortcut cell,
                        // move them to Trash instead of moving them on desktop
                        if(isTrash) {
                            auto paths = selectedFilePaths();
                            if(!paths.empty()) {
                                e->accept();
                                Settings& settings = static_cast<Application*>(qApp)->settings();
                                Fm::FileOperation::trashFiles(paths, settings.confirmTrash(), this);
                                // remove the drop indicator after the drop is finished
                                QTimer::singleShot(0, listView_, [this] {
                                    dropRect_ = QRect();
                                    listView_->viewport()->update();
                                });
                                return;
                            }
                        }
                        moveItem = true;
                    }
                }
            }
            else { // drop on a blank area (maybe, between other items)
                moveItem = true;
            }
        }
    }
    if(moveItem) {
        e->accept();
        // move selected items to the drop position, preserving their relative positions
        const QPoint dropPos = e->pos();
        if(curIndx.isValid()) {
            QPoint curPoint = listView_->visualRect(curIndx).topLeft();

            // first move the current item to the drop position
            auto file = proxyModel_->fileInfoFromIndex(curIndx);
            if(file) {
                QPoint pos = dropPos;
                stickToPosition(file->name(), pos, workArea, grid);
            }

            // then move the other items so that their relative postions are preserved
            const QModelIndexList selected = selectedIndexes();
            for(const QModelIndex& indx : selected) {
                if(indx == curIndx) {
                    continue;
                }
                file = proxyModel_->fileInfoFromIndex(indx);
                if(file) {
                    QPoint nxtDropPos = dropPos + listView_->visualRect(indx).topLeft() - curPoint;
                    nxtDropPos.setX(qBound(workArea.left(), nxtDropPos.x(), workArea.right() + 1));
                    nxtDropPos.setY(qBound(workArea.top(), nxtDropPos.y(), workArea.bottom() + 1));
                    stickToPosition(file->name(), nxtDropPos, workArea, grid);
                }
            }
        }
        saveItemPositions();
        queueRelayout();
    }
    else {
        // move items to Trash if they are dropped on Trash cell
        bool isTrash;
        QModelIndex dropIndex = indexForPos(&isTrash, e->pos(), workArea, grid);
        if(dropIndex.isValid() && isTrash) {
            if(mimeData->hasUrls()) {
                Fm::FilePathList paths;
                const QList<QUrl> urlList = mimeData->urls();
                for(const QUrl& url : urlList) {
                    QString uri = url.toDisplayString();
                    if(!uri.isEmpty()) {
                        paths.push_back(Fm::FilePath::fromUri(uri.toStdString().c_str()));
                    }
                }
                if(!paths.empty()) {
                    e->accept();
                    Settings& settings = static_cast<Application*>(qApp)->settings();
                    Fm::FileOperation::trashFiles(paths, settings.confirmTrash(), this);
                    // remove the drop indicator after the drop is finished
                    QTimer::singleShot(0, listView_, [this] {
                        dropRect_ = QRect();
                        listView_->viewport()->update();
                    });
                    return;
                }
            }
        }

        Fm::FolderView::childDropEvent(e);

        // remove the drop indicator after the drop is finished
        dropRect_ = QRect();
        listView_->viewport()->update();

        // position dropped items successively, starting with the drop rectangle
        if(mimeData->hasUrls()
           && (e->dropAction() == Qt::CopyAction
               || e->dropAction() == Qt::MoveAction
               || e->dropAction() == Qt::LinkAction)) {
            const QString desktopDir = XdgDir::readDesktopDir() + QString(QLatin1String("/"));
            QPoint dropPos = e->pos();
            const QList<QUrl> urlList = mimeData->urls();
            bool reachedLastCell = false;
            for(const QUrl& url : urlList) {
                QString name = url.fileName();
                if(!name.isEmpty()
                   // don't stick to the position if there is an overwrite prompt
                   && !QFile::exists(desktopDir + name)) {
                    reachedLastCell = stickToPosition(name.toStdString(), dropPos, workArea, grid, reachedLastCell);
                }
            }
            saveItemPositions();
        }
    }
}

// NOTE: This function positions items from top to bottom and left to right,
// starting from the drop point, and carries the existing sticky items with them,
// until it reaches the last cell and then puts the remaining items in the opposite
// direction. In this way, it creates a natural DND, especially with multiple files.
bool DesktopWindow::stickToPosition(const std::string& file, QPoint& pos, const QRect& workArea, const QSize& grid, bool reachedLastCell) {
    // normalize the position, depending on the positioning direction
    if(!reachedLastCell) { // default direction: top -> bottom, left -> right

        // put the drop point inside the work area to prevent unnatural jumps
        if(pos.y() + grid.height() > workArea.bottom() + 1) {
            pos.setY(workArea.bottom() + 1 - grid.height());
        }
        if(pos.x() + grid.width() > workArea.right() + 1) {
            pos.setX(workArea.right() + 1 - grid.width());
        }
        pos.setX(qMax(workArea.left(), pos.x()));
        pos.setY(qMax(workArea.top(), pos.y()));

        alignToGrid(pos, workArea.topLeft(), grid, listView_->spacing());
    }
    else { // backward direction: bottom -> top, right -> left
        if(pos.y() < workArea.top()) {
            // reached the top; go to the left bottom
            pos.setY(workArea.bottom() + 1 - grid.height());
            pos.setX(pos.x() - grid.width() - listView_->spacing());
        }

        alignToGrid(pos, workArea.topLeft(), grid, listView_->spacing());

        if (pos.x() < workArea.left()) {
            // there's no space to the left, which means that
            // the work area is exhausted, so ignore stickiness
            return reachedLastCell;
        }
    }

    // find if there is a sticky item at this position
    std::string otherFile;
    bool isTrash = false;
    auto oldItem = std::find_if(customItemPos_.cbegin(),
                                customItemPos_.cend(),
                                [pos](const std::pair<std::string, QPoint>& elem) {
                                    return elem.second == pos;
                                });
    if(oldItem != customItemPos_.cend()) {
        otherFile = oldItem->first;
        if(trashMonitor_ && otherFile != file && otherFile == "trash-can.desktop") {
            isTrash = true; // occupied by a sticky Trash
        }
    }

    if(!isTrash) {
        // stick to the position if it is not occupied by Trash
        // NOTE: In this way, a sticky Trash can be moved only explicitly.
        customItemPos_[file] = pos;
    }

    // check whether we are in the last visible cell if it isn't reached already
    if(!reachedLastCell
       && pos.y() + 2 * grid.height() + listView_->spacing() > workArea.bottom() + 1
       && pos.x() + 2 * grid.width() + listView_->spacing() > workArea.right() + 1) {
        reachedLastCell = true;
    }

    // find the next position
    if(reachedLastCell) {
        // when this is the last visible cell, reverse the positioning direction
        // to avoid off-screen items later
        pos.setY(pos.y() - grid.height() - listView_->spacing());
    }
    else {
        // the last visible cell is not reached yet; go forward
        if(pos.y() + 2 * grid.height() + listView_->spacing() > workArea.bottom() + 1) {
            pos.setY(workArea.top());
            pos.setX(pos.x() + grid.width() + listView_->spacing());
        }
        else {
            pos.setY(pos.y() + grid.height() + listView_->spacing());
        }
    }

    // if the position was ocupied by Trash, go to the next postiton
    if(isTrash) {
        reachedLastCell = stickToPosition(file, pos, workArea, grid, reachedLastCell);
    }
    // but if there was another sticky item at the same position, move it to the next position
    else if(!otherFile.empty() && otherFile != file) {
        reachedLastCell = stickToPosition(otherFile, pos, workArea, grid, reachedLastCell);
    }

    return reachedLastCell;
}

void DesktopWindow::alignToGrid(QPoint& pos, const QPoint& topLeft, const QSize& grid, const int spacing) {
    int w = (pos.x() - topLeft.x()) / (grid.width() + spacing); // can be negative with DND
    int h = (pos.y() - topLeft.y()) / (grid.height() + spacing); // can be negative with DND
    pos.setX(topLeft.x() + w * (grid.width() + spacing));
    pos.setY(topLeft.y() + h * (grid.height() + spacing));
}

// "FolderViewListView::indexAt()" finds the index only when the point is inside
// the text or icon rectangle but we make an exception for Trash because we want
// to trash dropped items once the drop point is inside the Trash cell.
QModelIndex DesktopWindow::indexForPos(bool* isTrash, const QPoint& pos, const QRect& workArea, const QSize& grid) const {
    // first normalize the position
    QPoint p(pos);
    if(p.y() + grid.height() > workArea.bottom() + 1) {
        p.setY(workArea.bottom() + 1 - grid.height());
    }
    if(p.x() + grid.width() > workArea.right() + 1) {
        p.setX(workArea.right() + 1 - grid.width());
    }
    p.setX(qMax(workArea.left(), p.x()));
    p.setY(qMax(workArea.top(), p.y()));
    alignToGrid(p, workArea.topLeft(), grid, listView_->spacing());
    // then put the point where the icon rectangle may be
    // (if there is any item, its icon is immediately below the middle of the top side)
    p.setX(p.x() + (grid.width() + listView_->spacing()) / 2);
    p.setY(p.y() + listView_->spacing() / 2 + 1);
    // if this is the Trash cell, return its index
    QModelIndex indx = listView_->indexAt(p);
    if(indx.isValid() && indx.model()) {
        QVariant data = indx.model()->data(indx, Fm::FolderModel::Role::FileInfoRole);
        auto info = data.value<std::shared_ptr<const Fm::FileInfo>>();
        if(isTrashCan(info)) {
            *isTrash = true;
            return indx;
        }
    }
    // if this is not the Trash cell, find the index in the usual way
    *isTrash = false;
    return listView_->indexAt(pos);
}

void DesktopWindow::closeEvent(QCloseEvent* event) {
    // prevent the desktop window from being closed.
    event->ignore();
}

void DesktopWindow::paintEvent(QPaintEvent* event) {
    paintBackground(event);
    QWidget::paintEvent(event);
}

void DesktopWindow::setScreenNum(int num) {
    if(screenNum_ != num) {
        screenNum_ = num;
        queueRelayout();
    }
}

QScreen* DesktopWindow::getDesktopScreen() const {
    QScreen* desktopScreen = nullptr;
    if(screenNum_ == -1) {
        desktopScreen = qApp->primaryScreen();
    }
    else {
        const auto allScreens = qApp->screens();
        if(allScreens.size() > screenNum_) {
            desktopScreen = allScreens.at(screenNum_);
        }
        if(desktopScreen == nullptr && windowHandle()) {
            desktopScreen = windowHandle()->screen();
        }
    }
    return desktopScreen;
}

} // namespace PCManFM
