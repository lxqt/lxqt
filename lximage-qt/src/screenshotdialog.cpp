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

#include "screenshotdialog.h"
#include "screenshotselectarea.h"
#include <QTimer>
#include <QPixmap>
#include <QImage>
#include <QPainter>

#include "application.h"
#include <QDesktopWidget>
#include <QScreen>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPushButton>
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>
#include <sstream>
#include <iostream>

/*
 * Used options for Artistic Style to format:
--style=google
--break-closing-braces
--break-one-line-headers
--add-braces
--break-elseifs
--indent=spaces=2
--unpad-paren
 * */
using namespace LxImage;

static bool hasXFixes() {
  int event_base, error_base;
  return XFixesQueryExtension(QX11Info::display(), &event_base, &error_base);
}

ScreenshotDialog::ScreenshotDialog(QWidget* parent, Qt::WindowFlags f): QDialog(parent, f), hasXfixes_(hasXFixes()) {
  ui.setupUi(this);
  ui.buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
  ui.buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
  Application* app = static_cast<Application*>(qApp);
  app->addWindow();
  if(!hasXfixes_) {
    ui.includeCursor->hide();
  }
}
ScreenshotDialog::~ScreenshotDialog() {
  Application* app = static_cast<Application*>(qApp);
  app->removeWindow();
}
void ScreenshotDialog::done(int r) {
  if(r == QDialog::Accepted) {
    hide();
    QDialog::done(r);
    XSync(QX11Info::display(), 0); // is this useful?

    int delay = ui.delay->value();
    if(delay == 0) {
      // NOTE:
      // Well, we need to give X and the window manager some time to
      // really hide our own dialog from the screen.
      // Nobody knows how long it will take, and there is no reliable
      // way to ensure that. Let's wait for 400 ms here for it.
      delay = 400;
    }
    else {
      delay *= 1000;
    }
    // the dialog object will be deleted in doScreenshot().
    QTimer::singleShot(delay, this, SLOT(doScreenshot()));
  }
  else {
    deleteLater();
  }
}

QRect ScreenshotDialog::windowFrame(WId wid) {
  QRect result;
  XWindowAttributes wa;
  if(XGetWindowAttributes(QX11Info::display(), wid, &wa)) {
    Window child;
    int x, y;
    // translate to root coordinate
    XTranslateCoordinates(QX11Info::display(), wid, wa.root, 0, 0, &x, &y, &child);
    //qDebug("%d, %d, %d, %d", x, y, wa.width, wa.height);
    result.setRect(x, y, wa.width, wa.height);

    // get the frame widths added by the window manager
    Atom atom = XInternAtom(QX11Info::display(), "_NET_FRAME_EXTENTS", false);
    unsigned long type, resultLen, rest;
    int format;
    unsigned char* data = nullptr;
    if(XGetWindowProperty(QX11Info::display(), wid, atom, 0, G_MAXLONG, false,
                          XA_CARDINAL, &type, &format, &resultLen, &rest, &data) == Success) {
    }
    if(data) {  // left, right, top, bottom
      long* offsets = reinterpret_cast<long*>(data);
      result.setLeft(result.left() - offsets[0]);
      result.setRight(result.right() + offsets[1]);
      result.setTop(result.top() - offsets[2]);
      result.setBottom(result.bottom() + offsets[3]);
      XFree(data);
    }
  }
  return result;
}

WId ScreenshotDialog::activeWindowId() {
  WId root = WId(QX11Info::appRootWindow());
  Atom atom = XInternAtom(QX11Info::display(), "_NET_ACTIVE_WINDOW", false);
  unsigned long type, resultLen, rest;
  int format;
  WId result = 0;
  unsigned char* data = nullptr;
  if(XGetWindowProperty(QX11Info::display(), root, atom, 0, 1, false,
                        XA_WINDOW, &type, &format, &resultLen, &rest, &data) == Success) {
    result = *reinterpret_cast<long*>(data);
    XFree(data);
  }
  return result;
}

