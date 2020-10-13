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

#include "preferencesdialog.h"
#include "application.h"
#include <QDir>
#include <QStringBuilder>
#include <QKeyEvent>
#include <QWindow>
#include <QScreen>
#include <glib.h>
#include <QPushButton>
using namespace LxImage;

static QHash<QString, QString> ACTION_DISPLAY_NAMES;

void KeySequenceEdit::keyPressEvent(QKeyEvent* event) {
    // by not allowing multiple shortcuts,
    // the Qt bug that makes Meta a non-modifier is worked around
    clear();
    QKeySequenceEdit::keyPressEvent (event);
}

QWidget* Delegate::createEditor(QWidget* parent,
                                const QStyleOptionViewItem& /*option*/,
                                const QModelIndex& /*index*/) const {
  return new KeySequenceEdit(parent);
}

bool Delegate::eventFilter(QObject* object, QEvent* event) {
  QWidget* editor = qobject_cast<QWidget*>(object);
  if(editor && event->type() == QEvent::KeyPress) {
    int k = static_cast<QKeyEvent *>(event)->key();
    if (k == Qt::Key_Return || k == Qt::Key_Enter) {
      Q_EMIT QAbstractItemDelegate::commitData(editor);
      Q_EMIT QAbstractItemDelegate::closeEditor(editor);
      return true;
    }
  }
  return QStyledItemDelegate::eventFilter(object, event);
}
/*************************/
PreferencesDialog::PreferencesDialog(QWidget* parent):
  QDialog(parent) {
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose);
  ui.buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
  ui.buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
  ui.warningLabel->setStyleSheet(QStringLiteral("QLabel {background-color: #7d0000; color: white; border-radius: 3px; margin: 2px; padding: 5px;}"));
  ui.warningLabel->hide();
  warningTimer_ = nullptr;
  Delegate *del = new Delegate(ui.tableWidget);
  ui.tableWidget->setItemDelegate(del);
  ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  ui.tableWidget->horizontalHeader()->setSectionsClickable(true);
  ui.tableWidget->sortByColumn(0, Qt::AscendingOrder);
  ui.tableWidget->setToolTip(tr("Use a modifier key to clear a shortcut\nin the editing mode."));

  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();
  app->addWindow();

  initIconThemes(settings);
  ui.bgColor->setColor(settings.bgColor());
  ui.fullScreenBgColor->setColor(settings.fullScreenBgColor());
  ui.maxRecentFiles->setValue(settings.maxRecentFiles());
  ui.slideShowInterval->setValue(settings.slideShowInterval());
  ui.oulineBox->setChecked(settings.isOutlineShown());
  ui.annotationBox->setChecked(settings.isAnnotationsToolbarShown());

  // shortcuts
  initShortcuts();
}

PreferencesDialog::~PreferencesDialog() {
  if(warningTimer_) {
    warningTimer_->stop();
    delete warningTimer_;
    warningTimer_ = nullptr;
  }
  Application* app = static_cast<Application*>(qApp);
  app->removeWindow();
}

void PreferencesDialog::showEvent(QShowEvent* event) {
  QSize prefSize = static_cast<Application*>(qApp)->settings().getPrefSize();
  if(QWindow *window = windowHandle()) {
    if(QScreen *sc = window->screen()) {
      prefSize = prefSize.boundedTo(sc->availableGeometry().size() - QSize(50, 100));
    }
  }
  resize(prefSize);

  QDialog::showEvent(event);
}

void PreferencesDialog::accept() {
  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();

  // apply icon theme
  if(settings.useFallbackIconTheme()) {
    // only apply the value if icon theme combo box is in use
    // the combo box is hidden when auto-detection of icon theme by qt works.
    QString newIconTheme = ui.iconTheme->itemData(ui.iconTheme->currentIndex()).toString();
    if(newIconTheme != settings.fallbackIconTheme()) {
      settings.setFallbackIconTheme(newIconTheme);
      QIcon::setThemeName(newIconTheme);
      // update the UI by emitting a style change event
      const auto allWidgets = QApplication::allWidgets();
      for(QWidget *widget : allWidgets) {
        QEvent event(QEvent::StyleChange);
        QApplication::sendEvent(widget, &event);
      }
    }
  }

  settings.setBgColor(ui.bgColor->color());
  settings.setFullScreenBgColor(ui.fullScreenBgColor->color());
  settings.setMaxRecentFiles(ui.maxRecentFiles->value());
  settings.setSlideShowInterval(ui.slideShowInterval->value());
  settings.showOutline(ui.oulineBox->isChecked());
  settings.showAnnotationsToolbar(ui.annotationBox->isChecked());

  applyNewShortcuts();
  settings.save();
  QDialog::accept();
  app->applySettings();
}

