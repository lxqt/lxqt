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


#ifndef LXQT_MAINMENU_H
#define LXQT_MAINMENU_H

#include "../panel/ilxqtpanelplugin.h"
#include <XdgMenu>

#ifdef HAVE_MENU_CACHE
#include <menu-cache/menu-cache.h>
#endif

#include <QLabel>
#include <QToolButton>
#include <QDomElement>
#include <QAction>
#include <QTimer>
#include <QKeySequence>

#include "menustyle.h"


class QMenu;
class QWidgetAction;
class QLineEdit;
class ActionView;
class LXQtBar;

namespace LXQt {
class PowerManager;
class ScreenSaver;
}

namespace GlobalKeyShortcut
{
class Action;
}

class LXQtMainMenu : public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT
public:
    LXQtMainMenu(const ILXQtPanelPluginStartupInfo &startupInfo);
    ~LXQtMainMenu();

    QString themeId() const { return QStringLiteral("MainMenu"); }
    virtual ILXQtPanelPlugin::Flags flags() const { return HaveConfigDialog ; }

    QWidget *widget() { return &mButton; }
    QDialog *configureDialog();

    bool isSeparate() const { return true; }

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    void setMenuFontSize();
    void setButtonIcon();

private:
    QToolButton mButton;
    QString mLogDir;
    QMenu* mMenu;
    GlobalKeyShortcut::Action *mShortcut;
    MenuStyle mTopMenuStyle;
    QWidgetAction * mSearchEditAction;
    QLineEdit * mSearchEdit;
    QWidgetAction * mSearchViewAction;
    ActionView * mSearchView;
    QAction * mMakeDirtyAction;
    bool mFilterMenu; //!< searching should perform hiding nonmatching items in menu
    bool mFilterShow; //!< searching should list matching items in top menu
    bool mFilterClear; //!< search field should be cleared upon showing the menu
    bool mFilterShowHideMenu; //!< while searching all (original) menu entries should be hidden
    bool mHeavyMenuChanges; //!< flag for filtering some mMenu events while heavy changes are performed

#ifdef HAVE_MENU_CACHE
    MenuCache* mMenuCache;
    MenuCacheNotifyId mMenuCacheNotify;
    static void menuCacheReloadNotify(MenuCache* cache, gpointer user_data);
#else
    XdgMenu mXdgMenu;
#endif

    QTimer mDelayedPopup;
    QTimer mHideTimer;
    QTimer mSearchTimer;
    QString mMenuFile;

protected slots:

    virtual void settingsChanged();
    void buildMenu();

private slots:
    void showMenu();
    void showHideMenu();
    void searchMenu();
    void setSearchFocus(QAction *action);
};

class LXQtMainMenuPluginLibrary: public QObject, public ILXQtPanelPluginLibrary
{
    Q_OBJECT
    // Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const { return new LXQtMainMenu(startupInfo);}
};

#endif
