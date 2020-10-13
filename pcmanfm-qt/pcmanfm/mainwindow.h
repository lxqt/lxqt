/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef FM_MAIN_WINDOW_H
#define FM_MAIN_WINDOW_H

#include "ui_main-win.h"
#include <QPointer>
#include <QMainWindow>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <QTabWidget>
#include <QMessageBox>
#include <QTabBar>
#include <QStackedWidget>
#include <QSplitter>
#include "launcher.h"
#include "tabbar.h"
#include <libfm-qt/core/filepath.h>
#include <libfm-qt/core/bookmarks.h>
#include <libfm-qt/mountoperation.h>

namespace Fm {
class PathEdit;
class PathBar;
}

namespace PCManFM {

class ViewFrame : public QFrame {
    Q_OBJECT
public:
    ViewFrame(QWidget* parent = nullptr);
    ~ViewFrame() {};

    void createTopBar(bool usePathButtons);
    void removeTopBar();

    QWidget* getTopBar() const {
        return topBar_;
    }
    TabBar* getTabBar() const {
        return tabBar_;
    }
    QStackedWidget* getStackedWidget() const {
        return stackedWidget_;
    }

private:
    QWidget* topBar_;
    TabBar* tabBar_;
    QStackedWidget* stackedWidget_;
};

//======================================================================

class TabPage;
class Settings;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(Fm::FilePath path = Fm::FilePath());
    virtual ~MainWindow();

    void chdir(Fm::FilePath path, ViewFrame* viewFrame);
    void chdir(Fm::FilePath path) {
        chdir(path, activeViewFrame_);
    }

    int addTab(Fm::FilePath path, ViewFrame* viewFrame);
    int addTab(Fm::FilePath path) {
        return addTab(path, activeViewFrame_);
    }

    TabPage* currentPage(ViewFrame* viewFrame) {
        return reinterpret_cast<TabPage*>(viewFrame->getStackedWidget()->currentWidget());
    }
    TabPage* currentPage() {
        return currentPage(activeViewFrame_);
    }

    void updateFromSettings(Settings& settings);

    static MainWindow* lastActive() {
        return lastActive_;
    }

    QList<Fm::MountOperation*> pendingMountOperations() const;

protected Q_SLOTS:

    void onPathEntryReturnPressed();
    void onPathBarChdir(const Fm::FilePath& dirPath);
    void onPathBarMiddleClickChdir(const Fm::FilePath &dirPath);

    void on_actionNewTab_triggered();
    void on_actionNewWin_triggered();
    void on_actionNewFolder_triggered();
    void on_actionNewBlankFile_triggered();
    void on_actionCloseTab_triggered();
    void on_actionCloseWindow_triggered();
    void on_actionFileProperties_triggered();
    void on_actionFolderProperties_triggered();

    void on_actionCut_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionDelete_triggered();
    void on_actionRename_triggered();
    void on_actionBulkRename_triggered();
    void on_actionSelectAll_triggered();
    void on_actionInvertSelection_triggered();
    void on_actionPreferences_triggered();

    void on_actionGoBack_triggered();
    void on_actionGoForward_triggered();
    void on_actionGoUp_triggered();
    void on_actionHome_triggered();
    void on_actionReload_triggered();
    void on_actionConnectToServer_triggered();

    void on_actionIconView_triggered();
    void on_actionCompactView_triggered();
    void on_actionDetailedList_triggered();
    void on_actionThumbnailView_triggered();

    void on_actionGo_triggered();
    void on_actionShowHidden_triggered(bool check);
    void on_actionShowThumbnails_triggered(bool check);
    void on_actionSplitView_triggered(bool check);
    void on_actionPreserveView_triggered(bool checked);

    void on_actionByFileName_triggered(bool checked);
    void on_actionByMTime_triggered(bool checked);
    void on_actionByDTime_triggered(bool checked);
    void on_actionByOwner_triggered(bool checked);
    void on_actionByGroup_triggered(bool checked);
    void on_actionByFileType_triggered(bool checked);
    void on_actionByFileSize_triggered(bool checked);
    void on_actionAscending_triggered(bool checked);
    void on_actionDescending_triggered(bool checked);
    void on_actionFolderFirst_triggered(bool checked);
    void on_actionHiddenLast_triggered(bool checked);
    void on_actionCaseSensitive_triggered(bool checked);
    void on_actionFilter_triggered(bool checked);
    void on_actionUnfilter_triggered();
    void on_actionShowFilter_triggered();

