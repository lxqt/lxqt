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

#include "mainwindow.h"
#include <QActionGroup>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QClipboard>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QScreen>
#include <QShortcut>
#include <QDockWidget>
#include <QScrollBar>
#include <QGraphicsSvgItem>
#include <QHeaderView>
#include <QStandardPaths>
#include <QDateTime>
#include <QX11Info>

#include "application.h"
#include <libfm-qt/folderview.h>
#include <libfm-qt/filepropsdialog.h>
#include <libfm-qt/fileoperation.h>
#include <libfm-qt/folderitemdelegate.h>

#include "mrumenu.h"
#include "resizeimagedialog.h"
#include "upload/uploaddialog.h"

using namespace LxImage;

MainWindow::MainWindow():
  QMainWindow(),
  contextMenu_(new QMenu(this)),
  slideShowTimer_(nullptr),
  image_(),
  // currentFileInfo_(nullptr),
  imageModified_(false),
  startMaximized_(false),
  folder_(nullptr),
  folderModel_(new Fm::FolderModel()),
  proxyModel_(new Fm::ProxyFolderModel()),
  modelFilter_(new ModelFilter()),
  thumbnailsDock_(nullptr),
  thumbnailsView_(nullptr),
  loadJob_(nullptr),
  saveJob_(nullptr),
  fileMenu_(nullptr) {

  setAttribute(Qt::WA_DeleteOnClose); // FIXME: check if current image is saved before close

  Application* app = static_cast<Application*>(qApp);
  app->addWindow();

  Settings& settings = app->settings();

  ui.setupUi(this);
  if(QX11Info::isPlatformX11()) {
    connect(ui.actionScreenshot, &QAction::triggered, app, &Application::screenshot);
  }
  else {
    ui.actionScreenshot->setVisible(false);
  }
  connect(ui.actionPreferences, &QAction::triggered, app , &Application::editPreferences);

  proxyModel_->addFilter(modelFilter_);
  proxyModel_->sort(Fm::FolderModel::ColumnFileName, Qt::AscendingOrder);
  proxyModel_->setSourceModel(folderModel_);

  // build context menu
  ui.view->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui.view, &QWidget::customContextMenuRequested, this, &MainWindow::onContextMenu);

  connect(ui.view, &ImageView::fileDropped, this, &MainWindow::onFileDropped);

  connect(ui.view, &ImageView::zooming, this, &MainWindow::onZooming);

  // install an event filter on the image view
  ui.view->installEventFilter(this);
  ui.view->setBackgroundBrush(QBrush(settings.bgColor()));
  ui.view->updateOutline();
  ui.view->showOutline(settings.isOutlineShown());
  ui.actionShowOutline->setChecked(settings.isOutlineShown());

  if(settings.showThumbnails())
    setShowThumbnails(true);

  ui.annotationsToolBar->setVisible(settings.isAnnotationsToolbarShown());
  ui.actionAnnotations->setChecked(settings.isAnnotationsToolbarShown());
  connect(ui.actionAnnotations, &QAction::triggered, [this](int checked) {
    if(!isFullScreen()) { // annotations toolbar is hidden in fullscreen
      ui.annotationsToolBar->setVisible(checked);
    }
  });
  // annotations toolbar visibility can change in its context menu
  connect(ui.annotationsToolBar, &QToolBar::visibilityChanged, [this](int visible) {
    if(!isFullScreen()) { // annotations toolbar is hidden in fullscreen
      ui.actionAnnotations->setChecked(visible);
    }
  });

  auto aGroup = new QActionGroup(this);
  ui.actionZoomFit->setActionGroup(aGroup);
  ui.actionOriginalSize->setActionGroup(aGroup);
  ui.actionZoomFit->setChecked(true);

  contextMenu_->addAction(ui.actionPrevious);
  contextMenu_->addAction(ui.actionNext);
  contextMenu_->addSeparator();
  contextMenu_->addAction(ui.actionZoomOut);
  contextMenu_->addAction(ui.actionZoomIn);
  contextMenu_->addAction(ui.actionOriginalSize);
  contextMenu_->addAction(ui.actionZoomFit);
  contextMenu_->addSeparator();
  contextMenu_->addAction(ui.actionSlideShow);
  contextMenu_->addAction(ui.actionFullScreen);
  contextMenu_->addAction(ui.actionShowOutline);
  contextMenu_->addAction(ui.actionAnnotations);
  contextMenu_->addSeparator();
  contextMenu_->addAction(ui.actionRotateClockwise);
  contextMenu_->addAction(ui.actionRotateCounterclockwise);
  contextMenu_->addAction(ui.actionFlipHorizontal);
  contextMenu_->addAction(ui.actionFlipVertical);
  contextMenu_->addAction(ui.actionFlipVertical);

  // Open images when MRU items are clicked
  ui.menuRecently_Opened_Files->setMaxItems(settings.maxRecentFiles());
  connect(ui.menuRecently_Opened_Files, &MruMenu::itemClicked, this, &MainWindow::onFileDropped);

  // Create an action group for the annotation tools
  QActionGroup *annotationGroup = new QActionGroup(this);
  annotationGroup->addAction(ui.actionDrawNone);
  annotationGroup->addAction(ui.actionDrawArrow);
  annotationGroup->addAction(ui.actionDrawRectangle);
  annotationGroup->addAction(ui.actionDrawCircle);
  annotationGroup->addAction(ui.actionDrawNumber);
  ui.actionDrawNone->setChecked(true);

  // the "Open With..." menu
  connect(ui.menu_File, &QMenu::aboutToShow, this, &MainWindow::fileMenuAboutToShow);
  connect(ui.openWithMenu, &QMenu::aboutToShow, this, &MainWindow::createOpenWithMenu);
  connect(ui.openWithMenu, &QMenu::aboutToHide, this, &MainWindow::deleteOpenWithMenu);

  // create keyboard shortcuts
  QShortcut* shortcut = new QShortcut(Qt::Key_Left, this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionPrevious_triggered);
  shortcut = new QShortcut(Qt::Key_Backspace, this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionPrevious_triggered);
  shortcut = new QShortcut(Qt::Key_Right, this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionNext_triggered);
  shortcut = new QShortcut(Qt::Key_Space, this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionNext_triggered);
  shortcut = new QShortcut(Qt::Key_Escape, this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::onKeyboardEscape);
}

