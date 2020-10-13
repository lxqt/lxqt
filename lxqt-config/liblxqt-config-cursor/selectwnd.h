/* coded by Ketmar // Vampire Avalon (psyc://ketmar.no-ip.org/~Ketmar)
 * (c)DWTFYW
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */

// 2014-04-10 modified by Hong Jen Yee (PCMan) for integration with lxqt-config-input

#ifndef SELECTWND_H
#define SELECTWND_H

#include <QObject>
#include <QWidget>
#include <QPersistentModelIndex>
#include <lxqtglobals.h>

namespace LXQt {
  class Settings;
}

namespace Ui {
class SelectWnd;
}

class XCursorThemeModel;

class LXQT_API SelectWnd : public QWidget
{
    Q_OBJECT

public:
    SelectWnd (LXQt::Settings* settings, QWidget *parent=0);
    ~SelectWnd ();

    void applyCusorTheme();

public slots:
    void setCurrent ();

signals:
    void settingsChanged();

protected:
    // void keyPressEvent (QKeyEvent *e);

private:
    bool iconsIsWritable () const;
    void selectRow (int) const;
    void selectRow (const QModelIndex &index) const { selectRow(index.row()); }

private slots:
    void currentChanged (const QModelIndex &current, const QModelIndex &previous);
    void on_btInstall_clicked ();
    void on_btRemove_clicked ();
    void handleWarning();
    void showDirInfo();
    void cursorSizeChaged(int size);

private:
    XCursorThemeModel *mModel;
    QPersistentModelIndex mAppliedIndex;
    LXQt::Settings* mSettings;
    Ui::SelectWnd *ui;
};

#endif
