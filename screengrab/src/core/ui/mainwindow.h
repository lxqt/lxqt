/***************************************************************************
 *   Copyright (C) 2009 - 2013 by Artem 'DOOMer' Galichkin                 *
 *   doomer3d@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "about.h"
#include "configwidget.h"

#include <QMenu>
#include <QSystemTrayIcon>
#include <QDesktopServices>
#include <QCloseEvent>
#include <QShortcut>
#include <QUrl>
#include <QSignalMapper>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow //, public screengrab
{
    Q_OBJECT
public:
    MainWindow( QWidget *parent = 0);
    ~MainWindow();
    void show();
    bool isTrayed() const;
    void showTrayMessage(const QString& header, const QString& message);
    void setConfig(Config *config);
    void updatePixmap(QPixmap *pMap);
    void updateModulesActions(const QList<QAction *> &list);
    void updateModulesMenus(const QList<QMenu *> &list);

public Q_SLOTS:
    void showWindow(const QString& str);
    void windowHideShow();
    void hideToShot();
    void restoreFromShot();

protected:
    void closeEvent(QCloseEvent *e);
    void changeEvent(QEvent *e);
    void resizeEvent(QResizeEvent *event); // event resuze window
    bool eventFilter(QObject *obj, QEvent *event);

private:
    Ui::MainWindow *_ui;

    QSystemTrayIcon *_trayIcon;
    QAction *actHideShow;
    QAction *actNew;
    QAction *actSave;
    QAction *actCopy;
    QAction *actOptions;
    QAction *actAbout;
    QAction *actHelp;
    QAction *actQuit;
    Config *_conf;
    QMenu *_trayMenu;
    QShortcut *_hideWnd;
    bool _trayed;
    QIcon appIcon;

    void displatScreenToolTip();
    void createTray();
    void killTray();
    void disableTrayMenuActions(bool disable);
    void updateShortcuts();

private Q_SLOTS:
    void saveScreen();
    void showHelp();
    void showOptions();
    void showAbout();
    void delayBoxChange(int);
    void typeScreenShotChange(int type);
    void checkIncludeCursor(bool include);
    void checkNoDecoration(bool noDecor);
    void checkZommMouseArea(bool zoom);
    void updateUI();
    void trayClick(QSystemTrayIcon::ActivationReason reason);
};

#endif // MAINWINDOW_H
