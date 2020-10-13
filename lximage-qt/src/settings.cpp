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

#include "settings.h"
#include <QSettings>
#include <QIcon>
#include <QKeySequence>

using namespace LxImage;

Settings::Settings():
  useFallbackIconTheme_(QIcon::themeName().isEmpty() || QIcon::themeName() == QLatin1String("hicolor")),
  bgColor_(255, 255, 255),
  fullScreenBgColor_(0, 0, 0),
  showThumbnails_(false),
  showSidePane_(false),
  slideShowInterval_(5),
  fallbackIconTheme_(QStringLiteral("oxygen")),
  maxRecentFiles_(5),
  fixedWindowWidth_(640),
  fixedWindowHeight_(480),
  lastWindowWidth_(640),
  lastWindowHeight_(480),
  lastWindowMaximized_(false),
  showOutline_(false),
  showAnnotationsToolbar_(false) {
}

Settings::~Settings() {
}

bool Settings::load() {
  QSettings settings(QStringLiteral("lximage-qt"), QStringLiteral("settings"));
  fallbackIconTheme_ = settings.value(QStringLiteral("fallbackIconTheme"), fallbackIconTheme_).toString();
  bgColor_ = settings.value(QStringLiteral("bgColor"), bgColor_).value<QColor>();
  fullScreenBgColor_ = settings.value(QStringLiteral("fullScreenBgColor"), fullScreenBgColor_).value<QColor>();
  // showThumbnails_;
  // showSidePane_;
  slideShowInterval_ = settings.value(QStringLiteral("slideShowInterval"), slideShowInterval_).toInt();
  maxRecentFiles_ = settings.value(QStringLiteral("maxRecentFiles"), maxRecentFiles_).toInt();
  recentlyOpenedFiles_ = settings.value(QStringLiteral("recentlyOpenedFiles")).toStringList();

  settings.beginGroup(QStringLiteral("Window"));
  fixedWindowWidth_ = settings.value(QStringLiteral("FixedWidth"), 640).toInt();
  fixedWindowHeight_ = settings.value(QStringLiteral("FixedHeight"), 480).toInt();
  lastWindowWidth_ = settings.value(QStringLiteral("LastWindowWidth"), 640).toInt();
  lastWindowHeight_ = settings.value(QStringLiteral("LastWindowHeight"), 480).toInt();
  lastWindowMaximized_ = settings.value(QStringLiteral("LastWindowMaximized"), false).toBool();
  rememberWindowSize_ = settings.value(QStringLiteral("RememberWindowSize"), true).toBool();
  showOutline_ = settings.value(QStringLiteral("ShowOutline"), false).toBool();
  showAnnotationsToolbar_ = settings.value(QStringLiteral("ShowAnnotationsToolbar"), false).toBool();
  prefSize_ = settings.value(QStringLiteral("PrefSize"), QSize(400, 400)).toSize();
  settings.endGroup();

  // shortcuts
  settings.beginGroup(QStringLiteral("Shortcuts"));
  const QStringList actions = settings.childKeys();
  for(const auto& action : actions) {
    QString str = settings.value(action).toString();
    addShortcut(action, str);
  }
  settings.endGroup();

  return true;
}

bool Settings::save() {
  QSettings settings(QStringLiteral("lximage-qt"), QStringLiteral("settings"));

  settings.setValue(QStringLiteral("fallbackIconTheme"), fallbackIconTheme_);
  settings.setValue(QStringLiteral("bgColor"), bgColor_);
  settings.setValue(QStringLiteral("fullScreenBgColor"), fullScreenBgColor_);
  settings.setValue(QStringLiteral("slideShowInterval"), slideShowInterval_);
  settings.setValue(QStringLiteral("maxRecentFiles"), maxRecentFiles_);
  settings.setValue(QStringLiteral("recentlyOpenedFiles"), recentlyOpenedFiles_);

  settings.beginGroup(QStringLiteral("Window"));
  settings.setValue(QStringLiteral("FixedWidth"), fixedWindowWidth_);
  settings.setValue(QStringLiteral("FixedHeight"), fixedWindowHeight_);
  settings.setValue(QStringLiteral("LastWindowWidth"), lastWindowWidth_);
  settings.setValue(QStringLiteral("LastWindowHeight"), lastWindowHeight_);
  settings.setValue(QStringLiteral("LastWindowMaximized"), lastWindowMaximized_);
  settings.setValue(QStringLiteral("RememberWindowSize"), rememberWindowSize_);
  settings.setValue(QStringLiteral("ShowOutline"), showOutline_);
  settings.setValue(QStringLiteral("ShowAnnotationsToolbar"), showAnnotationsToolbar_);
  settings.setValue(QStringLiteral("PrefSize"), prefSize_);
  settings.endGroup();

  // shortcuts
  settings.beginGroup(QStringLiteral("Shortcuts"));
  for(int i = 0; i < removedActions_.size(); ++i) {
    settings.remove(removedActions_.at(i));
  }
  QHash<QString, QString>::const_iterator it = actions_.constBegin();
  while(it != actions_.constEnd()) {
    settings.setValue(it.key(), it.value());
    ++it;
  }
  settings.endGroup();

  return true;
}