MainWindow::~MainWindow() {
  delete slideShowTimer_;
  delete thumbnailsView_;
  delete thumbnailsDock_;

  if(loadJob_) {
    loadJob_->cancel();
    // we don't need to do delete here. It will be done automatically
  }
  //if(currentFileInfo_)
  //  fm_file_info_unref(currentFileInfo_);
  delete folderModel_;
  delete proxyModel_;
  delete modelFilter_;

  Application* app = static_cast<Application*>(qApp);
  app->removeWindow();
}

void MainWindow::on_actionAbout_triggered() {
  QMessageBox::about(this, tr("About"),
                     tr("LXImage-Qt - a simple and fast image viewer\n\n"
                     "Copyright (C) 2013\n"
                     "LXQt Project: https://lxqt.org/\n\n"
                     "Authors:\n"
                     "Hong Jen Yee (PCMan) <pcman.tw@gmail.com>"));
}

void MainWindow::on_actionOriginalSize_triggered() {
  if(ui.actionOriginalSize->isChecked()) {
    ui.view->setAutoZoomFit(false);
    ui.view->zoomOriginal();
  }
}

void MainWindow::on_actionZoomFit_triggered() {
  if(ui.actionZoomFit->isChecked()) {
    ui.view->setAutoZoomFit(true);
    ui.view->zoomFit();
  }
}

void MainWindow::on_actionZoomIn_triggered() {
  if(!ui.view->image().isNull()) {
    ui.view->setAutoZoomFit(false);
    ui.view->zoomIn();
  }
}

void MainWindow::on_actionZoomOut_triggered() {
  if(!ui.view->image().isNull()) {
    ui.view->setAutoZoomFit(false);
    ui.view->zoomOut();
  }
}

void MainWindow::onZooming() {
  ui.actionZoomFit->setChecked(false);
  ui.actionOriginalSize->setChecked(false);
}

void MainWindow::on_actionDrawNone_triggered() {
  ui.view->activateTool(ImageView::ToolNone);
}

void MainWindow::on_actionDrawArrow_triggered() {
  ui.view->activateTool(ImageView::ToolArrow);
}

void MainWindow::on_actionDrawRectangle_triggered() {
  ui.view->activateTool(ImageView::ToolRectangle);
}

void MainWindow::on_actionDrawCircle_triggered() {
  ui.view->activateTool(ImageView::ToolCircle);
}

void MainWindow::on_actionDrawNumber_triggered() {
  ui.view->activateTool(ImageView::ToolNumber);
}


void MainWindow::onFolderLoaded() {
  // if currently we're showing a file, get its index in the folder now
  // since the folder is fully loaded.
  if(currentFile_ && !currentIndex_.isValid()) {
    currentIndex_ = indexFromPath(currentFile_);
    if(thumbnailsView_) { // showing thumbnails
      // select current file in the thumbnails view
      thumbnailsView_->childView()->setCurrentIndex(currentIndex_);
      thumbnailsView_->childView()->scrollTo(currentIndex_, QAbstractItemView::EnsureVisible);
    }
  }
  // this is used to open the first image of a folder
  else if (!currentFile_) {
    on_actionFirst_triggered();
  }
}

void MainWindow::openImageFile(const QString& fileName) {
  const Fm::FilePath path = Fm::FilePath::fromPathStr(qPrintable(fileName));
    // the same file! do not load it again
  if(currentFile_ && currentFile_ == path)
    return;

  if (QFileInfo(fileName).isDir()) {
    if(path == folderPath_)
      return;

    QList<QByteArray> formats = QImageReader::supportedImageFormats();
    QStringList formatsFilters;
    for (const QByteArray& format: formats)
      formatsFilters << QStringLiteral("*.") + QString::fromUtf8(format);
    QDir dir(fileName);
    dir.setNameFilters(formatsFilters);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    if(dir.entryList().isEmpty())
      return;

    currentFile_ = Fm::FilePath{};
    loadFolder(path);
  }
  else {
    // load the image file asynchronously
    loadImage(path);
    loadFolder(path.parent());
  }
}

// paste the specified image into the current view,
// reset the window, remove loaded folders, and
// invalidate current file name.
void MainWindow::pasteImage(QImage newImage) {
  // cancel loading of current image
  if(loadJob_) {
    loadJob_->cancel(); // the job object will be freed automatically later
    loadJob_ = nullptr;
  }
  setModified(true);

  currentIndex_ = QModelIndex(); // invaludate current index since we don't have a folder model now
  currentFile_ = Fm::FilePath{};

  image_ = newImage;
  ui.view->setImage(image_);
  // always fit the image on pasting
  ui.actionZoomFit->setChecked(true);
  ui.view->setAutoZoomFit(true);
  ui.view->zoomFit();

  updateUI();
}

// popup a file dialog and retrieve the selected image file name
QString MainWindow::openFileName() {
  QString filterStr;
  QList<QByteArray> formats = QImageReader::supportedImageFormats();
  QList<QByteArray>::iterator it = formats.begin();
  for(;;) {
    filterStr += QLatin1String("*.");
    filterStr += QString::fromUtf8((*it).toLower());
    ++it;
    if(it != formats.end())
      filterStr += QLatin1Char(' ');
    else
      break;
  }

  QString curFileName;
  if(currentFile_) {
    curFileName = QString::fromUtf8(currentFile_.displayName().get());
  }
  else {
    curFileName = QString::fromUtf8(Fm::FilePath::homeDir().toString().get());
  }
  QString fileName = QFileDialog::getOpenFileName(
    this, tr("Open File"), curFileName,
          tr("Image files (%1)").arg(filterStr));
  return fileName;
}

QString MainWindow::openDirectory() {
  QString curDirName;
  if(currentFile_ && currentFile_.parent()) {
    curDirName = QString::fromUtf8(currentFile_.parent().displayName().get());
  }
  else {
    curDirName = QString::fromUtf8(Fm::FilePath::homeDir().toString().get());
  }
  QString directory = QFileDialog::getExistingDirectory(this,
          tr("Open directory"), curDirName);
  return directory;
}

