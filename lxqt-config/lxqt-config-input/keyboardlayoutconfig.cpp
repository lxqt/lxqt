/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "keyboardlayoutconfig.h"
#include <QProcess>
#include <QFile>
#include <QHash>
#include <QDebug>
#include "selectkeyboardlayoutdialog.h"
#include <LXQt/Settings>

KeyboardLayoutConfig::KeyboardLayoutConfig(LXQt::Settings* _settings, QWidget* parent):
  QWidget(parent),
  settings(_settings),
  applyConfig_(false) {
  ui.setupUi(this);

  loadLists();
  loadSettings();
  initControls();

  connect(ui.addLayout, &QAbstractButton::clicked, this, &KeyboardLayoutConfig::onAddLayout);
  connect(ui.removeLayout, &QAbstractButton::clicked, this, &KeyboardLayoutConfig::onRemoveLayout);
  connect(ui.moveUp, &QAbstractButton::clicked, this, &KeyboardLayoutConfig::onMoveUp);
  connect(ui.moveDown, &QAbstractButton::clicked, this, &KeyboardLayoutConfig::onMoveDown);
  connect(ui.keyboardModel, QOverload<int>::of(&QComboBox::activated), [this](int /*index*/) {
    applyConfig_ = true;
    Q_EMIT settingsChanged();
  });
  connect(ui.switchKey, QOverload<int>::of(&QComboBox::activated), [this](int /*index*/) {
    applyConfig_ = true;
    Q_EMIT settingsChanged();
  });
}

KeyboardLayoutConfig::~KeyboardLayoutConfig() {
}

void KeyboardLayoutConfig::loadSettings() {
  // load current settings from the output of setxkbmap command
  QProcess setxkbmap;
  setxkbmap.start(QLatin1String("setxkbmap -query -verbose 5"));
  setxkbmap.waitForFinished();
  if(setxkbmap.exitStatus() == QProcess::NormalExit) {
    QList<QByteArray> layouts, variants;
    while(!setxkbmap.atEnd()) {
      QByteArray line = setxkbmap.readLine();
      if(line.startsWith("model:")) {
        keyboardModel_ = QString::fromLatin1(line.mid(6).trimmed());
      }
      else if(line.startsWith("layout:")) {
        layouts = line.mid(7).trimmed().split(',');
      }
      else if(line.startsWith("variant:")) {
        variants = line.mid(8).trimmed().split(',');
      }
      else if(line.startsWith("options:")) {
        const QList<QByteArray> options = line.mid(9).trimmed().split(',');
        for(const QByteArray &option : options) {
          if(option.startsWith("grp:"))
            switchKey_ = QString::fromLatin1(option);
          else
            currentOptions_ << QString::fromLatin1(option);
        }
      }
    }

    const int size = layouts.size(), variantsSize = variants.size();
    for(int i = 0; i < size; ++i) {
      currentLayouts_.append(QPair<QString, QString>(QString::fromUtf8(layouts.at(i)), variantsSize > 0 ? QString::fromUtf8(variants.at(i)) : QString()));
    }

    setxkbmap.close();
  }
}

enum ListSection{
  NoSection,
  ModelSection,
  LayoutSection,
  VariantSection,
  OptionSection
};

void KeyboardLayoutConfig::loadLists() {
  // load known lists from xkb data files
  // XKBD_BASELIST_PATH is os dependent see keyboardlayoutconfig.h
  QFile file(QLatin1String(XKBD_BASELIST_PATH));
  if(file.open(QIODevice::ReadOnly)) {
    ListSection section = NoSection;
    while(!file.atEnd()) {
      QByteArray line = file.readLine().trimmed();
      if(section == NoSection) {
        if(line.startsWith("! model"))
          section = ModelSection;
        else if(line.startsWith("! layout"))
          section = LayoutSection;
        else if(line.startsWith("! variant"))
          section = VariantSection;
        else if(line.startsWith("! option"))
          section = OptionSection;
      }
      else {
        if(line.isEmpty()) {
          section = NoSection;
          continue;
        }
        int sep = line.indexOf(' ');
        QString name = QString::fromLatin1(line.constData(), sep);
        while(line[sep] ==' ') // skip spaces
          ++sep;
        QString description = QString::fromUtf8(line.constData() + sep);

        switch(section) {
          case ModelSection: {
            ui.keyboardModel->addItem(description, name);
            break;
          }
          case LayoutSection:
            knownLayouts_[name] = KeyboardLayoutInfo(description);
            break;
          case VariantSection: {
            // the descriptions of variants are prefixed by their language ids
            sep = description.indexOf(QLatin1String(": "));
            if(sep >= 0) {
              QString lang = description.left(sep);
              QMap<QString, KeyboardLayoutInfo>::iterator it = knownLayouts_.find(lang);
              if(it != knownLayouts_.end()) {
                KeyboardLayoutInfo& info = *it;
                info.variants.append(LayoutVariantInfo(name, description.mid(sep + 2)));
              }
            }
            break;
          }
          case OptionSection:
            if(line.startsWith("grp:")) { // key used to switch to another layout
              ui.switchKey->addItem(description, name);
            }
            break;
          default:;
        }
      }
    }
    file.close();
  }
}