static void findIconThemesInDir(QHash<QString, QString>& iconThemes, const QString& dirName) {
  QDir dir(dirName);
  const QStringList subDirs = dir.entryList(QDir::AllDirs);
  GKeyFile* kf = g_key_file_new();
  for(const QString& subDir : subDirs) {
    QString indexFile = dirName + QLatin1Char('/') + subDir + QStringLiteral("/index.theme");
    if(g_key_file_load_from_file(kf, indexFile.toLocal8Bit().constData(), GKeyFileFlags(0), nullptr)) {
      // FIXME: skip hidden ones
      // icon theme must have this key, so it has icons if it has this key
      // otherwise, it might be a cursor theme or any other kind of theme.
      if(g_key_file_has_key(kf, "Icon Theme", "Directories", nullptr)) {
        char* dispName = g_key_file_get_locale_string(kf, "Icon Theme", "Name", nullptr, nullptr);
        // char* comment = g_key_file_get_locale_string(kf, "Icon Theme", "Comment", NULL, NULL);
        iconThemes[subDir] = QString::fromUtf8(dispName);
        g_free(dispName);
      }
    }
  }
  g_key_file_free(kf);
}

void PreferencesDialog::initIconThemes(Settings& settings) {
  // check if auto-detection is done (for example, from xsettings)
  if(settings.useFallbackIconTheme()) { // auto-detection failed
    // load xdg icon themes and select the current one
    QHash<QString, QString> iconThemes;
    // user customed icon themes
    findIconThemesInDir(iconThemes, QString::fromUtf8(g_get_home_dir()) + QStringLiteral("/.icons"));

    // search for icons in system data dir
    const char* const* dataDirs = g_get_system_data_dirs();
    for(const char* const* dataDir = dataDirs; *dataDir; ++dataDir) {
      findIconThemesInDir(iconThemes, QString::fromUtf8((*dataDir)) + QStringLiteral("/icons"));
    }

    iconThemes.remove(QStringLiteral("hicolor")); // remove hicolor, which is only a fallback
    QHash<QString, QString>::const_iterator it;
    for(it = iconThemes.constBegin(); it != iconThemes.constEnd(); ++it) {
      ui.iconTheme->addItem(it.value(), it.key());
    }
    ui.iconTheme->model()->sort(0); // sort the list of icon theme names

    // select current theme name
    int n = ui.iconTheme->count();
    int i;
    for(i = 0; i < n; ++i) {
      QVariant itemData = ui.iconTheme->itemData(i);
      if(itemData == settings.fallbackIconTheme()) {
        break;
      }
    }
    if(i >= n)
      i = 0;
    ui.iconTheme->setCurrentIndex(i);
  }
  else { // auto-detection of icon theme works, hide the fallback icon theme combo box.
    ui.iconThemeLabel->hide();
    ui.iconTheme->hide();
  }
}

void PreferencesDialog::showWarning(const QString& text, bool temporary) {
  if(text.isEmpty()) {
    permanentWarning_.clear();
    ui.warningLabel->clear();
    ui.warningLabel->hide();
  }
  else {
    ui.warningLabel->setText(text);
    ui.warningLabel->show();
    if(!temporary) {
      permanentWarning_ = text;
    }
    else {
      if(warningTimer_ == nullptr) {
        warningTimer_ = new QTimer();
        warningTimer_->setSingleShot (true);
        connect(warningTimer_, &QTimer::timeout, [this] {
          ui.warningLabel->setText(permanentWarning_);
          ui.warningLabel->setVisible(!permanentWarning_.isEmpty());
        });
      }
      warningTimer_->start(2500);
    }
  }
}