// popup a file dialog and retrieve the selected image file name
QString MainWindow::saveFileName(const QString& defaultName) {
  QString filterStr;
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  QList<QByteArray>::iterator it = formats.begin();
  for(;;) {
    filterStr += QLatin1String("*.");
    filterStr += QString::fromUtf8((*it).toLower());
    ++it;
    if(it != formats.end())
      filterStr += QLatin1Char(' ');
    else
      break;
  }
  // FIXME: should we generate better filter strings? one format per item?

  QString fileName;
  QFileDialog diag(this, tr("Save File"), defaultName,
               tr("Image files (%1)").arg(filterStr));
  diag.setAcceptMode(QFileDialog::AcceptMode::AcceptSave);
  diag.setFileMode(QFileDialog::FileMode::AnyFile);
  diag.setDefaultSuffix(QStringLiteral("png"));

  if (diag.exec()) {
    fileName = diag.selectedFiles().at(0);
  }

  return fileName;
}

void MainWindow::on_actionOpenFile_triggered() {
  QString fileName = openFileName();
  if(!fileName.isEmpty()) {
    openImageFile(fileName);
  }
}

void MainWindow::on_actionOpenDirectory_triggered() {
  QString directory = openDirectory();
  if(!directory.isEmpty()) {
    openImageFile(directory);
  }
}

void MainWindow::on_actionReload_triggered() {
  if (currentFile_) {
    loadImage(currentFile_);
  }
}

void MainWindow::on_actionNewWindow_triggered() {
  Application* app = static_cast<Application*>(qApp);
  MainWindow* window = new MainWindow();
  window->resize(app->settings().windowWidth(), app->settings().windowHeight());

  if(app->settings().windowMaximized())
        window->setWindowState(window->windowState() | Qt::WindowMaximized);

  window->show();
}

void MainWindow::on_actionSave_triggered() {
  if(saveJob_) // if we're currently saving another file
    return;

  if(!image_.isNull()) {
    if(currentFile_)
      saveImage(currentFile_);
    else
      on_actionSaveAs_triggered();
  }
}

void MainWindow::on_actionSaveAs_triggered() {
  if(saveJob_) // if we're currently saving another file
    return;
  QString curFileName;
  if(currentFile_) {
    curFileName = QString::fromUtf8(currentFile_.displayName().get());
  }

  QString fileName = saveFileName(curFileName);
  if(!fileName.isEmpty()) {
    const Fm::FilePath path = Fm::FilePath::fromPathStr(qPrintable(fileName));
    // save the image file asynchronously
    saveImage(path);

    if(!currentFile_) { // if we haven't loaded any file yet
      currentFile_ = path;
      loadFolder(path.parent());
    }
  }
}

void MainWindow::on_actionDelete_triggered() {
  // delete the current file
  if(currentFile_)
    Fm::FileOperation::deleteFiles({currentFile_});
}

void MainWindow::on_actionFileProperties_triggered() {
  if(currentIndex_.isValid()) {
    const auto file = proxyModel_->fileInfoFromIndex(currentIndex_);
    // it's better to use an async job to query the file info since it's
    // possible that loading of the folder is not finished and the file info is
    // not available yet, but it's overkill for a rarely used function.
    if(file)
      Fm::FilePropsDialog::showForFile(std::move(file));
  }
}

void MainWindow::on_actionClose_triggered() {
  deleteLater();
}

void MainWindow::on_actionNext_triggered() {
  if(proxyModel_->rowCount() <= 1)
    return;
  if(currentIndex_.isValid()) {
    QModelIndex index;
    if(currentIndex_.row() < proxyModel_->rowCount() - 1)
      index = proxyModel_->index(currentIndex_.row() + 1, 0);
    else
      index = proxyModel_->index(0, 0);
    const auto info = proxyModel_->fileInfoFromIndex(index);
    if(info)
      loadImage(info->path(), index);
  }
}

void MainWindow::on_actionPrevious_triggered() {
  if(proxyModel_->rowCount() <= 1)
    return;
  if(currentIndex_.isValid()) {
    QModelIndex index;
    if(currentIndex_.row() > 0)
      index = proxyModel_->index(currentIndex_.row() - 1, 0);
    else
      index = proxyModel_->index(proxyModel_->rowCount() - 1, 0);
    const auto info = proxyModel_->fileInfoFromIndex(index);
    if(info)
      loadImage(info->path(), index);
  }
}

void MainWindow::on_actionFirst_triggered() {
  QModelIndex index = proxyModel_->index(0, 0);
  if(index.isValid()) {
    const auto info = proxyModel_->fileInfoFromIndex(index);
    if(info)
      loadImage(info->path(), index);
  }
}

void MainWindow::on_actionLast_triggered() {
  QModelIndex index = proxyModel_->index(proxyModel_->rowCount() - 1, 0);
  if(index.isValid()) {
    const auto info = proxyModel_->fileInfoFromIndex(index);
    if(info)
      loadImage(info->path(), index);
  }
}

void MainWindow::loadFolder(const Fm::FilePath & newFolderPath) {
  if(folder_) { // an folder is already loaded
    if(newFolderPath == folderPath_) { // same folder, ignore
      return;
    }
    disconnect(folder_.get(), nullptr, this, nullptr); // disconnect from all signals
  }

  folderPath_ = newFolderPath;
  folder_ = Fm::Folder::fromPath(folderPath_);
  currentIndex_ = QModelIndex(); // set current index to invalid
  folderModel_->setFolder(folder_);
  if(folder_->isLoaded()) { // the folder may be already loaded elsewhere
      onFolderLoaded();
  }
  connect(folder_.get(), &Fm::Folder::finishLoading, this, &MainWindow::onFolderLoaded);
  connect(folder_.get(), &Fm::Folder::filesRemoved, this, &MainWindow::onFilesRemoved);
}

