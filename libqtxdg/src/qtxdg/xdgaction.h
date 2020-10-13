/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef QTXDG_XDGACTION_H
#define QTXDG_XDGACTION_H

#include "xdgmacros.h"
#include "xdgdesktopfile.h"

#include <QAction>
#include <QString>


/*******************************************************************/ /*!
  @brief The XdgAction class provides an QAction object based on XdgDesktopFile.

  The following properties of the action are set based on the desktopFile.
    Text    - XdgDesktopFile localizeValue("Name")
    Icon    - XdgDesktopFile icon()
    ToolTip - XdgDesktopFile localizeValue("Comment")

  Internally this function will create a copy of the desktopFile, so you
  can delete original XdgDesktopFile object.

  When an action is activated by the user; for example, when the user clicks
  a menu option, toolbar button or when trigger() was called, XdgAction start
  the application defined in XdgDesktopFile. @sa XdgDesktopFile::startDetached.
****************************************/
//*******************************************************************
class QTXDG_API XdgAction : public QAction
{
    Q_OBJECT
public:
    explicit XdgAction(QObject *parent=nullptr);
    explicit XdgAction(const XdgDesktopFile& desktopFile, QObject *parent=nullptr);
    explicit XdgAction(const XdgDesktopFile* desktopFile, QObject *parent=nullptr);
    explicit XdgAction(const QString& desktopFileName, QObject *parent=nullptr);
    // Constructs a XdgAction that is a copy of the given XdgAction.
    explicit XdgAction(const XdgAction& other, QObject *parent=nullptr);

    /// Destroys the object and frees allocated resources.
    ~XdgAction() override;
    XdgAction& operator=(const XdgAction& other);

    //! Returns true if the XdgAction is valid; otherwise returns false.
    bool isValid() const;

    const XdgDesktopFile& desktopFile() const { return mDesktopFile; }

public Q_SLOTS:
    void updateIcon();

private Q_SLOTS:
    void runConmmand() const;

private:
    void load(const XdgDesktopFile& desktopFile);

    XdgDesktopFile mDesktopFile;
};

#endif // QTXDG_XDGACTION_H
