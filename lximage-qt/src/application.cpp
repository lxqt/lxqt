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

#include "application.h"
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QPixmapCache>
#include <QX11Info>
#include "applicationadaptor.h"
#include "screenshotdialog.h"
#include "mainwindow.h"

using namespace LxImage;

static const char* serviceName = "org.lxde.LxImage";
static const char* ifaceName = "org.lxde.LxImage.Application";

Application::Application(int& argc, char** argv):
  QApplication(argc, argv),
  libFm(),
  windowCount_(0) {
  setApplicationVersion(QStringLiteral(LXIMAGE_VERSION));
}

bool Application::init(int argc, char** argv) {
  Q_UNUSED(argc)
  Q_UNUSED(argv)

  setAttribute(Qt::AA_UseHighDpiPixmaps, true);

  // install the translations built-into Qt itself
  qtTranslator.load(QStringLiteral("qt_") + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  installTranslator(&qtTranslator);

  // install libfm-qt translator
  installTranslator(libFm.translator());

  // install our own tranlations
  translator.load(QStringLiteral("lximage-qt_") + QLocale::system().name(), QStringLiteral(LXIMAGE_DATA_DIR) + QStringLiteral("/translations"));
  installTranslator(&translator);

  // initialize dbus
  QDBusConnection dbus = QDBusConnection::sessionBus();
  if(dbus.registerService(QLatin1String(serviceName))) {
    settings_.load(); // load settings
    // we successfully registered the service
    isPrimaryInstance = true;
    setQuitOnLastWindowClosed(false); // do not quit even when there're no windows

    new ApplicationAdaptor(this);
    dbus.registerObject(QStringLiteral("/Application"), this);

    connect(this, &Application::aboutToQuit, this, &Application::onAboutToQuit);

    if(settings_.useFallbackIconTheme())
      QIcon::setThemeName(settings_.fallbackIconTheme());
  }
  else {
    // an service of the same name is already registered.
    // we're not the first instance
    isPrimaryInstance = false;
  }

  QPixmapCache::setCacheLimit(1024); // avoid pixmap caching.

  return parseCommandLineArgs();
}

bool Application::parseCommandLineArgs() {
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();

  const bool isX11 = QX11Info::isPlatformX11();

  QCommandLineOption screenshotOption(
    QStringList() << QStringLiteral("s") << QStringLiteral("screenshot"),
    tr("Take a screenshot (deprecated, please use screengrab instead)")
  );
  if(isX11) {
    parser.addOption(screenshotOption);
  }


  QCommandLineOption screenshotOptionDir(
    QStringList() << QStringLiteral("d") << QStringLiteral("dirscreenshot"),
    tr("Take a screenshot and save it to the directory without showing the GUI (deprecated, please use screengrab instead)"), tr("DIR")
  );
  if(isX11) {
    parser.addOption(screenshotOptionDir);
  }

  const QString files = tr("[FILE1, FILE2,...]");
  parser.addPositionalArgument(QStringLiteral("files"), files, files);

  parser.process(*this);

  const QStringList args = parser.positionalArguments();
  bool screenshotTool = false;
  if(isX11)
    screenshotTool = parser.isSet(screenshotOption);

  QStringList paths;
  for(const QString& arg : args) {
    QFileInfo info(arg);
    paths.push_back(info.absoluteFilePath());
  }


  //silent no-gui screenshot
  if(isX11) {
    if (parser.isSet(screenshotOptionDir)) {
        ScreenshotDialog::cmdTopShotToDir(parser.value(screenshotOptionDir));
        return false;
    }
  }

  bool keepRunning = false;
  if(isPrimaryInstance) {
    settings_.load();
    keepRunning = true;
    if(screenshotTool) {
      screenshot();
    }
    else
        newWindow(paths);
  }
  else {
    // we're not the primary instance.
    // call the primary instance via dbus to do operations
    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusInterface iface(QLatin1String(serviceName), QStringLiteral("/Application"), QLatin1String(ifaceName), dbus, this);
    if(screenshotTool)
      iface.call(QStringLiteral("screenshot"));
    else
      iface.call(QStringLiteral("newWindow"), paths);
  }
  return keepRunning;
}

MainWindow* Application::createWindow() {
  LxImage::MainWindow* window;
  window = new LxImage::MainWindow();

  // get default shortcuts from the first window
  if(defaultShortcuts_.isEmpty()) {
    const auto actions = window->findChildren<QAction*>();
    for(const auto& action : actions) {
      if(action->objectName().isEmpty() || action->text().isEmpty()) {
        continue;
      }
      QKeySequence seq = action->shortcut();
      ShortcutDescription s;
      s.displayText = action->text().remove QStringLiteral("&"); // without mnemonics
      s.shortcut = seq;
      defaultShortcuts_.insert(action->objectName(), s);
    }
  }

  // apply custom shortcuts to this window
  QHash<QString, QString> ca = settings_.customShortcutActions();
  const auto actions = window->findChildren<QAction*>();
  for(const auto& action : actions) {
    const QString objectName = action->objectName();
    if(ca.contains(objectName)) {
      auto shortcut = ca.take(objectName);
      // custom shortcuts are saved in the PortableText format.
      action->setShortcut(QKeySequence(shortcut, QKeySequence::PortableText));
    }
    if(ca.isEmpty()) {
      break;
    }
  }

  return window;
}

void Application::newWindow(QStringList files) {
  LxImage::MainWindow* window;
  if(files.empty()) {
    window = createWindow();

    window->resize(settings_.windowWidth(), settings_.windowHeight());
    if(settings_.windowMaximized())
      window->setWindowState(window->windowState() | Qt::WindowMaximized);

    window->show();
  }
  else {
    for(const QString& fileName : qAsConst(files)) {
      window = createWindow();
      window->openImageFile(fileName);

      window->resize(settings_.windowWidth(), settings_.windowHeight());
      if(settings_.windowMaximized())
        window->setWindowState(window->windowState() | Qt::WindowMaximized);

      /* when there's an image, we show the window AFTER resizing
         and centering it appropriately at MainWindow::updateUI() */
      //window->show();
    }
  }
}

void Application::applySettings() {
  const auto windows = topLevelWidgets();
  for(QWidget* window : windows) {
    if(window->inherits("LxImage::MainWindow")) {
      static_cast<MainWindow*>(window)->applySettings();
    }
  }
}

void Application::screenshot() {
  ScreenshotDialog* dlg = new ScreenshotDialog();
  dlg->show();
}

void Application::editPreferences() {
  // open Preferences dialog only if default shortcuts are known
  if(defaultShortcuts_.isEmpty()) {
    return;
  }
  if(preferencesDialog_ == nullptr) {
    preferencesDialog_ = new PreferencesDialog();
  }
  preferencesDialog_.data()->show();
  preferencesDialog_.data()->raise();
  preferencesDialog_.data()->activateWindow();
}

void Application::onAboutToQuit() {
  settings_.save();
}