// the image is loaded (the method is only called if the loading is not cancelled)
void MainWindow::onImageLoaded() {
  // Note: As the signal finished() is emitted from different thread,
  // we can get it even after canceling the job (and setting the loadJob_
  // to nullptr). This simple check should be enough.
  if (sender() == loadJob_)
  {
    // Add to the MRU menu
    ui.menuRecently_Opened_Files->addItem(QString::fromUtf8(loadJob_->filePath().localPath().get()));

    image_ = loadJob_->image();
    exifData_ = loadJob_->getExifData();

    loadJob_ = nullptr; // the job object will be freed later automatically

    ui.view->setAutoZoomFit(ui.actionZoomFit->isChecked());
    if(ui.actionOriginalSize->isChecked()) {
      ui.view->zoomOriginal();
    }
    ui.view->setImage(image_);

   // currentIndex_ should be corrected after loading
   currentIndex_ = indexFromPath(currentFile_);

    updateUI();

    /* we resized and moved the window without showing
       it in updateUI(), so we need to show it here */
    if(!isVisible()) {
      if(startMaximized_) {
        setWindowState(windowState() | Qt::WindowMaximized);
      }
      show();
    }
  }
}

void MainWindow::onImageSaved() {
  if(!saveJob_->failed()) {
    setModified(false);
  }
  saveJob_ = nullptr;
}

// filter events of other objects, mainly the image view.
bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if(watched == ui.view) { // we got an event for the image view
    switch(event->type()) {
      case QEvent::Wheel: { // mouse wheel event
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        if(wheelEvent->modifiers() == 0) {
          int delta = wheelEvent->delta();
          if(delta < 0)
            on_actionNext_triggered(); // next image
          else
            on_actionPrevious_triggered(); // previous image
        }
        break;
      }
      case QEvent::MouseButtonDblClick: {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if(mouseEvent->button() == Qt::LeftButton)
          ui.actionFullScreen->trigger();
        break;
      }
      default:;
    }
  }
  else if(thumbnailsView_ && watched == thumbnailsView_->childView()) {
    // scroll the thumbnail view with mouse wheel
    switch(event->type()) {
      case QEvent::Wheel: { // mouse wheel event
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        if(wheelEvent->modifiers() == 0) {
          int delta = wheelEvent->delta();
          QScrollBar* hscroll = thumbnailsView_->childView()->horizontalScrollBar();
          if(hscroll)
            hscroll->setValue(hscroll->value() - delta);
          return true;
        }
        break;
      }
      default:;
    }
  }
  return QObject::eventFilter(watched, event);
}

QModelIndex MainWindow::indexFromPath(const Fm::FilePath & filePath) {
  // if the folder is already loaded, figure out our index
  // otherwise, it will be done again in onFolderLoaded() when the folder is fully loaded.
  if(folder_ && folder_->isLoaded()) {
    QModelIndex index;
    int count = proxyModel_->rowCount();
    for(int row = 0; row < count; ++row) {
      index = proxyModel_->index(row, 0);
      const auto info = proxyModel_->fileInfoFromIndex(index);
      if(info && filePath == info->path()) {
        return index;
      }
    }
  }
  return QModelIndex();
}


void MainWindow::updateUI() {
  if(currentIndex_.isValid()) {
    if(thumbnailsView_) { // showing thumbnails
      // select current file in the thumbnails view
      thumbnailsView_->childView()->setCurrentIndex(currentIndex_);
      thumbnailsView_->childView()->scrollTo(currentIndex_, QAbstractItemView::EnsureVisible);
    }

    if(exifDataDock_) {
      setShowExifData(true);
    }
  }

  QString title;
  if(currentFile_) {
    const Fm::CStrPtr dispName = currentFile_.displayName();
    if(loadJob_) { // if loading is in progress
      title = tr("[*]%1 (Loading...) - Image Viewer")
                .arg(QString::fromUtf8(dispName.get()));
      ui.statusBar->setText();
    }
    else {
      if(image_.isNull()) {
        title = tr("[*]%1 (Failed to Load) - Image Viewer")
                  .arg(QString::fromUtf8(dispName.get()));
        ui.statusBar->setText();
      }
      else {
        const QString filePath = QString::fromUtf8(dispName.get());
        title = tr("[*]%1 (%2x%3) - Image Viewer")
                  .arg(filePath)
                  .arg(image_.width())
                  .arg(image_.height());
        ui.statusBar->setText(QStringLiteral("%1×%2").arg(image_.width()).arg(image_.height()),
                              filePath);
        if (!isVisible()) {
          /* Here we try to implement the following behavior as far as possible:
              (1) A minimum size of 400x400 is assumed;
              (2) The window is scaled to fit the image;
              (3) But for too big images, the window is scaled down;
              (4) The window is centered on the screen.

             To have a correct position, we should move the window BEFORE
             it's shown and we also need to know the dimensions of its view.
             Therefore, we first use show() without really showing the window. */

          // the maximization setting may be lost in resizeEvent because we resize the window below
          startMaximized_ = static_cast<Application*>(qApp)->settings().windowMaximized();
          setAttribute(Qt::WA_DontShowOnScreen);
          show();
          int scrollThickness = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
          QSize newSize = size() + image_.size() - ui.view->size() + QSize(scrollThickness, scrollThickness);
          const QScreen *primaryScreen = QGuiApplication::primaryScreen();
          const QRect ag = primaryScreen ? primaryScreen->availableGeometry() : QRect();
          // since the window isn't decorated yet, we have to assume a max thickness for its frame
          QSize maxFrame = QSize(50, 100);
          if(newSize.width() > ag.width() - maxFrame.width()
             || newSize.height() > ag.height() - maxFrame.height()) {
            newSize.scale(ag.width() - maxFrame.width(), ag.height() - maxFrame.height(), Qt::KeepAspectRatio);
          }
          newSize = newSize.expandedTo(QSize(400, 400)); // a minimum size of 400x400 is good
          move(ag.x() + (ag.width() - newSize.width())/2,
               ag.y() + (ag.height() - newSize.height())/2);
          resize(newSize);
          hide(); // hide it to show it again later, at onImageLoaded()
          setAttribute(Qt::WA_DontShowOnScreen, false);
        }
      }
    }
    // TODO: update status bar, show current index in the folder
  }
  else {
    title = tr("[*]Image Viewer");
    ui.statusBar->setText();
  }
  setWindowTitle(title);
  setWindowModified(imageModified_);
}

