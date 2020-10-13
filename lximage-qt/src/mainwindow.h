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

#ifndef LXIMAGE_MAINWINDOW_H
#define LXIMAGE_MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

#include "imageview.h"
#include <QImage>

#include <QTableWidget>
#include <QLabel>

#include <libfm-qt/foldermodel.h>
#include <libfm-qt/proxyfoldermodel.h>
#include <libfm-qt/filemenu.h>
#include <gio/gio.h>

#include "modelfilter.h"
#include "loadimagejob.h"
#include "saveimagejob.h"

class QTimer;
class QDockWidget;

namespace Fm {
  class FolderView;
}

namespace LxImage {

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  friend class LoadImageJob;
  friend class SaveImageJob;

  MainWindow();
  virtual ~MainWindow();

  void openImageFile(const QString& fileName);

  QImage image() const {
    return image_;
  }

  void pasteImage(QImage newImage);

  const Fm::FilePath & currentFile() const {
    return currentFile_;
  }

  void setShowThumbnails(bool show);
  void setShowExifData(bool show);
  void applySettings();

protected:
  void loadImage(const Fm::FilePath & filePath, QModelIndex index = QModelIndex());
  void saveImage(const Fm::FilePath & filePath); // save current image to a file
  void loadFolder(const Fm::FilePath & newFolderPath);
  QString openFileName();
  QString openDirectory();
  QString saveFileName(const QString& defaultName = QString());
  virtual void changeEvent(QEvent * event);
  virtual void resizeEvent(QResizeEvent *event);
  virtual void closeEvent(QCloseEvent *event);

  void onImageLoaded();
  void onImageSaved();

  virtual bool eventFilter(QObject* watched, QEvent* event);
private Q_SLOTS:
  void on_actionAbout_triggered();

  void on_actionOpenFile_triggered();
  void on_actionOpenDirectory_triggered();
  void on_actionReload_triggered();
  void on_actionNewWindow_triggered();
  void on_actionSave_triggered();
  void on_actionSaveAs_triggered();
  void on_actionPrint_triggered();
  void on_actionDelete_triggered();
  void on_actionFileProperties_triggered();
  void on_actionClose_triggered();

  void on_actionRotateClockwise_triggered();
  void on_actionRotateCounterclockwise_triggered();
  void on_actionFlipVertical_triggered();
  void on_actionFlipHorizontal_triggered();
  void on_actionResize_triggered();
  void on_actionCopy_triggered();
  void on_actionCopyPath_triggered();
  void on_actionPaste_triggered();
  void on_actionUpload_triggered();

  void on_actionShowThumbnails_triggered(bool checked);
  void on_actionShowOutline_triggered(bool checked);
  void on_actionShowExifData_triggered(bool checked);
  void on_actionFullScreen_triggered(bool checked);
  void on_actionSlideShow_triggered(bool checked);

  void on_actionPrevious_triggered();
  void on_actionNext_triggered();
  void on_actionFirst_triggered();
  void on_actionLast_triggered();

  void on_actionZoomIn_triggered();
  void on_actionZoomOut_triggered();
  void on_actionOriginalSize_triggered();
  void on_actionZoomFit_triggered();

  void onZooming();

  void on_actionDrawNone_triggered();
  void on_actionDrawArrow_triggered();
  void on_actionDrawRectangle_triggered();
  void on_actionDrawCircle_triggered();
  void on_actionDrawNumber_triggered();

  void onContextMenu(QPoint pos);
  void onKeyboardEscape();

  void onThumbnailSelChanged(const QItemSelection & selected, const QItemSelection & deselected);
  void onFilesRemoved(const Fm::FileInfoList& files);

  void onFileDropped(const QString path);

  void fileMenuAboutToShow();
  void createOpenWithMenu();
  void deleteOpenWithMenu();

private:
  void onFolderLoaded();
  void updateUI();
  void setModified(bool modified);
  QModelIndex indexFromPath(const Fm::FilePath & filePath);

private:
  Ui::MainWindow ui;
  QMenu* contextMenu_;
  QTimer* slideShowTimer_;

  QImage image_; // the image currently shown
  Fm::FilePath currentFile_; // path to current image file
  // FmFileInfo* currentFileInfo_; // info of the current file, can be NULL
  bool imageModified_; // the current image is modified by rotation, flip, or others and needs to be saved

  bool startMaximized_;

  // folder browsing
  std::shared_ptr<Fm::Folder> folder_;
  Fm::FilePath folderPath_;
  Fm::FolderModel* folderModel_;
  Fm::ProxyFolderModel* proxyModel_;
  ModelFilter* modelFilter_;
  QModelIndex currentIndex_;

  QDockWidget* thumbnailsDock_;
  Fm::FolderView* thumbnailsView_;

  // EXIF Data
  QDockWidget* exifDataDock_ = nullptr;
  QWidget* exifDataDockView_;
  QVBoxLayout* exifDataDockViewContent_;
  QTableWidget* exifDataContentTable_;

  QMap<QString, QString> exifData_;

  // multi-threading loading of images
  LoadImageJob* loadJob_;
  SaveImageJob* saveJob_;

  QPointer<Fm::FileMenu> fileMenu_;
};

}

#endif // LXIMAGE_MAINWINDOW_H