void KeyboardLayoutConfig::initControls() {
  QList<QPair<QString, QString> >::iterator it;
  for(it = currentLayouts_.begin(); it != currentLayouts_.end(); ++it) {
    QString name = it->first;
    QString variant = it->second;
    addLayout(name, variant);
  }

  int n = ui.keyboardModel->count();
  int row;
  for(row = 0; row < n; ++row) {
    if(ui.keyboardModel->itemData(row, Qt::UserRole).toString() == keyboardModel_) {
      ui.keyboardModel->setCurrentIndex(row);
      break;
    }
  }

  n = ui.switchKey->count();
  for(row = 0; row < n; ++row) {
    if(ui.switchKey->itemData(row, Qt::UserRole).toString() == switchKey_) {
      ui.switchKey->setCurrentIndex(row);
      break;
    }
  }

}

void KeyboardLayoutConfig::addLayout(QString name, QString variant) {
  qDebug() << "add" << name << variant;
  const KeyboardLayoutInfo& info = knownLayouts_.value(name);
  QTreeWidgetItem* item = new QTreeWidgetItem();
  item->setData(0, Qt::DisplayRole, info.description);
  item->setData(0, Qt::UserRole, name);
  const LayoutVariantInfo* vinfo = info.findVariant(variant);
  if(vinfo) {
    item->setData(1, Qt::DisplayRole, vinfo->description);
    item->setData(1, Qt::UserRole, variant);
  }
  ui.layouts->addTopLevelItem(item);
}

void KeyboardLayoutConfig::reset() {
  applyConfig_ = true;
  ui.layouts->clear();
  initControls();
  applyConfig();
}

void KeyboardLayoutConfig::applyConfig() {
  if(!applyConfig_)
    return;
  applyConfig_ = false;

  // call setxkbmap to apply the changes
  QProcess setxkbmap;
  // clear existing options
  setxkbmap.start(QStringLiteral("setxkbmap -option"));
  setxkbmap.waitForFinished();
  setxkbmap.close();

  QString command = QStringLiteral("setxkbmap");
  // set keyboard model
  QString model;
  int cur_model = ui.keyboardModel->currentIndex();
  if(cur_model >= 0) {
    model = ui.keyboardModel->itemData(cur_model, Qt::UserRole).toString();
    command += QLatin1String(" -model ");
    command += model;
  }

  // set keyboard layout
  int n = ui.layouts->topLevelItemCount();
  QString layouts, variants;
  if(n > 0) {
    for(int row = 0; row < n; ++row) {
      QTreeWidgetItem* item = ui.layouts->topLevelItem(row);
      layouts += item->data(0, Qt::UserRole).toString();
      variants += item->data(1, Qt::UserRole).toString();
      if(row < n - 1) { // not the last row
        layouts += QLatin1Char(',');
        variants += QLatin1Char(',');
      }
    }
    command += QLatin1String(" -layout ");
    command += layouts;

    if (variants.indexOf(QLatin1Char(',')) > -1 || !variants.isEmpty()) {
      command += QLatin1String(" -variant ");
      command += variants;
    }
  }

  for(const QString& option : qAsConst(currentOptions_)) {
    if (!option.startsWith(QLatin1String("grp:"))) {
      command += QLatin1String(" -option ");
      command += option;
    }
  }

  QString switchKey;
  int cur_switch_key = ui.switchKey->currentIndex();
  if(cur_switch_key > 0) { // index 0 is "None"
    switchKey = ui.switchKey->itemData(cur_switch_key, Qt::UserRole).toString();
    command += QLatin1String(" -option ");
    command += switchKey;
  }

  qDebug() << command;

  // execute the command line
  setxkbmap.start(command);
  setxkbmap.waitForFinished();

  // save to lxqt-session config file.
  settings->beginGroup(QStringLiteral("Keyboard"));
  settings->setValue(QStringLiteral("layout"), layouts);
  settings->setValue(QStringLiteral("variant"), variants);
  settings->setValue(QStringLiteral("model"), model);
  if(switchKey.isEmpty() && currentOptions_ .isEmpty())
    settings->remove(QStringLiteral("options"));
  else
    settings->setValue(QStringLiteral("options"), switchKey.isEmpty() ? currentOptions_ : (currentOptions_ << switchKey));
  settings->endGroup();
}

void KeyboardLayoutConfig::onAddLayout() {
  SelectKeyboardLayoutDialog dlg(knownLayouts_, this);
  if(dlg.exec() == QDialog::Accepted) {
    addLayout(dlg.selectedLayout(), dlg.selectedVariant());
    applyConfig_ = true;
    Q_EMIT settingsChanged();
  }
}

void KeyboardLayoutConfig::onRemoveLayout() {
  if(ui.layouts->topLevelItemCount() > 1) {
    QTreeWidgetItem* item = ui.layouts->currentItem();
    if(item) {
      delete item;
      applyConfig_ = true;
      Q_EMIT settingsChanged();
    }
  }
}

void KeyboardLayoutConfig::onMoveDown() {
  QTreeWidgetItem* item = ui.layouts->currentItem();
  if(!item)
    return;
  int pos = ui.layouts->indexOfTopLevelItem(item);
  if(pos < ui.layouts->topLevelItemCount() - 1) { // not the last item
    ui.layouts->takeTopLevelItem(pos);
    ui.layouts->insertTopLevelItem(pos + 1, item);
    ui.layouts->setCurrentItem(item);
    applyConfig_ = true;
    Q_EMIT settingsChanged();
  }
}

void KeyboardLayoutConfig::onMoveUp() {
  QTreeWidgetItem* item = ui.layouts->currentItem();
  if(!item)
    return;
  int pos = ui.layouts->indexOfTopLevelItem(item);
  if(pos > 0) { // not the first item
    ui.layouts->takeTopLevelItem(pos);
    ui.layouts->insertTopLevelItem(pos - 1, item);
    ui.layouts->setCurrentItem(item);
    applyConfig_ = true;
    Q_EMIT settingsChanged();
  }
}