// Load the specified image file asynchronously in a worker thread.
// When the loading is finished, onImageLoaded() will be called.
void MainWindow::loadImage(const Fm::FilePath & filePath, QModelIndex index) {
  // cancel loading of current image
  if(loadJob_) {
    loadJob_->cancel(); // the job object will be freed automatically later
    loadJob_ = nullptr;
  }
  if(imageModified_) {
    // TODO: ask the user to save the modified image?
    // this should be made optional
    setModified(false);
  }

  currentIndex_ = index;
  currentFile_ = filePath;
  // clear current image, but do not update the view now to prevent flickers
  image_ = QImage();

  const Fm::CStrPtr basename = currentFile_.baseName();
  char* mime_type = g_content_type_guess(basename.get(), nullptr, 0, nullptr);
  QString mimeType;
  if (mime_type) {
    mimeType = QString::fromUtf8(mime_type);
    g_free(mime_type);
  }
  if(mimeType == QLatin1String("image/gif")
     || mimeType == QLatin1String("image/svg+xml") || mimeType == QLatin1String("image/svg+xml-compressed")) {
    if(!currentIndex_.isValid()) {
      // since onImageLoaded is not called here,
      // currentIndex_ should be set
      currentIndex_ = indexFromPath(currentFile_);
    }
    const Fm::CStrPtr file_name = currentFile_.toString();

    // like in onImageLoaded()
    ui.view->setAutoZoomFit(ui.actionZoomFit->isChecked());
    if(ui.actionOriginalSize->isChecked()) {
      ui.view->zoomOriginal();
    }

    if(mimeType == QLatin1String("image/gif"))
      ui.view->setGifAnimation(QString::fromUtf8(file_name.get()));
    else
      ui.view->setSVG(QString::fromUtf8(file_name.get()));
    image_ = ui.view->image();
    updateUI();
    if(!isVisible()) {
      if(startMaximized_) {
        setWindowState(windowState() | Qt::WindowMaximized);
      }
      show();
    }
  }
  else {
    // start a new gio job to load the specified image
    loadJob_ = new LoadImageJob(currentFile_);
    connect(loadJob_, &Fm::Job::finished, this, &MainWindow::onImageLoaded);
    connect(loadJob_, &Fm::Job::error, this
        , [] (const Fm::GErrorPtr & err, Fm::Job::ErrorSeverity /*severity*/, Fm::Job::ErrorAction & /*response*/)
          {
            // TODO: show a info bar?
            qWarning().noquote() << "lximage-qt:" << err.message();
          }
        , Qt::BlockingQueuedConnection);
    loadJob_->runAsync();

    updateUI();
  }
}

void MainWindow::saveImage(const Fm::FilePath & filePath) {
  if(saveJob_) // do not launch a new job if the current one is still in progress
    return;
  // start a new gio job to save current image to the specified path
  saveJob_ = new SaveImageJob(ui.view->image(), filePath);
  connect(saveJob_, &Fm::Job::finished, this, &MainWindow::onImageSaved);
  connect(saveJob_, &Fm::Job::error, this
        , [this] (const Fm::GErrorPtr & err, Fm::Job::ErrorSeverity severity, Fm::Job::ErrorAction & /*response*/)
        {
          // TODO: show a info bar?
          if(severity > Fm::Job::ErrorSeverity::MODERATE) {
            QMessageBox::critical(this, QObject::tr("Error"), err.message());
          }
          else {
            qWarning().noquote() << "lximage-qt:" << err.message();
          }
        }
      , Qt::BlockingQueuedConnection);
  saveJob_->runAsync();
  // FIXME: add a cancel button to the UI? update status bar?
}

void MainWindow::on_actionRotateClockwise_triggered() {
  QGraphicsItem *imageItem = ui.view->imageGraphicsItem();
  bool isGifOrSvg ((imageItem && imageItem->isWidget()) // we have gif animation
                   || dynamic_cast<QGraphicsSvgItem*>(imageItem)); // an SVG image;
  if(!image_.isNull()) {
    QTransform transform;
    transform.rotate(90.0);
    image_ = image_.transformed(transform, Qt::SmoothTransformation);
    /* when this is GIF or SVG, we need to rotate its corresponding QImage
       without showing it to have the right measure for auto-zooming */
    ui.view->setImage(image_, isGifOrSvg ? false : true);
    setModified(true);
  }

  if(isGifOrSvg) {
    QTransform transform;
    transform.translate(imageItem->sceneBoundingRect().height(), 0);
    transform.rotate(90);
    // we need to apply transformations in the reverse order
    QTransform prevTrans = imageItem->transform();
    imageItem->setTransform(transform, false);
    imageItem->setTransform(prevTrans, true);
    // apply transformations to the outline item too
    if(QGraphicsItem *outlineItem = ui.view->outlineGraphicsItem()){
      outlineItem->setTransform(transform, false);
      outlineItem->setTransform(prevTrans, true);
    }
  }
}

void MainWindow::on_actionRotateCounterclockwise_triggered() {
  QGraphicsItem *imageItem = ui.view->imageGraphicsItem();
  bool isGifOrSvg ((imageItem && imageItem->isWidget())
                   || dynamic_cast<QGraphicsSvgItem*>(imageItem));
  if(!image_.isNull()) {
    QTransform transform;
    transform.rotate(-90.0);
    image_ = image_.transformed(transform, Qt::SmoothTransformation);
    ui.view->setImage(image_, isGifOrSvg ? false : true);
    setModified(true);
  }

  if(isGifOrSvg) {
    QTransform transform;
    transform.translate(0, imageItem->sceneBoundingRect().width());
    transform.rotate(-90);
    QTransform prevTrans = imageItem->transform();
    imageItem->setTransform(transform, false);
    imageItem->setTransform(prevTrans, true);
    if(QGraphicsItem *outlineItem = ui.view->outlineGraphicsItem()){
      outlineItem->setTransform(transform, false);
      outlineItem->setTransform(prevTrans, true);
    }
  }
}

void MainWindow::on_actionCopy_triggered() {
  QClipboard *clipboard = QApplication::clipboard();
  QImage copiedImage = image_;
  // FIXME: should we copy the currently scaled result instead of the original image?
  /*
  double factor = ui.view->scaleFactor();
  if(factor == 1.0)
    copiedImage = image_;
  else
    copiedImage = image_.scaled();
  */
  clipboard->setImage(copiedImage);
}

void MainWindow::on_actionCopyPath_triggered() {
  if(currentFile_) {
    const Fm::CStrPtr dispName = currentFile_.displayName();
    QApplication::clipboard()->setText(QString::fromUtf8(dispName.get()));
  }
}

