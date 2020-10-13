/*
 * trayicon.h - system-independent trayicon class (adapted from Qt example)
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 2003  Justin Karneges
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef CS_TRAYICON_H
#define CS_TRAYICON_H

#include <QWidget>
#include <QMenu>

#include <QSystemTrayIcon>

//----------------------------------------------------------------------------
// TrayIconPrivate
//----------------------------------------------------------------------------

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT
  public:
    TrayIcon(const QPixmap &, const QString &, QMenu *popup = nullptr,
             QWidget *parent = nullptr, const char *name = nullptr);
    ~TrayIcon() override;

    // use WindowMaker dock mode.  ignored on non-X11 platforms
    void setWMDock(bool use) { isWMDock = use; }
    bool checkWMDock() { return isWMDock; }
    bool hasSysTray;
    void setSysTray(bool /*val*/)
    { // boolSysTray=val;
    }

    // Set a popup menu to handle RMB
    void setPopup(QMenu *);
    QMenu *popup() const;

    QPixmap icon() const;

    void sysInstall();
    void sysRemove();

    void init_TrayIconFreeDesktop();
    void init_WindowMakerDock();

  public slots:
    void setIcon(const QPixmap &icon);
    void setToolTip(const QString &tip);
signals:
    void clicked(const QPoint &);
    void clicked(const QPoint &, int);
    void doubleClicked(const QPoint &);
    void closed();

  protected:
    //	bool event( QEvent *e );
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);
    virtual void leaveEvent(QEvent *);

    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *e);
    //	void hideEvent ( QHideEvent *e ); // called after hide()
    void closeEvent(QCloseEvent *e);

  private:
    QMenu *pop;
    QPixmap pm;
    QString tip;
    QPoint tip_pos;

    bool isWMDock;
    //	bool boolSysTray; //DEL?
    bool inTray;
    bool flag_systray_ready;
    bool flag_show_tip;

    // DEL void sysUpdateToolTip();
};

#endif // CS_TRAYICON_H
