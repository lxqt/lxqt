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

#ifndef LXIMAGE_PREFERENCESDIALOG_H
#define LXIMAGE_PREFERENCESDIALOG_H

#include <QDialog>
#include <QStyledItemDelegate>
#include <QKeySequenceEdit>
#include <QTimer>
#include "ui_preferencesdialog.h"

namespace LxImage {

class Settings;

class KeySequenceEdit : public QKeySequenceEdit {
  Q_OBJECT
public:
  KeySequenceEdit(QWidget* parent = nullptr): QKeySequenceEdit(parent) {}

protected:
  virtual void keyPressEvent(QKeyEvent* event);
};

class Delegate : public QStyledItemDelegate
{
  Q_OBJECT
public:
  Delegate (QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

  virtual QWidget* createEditor(QWidget* parent,
                                const QStyleOptionViewItem&,
                                const QModelIndex&) const;

protected:
  virtual bool eventFilter(QObject* object, QEvent *event);
};

class PreferencesDialog : public QDialog {
  Q_OBJECT
public:
  explicit PreferencesDialog(QWidget* parent = nullptr);
  virtual ~PreferencesDialog();

  virtual void accept();
  virtual void done(int r);

protected:
  virtual void showEvent(QShowEvent* event);

private Q_SLOTS:
  void onShortcutChange(QTableWidgetItem* item);
  void restoreDefaultShortcuts();

private:
  void initIconThemes(Settings& settings);
  void initShortcuts();
  void applyNewShortcuts();
  void showWarning(const QString& text, bool temporary = true);

private:
  Ui::PreferencesDialog ui;
  QHash<QString, QString> modifiedShortcuts_;
  QHash<QString, QString> allShortcuts_; // only used for checking ambiguity
  QString permanentWarning_;
  QTimer *warningTimer_;
};

}

#endif // LXIMAGE_PREFERENCESDIALOG_H