void MainWindow::on_actionPaste_triggered() {
  QClipboard *clipboard = QApplication::clipboard();
  QImage image = clipboard->image();
  if(!image.isNull()) {
    pasteImage(image);
  }
}

void MainWindow::on_actionUpload_triggered()
{
    if(currentFile_.isValid()) {
      UploadDialog(this, QString::fromUtf8(currentFile_.localPath().get())).exec();
    }
    // if there is no open file, save the image to "/tmp" and delete it after upload
    else if(!ui.view->image().isNull()) {
      QString tmpFileName;
      QString tmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
      if(!tmp.isEmpty()) {
        QDir tmpDir(tmp);
        if(tmpDir.exists()) {
          const QString curTime = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"));
          tmpFileName = tmp + QStringLiteral("/lximage-") + curTime + QStringLiteral(".png");
        }
      }
      if(!tmpFileName.isEmpty() && saveJob_ == nullptr) {
        const Fm::FilePath filePath = Fm::FilePath::fromPathStr(qPrintable(tmpFileName));
        saveJob_ = new SaveImageJob(ui.view->image(), filePath);
        connect(saveJob_, &Fm::Job::finished, this, [this, tmpFileName] {
          saveJob_ = nullptr;
          UploadDialog(this, tmpFileName).exec();
          QFile::remove(tmpFileName);
        });
        saveJob_->runAsync();
      }
    }
}

void MainWindow::on_actionFlipVertical_triggered() {
  bool hasQGraphicsItem(false);
  if(QGraphicsItem *imageItem = ui.view->imageGraphicsItem()) {
    hasQGraphicsItem = true;
    QTransform transform;
    transform.scale(1, -1);
    transform.translate(0, -imageItem->sceneBoundingRect().height());
    QTransform prevTrans = imageItem->transform();
    imageItem->setTransform(transform, false);
    imageItem->setTransform(prevTrans, true);
    if(QGraphicsItem *outlineItem = ui.view->outlineGraphicsItem()){
      outlineItem->setTransform(transform, false);
      outlineItem->setTransform(prevTrans, true);
    }
  }
  if(!image_.isNull()) {
    image_ = image_.mirrored(false, true);
    ui.view->setImage(image_, !hasQGraphicsItem);
    setModified(true);
  }
}

void MainWindow::on_actionFlipHorizontal_triggered() {
  bool hasQGraphicsItem(false);
  if(QGraphicsItem *imageItem = ui.view->imageGraphicsItem()) {
    hasQGraphicsItem = true;
    QTransform transform;
    transform.scale(-1, 1);
    transform.translate(-imageItem->sceneBoundingRect().width(), 0);
    QTransform prevTrans = imageItem->transform();
    imageItem->setTransform(transform, false);
    imageItem->setTransform(prevTrans, true);
    if(QGraphicsItem *outlineItem = ui.view->outlineGraphicsItem()){
      outlineItem->setTransform(transform, false);
      outlineItem->setTransform(prevTrans, true);
    }
  }
  if(!image_.isNull()) {
    image_ = image_.mirrored(true, true);
    ui.view->setImage(image_, !hasQGraphicsItem);
    setModified(true);
  }
}