QImage ScreenshotDialog::takeScreenshot(const WId& wid, const QRect& rect, bool takeCursor) {
  QImage image;
  QScreen *screen = QGuiApplication::primaryScreen();
  if(screen) {
    QPixmap pixmap = screen->grabWindow(wid, rect.x(), rect.y(), rect.width(), rect.height());
    image = pixmap.toImage();

    //call to hasXFixes() maybe executed here from cmd line with no gui mode (some day though, currently ignore cursor)
    if(takeCursor &&  hasXFixes()) {
      // capture the cursor if needed
      XFixesCursorImage* cursor = XFixesGetCursorImage(QX11Info::display());
      if(cursor) {
        if(cursor->pixels) {  // pixles should be an ARGB array
          QImage cursorImage;
          if(sizeof(long) == 4) {
            // FIXME: will we encounter byte-order problems here?
            cursorImage = QImage((uchar*)cursor->pixels, cursor->width, cursor->height, QImage::Format_ARGB32);
          }
          else { // XFixes returns long integers which is not 32 bit on 64 bit systems.
            long len = cursor->width * cursor->height;
            quint32* buf = new quint32[len];
            for(long i = 0; i < len; ++i) {
              buf[i] = (quint32)cursor->pixels[i];
            }
            cursorImage = QImage((uchar*)buf, cursor->width, cursor->height, QImage::Format_ARGB32, [](void* b) {
              delete[](quint32*)b;
            }, buf);
          }
          // paint the cursor on the current image
          QPainter painter(&image);
          painter.drawImage(cursor->x - cursor->xhot, cursor->y - cursor->yhot, cursorImage);
        }
        XFree(cursor);
      }
    }
  }
  return image;
}

void ScreenshotDialog::doScreenshot() {
  WId wid = 0;
  QRect rect{0, 0, -1, -1};

  wid = QApplication::desktop()->winId(); // get desktop window
  if(ui.currentWindow->isChecked()) {
    WId activeWid = activeWindowId();
    if(activeWid) {
      if(ui.includeFrame->isChecked()) {
        rect = windowFrame(activeWid);
      }
      else {
        wid = activeWid;
      }
    }
  }

  //using stored hasXfixes_ so avoid extra call to function later
  QImage image{takeScreenshot(wid, rect, hasXfixes_ && ui.includeCursor->isChecked())};

  if(ui.screenArea->isChecked() && !image.isNull()) {
    ScreenshotSelectArea selectArea(image);
    if(QDialog::Accepted == selectArea.exec()) {
      image = image.copy(selectArea.selectedArea());
    }
  }

  Application* app = static_cast<Application*>(qApp);
  MainWindow* window = app->createWindow();
  window->resize(app->settings().windowWidth(), app->settings().windowHeight());
  if(!image.isNull()) {
    window->pasteImage(image);
  }
  window->show();

  deleteLater(); // destroy ourself
}

static QString buildNumericFnPart() {
  //we may have many copies running with no gui, for example user presses hot keys fast
  //so they must have different file names to save, lets do it time + pid
  const auto now = QDateTime::currentDateTime().toMSecsSinceEpoch();
  const auto pid = getpid();
  return QStringLiteral("%1_%2").arg(now).arg(pid);
}

static QString getWindowName(WId wid) {
  QString result;
  if(wid) {
    static const char* atoms[] = {
      "WM_NAME",
      "_NET_WM_NAME",
      "STRING",
      "UTF8_STRING",
    };


    const auto display = QX11Info::display();

    Atom a = None, type;


    for(const auto& c : atoms) {
      if(None != (a = XInternAtom(display, c, true))) {
        int form;
        unsigned long remain, len;
        unsigned char *list;

        errno = 0;
        if(XGetWindowProperty(display, wid, a, 0, 1024, False, XA_STRING,
                              &type, &form, &len, &remain, &list) == Success) {

          if(list && *list) {

            std::string dump((const char*)list);
            std::stringstream ss;
            for(const auto& sym : dump) {
              if(std::isalnum(sym)) {
                ss.put(sym);
              }
            }
            result = QString::fromStdString(ss.str());
            break;
          }
        }

      }
    }
  }
  return (result.isEmpty()) ? QStringLiteral("UKNOWN") : result;
}

void ScreenshotDialog::cmdTopShotToDir(QString path) {

  WId activeWid = activeWindowId();
  const QRect rect = (activeWid) ? windowFrame(activeWid) : QRect{0, 0, -1, -1};
  QImage img{takeScreenshot(QApplication::desktop()->winId(), rect, false)};

  QDir d;
  d.mkpath(path);
  QFileInfo fi(path);
  if(!fi.exists() || !fi.isDir() || !fi.isWritable()) {
    path = QDir::homePath();
  }
  const QString filename = QStringLiteral("%1/%2_%3").arg(path).arg(getWindowName(activeWid)).arg(buildNumericFnPart());

  const auto static png = QStringLiteral(".png");
  QString finalName = filename % png;

  //most unlikelly this will happen ... but user might change system clock or so and we dont want to overwrite file
  for(int counter = 0; QFile::exists(finalName) && counter < 5000; ++counter) {
    finalName = QStringLiteral("%1_%2%3").arg(filename).arg(counter).arg(png);
  }
  //std::cout << finalName.toStdString() << std::endl;
  img.save(finalName);
}