    void on_actionLocationBar_triggered(bool checked);
    void on_actionPathButtons_triggered(bool checked);

    void on_actionApplications_triggered();
    void on_actionComputer_triggered();
    void on_actionTrash_triggered();
    void on_actionNetwork_triggered();
    void on_actionDesktop_triggered();
    void on_actionAddToBookmarks_triggered();
    void on_actionEditBookmarks_triggered();

    void on_actionOpenTerminal_triggered();
    void on_actionOpenAsRoot_triggered();
    void on_actionCopyFullPath_triggered();
    void on_actionFindFiles_triggered();

    void on_actionAbout_triggered();
    void on_actionHiddenShortcuts_triggered();

    void onBookmarkActionTriggered();

    void onTabBarCloseRequested(int index);
    void onTabBarCurrentChanged(int index);
    void onTabBarTabMoved(int from, int to);

    void onShortcutPrevTab();
    void onShortcutNextTab();
    void onShortcutJumpToTab();

    void onStackedWidgetWidgetRemoved(int index);

    void onTabPageTitleChanged(QString title);
    void onTabPageStatusChanged(int type, QString statusText);
    void onTabPageSortFilterChanged();

    void onSidePaneChdirRequested(int type, const Fm::FilePath &path);
    void onSidePaneOpenFolderInNewWindowRequested(const Fm::FilePath &path);
    void onSidePaneOpenFolderInNewTabRequested(const Fm::FilePath &path);
    void onSidePaneOpenFolderInTerminalRequested(const Fm::FilePath &path);
    void onSidePaneCreateNewFolderRequested(const Fm::FilePath &path);
    void onSidePaneModeChanged(Fm::SidePane::Mode mode);
    void on_actionSidePane_triggered(bool check);
    void onSplitterMoved(int pos, int index);
    void onResetFocus();

    void onBackForwardContextMenu(QPoint pos);

    void onFolderUnmounted();

    void tabContextMenu(const QPoint& pos);
    void onTabBarClicked(int index);
    void closeLeftTabs();
    void closeRightTabs();
    void closeOtherTabs() {
        closeLeftTabs();
        closeRightTabs();
    }
    void focusPathEntry();
    void toggleMenuBar(bool checked);
    void detachTab();

    void onBookmarksChanged();

    void onSettingHiddenPlace(const QString& str, bool hide);

protected:
    bool event(QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void closeTab(int index, ViewFrame* viewFrame);
    void closeTab(int index) {
        closeTab(index, activeViewFrame_);
    }
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void closeEvent(QCloseEvent* event) override;
    virtual void dragEnterEvent(QDragEnterEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;
    virtual bool eventFilter(QObject* watched, QEvent* event);

private:
    void loadBookmarksMenu();
    void updateUIForCurrentPage(bool setFocus = true);
    void updateViewMenuForCurrentPage();
    void updateEditSelectedActions();
    void updateStatusBarForCurrentPage();
    void setRTLIcons(bool isRTL);
    void createPathBar(bool usePathButtons);
    void addViewFrame(const Fm::FilePath& path);
    ViewFrame* viewFrameForTabPage(TabPage* page);
    int addTabWithPage(TabPage* page, ViewFrame* viewFrame, Fm::FilePath path = Fm::FilePath());
    void dropTab();

private:
    Ui::MainWindow ui;
    Fm::PathEdit* pathEntry_;
    Fm::PathBar* pathBar_;
    QLabel* fsInfoLabel_;
    std::shared_ptr<Fm::Bookmarks> bookmarks_;
    Launcher fileLauncher_;
    int rightClickIndex_;
    bool updatingViewMenu_;
    QAction* menuSep_;
    QAction* menuSpacer_;

    ViewFrame* activeViewFrame_;
    // The split mode of this window is changed only from its settings,
    // not from another window. So, we get the mode at the start and keep it.
    bool splitView_;

    static QPointer<MainWindow> lastActive_;
};

}

#endif // FM_MAIN_WINDOW_H