void MainWindow::on_actionResize_triggered() {
  if(image_.isNull()) {
    return;
  }
  QGraphicsItem *imageItem = ui.view->imageGraphicsItem();
  bool isSVG(dynamic_cast<QGraphicsSvgItem*>(imageItem));
  bool isGifOrSvg(isSVG || (imageItem && imageItem->isWidget()));
  QSize imgSize(image_.size());
  ResizeImageDialog *dialog = new ResizeImageDialog(this);
  dialog->setOriginalSize(image_.size());
  if(dialog->exec() == QDialog::Accepted) {
    QSize newSize = dialog->scaledSize();
    if(!isSVG) { // with SVG, we get a sharp image below
      image_ = image_.scaled(newSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
      ui.view->setImage(image_, isGifOrSvg ? false : true);
    }
    if(isGifOrSvg) {
      qreal sx = static_cast<qreal>(newSize.width()) / imgSize.width();
      qreal sy = static_cast<qreal>(newSize.height()) / imgSize.height();
      QTransform transform;
      transform.scale(sx, sy);
      QTransform prevTrans = imageItem->transform();
      imageItem->setTransform(transform, false);
      imageItem->setTransform(prevTrans, true);
      if(QGraphicsItem *outlineItem = ui.view->outlineGraphicsItem()) {
        outlineItem->setTransform(transform, false);
        outlineItem->setTransform(prevTrans, true);
      }

      if(isSVG) {
        // create and set a sharp scaled image with SVG
        QPixmap pixmap(newSize);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setTransform(imageItem->transform());
        painter.setRenderHint(QPainter::Antialiasing);
        QStyleOptionGraphicsItem opt;
        imageItem->paint(&painter, &opt);
        image_ = pixmap.toImage();
        ui.view->setImage(image_, false);
      }
    }

    setModified(true);
  }
  dialog->deleteLater();
}

void MainWindow::setModified(bool modified) {
  imageModified_ = modified;
  updateUI();
}

void MainWindow::applySettings() {
  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();
  if(isFullScreen())
    ui.view->setBackgroundBrush(QBrush(settings.fullScreenBgColor()));
  else
    ui.view->setBackgroundBrush(QBrush(settings.bgColor()));
  ui.view->updateOutline();
  ui.menuRecently_Opened_Files->setMaxItems(settings.maxRecentFiles());

  // also, update shortcuts
  QHash<QString, QString> ca = settings.customShortcutActions();
  const auto defaultShortcuts = app->defaultShortcuts();
  const auto actions = findChildren<QAction*>();
  for(const auto& action : actions) {
     const QString objectName = action->objectName();
     if(ca.contains(objectName)) {
       // custom shortcuts are saved in the PortableText format
       action->setShortcut(QKeySequence(ca.value(objectName), QKeySequence::PortableText));
     }
     else { // default shortcuts include all actions
       action->setShortcut(defaultShortcuts.value(objectName).shortcut);
     }
  }
}

void MainWindow::on_actionPrint_triggered() {
  // QPrinter printer(QPrinter::HighResolution);
  QPrinter printer;
  QPrintDialog dlg(&printer);
  if(dlg.exec() == QDialog::Accepted) {
    QPainter painter;
    painter.begin(&printer);

    // fit the target rectangle into the viewport if needed and center it
    const QRectF viewportRect = painter.viewport();
    QRectF targetRect = image_.rect();
    if(viewportRect.width() < targetRect.width()) {
      targetRect.setSize(QSize(viewportRect.width(), targetRect.height() * (viewportRect.width() / targetRect.width())));
    }
    if(viewportRect.height() < targetRect.height()) {
      targetRect.setSize(QSize(targetRect.width() * (viewportRect.height() / targetRect.height()), viewportRect.height()));
    }
    targetRect.moveCenter(viewportRect.center());

    // set the viewport and window of the painter and paint the image
    painter.setViewport(targetRect.toRect());
    painter.setWindow(image_.rect());
    painter.drawImage(0, 0, image_);

    painter.end();

    // FIXME: The following code divides the image into columns and could be used later as an option.
    /*QRect pageRect = printer.pageRect();
    int cols = (image_.width() / pageRect.width()) + (image_.width() % pageRect.width() ? 1 : 0);
    int rows = (image_.height() / pageRect.height()) + (image_.height() % pageRect.height() ? 1 : 0);
    for(int row = 0; row < rows; ++row) {
      for(int col = 0; col < cols; ++col) {
        QRect srcRect(pageRect.width() * col, pageRect.height() * row, pageRect.width(), pageRect.height());
        painter.drawImage(QPoint(0, 0), image_, srcRect);
        if(col + 1 == cols && row + 1 == rows) // this is the last page
          break;
        printer.newPage();
      }
    }
    painter.end();*/
  }
}

// TODO: This can later be used for doing slide show
void MainWindow::on_actionFullScreen_triggered(bool checked) {
  if(checked)
    showFullScreen();
  else
    showNormal();
}

void MainWindow::on_actionSlideShow_triggered(bool checked) {
  if(checked) {
    if(!slideShowTimer_) {
      slideShowTimer_ = new QTimer();
      // switch to the next image when timeout
      connect(slideShowTimer_, &QTimer::timeout, this, &MainWindow::on_actionNext_triggered);
    }
    Application* app = static_cast<Application*>(qApp);
    slideShowTimer_->start(app->settings().slideShowInterval() * 1000);
    // showFullScreen();
    ui.actionSlideShow->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-stop")));
  }
  else {
    if(slideShowTimer_) {
      delete slideShowTimer_;
      slideShowTimer_ = nullptr;
      ui.actionSlideShow->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    }
  }
}

void MainWindow::on_actionShowThumbnails_triggered(bool checked) {
  setShowThumbnails(checked);
}

void MainWindow::on_actionShowOutline_triggered(bool checked) {
  ui.view->showOutline(checked);
}

void MainWindow::on_actionShowExifData_triggered(bool checked) {
  setShowExifData(checked);
}

void MainWindow::setShowThumbnails(bool show) {
  if(show) {
    if(!thumbnailsDock_) {
      thumbnailsDock_ = new QDockWidget(this);
      thumbnailsDock_->setFeatures(QDockWidget::NoDockWidgetFeatures); // FIXME: should use DockWidgetClosable
      thumbnailsDock_->setWindowTitle(tr("Thumbnails"));
      thumbnailsView_ = new Fm::FolderView(Fm::FolderView::IconMode);
      thumbnailsView_->setIconSize(Fm::FolderView::IconMode, QSize(64, 64));
      thumbnailsView_->setAutoSelectionDelay(0);
      thumbnailsDock_->setWidget(thumbnailsView_);
      addDockWidget(Qt::BottomDockWidgetArea, thumbnailsDock_);
      QListView* listView = static_cast<QListView*>(thumbnailsView_->childView());
      listView->setFlow(QListView::TopToBottom);
      listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      listView->setSelectionMode(QAbstractItemView::SingleSelection);
      listView->installEventFilter(this);
      // FIXME: optimize the size of the thumbnail view
      // FIXME if the thumbnail view is docked elsewhere, update the settings.
      if(Fm::FolderItemDelegate* delegate = static_cast<Fm::FolderItemDelegate*>(listView->itemDelegateForColumn(Fm::FolderModel::ColumnFileName))) {
        int scrollHeight = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        thumbnailsView_->setFixedHeight(delegate->itemSize().height() + scrollHeight);
      }
      thumbnailsView_->setModel(proxyModel_);
      proxyModel_->setShowThumbnails(true);
      if (currentFile_) { // select the loaded image
        currentIndex_ = indexFromPath(currentFile_);
        listView->setCurrentIndex(currentIndex_);
        // wait to center the selection
        QCoreApplication::processEvents();
        listView->scrollTo(currentIndex_, QAbstractItemView::PositionAtCenter);
      }
      connect(thumbnailsView_->selectionModel(), &QItemSelectionModel::selectionChanged,
              this, &MainWindow::onThumbnailSelChanged);
    }
  }
  else {
    if(thumbnailsDock_) {
      delete thumbnailsView_;
      thumbnailsView_ = nullptr;
      delete thumbnailsDock_;
      thumbnailsDock_ = nullptr;
    }
    proxyModel_->setShowThumbnails(false);
  }
}

void MainWindow::setShowExifData(bool show) {
  // Close the dock if it exists and show is false
  if (exifDataDock_ && !show) {
    exifDataDock_->close();
    exifDataDock_ = nullptr;
  }

  // Be sure the dock was created before rendering content to it
  if (show && !exifDataDock_) {
    exifDataDock_ = new QDockWidget(tr("EXIF Data"), this);
    exifDataDock_->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::RightDockWidgetArea, exifDataDock_);
  }

  // Render the content to the dock
  if (show) {
    QWidget* exifDataDockView_ = new QWidget();

    QVBoxLayout* exifDataDockViewContent_ = new QVBoxLayout();
    QTableWidget* exifDataContentTable_ = new QTableWidget();

    // Table setup
    exifDataContentTable_->setColumnCount(2);
    exifDataContentTable_->setShowGrid(false);
    exifDataContentTable_->horizontalHeader()->hide();
    exifDataContentTable_->verticalHeader()->hide();

    // The table is not editable
    exifDataContentTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Write the EXIF Data to the table
    for (const QString& key : exifData_.keys()) {
      int rowCount = exifDataContentTable_->rowCount();

      exifDataContentTable_->insertRow(rowCount);
      exifDataContentTable_->setItem(rowCount,0, new QTableWidgetItem(key));
      exifDataContentTable_->setItem(rowCount,1, new QTableWidgetItem(exifData_.value(key)));
    }

    // Table setup after content was added
    exifDataContentTable_->resizeColumnsToContents();
    exifDataContentTable_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    exifDataDockViewContent_->addWidget(exifDataContentTable_);
    exifDataDockView_->setLayout(exifDataDockViewContent_);
    exifDataDock_->setWidget(exifDataDockView_);
  }
}