void PreferencesDialog::initShortcuts() {
  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();

  // pair display and object names together (to get the latter from the former)
  if(ACTION_DISPLAY_NAMES.isEmpty()) {
    QHash<QString, Application::ShortcutDescription>::const_iterator iter = app->defaultShortcuts().constBegin();
    while (iter != app->defaultShortcuts().constEnd()) {
      ACTION_DISPLAY_NAMES.insert(iter.value().displayText, iter.key());
      ++ iter;
    }
  }

  // fill the table widget
  ui.tableWidget->setRowCount(ACTION_DISPLAY_NAMES.size());
  ui.tableWidget->setSortingEnabled(false);
  int index = 0;
  QHash<QString, QString> ca = settings.customShortcutActions();
  QHash<QString, QString>::const_iterator iter = ACTION_DISPLAY_NAMES.constBegin();
  while(iter != ACTION_DISPLAY_NAMES.constEnd()) {
    // add the action item
    QTableWidgetItem *item = new QTableWidgetItem(iter.key());
    item->setFlags(item->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
    ui.tableWidget->setItem(index, 0, item);

    // NOTE: Shortcuts are saved in the PortableText format but
    // their texts should be added to the table in the NativeText format.
    QString shortcut;
    if(ca.contains(iter.value())) { // a cusrom shortcut
      shortcut = ca.value(iter.value());
      QKeySequence keySeq(shortcut, QKeySequence::PortableText);
      shortcut = keySeq.toString(QKeySequence::NativeText);
    }
    else { // a default shortcut
      shortcut = app->defaultShortcuts().value(iter.value()).shortcut.toString(QKeySequence::NativeText);
    }
    ui.tableWidget->setItem(index, 1, new QTableWidgetItem(shortcut));
    allShortcuts_.insert(iter.key(), shortcut);

    ++ iter;
    ++ index;
  }
  ui.tableWidget->setSortingEnabled(true);
  ui.tableWidget->setCurrentCell(0, 1);

  const auto shortcuts = allShortcuts_.values();
  for(int i = 0; i < shortcuts.size(); ++i) {
    if(!shortcuts.at(i).isEmpty() && shortcuts.indexOf(shortcuts.at(i), i + 1) > -1) {
      showWarning(tr("<b>Warning: Ambiguous shortcut detected!</b>"), false);
      break;
    }
  }

  connect(ui.tableWidget, &QTableWidget::itemChanged, this, &PreferencesDialog::onShortcutChange);
  connect(ui.defaultButton, &QAbstractButton::clicked, this, &PreferencesDialog::restoreDefaultShortcuts);
  ui.defaultButton->setDisabled(ca.isEmpty());
}

void PreferencesDialog::onShortcutChange(QTableWidgetItem* item) {
  QString desc = ui.tableWidget->item(ui.tableWidget->currentRow(), 0)->text();
  QString txt = item->text();

  const auto shortcuts = allShortcuts_.values();
  for(const auto& s : shortcuts) {
    if(!s.isEmpty() && s == txt) {
      showWarning(tr("<b>Ambiguous shortcut not accepted.</b>"));
      disconnect(ui.tableWidget, &QTableWidget::itemChanged, this, &PreferencesDialog::onShortcutChange);
      item->setText(allShortcuts_.value (desc)); // restore the previous shortcut
      connect(ui.tableWidget, &QTableWidget::itemChanged, this, &PreferencesDialog::onShortcutChange);
      return;
    }
  }
  showWarning(QString()); // ambiguous shortcuts might have been added manually

  allShortcuts_.insert(desc, txt);

  if(!txt.isEmpty()) {
    // NOTE: The QKeySequenceEdit text is in the NativeText format but
    // it should be converted into the PortableText format for saving.
    QKeySequence keySeq(txt, QKeySequence::NativeText);
    txt = keySeq.toString(QKeySequence::PortableText);
  }
  modifiedShortcuts_.insert(ACTION_DISPLAY_NAMES.value(desc), txt);

  // also set the state of the Default button
  Application* app = static_cast<Application*>(qApp);
  for(int i = 0; i < ui.tableWidget->rowCount(); ++i) {
    const QString objectName = ACTION_DISPLAY_NAMES.value(ui.tableWidget->item(i, 0)->text());
    if(app->defaultShortcuts().value(objectName).shortcut.toString(QKeySequence::PortableText)
       != ui.tableWidget->item(i, 1)->text()) {
      ui.defaultButton->setEnabled(true);
      return;
    }
  }
  ui.defaultButton->setEnabled(false);
}

void PreferencesDialog::restoreDefaultShortcuts() {
  Application* app = static_cast<Application*>(qApp);
  if (modifiedShortcuts_.isEmpty()
      && app->settings().customShortcutActions().isEmpty()) {
    // do nothing if there's no custom shortcut
    return;
  }

  showWarning(QString());

  disconnect(ui.tableWidget, &QTableWidget::itemChanged, this, &PreferencesDialog::onShortcutChange);
  int cur = ui.tableWidget->currentColumn() == 0 ? 0 : ui.tableWidget->currentRow();
  ui.tableWidget->setSortingEnabled(false);

  // consider all shortcuts modified
  QHash<QString, Application::ShortcutDescription>::const_iterator iter = app->defaultShortcuts().constBegin();
  while (iter != app->defaultShortcuts().constEnd()) {
    modifiedShortcuts_.insert(iter.key(), iter.value().shortcut.toString(QKeySequence::PortableText));
    ++ iter;
  }

  // restore default shortcuts in the GUI
  for(int i = 0; i < ui.tableWidget->rowCount(); ++i) {
    const QString objectName = ACTION_DISPLAY_NAMES.value(ui.tableWidget->item(i, 0)->text());
    QString s = app->defaultShortcuts().value(objectName).shortcut.toString(QKeySequence::PortableText);
    ui.tableWidget->item(i, 1)->setText(s);
  }

  ui.tableWidget->setSortingEnabled(true);
  ui.tableWidget->setCurrentCell(cur, 1);
  connect(ui.tableWidget, &QTableWidget::itemChanged, this, &PreferencesDialog::onShortcutChange);
  ui.defaultButton->setEnabled(false);
}

void PreferencesDialog::applyNewShortcuts() {
  // remember the modified shortcuts if they are different from the default ones
  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();
  QHash<QString, QString>::const_iterator it = modifiedShortcuts_.constBegin();
  while(it != modifiedShortcuts_.constEnd()) {
    if(app->defaultShortcuts().value(it.key()).shortcut.toString(QKeySequence::PortableText) == it.value()) {
      settings.removeShortcut(it.key());
    }
    else {
      settings.addShortcut(it.key(), it.value());
    }
    ++it;
  }
}

void PreferencesDialog::done(int r) {
  // remember size
  Settings& settings = static_cast<Application*>(qApp)->settings();
  settings.setPrefSize(size());

  QDialog::done(r);
  deleteLater();
}