void MainWindow::changeEvent(QEvent* event) {
  // TODO: hide menu/toolbars in full screen mode and make the background black.
  if(event->type() == QEvent::WindowStateChange) {
    Application* app = static_cast<Application*>(qApp);
    if(isFullScreen()) { // changed to fullscreen mode
      ui.view->setFrameStyle(QFrame::NoFrame);
      ui.view->setBackgroundBrush(QBrush(app->settings().fullScreenBgColor()));
      ui.view->updateOutline();
      ui.toolBar->hide();
      ui.annotationsToolBar->hide();
      ui.statusBar->hide();
      if(thumbnailsDock_)
        thumbnailsDock_->hide();
      // NOTE: in fullscreen mode, all shortcut keys in the menu are disabled since the menu
      // is disabled. We needs to add the actions to the main window manually to enable the
      // shortcuts again.
      ui.menubar->hide();
      const auto actions = ui.menubar->actions();
      for(QAction* action : qAsConst(actions)) {
        if(!action->shortcut().isEmpty())
          addAction(action);
      }
      addActions(ui.menubar->actions());
      ui.view->hideCursor(true);
    }
    else { // restore to normal window mode
      ui.view->setFrameStyle(QFrame::StyledPanel|QFrame::Sunken);
      ui.view->setBackgroundBrush(QBrush(app->settings().bgColor()));
      ui.view->updateOutline();
      // now we're going to re-enable the menu, so remove the actions previously added.
      const auto actions_ = ui.menubar->actions();
      for(QAction* action : qAsConst(actions_)) {
        if(!action->shortcut().isEmpty())
          removeAction(action);
      }
      ui.menubar->show();
      ui.toolBar->show();
      if(ui.actionAnnotations->isChecked()){
          ui.annotationsToolBar->show();
      }
      ui.statusBar->show();
      if(thumbnailsDock_)
        thumbnailsDock_->show();
      ui.view->hideCursor(false);
    }
  }
  QWidget::changeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  Settings& settings = static_cast<Application*>(qApp)->settings();
  if(settings.rememberWindowSize()) {
    settings.setLastWindowMaximized(isMaximized());

    if(!isMaximized()) {
        settings.setLastWindowWidth(width());
        settings.setLastWindowHeight(height());
    }
  }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  QWidget::closeEvent(event);
  Settings& settings = static_cast<Application*>(qApp)->settings();
  if(settings.rememberWindowSize()) {
    settings.setLastWindowMaximized(isMaximized());

    if(!isMaximized()) {
        settings.setLastWindowWidth(width());
        settings.setLastWindowHeight(height());
    }
  }
}

void MainWindow::onContextMenu(QPoint pos) {
  contextMenu_->exec(ui.view->mapToGlobal(pos));
}

void MainWindow::onKeyboardEscape() {
  if(isFullScreen())
    ui.actionFullScreen->trigger(); // will also "uncheck" the menu entry
  else
    on_actionClose_triggered();
}

void MainWindow::onThumbnailSelChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/) {
  // the selected item of thumbnail view is changed
  if(!selected.isEmpty()) {
    QModelIndex index = selected.indexes().first();
    if(index.isValid()) {
      // WARNING: Adding the condition index != currentIndex_ would be wrong because currentIndex_ may not be updated yet
      const auto file = proxyModel_->fileInfoFromIndex(index);
      if(file) {
        loadImage(file->path(), index);
        return;
      }
    }
  }
  // no image to show; reload to show a blank view and update variables
  on_actionReload_triggered();
}

void MainWindow::onFilesRemoved(const Fm::FileInfoList& files) {
  if(thumbnailsView_) {
    return; // onThumbnailSelChanged() will do the job
  }
  for(auto& file : files) {
    if(file->path() == currentFile_) {
      if(proxyModel_->rowCount() >= 1 && currentIndex_.isValid()) {
        QModelIndex index;
        if(currentIndex_.row() < proxyModel_->rowCount()) {
          index = currentIndex_;
        }
        else {
          index = proxyModel_->index(proxyModel_->rowCount() - 1, 0);
        }
        const auto info = proxyModel_->fileInfoFromIndex(index);
        if(info) {
          loadImage(info->path(), index);
          return;
        }
      }
      // no image to show; reload to show a blank view
      on_actionReload_triggered();
      return;
    }
  }
}

void MainWindow::onFileDropped(const QString path) {
    openImageFile(path);
}

void MainWindow::fileMenuAboutToShow() {
  // the "Open With..." submenu of Fm::FileMenu is shown
  // only if there is a file with a valid mime type
  if(currentIndex_.isValid()) {
    if(const auto file = proxyModel_->fileInfoFromIndex(currentIndex_)) {
      if(file->mimeType()) {
        ui.openWithMenu->setEnabled(true);
        return;
      }
    }
  }
  ui.openWithMenu->setEnabled(false);
}

void MainWindow::createOpenWithMenu() {
  if(currentIndex_.isValid()) {
    if(const auto file = proxyModel_->fileInfoFromIndex(currentIndex_)) {
      if(file->mimeType()) {
        // We want the "Open With..." submenu. It will be deleted alongside
        // fileMenu_ when openWithMenu hides (-> deleteOpenWithMenu)
        Fm::FileInfoList files;
        files.push_back(file);
        fileMenu_ = new Fm::FileMenu(files, file, Fm::FilePath());
        if(QMenu* menu = fileMenu_->openWithMenuAction()->menu()) {
          ui.openWithMenu->addActions(menu->actions());
        }
      }
    }
  }
}

void MainWindow::deleteOpenWithMenu() {
  if(fileMenu_) {
    fileMenu_->deleteLater();
  }
}
