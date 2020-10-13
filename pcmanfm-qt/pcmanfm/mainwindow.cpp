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

#include "mainwindow.h"

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QSplitter>
#include <QToolButton>
#include <QShortcut>
#include <QKeySequence>
#include <QSettings>
#include <QMimeData>
#include <QStandardPaths>
#include <QClipboard>
#include <QDebug>
#include <QPushButton>
#include "tabpage.h"
#include "launcher.h"
#include <libfm-qt/filemenu.h>
#include <libfm-qt/bookmarkaction.h>
#include <libfm-qt/fileoperation.h>
#include <libfm-qt/utilities.h>
#include <libfm-qt/filepropsdialog.h>
#include <libfm-qt/pathedit.h>
#include <libfm-qt/pathbar.h>
#include <libfm-qt/core/fileinfo.h>
#include "ui_about.h"
#include "ui_shortcuts.h"
#include "application.h"
#include "bulkrename.h"

using namespace Fm;

namespace PCManFM {

ViewFrame::ViewFrame(QWidget* parent):
    QFrame(parent),
    topBar_(nullptr) {
    QVBoxLayout* vBox = new QVBoxLayout;
    vBox->setContentsMargins(0, 0, 0, 0);

    tabBar_ = new TabBar;
    tabBar_->setFocusPolicy(Qt::NoFocus);
    stackedWidget_ = new QStackedWidget;
    vBox->addWidget(tabBar_);
    vBox->addWidget(stackedWidget_, 1);
    setLayout(vBox);

    // tabbed browsing interface
    tabBar_->setDocumentMode(true);
    tabBar_->setElideMode(Qt::ElideRight);
    tabBar_->setExpanding(false);
    tabBar_->setMovable(true); // reorder the tabs by dragging
    // switch to the tab under the cursor during dnd.
    tabBar_->setChangeCurrentOnDrag(true);
    tabBar_->setAcceptDrops(true);
    tabBar_->setContextMenuPolicy(Qt::CustomContextMenu);
}

void ViewFrame::createTopBar(bool usePathButtons) {
    if(QVBoxLayout* vBox = qobject_cast<QVBoxLayout*>(layout())) {
        if(usePathButtons) {
            if (qobject_cast<Fm::PathEdit*>(topBar_)) {
                delete topBar_;
                topBar_ = nullptr;
            }
            if(topBar_ == nullptr) {
                topBar_ = new Fm::PathBar();
                vBox->insertWidget(0, topBar_);
            }
        }
        else {
            if(qobject_cast<Fm::PathBar*>(topBar_)) {
                delete topBar_;
                topBar_ = nullptr;
            }
            if(topBar_ == nullptr) {
                topBar_ = new Fm::PathEdit();
                vBox->insertWidget(0, topBar_);
            }
        }
    }
}

void ViewFrame::removeTopBar() {
    if(topBar_ != nullptr) {
        if(QVBoxLayout* vBox = qobject_cast<QVBoxLayout*>(layout())) {
            vBox->removeWidget(topBar_);
            delete topBar_;
            topBar_ = nullptr;
        }
    }
}

//======================================================================

// static
QPointer<MainWindow> MainWindow::lastActive_;

MainWindow::MainWindow(Fm::FilePath path):
    QMainWindow(),
    pathEntry_(nullptr),
    pathBar_(nullptr),
    bookmarks_{Fm::Bookmarks::globalInstance()},
    fileLauncher_(this),
    rightClickIndex_(-1),
    updatingViewMenu_(false),
    menuSpacer_(nullptr),
    activeViewFrame_(nullptr) {

    Settings& settings = static_cast<Application*>(qApp)->settings();
    setAttribute(Qt::WA_DeleteOnClose);
    // setup user interface
    ui.setupUi(this);

    // add a warning label to the root instance
    if(geteuid() == 0) {
        QLabel *warningLabel = new QLabel(tr("Root Instance"));
        warningLabel->setAlignment(Qt::AlignCenter);
        warningLabel->setTextInteractionFlags(Qt::NoTextInteraction);
        warningLabel->setStyleSheet(QLatin1String("QLabel {background-color: #7d0000; color: white; font-weight:bold; border-radius: 3px; margin: 2px; padding: 5px;}"));
        ui.verticalLayout->addWidget(warningLabel);
        ui.verticalLayout->setStretch(0, 1);
    }

    splitView_ = settings.splitView();

    // hide menu items that are not usable
    //if(!uriExists("computer:///"))
    //  ui.actionComputer->setVisible(false);
    if(!settings.supportTrash()) {
        ui.actionTrash->setVisible(false);
    }

    // add a context menu for showing browse history to back and forward buttons
    QToolButton* forwardButton = static_cast<QToolButton*>(ui.toolBar->widgetForAction(ui.actionGoForward));
    forwardButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(forwardButton, &QToolButton::customContextMenuRequested, this, &MainWindow::onBackForwardContextMenu);
    QToolButton* backButton = static_cast<QToolButton*>(ui.toolBar->widgetForAction(ui.actionGoBack));
    backButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(backButton, &QToolButton::customContextMenuRequested, this, &MainWindow::onBackForwardContextMenu);

    connect(ui.actionCloseRight, &QAction::triggered, this, &MainWindow::closeRightTabs);
    connect(ui.actionCloseLeft, &QAction::triggered, this, &MainWindow::closeLeftTabs);
    connect(ui.actionCloseOther, &QAction::triggered, this, &MainWindow::closeOtherTabs);

    ui.actionFilter->setChecked(settings.showFilter());
    ui.actionShowThumbnails->setChecked(settings.showThumbnails());

    // menu
    ui.actionDelete->setText(settings.useTrash() ? tr("&Move to Trash") : tr("&Delete"));
    ui.actionDelete->setIcon(settings.useTrash() ? QIcon::fromTheme(QStringLiteral("user-trash")) : QIcon::fromTheme(QStringLiteral("edit-delete")));

    // side pane
    ui.sidePane->setVisible(settings.isSidePaneVisible());
    ui.actionSidePane->setChecked(settings.isSidePaneVisible());
    ui.sidePane->setIconSize(QSize(settings.sidePaneIconSize(), settings.sidePaneIconSize()));
    ui.sidePane->setMode(settings.sidePaneMode());
    ui.sidePane->restoreHiddenPlaces(settings.getHiddenPlaces());
    connect(ui.sidePane, &Fm::SidePane::chdirRequested, this, &MainWindow::onSidePaneChdirRequested);
    connect(ui.sidePane, &Fm::SidePane::openFolderInNewWindowRequested, this, &MainWindow::onSidePaneOpenFolderInNewWindowRequested);
    connect(ui.sidePane, &Fm::SidePane::openFolderInNewTabRequested, this, &MainWindow::onSidePaneOpenFolderInNewTabRequested);
    connect(ui.sidePane, &Fm::SidePane::openFolderInTerminalRequested, this, &MainWindow::onSidePaneOpenFolderInTerminalRequested);
    connect(ui.sidePane, &Fm::SidePane::createNewFolderRequested, this, &MainWindow::onSidePaneCreateNewFolderRequested);
    connect(ui.sidePane, &Fm::SidePane::modeChanged, this, &MainWindow::onSidePaneModeChanged);
    connect(ui.sidePane, &Fm::SidePane::hiddenPlaceSet, this, &MainWindow::onSettingHiddenPlace);

    // detect change of splitter position
    connect(ui.splitter, &QSplitter::splitterMoved, this, &MainWindow::onSplitterMoved);

    // add filesystem info to status bar
    fsInfoLabel_ = new QLabel(ui.statusbar);
    ui.statusbar->addPermanentWidget(fsInfoLabel_);

    // setup the splitter
    ui.splitter->setStretchFactor(1, 1); // only the right pane can be stretched
    QList<int> sizes;
    sizes.append(settings.splitterPos());
    sizes.append(300);
    ui.splitter->setSizes(sizes);

    // load bookmark menu
    connect(bookmarks_.get(), &Fm::Bookmarks::changed, this, &MainWindow::onBookmarksChanged);
    loadBookmarksMenu();

    // use generic icons for view actions only if theme icons don't exist
    ui.actionIconView->setIcon(QIcon::fromTheme(QLatin1String("view-list-icons"), style()->standardIcon(QStyle::SP_FileDialogContentsView)));
    ui.actionThumbnailView->setIcon(QIcon::fromTheme(QLatin1String("view-preview"), style()->standardIcon(QStyle::SP_FileDialogInfoView)));
    ui.actionCompactView->setIcon(QIcon::fromTheme(QLatin1String("view-list-text"), style()->standardIcon(QStyle::SP_FileDialogListView)));
    ui.actionDetailedList->setIcon(QIcon::fromTheme(QLatin1String("view-list-details"), style()->standardIcon(QStyle::SP_FileDialogDetailedView)));

    // Fix the menu groups which is not done by Qt designer
    // To my suprise, this was supported in Qt designer 3 :-(
    QActionGroup* group = new QActionGroup(ui.menu_View);
    group->setExclusive(true);
    group->addAction(ui.actionIconView);
    group->addAction(ui.actionCompactView);
    group->addAction(ui.actionThumbnailView);
    group->addAction(ui.actionDetailedList);

    group = new QActionGroup(ui.menuSorting);
    group->setExclusive(true);
    group->addAction(ui.actionByFileName);
    group->addAction(ui.actionByMTime);
    group->addAction(ui.actionByDTime);
    group->addAction(ui.actionByFileSize);
    group->addAction(ui.actionByFileType);
    group->addAction(ui.actionByOwner);
    group->addAction(ui.actionByGroup);

    group = new QActionGroup(ui.menuSorting);
    group->setExclusive(true);
    group->addAction(ui.actionAscending);
    group->addAction(ui.actionDescending);

    group = new QActionGroup(ui.menuPathBarStyle);
    group->setExclusive(true);
    group->addAction(ui.actionLocationBar);
    group->addAction(ui.actionPathButtons);

    // Add menubar actions to the main window this is necessary so that actions
    // shortcuts are still working when the menubar is hidden.
    addActions(ui.menubar->actions());

    // Show or hide the menu bar
    QMenu* menu = new QMenu(ui.toolBar);
    menu->addMenu(ui.menu_File);
    menu->addMenu(ui.menu_Edit);
    menu->addMenu(ui.menu_View);
    menu->addMenu(ui.menu_Go);
    menu->addMenu(ui.menu_Bookmarks);
    menu->addMenu(ui.menu_Tool);
    menu->addMenu(ui.menu_Help);
    ui.actionMenu->setMenu(menu);
    if(ui.actionMenu->icon().isNull()) {
        ui.actionMenu->setIcon(QIcon::fromTheme(QStringLiteral("applications-system")));
    }
    QToolButton* menuBtn = static_cast<QToolButton*>(ui.toolBar->widgetForAction(ui.actionMenu));
    menuBtn->setPopupMode(QToolButton::InstantPopup);

    menuSep_ = ui.toolBar->insertSeparator(ui.actionMenu);
    menuSep_->setVisible(!settings.showMenuBar() && !splitView_);
    ui.actionMenu->setVisible(!settings.showMenuBar());
    ui.menubar->setVisible(settings.showMenuBar());
    ui.actionMenu_bar->setChecked(settings.showMenuBar());
    connect(ui.actionMenu_bar, &QAction::triggered, this, &MainWindow::toggleMenuBar);

    // create shortcuts
    QShortcut* shortcut;
    shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(shortcut, &QShortcut::activated, [this] {
        if(currentPage()) {
            currentPage()->clearFilter();
            currentPage()->folderView()->childView()->setFocus();
        }
    });

    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Escape), this);
    connect(shortcut, &QShortcut::activated, [this] {
        if(ui.sidePane->isVisible() && ui.sidePane->view()) {
            ui.sidePane->view()->setFocus();
        }
    });

    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_L), this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::focusPathEntry);

    shortcut = new QShortcut(Qt::ALT + Qt::Key_D, this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::focusPathEntry);

    shortcut = new QShortcut(Qt::CTRL + Qt::Key_Tab, this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutNextTab);

    shortcut = new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Tab, this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutPrevTab);

    // Add Ctrl+PgUp and Ctrl+PgDown as well, because they are common in Firefox
    // , Opera, Google Chromium/Google Chrome and most other tab-using
    // applications.
    shortcut = new QShortcut(Qt::CTRL + Qt::Key_PageDown, this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutNextTab);

    shortcut = new QShortcut(Qt::CTRL + Qt::Key_PageUp, this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutPrevTab);

    int i;
    for(i = 0; i < 10; ++i) {
        shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_0 + i), this);
        connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutJumpToTab);

        shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_0 + i), this);
        connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutJumpToTab);
    }

    shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), this);
    connect(shortcut, &QShortcut::activated, [this, &settings] {
        // pass Backspace to current page if it has a visible, transient filter-bar
        if(!settings.showFilter() && currentPage() && currentPage()->isFilterBarVisible()) {
            currentPage()->backspacePressed();
            return;
        }
        on_actionGoUp_triggered();
    });

    shortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete), this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionDelete_triggered);

    // in addition to F3, for convenience
    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), this);
    connect(shortcut, &QShortcut::activated, ui.actionFindFiles, &QAction::trigger);

    // in addition to Alt+Return, for convenience
    shortcut = new QShortcut(Qt::ALT + Qt::Key_Enter, this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionFileProperties_triggered);

    addViewFrame(path);
    if(splitView_) {
        // put the menu button on the right (there's no path bar/entry on the toolbar)
        QWidget* w = new QWidget(this);
        w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        menuSpacer_ = ui.toolBar->insertWidget(ui.actionMenu, w);

        ui.actionSplitView->setChecked(true);
        addViewFrame(path);
        qApp->removeEventFilter(this); // precaution
        qApp->installEventFilter(this);
    }
    else {
        ui.actionSplitView->setChecked(false);
        setAcceptDrops(true); // we want tab dnd in the simple mode
    }
    createPathBar(settings.pathBarButtons());

    if(settings.pathBarButtons()) {
        ui.actionPathButtons->setChecked(true);
    }
    else {
        ui.actionLocationBar->setChecked(true);
    }

    // size from settings
    resize(settings.windowWidth(), settings.windowHeight());
    if(settings.rememberWindowSize() && settings.windowMaximized()) {
        setWindowState(windowState() | Qt::WindowMaximized);
    }

    if(QApplication::layoutDirection() == Qt::RightToLeft) {
        setRTLIcons(true);
    }
}

MainWindow::~MainWindow() {
}

// Activate a view frame appropriately and give a special style to the inactive one(s).
// NOTE: This function is called only with the split mode.
bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if(qobject_cast<QWidget*>(watched)) {
        if(event->type() == QEvent::FocusIn
                // the event has happened inside the splitter
                && ui.viewSplitter->isAncestorOf(qobject_cast<QWidget*>(watched))) {
            for(int i = 0; i < ui.viewSplitter->count(); ++i) {
                if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(ui.viewSplitter->widget(i))) {
                    if(viewFrame->isAncestorOf(qobject_cast<QWidget*>(watched))) {
                        // a widget inside this view frame has gained focus; ensure the view is active
                        if(activeViewFrame_ != viewFrame) {
                            activeViewFrame_ = viewFrame;
                            updateUIForCurrentPage(false); // WARNING: never set focus here!
                        }
                        if(viewFrame->palette().color(QPalette::Base)
                                != qApp->palette().color(QPalette::Base)) {
                            viewFrame->setPalette(qApp->palette()); // restore the main palette
                        }
                    }
                    else if (viewFrame->palette().color(QPalette::Base)
                             == qApp->palette().color(QPalette::Base)) {
                        // Change the text and base palettes of an inactive view frame a little.
                        // NOTE: Style-sheets aren't used because they can interfere with QStyle.
                        QPalette palette = viewFrame->palette();
                        QColor txtCol = palette.color(QPalette::Text);
                        txtCol.setAlphaF(txtCol.alphaF() * 0.7);
                        palette.setColor(QPalette::Text, txtCol);
                        palette.setColor(QPalette::WindowText, txtCol); // tabs
                        // the disabled text color of path-bars shouldn't change because it may be used by arrows
                        palette.setColor(QPalette::Active, QPalette::ButtonText, txtCol);
                        palette.setColor(QPalette::Inactive, QPalette::ButtonText, txtCol);

                        // There are various ways of getting a distinct color near the base color
                        // but this one gives the best results with almost all palettes:
                        QColor baseCol = palette.color(QPalette::Base);
                        baseCol.setRgbF(0.9 * baseCol.redF()   + 0.1 * txtCol.redF(),
                                        0.9 * baseCol.greenF() + 0.1 * txtCol.greenF(),
                                        0.9 * baseCol.blueF()  + 0.1 * txtCol.blueF(),
                                        baseCol.alphaF());
                        palette.setColor(QPalette::Base, baseCol);

                        viewFrame->setPalette(palette);
                    }
                }
            }
        }
        // Use the Tab key for switching between view frames
        else if (event->type() == QEvent::KeyPress) {
            if(QKeyEvent *ke = static_cast<QKeyEvent*>(event)) {
                if(ke->key() == Qt::Key_Tab && ke->modifiers() == Qt::NoModifier) {
                    if(!qobject_cast<QTextEdit*>(watched) // not during inline renaming
                            && ui.viewSplitter->isAncestorOf(qobject_cast<QWidget*>(watched))) {
                        // wrap the focus
                        for(int i = 0; i < ui.viewSplitter->count(); ++i) {
                            if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(ui.viewSplitter->widget(i))) {
                                if(activeViewFrame_ == viewFrame) {
                                    int n = i < ui.viewSplitter->count() - 1 ? i + 1 : 0;
                                    activeViewFrame_ = qobject_cast<ViewFrame*>(ui.viewSplitter->widget(n));
                                    updateUIForCurrentPage(); // focuses the view and calls this function again
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::addViewFrame(const Fm::FilePath& path) {
    ui.actionGo->setVisible(false);
    Settings& settings = static_cast<Application*>(qApp)->settings();
    ViewFrame* viewFrame = new ViewFrame();
    viewFrame->getTabBar()->setDetachable(!splitView_); // no tab DND with the split view
    viewFrame->getTabBar()->setTabsClosable(settings.showTabClose());
    ui.viewSplitter->addWidget(viewFrame); // the splitter takes ownership of viewFrame
    if(ui.viewSplitter->count() == 1) {
        activeViewFrame_ = viewFrame;
    }
    else { // give equal widths to all view frames
        QTimer::singleShot(0, this, [this] {
            QList<int> sizes;
            for(int i = 0; i < ui.viewSplitter->count(); ++i) {
                sizes << ui.viewSplitter->width() / ui.viewSplitter->count();
            }
            ui.viewSplitter->setSizes(sizes);
        });
    }

    connect(viewFrame->getTabBar(), &QTabBar::currentChanged, this, &MainWindow::onTabBarCurrentChanged);
    connect(viewFrame->getTabBar(), &QTabBar::tabCloseRequested, this, &MainWindow::onTabBarCloseRequested);
    connect(viewFrame->getTabBar(), &QTabBar::tabMoved, this, &MainWindow::onTabBarTabMoved);
    connect(viewFrame->getTabBar(), &QTabBar::tabBarClicked, this, &MainWindow::onTabBarClicked);
    connect(viewFrame->getTabBar(), &QTabBar::customContextMenuRequested, this, &MainWindow::tabContextMenu);
    connect(viewFrame->getTabBar(), &TabBar::tabDetached, this, &MainWindow::detachTab);
    connect(viewFrame->getStackedWidget(), &QStackedWidget::widgetRemoved, this, &MainWindow::onStackedWidgetWidgetRemoved);

    if(path) {
        addTab(path, viewFrame);
    }
}

void MainWindow::on_actionSplitView_triggered(bool checked) {
    if(splitView_ == checked) {
        return;
    }
    Settings& settings = static_cast<Application*>(qApp)->settings();
    splitView_ = checked;
    settings.setSplitView(splitView_);
    if(splitView_) { // split the view
        // remove the path bar/entry from the toolbar
        ui.actionGo->setVisible(false);
        menuSep_->setVisible(false);
        if(pathBar_ != nullptr) {
            delete pathBar_;
            pathBar_ = nullptr;
        }
        else if(pathEntry_ != nullptr) {
            delete pathEntry_;
            pathEntry_ = nullptr;
        }

        // add a spacer before the menu action if not exisitng
        if(menuSpacer_ == nullptr) {
            QWidget* w = new QWidget(this);
            w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            menuSpacer_ = ui.toolBar->insertWidget(ui.actionMenu, w);
        }
        menuSpacer_->setVisible(true);

        // disable tab DND
        activeViewFrame_->getTabBar()->setDetachable(false);
        setAcceptDrops(false);

        // add the current path to a new view frame
        Fm::FilePath path;
        TabPage* page = currentPage();
        if(page) {
            path = page->path();
        }
        addViewFrame(path);
        qApp->removeEventFilter(this); // precaution
        qApp->installEventFilter(this);
        createPathBar(settings.pathBarButtons());

        // reset the focus for the inactive view frame(s) to be styled by MainWindow::eventFilter()
        if(page) {
            page->folderView()->childView()->clearFocus();
            page->folderView()->childView()->setFocus();
        }
    }
    else { // remove splitting
        menuSep_->setVisible(!settings.showMenuBar());
        qApp->removeEventFilter(this);
        for(int i = 0; i < ui.viewSplitter->count(); ++i) {
            if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(ui.viewSplitter->widget(i))) {
                if(viewFrame != activeViewFrame_) {
                    viewFrame->deleteLater(); // this may be called by onStackedWidgetWidgetRemoved()
                }
            }
        }

        // enable tab DND
        activeViewFrame_->getTabBar()->setDetachable(true);
        setAcceptDrops(true);

        activeViewFrame_->removeTopBar();
        if(menuSpacer_ != nullptr) {
            menuSpacer_->setVisible(false);
        }
        createPathBar(settings.pathBarButtons());
    }
}

ViewFrame* MainWindow::viewFrameForTabPage(TabPage* page) {
    if(page) {
        if(QStackedWidget* sw = qobject_cast<QStackedWidget*>(page->parentWidget())) {
            if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(sw->parentWidget())) {
                return viewFrame;
            }
        }
    }
    return nullptr;
}

void MainWindow::chdir(Fm::FilePath path, ViewFrame* viewFrame) {
    // wait until queued events are processed
    QTimer::singleShot(0, viewFrame, [this, path, viewFrame] {
        if(TabPage* page = currentPage(viewFrame)) {
            page->chdir(path, true);
            if(viewFrame == activeViewFrame_) {
                updateUIForCurrentPage();
            }
            else {
                if(Fm::PathBar* pathBar = qobject_cast<Fm::PathBar*>(viewFrame->getTopBar())) {
                    pathBar->setPath(page->path());
                }
                else if(Fm::PathEdit* pathEntry = qobject_cast<Fm::PathEdit*>(viewFrame->getTopBar())) {
                    pathEntry->setText(page->pathName());
                }
            }
        }
    });
}

void MainWindow::createPathBar(bool usePathButtons) {
    // NOTE: Path bars/entries may be created after tab pages; so, their paths/texts should be set.
    if(splitView_) {
        for(int i = 0; i < ui.viewSplitter->count(); ++i) {
            if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(ui.viewSplitter->widget(i))) {
                viewFrame->createTopBar(usePathButtons);
                TabPage* curPage = currentPage(viewFrame);
                if(Fm::PathBar* pathBar = qobject_cast<Fm::PathBar*>(viewFrame->getTopBar())) {
                    connect(pathBar, &Fm::PathBar::chdir, this, &MainWindow::onPathBarChdir);
                    connect(pathBar, &Fm::PathBar::middleClickChdir, this, &MainWindow::onPathBarMiddleClickChdir);
                    connect(pathBar, &Fm::PathBar::editingFinished, this, &MainWindow::onResetFocus);
                    if(curPage) {
                        pathBar->setPath(curPage->path());
                    }
                }
                else if(Fm::PathEdit* pathEntry = qobject_cast<Fm::PathEdit*>(viewFrame->getTopBar())) {
                    connect(pathEntry, &Fm::PathEdit::returnPressed, this, &MainWindow::onPathEntryReturnPressed);
                    if(curPage) {
                        pathEntry->setText(curPage->pathName());
                    }
                }
            }
        }
    }
    else {
        QWidget* bar = nullptr;
        TabPage* curPage = currentPage();
        if(usePathButtons) {
            if(pathEntry_ != nullptr) {
                delete pathEntry_;
                pathEntry_ = nullptr;
            }
            if(pathBar_ == nullptr) {
                bar = pathBar_ = new Fm::PathBar(this);
                connect(pathBar_, &Fm::PathBar::chdir, this, &MainWindow::onPathBarChdir);
                connect(pathBar_, &Fm::PathBar::middleClickChdir, this, &MainWindow::onPathBarMiddleClickChdir);
                connect(pathBar_, &Fm::PathBar::editingFinished, this, &MainWindow::onResetFocus);
                if(curPage) {
                    pathBar_->setPath(currentPage()->path());
                }
            }
        }
        else {
            if(pathBar_ != nullptr) {
                delete pathBar_;
                pathBar_ = nullptr;
            }
            if(pathEntry_ == nullptr) {
                bar = pathEntry_ = new Fm::PathEdit(this);
                connect(pathEntry_, &Fm::PathEdit::returnPressed, this, &MainWindow::onPathEntryReturnPressed);
                if(curPage) {
                    pathEntry_->setText(curPage->pathName());
                }
            }
        }
        if(bar != nullptr) {
            ui.toolBar->insertWidget(ui.actionGo, bar);
            ui.actionGo->setVisible(!usePathButtons);
        }
    }
}

int MainWindow::addTabWithPage(TabPage* page, ViewFrame* viewFrame, Fm::FilePath path) {
    if(page == nullptr || viewFrame == nullptr) {
        return -1;
    }
    page->setFileLauncher(&fileLauncher_);
    int index = viewFrame->getStackedWidget()->addWidget(page);
    connect(page, &TabPage::titleChanged, this, &MainWindow::onTabPageTitleChanged);
    connect(page, &TabPage::statusChanged, this, &MainWindow::onTabPageStatusChanged);
    connect(page, &TabPage::sortFilterChanged, this, &MainWindow::onTabPageSortFilterChanged);
    connect(page, &TabPage::backwardRequested, this, &MainWindow::on_actionGoBack_triggered);
    connect(page, &TabPage::forwardRequested, this, &MainWindow::on_actionGoForward_triggered);
    connect(page, &TabPage::folderUnmounted, this, &MainWindow::onFolderUnmounted);

    if(path) {
        page->chdir(path, true);
    }
    viewFrame->getTabBar()->insertTab(index, page->windowTitle());

    Settings& settings = static_cast<Application*>(qApp)->settings();
    if(settings.switchToNewTab()) {
        viewFrame->getTabBar()->setCurrentIndex(index);
        if (isMinimized()) {
            setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            show();
        }
    }
    if(!settings.alwaysShowTabs()) {
        viewFrame->getTabBar()->setVisible(viewFrame->getTabBar()->count() > 1);
    }
    return index;
}

// add a new tab
int MainWindow::addTab(Fm::FilePath path, ViewFrame* viewFrame) {
    TabPage* newPage = new TabPage(this);
    return addTabWithPage(newPage, viewFrame, path);
}
void MainWindow::toggleMenuBar(bool /*checked*/) {

    Settings& settings = static_cast<Application*>(qApp)->settings();
    bool showMenuBar = !settings.showMenuBar();
    if(!showMenuBar) {
        if(QMessageBox::Cancel){
            QMessageBox messageBox(QMessageBox::Warning,
                                   tr("Hide menu bar"),
                                   tr("This will hide the menu bar completely, use Ctrl+M to show it again."),
                                   QMessageBox::Yes | QMessageBox::No,  this);
            messageBox.setButtonText(QMessageBox::Yes, tr("Yes"));
            messageBox.setButtonText(QMessageBox::No, tr("No"));
            messageBox.exec();
        }
    }
    ui.menubar->setVisible(showMenuBar);
    ui.actionMenu_bar->setChecked(showMenuBar);
    menuSep_->setVisible(!showMenuBar);
    ui.actionMenu->setVisible(!showMenuBar);

    settings.setShowMenuBar(showMenuBar);
}
void MainWindow::onPathEntryReturnPressed() {
    Fm::PathEdit* pathEntry = pathEntry_;
    if(pathEntry == nullptr) {
        pathEntry = static_cast<Fm::PathEdit*>(sender());
    }
    if(pathEntry != nullptr) {
        QString text = pathEntry->text();
        QByteArray utext = text.toLocal8Bit();
        chdir(Fm::FilePath::fromPathStr(utext.constData()));
    }
}

void MainWindow::onPathBarChdir(const Fm::FilePath& dirPath) {
    TabPage* page = nullptr;
    ViewFrame* viewFrame = nullptr;
    if(pathBar_ != nullptr) {
        page = currentPage();
        viewFrame = activeViewFrame_;
    }
    else {
        Fm::PathBar* pathBar = static_cast<Fm::PathBar*>(sender());
        viewFrame = qobject_cast<ViewFrame*>(pathBar->parentWidget());
        if(viewFrame != nullptr) {
            page = currentPage(viewFrame);
        }
    }
    if(page && dirPath != page->path()) {
        chdir(dirPath, viewFrame);
    }
}

void MainWindow::onPathBarMiddleClickChdir(const Fm::FilePath& dirPath) {
    ViewFrame* viewFrame = nullptr;
    if(pathBar_ != nullptr) {
        viewFrame = activeViewFrame_;
    }
    else {
        Fm::PathBar* pathBar = static_cast<Fm::PathBar*>(sender());
        viewFrame = qobject_cast<ViewFrame*>(pathBar->parentWidget());
    }
    if(viewFrame) {
        addTab(dirPath, viewFrame);
    }
}

void MainWindow::on_actionGoUp_triggered() {
    QTimer::singleShot(0, this, [this] {
        if(TabPage* page = currentPage()) {
            page->up();
            updateUIForCurrentPage();
        }
    });
}

void MainWindow::on_actionGoBack_triggered() {
    QTimer::singleShot(0, this, [this] {
        if(TabPage* page = currentPage()) {
            page->backward();
            updateUIForCurrentPage();
        }
    });
}

void MainWindow::on_actionGoForward_triggered() {
    QTimer::singleShot(0, this, [this] {
        if(TabPage* page = currentPage()) {
            page->forward();
            updateUIForCurrentPage();
        }
    });

}

void MainWindow::on_actionHome_triggered() {
    chdir(Fm::FilePath::homeDir());
}

void MainWindow::on_actionReload_triggered() {
    currentPage()->reload();
    if(pathEntry_ != nullptr) {
        pathEntry_->setText(currentPage()->pathName());
    }
}

void MainWindow::on_actionConnectToServer_triggered() {
    Application* app = static_cast<Application*>(qApp);
    app->connectToServer();
}

void MainWindow::on_actionGo_triggered() {
    onPathEntryReturnPressed();
}

void MainWindow::on_actionNewTab_triggered() {
    auto path = currentPage()->path();
    addTab(path);
}

void MainWindow::on_actionNewWin_triggered() {
    auto path = currentPage()->path();
    (new MainWindow(path))->show();
}

void MainWindow::on_actionNewFolder_triggered() {
    if(TabPage* tabPage = currentPage()) {
        auto dirPath = tabPage->folderView()->path();
        if(dirPath) {
            createFileOrFolder(CreateNewFolder, dirPath, nullptr, this);
        }
    }
}

void MainWindow::on_actionNewBlankFile_triggered() {
    if(TabPage* tabPage = currentPage()) {
        auto dirPath = tabPage->folderView()->path();
        if(dirPath) {
            createFileOrFolder(CreateNewTextFile, dirPath, nullptr, this);
        }
    }
}

void MainWindow::on_actionCloseTab_triggered() {
    closeTab(activeViewFrame_->getTabBar()->currentIndex());
}

void MainWindow::on_actionCloseWindow_triggered() {
    // FIXME: should we save state here?
    close();
    // the window will be deleted automatically on close
}

void MainWindow::on_actionFileProperties_triggered() {
    TabPage* page = currentPage();
    if(page) {
        auto files = page->selectedFiles();
        if(!files.empty()) {
            Fm::FilePropsDialog::showForFiles(files);
        }
    }
}

void MainWindow::on_actionFolderProperties_triggered() {
    TabPage* page = currentPage();
    if(page) {
        auto folder = page->folder();
        if(folder) {
            auto info = folder->info();
            if(info) {
                Fm::FilePropsDialog::showForFile(info);
            }
        }
    }
}

void MainWindow::on_actionShowHidden_triggered(bool checked) {
    currentPage()->setShowHidden(checked);
    ui.sidePane->setShowHidden(checked);
}

void MainWindow::on_actionShowThumbnails_triggered(bool checked) {
    currentPage()->setShowThumbnails(checked);
}

void MainWindow::on_actionByFileName_triggered(bool /*checked*/) {
    currentPage()->sort(Fm::FolderModel::ColumnFileName, currentPage()->sortOrder());
}

void MainWindow::on_actionByMTime_triggered(bool /*checked*/) {
    currentPage()->sort(Fm::FolderModel::ColumnFileMTime, currentPage()->sortOrder());
}

void MainWindow::on_actionByDTime_triggered(bool /*checked*/) {
    currentPage()->sort(Fm::FolderModel::ColumnFileDTime, currentPage()->sortOrder());
}

void MainWindow::on_actionByOwner_triggered(bool /*checked*/) {
    currentPage()->sort(Fm::FolderModel::ColumnFileOwner, currentPage()->sortOrder());
}

void MainWindow::on_actionByGroup_triggered(bool /*checked*/) {
    currentPage()->sort(Fm::FolderModel::ColumnFileGroup, currentPage()->sortOrder());
}

void MainWindow::on_actionByFileSize_triggered(bool /*checked*/) {
    currentPage()->sort(Fm::FolderModel::ColumnFileSize, currentPage()->sortOrder());
}

void MainWindow::on_actionByFileType_triggered(bool /*checked*/) {
    currentPage()->sort(Fm::FolderModel::ColumnFileType, currentPage()->sortOrder());
}

void MainWindow::on_actionAscending_triggered(bool /*checked*/) {
    currentPage()->sort(currentPage()->sortColumn(), Qt::AscendingOrder);
}

void MainWindow::on_actionDescending_triggered(bool /*checked*/) {
    currentPage()->sort(currentPage()->sortColumn(), Qt::DescendingOrder);
}

void MainWindow::on_actionCaseSensitive_triggered(bool checked) {
    currentPage()->setSortCaseSensitive(checked);
}

void MainWindow::on_actionFolderFirst_triggered(bool checked) {
    currentPage()->setSortFolderFirst(checked);
}

void MainWindow::on_actionHiddenLast_triggered(bool checked) {
    currentPage()->setSortHiddenLast(checked);
}

void MainWindow::on_actionPreserveView_triggered(bool /*checked*/) {
    TabPage* page = currentPage();
    page->setCustomizedView(!page->hasCustomizedView());
}

void MainWindow::on_actionFilter_triggered(bool checked) {
    static_cast<Application*>(qApp)->settings().setShowFilter(checked);
    // show/hide filter-bars and disable/enable their transience for all tabs
    // (of all view frames) in all windows because this is a global setting
    QWidgetList windows = static_cast<Application*>(qApp)->topLevelWidgets();
    QWidgetList::iterator it;
    for(it = windows.begin(); it != windows.end(); ++it) {
        QWidget* window = *it;
        if(window->inherits("PCManFM::MainWindow")) {
            MainWindow* mainWindow = static_cast<MainWindow*>(window);
            mainWindow->ui.actionFilter->setChecked(checked); // doesn't call this function
            for(int i = 0; i < mainWindow->ui.viewSplitter->count(); ++i) {
                if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(mainWindow->ui.viewSplitter->widget(i))) {
                    int n = viewFrame->getStackedWidget()->count();
                    for(int j = 0; j < n; ++j) {
                        if(TabPage* page = static_cast<TabPage*>(viewFrame->getStackedWidget()->widget(j))) {
                            page->transientFilterBar(!checked);
                        }
                    }
                }
            }
        }
    }
}

void MainWindow::on_actionUnfilter_triggered() {
    // clear filters for all tabs (of all view frames)
    for(int i = 0; i < ui.viewSplitter->count(); ++i) {
        if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(ui.viewSplitter->widget(i))) {
            int n = viewFrame->getStackedWidget()->count();
            for(int j = 0; j < n; ++j) {
                if(TabPage* page = static_cast<TabPage*>(viewFrame->getStackedWidget()->widget(j))) {
                    page->clearFilter();
                }
            }
        }
    }
}

void MainWindow::on_actionShowFilter_triggered() {
    if(TabPage* page = currentPage()) {
        page->showFilterBar();
    }
}

void MainWindow::on_actionLocationBar_triggered(bool checked) {
    if(checked) {
        // show current path in a location bar entry
        createPathBar(false);
        static_cast<Application*>(qApp)->settings().setPathBarButtons(false);
    }
}

void MainWindow::on_actionPathButtons_triggered(bool checked) {
    if(checked) {
        // show current path as buttons
        createPathBar(true);
        static_cast<Application*>(qApp)->settings().setPathBarButtons(true);
    }
}

void MainWindow::on_actionComputer_triggered() {
    chdir(Fm::FilePath::fromUri("computer:///"));
}

void MainWindow::on_actionApplications_triggered() {
    chdir(Fm::FilePath::fromUri("menu://applications/"));
}

void MainWindow::on_actionTrash_triggered() {
    chdir(Fm::FilePath::fromUri("trash:///"));
}

void MainWindow::on_actionNetwork_triggered() {
    chdir(Fm::FilePath::fromUri("network:///"));
}

void MainWindow::on_actionDesktop_triggered() {
    auto desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation).toLocal8Bit();
    chdir(Fm::FilePath::fromLocalPath(desktop.constData()));
}

void MainWindow::on_actionAddToBookmarks_triggered() {
    TabPage* page = currentPage();
    if(page) {
        auto cwd = page->path();
        if(cwd) {
            QString bookmarkName;
            auto parent = cwd.parent();
            if(!parent.isValid() || parent == cwd) { // a root path
                bookmarkName = QString::fromUtf8(cwd.displayName().get());
                auto parts = bookmarkName.split(QLatin1Char('/'), QString::SkipEmptyParts);
                if(!parts.isEmpty()) {
                    bookmarkName = parts.last();
                }
            }
            else {
                bookmarkName = QString::fromUtf8(cwd.baseName().get());
            }
            bookmarks_->insert(cwd, bookmarkName, -1);
        }
    }
}

void MainWindow::on_actionEditBookmarks_triggered() {
    Application* app = static_cast<Application*>(qApp);
    app->editBookmarks();
}
void MainWindow::on_actionAbout_triggered() {
    // the about dialog
    class AboutDialog : public QDialog {
    public:
        explicit AboutDialog(QWidget* parent = 0, Qt::WindowFlags f = Qt::WindowFlags()) : QDialog(parent, f) {
            ui.setupUi(this);
            ui.buttonBox_about->button(QDialogButtonBox::Close)->setText(tr("Close"));
            ui.version->setText(tr("Version: %1").arg(QStringLiteral(PCMANFM_QT_VERSION)));
        }
    private:
        Ui::AboutDialog ui;
    };
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_actionHiddenShortcuts_triggered() {
    class HiddenShortcutsDialog : public QDialog {
    public:
        explicit HiddenShortcutsDialog(QWidget* parent = 0, Qt::WindowFlags f = Qt::WindowFlags()) : QDialog(parent, f) {
            ui.setupUi(this);
            ui.buttonBox_shortcut->button(QDialogButtonBox::Close)->setText(tr(" Close"));
            ui.treeWidget->setRootIsDecorated(false);
            ui.treeWidget->header()->setSectionResizeMode(QHeaderView::Stretch);
            ui.treeWidget->header()->setSectionsClickable(true);
            ui.treeWidget->sortByColumn(0, Qt::AscendingOrder);
            ui.treeWidget->setSortingEnabled(true);
        }
    private:
        Ui::HiddenShortcutsDialog ui;
    };
    HiddenShortcutsDialog dialog(this);
    dialog.exec();
}
void MainWindow::on_actionIconView_triggered() {
    currentPage()->setViewMode(Fm::FolderView::IconMode);
}

void MainWindow::on_actionCompactView_triggered() {
    currentPage()->setViewMode(Fm::FolderView::CompactMode);
}

void MainWindow::on_actionDetailedList_triggered() {
    currentPage()->setViewMode(Fm::FolderView::DetailedListMode);
}

void MainWindow::on_actionThumbnailView_triggered() {
    currentPage()->setViewMode(Fm::FolderView::ThumbnailMode);
}

void MainWindow::onTabBarCloseRequested(int index) {
    TabBar* tabBar = static_cast<TabBar*>(sender());
    if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(tabBar->parentWidget())) {
        closeTab(index, viewFrame);
    }
}

void MainWindow::onResetFocus() {
    if(TabPage* page = currentPage()) {
        page->folderView()->childView()->setFocus();
    }
}

void MainWindow::onTabBarTabMoved(int from, int to) {
    TabBar* tabBar = static_cast<TabBar*>(sender());
    if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(tabBar->parentWidget())) {
        // a tab in the tab bar is moved by the user, so we have to move the
        //  corredponding tab page in the stacked widget to the new position, too.
        QWidget* page = viewFrame->getStackedWidget()->widget(from);
        if(page) {
            // we're not going to delete the tab page, so here we block signals
            // to avoid calling the slot onStackedWidgetWidgetRemoved() before
            // removing the page. Otherwise the page widget will be destroyed.
            viewFrame->getStackedWidget()->blockSignals(true);
            viewFrame->getStackedWidget()->removeWidget(page);
            viewFrame->getStackedWidget()->insertWidget(to, page); // insert the page to the new position
            viewFrame->getStackedWidget()->blockSignals(false); // unblock signals
            viewFrame->getStackedWidget()->setCurrentWidget(page);
        }
    }
}

QList<Fm::MountOperation*> MainWindow::pendingMountOperations() const {
    return ui.sidePane->findChildren<MountOperation*>();
}

void MainWindow::onFolderUnmounted() {
    TabPage* tabPage = static_cast<TabPage*>(sender());
    if(ViewFrame* viewFrame = viewFrameForTabPage(tabPage)) {
        const QList<MountOperation*> ops = pendingMountOperations();
        if(ops.isEmpty()) { // unmounting is done somewhere else
            Settings& settings = static_cast<Application*>(qApp)->settings();
            if(settings.closeOnUnmount()) {
                viewFrame->getStackedWidget()->removeWidget(tabPage);
                // NOTE: Since Fm::Folder queues a folder reload after emitting the unmount signal,
                // pending events may be waiting to be delivered at this very moment. Therefore,
                // if the tab page is deleted immediately, a crash will be imminent for various reasons.
                tabPage->deleteLater();
            }
            else {
                tabPage->chdir(Fm::FilePath::homeDir(), viewFrame);
                updateUIForCurrentPage();
            }
        }
        else { // wait for all (un-)mount operations to be finished (otherwise, they might be cancelled)
            for(const MountOperation* op : ops) {
                connect(op, &QObject::destroyed, tabPage, [this, tabPage, viewFrame] {
                    if(pendingMountOperations().isEmpty()) {
                        Settings& settings = static_cast<Application*>(qApp)->settings();
                        if(settings.closeOnUnmount()) {
                            viewFrame->getStackedWidget()->removeWidget(tabPage);
                            tabPage->deleteLater();
                        }
                        else {
                            tabPage->chdir(Fm::FilePath::homeDir(), viewFrame);
                            updateUIForCurrentPage();
                        }
                    }
                });
            }
        }
    }
}

void MainWindow::closeTab(int index, ViewFrame* viewFrame) {
    QWidget* page = viewFrame->getStackedWidget()->widget(index);
    if(page) {
        viewFrame->getStackedWidget()->removeWidget(page); // this does not delete the page widget
        delete page;
        // NOTE: we do not remove the tab here.
        // it'll be done in onStackedWidgetWidgetRemoved()
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    Settings& settings = static_cast<Application*>(qApp)->settings();
    if(settings.rememberWindowSize()) {
        settings.setLastWindowMaximized(isMaximized());

        if(!isMaximized()) {
            settings.setLastWindowWidth(width());
            settings.setLastWindowHeight(height());
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if(lastActive_ == this) {
        lastActive_ = nullptr;
    }

    QWidget::closeEvent(event);
    Settings& settings = static_cast<Application*>(qApp)->settings();
    if(settings.rememberWindowSize()) {
        settings.setLastWindowMaximized(isMaximized());

        if(!isMaximized()) {
            settings.setLastWindowWidth(width());
            settings.setLastWindowHeight(height());
        }
    }

    // remember last tab paths only if this is the last window
    QStringList tabPaths;
    if(lastActive_ == nullptr && settings.reopenLastTabs()) {
        for(int i = 0; i < ui.viewSplitter->count(); ++i) {
            if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(ui.viewSplitter->widget(i))) {
                int n = viewFrame->getStackedWidget()->count();
                for(int j = 0; j < n; ++j) {
                    if(TabPage* page = static_cast<TabPage*>(viewFrame->getStackedWidget()->widget(j))) {
                        tabPaths.append(QString::fromUtf8(page->path().toString().get()));
                    }
                }
            }
        }
        tabPaths.removeDuplicates();
    }
    settings.setTabPaths(tabPaths);
}

void MainWindow::onTabBarCurrentChanged(int index) {
    TabBar* tabBar = static_cast<TabBar*>(sender());
    if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(tabBar->parentWidget())) {
        viewFrame->getStackedWidget()->setCurrentIndex(index);
        if(viewFrame == activeViewFrame_) {
            updateUIForCurrentPage();
        }
        else {
            if(TabPage* page = currentPage(viewFrame)) {
                if(Fm::PathBar* pathBar = qobject_cast<Fm::PathBar*>(viewFrame->getTopBar())) {
                    pathBar->setPath(page->path());
                }
                else if(Fm::PathEdit* pathEntry = qobject_cast<Fm::PathEdit*>(viewFrame->getTopBar())) {
                    pathEntry->setText(page->pathName());
                }
            }
        }
    }
}

void MainWindow::updateStatusBarForCurrentPage() {
    TabPage* tabPage = currentPage();
    QString text = tabPage->statusText(TabPage::StatusTextSelectedFiles);
    if(text.isEmpty()) {
        text = tabPage->statusText(TabPage::StatusTextNormal);
    }
    ui.statusbar->showMessage(text);

    text = tabPage->statusText(TabPage::StatusTextFSInfo);
    fsInfoLabel_->setText(text);
    fsInfoLabel_->setVisible(!text.isEmpty());
}

void MainWindow::updateViewMenuForCurrentPage() {
    if(updatingViewMenu_) { // prevent recursive calls
        return;
    }
    updatingViewMenu_ = true;
    TabPage* tabPage = currentPage();
    if(tabPage) {
        // update menus. FIXME: should we move this to another method?
        ui.actionShowHidden->setChecked(tabPage->showHidden());
        ui.actionPreserveView->setChecked(tabPage->hasCustomizedView());

        // view mode
        QAction* modeAction = nullptr;

        switch(tabPage->viewMode()) {
        case Fm::FolderView::IconMode:
            modeAction = ui.actionIconView;
            break;

        case Fm::FolderView::CompactMode:
            modeAction = ui.actionCompactView;
            break;

        case Fm::FolderView::DetailedListMode:
            modeAction = ui.actionDetailedList;
            break;

        case Fm::FolderView::ThumbnailMode:
            modeAction = ui.actionThumbnailView;
            break;
        }

        Q_ASSERT(modeAction != nullptr);
        modeAction->setChecked(true);

        // sort menu
        // WARNING: Since libfm-qt may have a column that is not handled here,
        // we should prevent a crash by setting all actions to null first and
        // check their action group later.
        QAction* sortActions[Fm::FolderModel::NumOfColumns];
        for(int i = 0; i < Fm::FolderModel::NumOfColumns; ++i) {
            sortActions[i] = nullptr;
        }
        sortActions[Fm::FolderModel::ColumnFileName] = ui.actionByFileName;
        sortActions[Fm::FolderModel::ColumnFileMTime] = ui.actionByMTime;
        sortActions[Fm::FolderModel::ColumnFileDTime] = ui.actionByDTime;
        sortActions[Fm::FolderModel::ColumnFileSize] = ui.actionByFileSize;
        sortActions[Fm::FolderModel::ColumnFileType] = ui.actionByFileType;
        sortActions[Fm::FolderModel::ColumnFileOwner] = ui.actionByOwner;
        sortActions[Fm::FolderModel::ColumnFileGroup] = ui.actionByGroup;
        if (auto group = ui.actionByFileName->actionGroup()) {
            const auto actions = group->actions();
            auto action = sortActions[tabPage->sortColumn()];
            if(actions.contains(action)) {
                action->setChecked(true);
            }
            else {
                for(auto a : actions) {
                    a->setChecked(false);
                }
            }
        }

        if(auto path = tabPage->path()) {
            ui.actionByDTime->setVisible(strcmp(path.toString().get(), "trash:///") == 0);
        }

        if(tabPage->sortOrder() == Qt::AscendingOrder) {
            ui.actionAscending->setChecked(true);
        }
        else {
            ui.actionDescending->setChecked(true);
        }
        ui.actionCaseSensitive->setChecked(tabPage->sortCaseSensitive());
        ui.actionFolderFirst->setChecked(tabPage->sortFolderFirst());
        ui.actionHiddenLast->setChecked(tabPage->sortHiddenLast());
    }
    updatingViewMenu_ = false;
}

// Update the enabled state of Edit actions for selected files
void MainWindow::updateEditSelectedActions() {
    bool hasAccessible(false);
    bool hasDeletable(false);
    int renamable(0);
    if(TabPage* page = currentPage()) {
        auto files = page->selectedFiles();
        for(auto& file: files) {
            if(file->isAccessible()) {
                hasAccessible = true;
            }
            if(file->isDeletable()) {
                hasDeletable = true;
            }
            if(file->canSetName()) {
                ++renamable;
            }
            if (hasAccessible && hasDeletable && renamable > 1) {
                break;
            }
        }
        ui.actionCopyFullPath->setEnabled(files.size() == 1);
    }
    ui.actionCopy->setEnabled(hasAccessible);
    ui.actionCut->setEnabled(hasDeletable);
    ui.actionDelete->setEnabled(hasDeletable);
    ui.actionRename->setEnabled(renamable > 0);
    ui.actionBulkRename->setEnabled(renamable > 1);
}

void MainWindow::updateUIForCurrentPage(bool setFocus) {
    TabPage* tabPage = currentPage();

    if(tabPage) {
        setWindowTitle(tabPage->windowTitle());
        if(splitView_) {
            if(Fm::PathBar* pathBar = qobject_cast<Fm::PathBar*>(activeViewFrame_->getTopBar())) {
                pathBar->setPath(tabPage->path());
            }
            else if(Fm::PathEdit* pathEntry = qobject_cast<Fm::PathEdit*>(activeViewFrame_->getTopBar())) {
                pathEntry->setText(tabPage->pathName());
            }
        }
        else {
            if(pathEntry_ != nullptr) {
                pathEntry_->setText(tabPage->pathName());
            }
            else if(pathBar_ != nullptr) {
                pathBar_->setPath(tabPage->path());
            }
        }
        ui.statusbar->showMessage(tabPage->statusText());
        fsInfoLabel_->setText(tabPage->statusText(TabPage::StatusTextFSInfo));
        if(setFocus) {
            tabPage->folderView()->childView()->setFocus();
        }

        // update side pane
        ui.sidePane->setCurrentPath(tabPage->path());
        ui.sidePane->setShowHidden(tabPage->showHidden());

        // update back/forward/up toolbar buttons
        ui.actionGoUp->setEnabled(tabPage->canUp());
        ui.actionGoBack->setEnabled(tabPage->canBackward());
        ui.actionGoForward->setEnabled(tabPage->canForward());

        updateViewMenuForCurrentPage();
        updateStatusBarForCurrentPage();
    }

    // also update the enabled state of Edit actions
    updateEditSelectedActions();
    bool isWritable(false);
    if(tabPage && tabPage->folder()) {
        if(auto info = tabPage->folder()->info()) {
            isWritable = info->isWritable();
        }
    }
    ui.actionPaste->setEnabled(isWritable);
    ui.menuCreateNew->setEnabled(isWritable);
    // disable creation shortcuts too
    ui.actionNewFolder->setEnabled(isWritable);
    ui.actionNewBlankFile->setEnabled(isWritable);
}

void MainWindow::onStackedWidgetWidgetRemoved(int index) {
    QStackedWidget* sw = static_cast<QStackedWidget*>(sender());
    if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(sw->parentWidget())) {
        // qDebug("onStackedWidgetWidgetRemoved: %d", index);
        // need to remove associated tab from tabBar
        viewFrame->getTabBar()->removeTab(index);
        if(viewFrame->getTabBar()->count() == 0) { // this is the last one
            if(!splitView_) {
                deleteLater(); // destroy the whole window
                // qDebug("delete window");
            }
            else {
                // if we are in the split mode and the last tab of a view frame is closed,
                // remove that view frame and go to the simple mode
                for(int i = 0; i < ui.viewSplitter->count(); ++i) {
                    // first find and activate the next view frame
                    if(ViewFrame* thisViewFrame = qobject_cast<ViewFrame*>(ui.viewSplitter->widget(i))) {
                        if(thisViewFrame == viewFrame) {
                            int n = i < ui.viewSplitter->count() - 1 ? i + 1 : 0;
                            if(ViewFrame* nextViewFrame = qobject_cast<ViewFrame*>(ui.viewSplitter->widget(n))) {
                                if(activeViewFrame_ != nextViewFrame) {
                                    activeViewFrame_ = nextViewFrame;
                                    updateUIForCurrentPage();
                                    // if the window isn't active, eventFilter() won't be called,
                                    // so we should revert to the main palette here
                                    if(activeViewFrame_->palette().color(QPalette::Base)
                                            != qApp->palette().color(QPalette::Base)) {
                                        activeViewFrame_->setPalette(qApp->palette());
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
                ui.actionSplitView->setChecked(false);
                on_actionSplitView_triggered(false);
            }
        }
        else {
            Settings& settings = static_cast<Application*>(qApp)->settings();
            if(!settings.alwaysShowTabs() && viewFrame->getTabBar()->count() == 1) {
                viewFrame->getTabBar()->setVisible(false);
            }
        }
    }
}

void MainWindow::onTabPageTitleChanged(QString title) {
    TabPage* tabPage = static_cast<TabPage*>(sender());
    if(ViewFrame* viewFrame = viewFrameForTabPage(tabPage)) {
        int index = viewFrame->getStackedWidget()->indexOf(tabPage);
        if(index >= 0) {
            viewFrame->getTabBar()->setTabText(index, title);
        }

        if(viewFrame == activeViewFrame_) {
            if(tabPage == currentPage()) {
                setWindowTitle(title);

                // Since TabPage::titleChanged is emitted on changing directory,
                // the enabled state of Paste action should be updated here
                bool isWritable(false);
                if(tabPage && tabPage->folder()) {
                    if(auto info = tabPage->folder()->info()) {
                        isWritable = info->isWritable();
                    }
                }
                ui.actionPaste->setEnabled(isWritable);
                ui.menuCreateNew->setEnabled(isWritable);
                ui.actionNewFolder->setEnabled(isWritable);
                ui.actionNewBlankFile->setEnabled(isWritable);
            }
        }
    }
}

void MainWindow::onTabPageStatusChanged(int type, QString statusText) {
    TabPage* tabPage = static_cast<TabPage*>(sender());
    if(tabPage == currentPage()) {
        switch(type) {
        case TabPage::StatusTextNormal:
        case TabPage::StatusTextSelectedFiles: {
            // although the status text may change very frequently,
            // the text of PCManFM::StatusBar is updated with a delay
            QString text = tabPage->statusText(TabPage::StatusTextSelectedFiles);
            if(text.isEmpty()) {
                ui.statusbar->showMessage(tabPage->statusText(TabPage::StatusTextNormal));
            }
            else {
                ui.statusbar->showMessage(text);
            }
            break;
        }
        case TabPage::StatusTextFSInfo:
            fsInfoLabel_->setText(tabPage->statusText(TabPage::StatusTextFSInfo));
            fsInfoLabel_->setVisible(!statusText.isEmpty());
            break;
        }
    }

    // Since TabPage::statusChanged is always emitted after View::selChanged,
    // there is no need to connect a separate slot to the latter signal
    updateEditSelectedActions();
}

void MainWindow::onTabPageSortFilterChanged() { // NOTE: This may be called from context menu too.
    TabPage* tabPage = static_cast<TabPage*>(sender());
    if(tabPage == currentPage()) {
        updateViewMenuForCurrentPage();
        if(!tabPage->hasCustomizedView()) { // remember sort settings globally
            Settings& settings = static_cast<Application*>(qApp)->settings();
            settings.setSortColumn(static_cast<Fm::FolderModel::ColumnId>(tabPage->sortColumn()));
            settings.setSortOrder(tabPage->sortOrder());
            settings.setSortFolderFirst(tabPage->sortFolderFirst());
            settings.setSortHiddenLast(tabPage->sortHiddenLast());
            settings.setSortCaseSensitive(tabPage->sortCaseSensitive());
            settings.setShowHidden(tabPage->showHidden());
        }
    }
}


void MainWindow::onSidePaneChdirRequested(int type, const Fm::FilePath &path) {
    // FIXME: use enum for type value or change it to button.
    if(type == 0) { // left button (default)
        chdir(path);
    }
    else if(type == 1) { // middle button
        addTab(path);
    }
    else if(type == 2) { // new window
        (new MainWindow(path))->show();
    }
}

void MainWindow::onSidePaneOpenFolderInNewWindowRequested(const Fm::FilePath &path) {
    (new MainWindow(path))->show();
}

void MainWindow::onSidePaneOpenFolderInNewTabRequested(const Fm::FilePath &path) {
    addTab(path);
}

void MainWindow::onSidePaneOpenFolderInTerminalRequested(const Fm::FilePath &path) {
    Application* app = static_cast<Application*>(qApp);
    app->openFolderInTerminal(path);
}

void MainWindow::onSidePaneCreateNewFolderRequested(const Fm::FilePath &path) {
    createFileOrFolder(CreateNewFolder, path, nullptr, this);
}

void MainWindow::onSidePaneModeChanged(Fm::SidePane::Mode mode) {
    static_cast<Application*>(qApp)->settings().setSidePaneMode(mode);
}

void MainWindow::onSettingHiddenPlace(const QString& str, bool hide) {
    static_cast<Application*>(qApp)->settings().setHiddenPlace(str, hide);
}

void MainWindow::on_actionSidePane_triggered(bool checked) {
    Application* app = static_cast<Application*>(qApp);
    app->settings().showSidePane(checked);
    ui.sidePane->setVisible(checked);
}

void MainWindow::onSplitterMoved(int pos, int /*index*/) {
    Application* app = static_cast<Application*>(qApp);
    app->settings().setSplitterPos(pos);
}

void MainWindow::loadBookmarksMenu() {
    QAction* before = ui.actionAddToBookmarks;
    for(auto& item: bookmarks_->items()) {
        BookmarkAction* action = new BookmarkAction(item, ui.menu_Bookmarks);
        connect(action, &QAction::triggered, this, &MainWindow::onBookmarkActionTriggered);
        ui.menu_Bookmarks->insertAction(before, action);
    }

    ui.menu_Bookmarks->insertSeparator(before);
}

void MainWindow::onBookmarksChanged() {
    // delete existing items
    QList<QAction*> actions = ui.menu_Bookmarks->actions();
    QList<QAction*>::const_iterator it = actions.constBegin();
    QList<QAction*>::const_iterator last_it = actions.constEnd() - 2;

    while(it != last_it) {
        QAction* action = *it;
        ++it;
        ui.menu_Bookmarks->removeAction(action);
    }

    loadBookmarksMenu();
}

void MainWindow::onBookmarkActionTriggered() {
    BookmarkAction* action = static_cast<BookmarkAction*>(sender());
    auto path = action->path();
    if(path) {
        Application* app = static_cast<Application*>(qApp);
        Settings& settings = app->settings();
        switch(settings.bookmarkOpenMethod()) {
        case OpenInCurrentTab: /* current tab */
        default:
            chdir(path);
            break;
        case OpenInNewTab: /* new tab */
            addTab(path);
            break;
        case OpenInNewWindow: /* new window */
            (new MainWindow(path))->show();
            break;
        }
    }
}

void MainWindow::on_actionCopy_triggered() {
    TabPage* page = currentPage();
    auto paths = page->selectedFilePaths();
    copyFilesToClipboard(paths);
}

void MainWindow::on_actionCut_triggered() {
    TabPage* page = currentPage();
    auto paths = page->selectedFilePaths();
    cutFilesToClipboard(paths);
}

void MainWindow::on_actionPaste_triggered() {
    pasteFilesFromClipboard(currentPage()->path(), this);
}

void MainWindow::on_actionDelete_triggered() {
    Application* app = static_cast<Application*>(qApp);
    Settings& settings = app->settings();
    TabPage* page = currentPage();
    auto paths = page->selectedFilePaths();
    auto path_it = paths.cbegin();
    bool trashed(path_it != paths.cend() && (*path_it).hasUriScheme("trash"));

    bool shiftPressed = (qApp->keyboardModifiers() & Qt::ShiftModifier ? true : false);
    if(settings.useTrash() && !shiftPressed
            // trashed files should be deleted
            && !trashed) {
        FileOperation::trashFiles(paths, settings.confirmTrash(), this);
    }
    else {
        FileOperation::deleteFiles(paths, settings.confirmDelete(), this);
    }
}

void MainWindow::on_actionRename_triggered() {
    // do inline renaming if only one item is selected,
    // otherwise use the renaming dialog
    TabPage* page = currentPage();
    auto files = page->selectedFiles();
    if(files.size() == 1) {
        QAbstractItemView* view = page->folderView()->childView();
        QModelIndexList selIndexes = view->selectionModel()->selectedIndexes();
        if(selIndexes.size() > 1) { // in the detailed list mode, only the first index is editable
            view->setCurrentIndex(selIndexes.at(0));
        }
        QModelIndex cur = view->currentIndex();
        if (cur.isValid()) {
            view->scrollTo(cur);
            view->edit(cur);
            return;
        }
    }
    if(!files.empty()) {
        for(auto& file: files) {
            if(!Fm::renameFile(file, nullptr)) {
                break;
            }
        }
    }
}

void MainWindow::on_actionBulkRename_triggered() {
    BulkRenamer(currentPage()->selectedFiles(), this);
}

void MainWindow::on_actionSelectAll_triggered() {
    currentPage()->selectAll();
}

void MainWindow::on_actionInvertSelection_triggered() {
    currentPage()->invertSelection();
}

void MainWindow::on_actionPreferences_triggered() {
    Application* app = reinterpret_cast<Application*>(qApp);
    app->preferences(QString());
}

// change some icons according to layout direction
void MainWindow::setRTLIcons(bool isRTL) {
    QIcon nxtIcn = QIcon::fromTheme(QStringLiteral("go-next"));
    QIcon prevIcn = QIcon::fromTheme(QStringLiteral("go-previous"));
    if(isRTL) {
        ui.actionGoBack->setIcon(nxtIcn);
        ui.actionCloseLeft->setIcon(nxtIcn);
        ui.actionGoForward->setIcon(prevIcn);
        ui.actionCloseRight->setIcon(prevIcn);
    }
    else {
        ui.actionGoBack->setIcon(prevIcn);
        ui.actionCloseLeft->setIcon(prevIcn);
        ui.actionGoForward->setIcon(nxtIcn);
        ui.actionCloseRight->setIcon(nxtIcn);
    }
}

bool MainWindow::event(QEvent* event) {
    switch(event->type()) {
    case QEvent::WindowActivate:
        lastActive_ = this;
    default:
        break;
    }
    return QMainWindow::event(event);
}

void MainWindow::changeEvent(QEvent* event) {
    switch(event->type()) {
    case QEvent::LayoutDirectionChange:
        setRTLIcons(QApplication::layoutDirection() == Qt::RightToLeft);
        break;
    default:
        break;
    }
    QWidget::changeEvent(event);
}

void MainWindow::onBackForwardContextMenu(QPoint pos) {
    // show a popup menu for browsing history here.
    QToolButton* btn = static_cast<QToolButton*>(sender());
    TabPage* page = currentPage();
    Fm::BrowseHistory& history = page->browseHistory();
    int current = history.currentIndex();
    QMenu menu;
    for(size_t i = 0; i < history.size(); ++i) {
        const BrowseHistoryItem& item = history.at(i);
        auto path = item.path();
        auto name = path.displayName();
        QAction* action = menu.addAction(QString::fromUtf8(name.get()));
        if(i == static_cast<size_t>(current)) {
            // make the current path bold and checked
            action->setCheckable(true);
            action->setChecked(true);
            QFont font = menu.font();
            font.setBold(true);
            action->setFont(font);
        }
    }
    QAction* selectedAction = menu.exec(btn->mapToGlobal(pos));
    if(selectedAction) {
        int index = menu.actions().indexOf(selectedAction);
        page->jumpToHistory(index);
        updateUIForCurrentPage();
    }
}

void MainWindow::onTabBarClicked(int /*index*/) {
    TabBar* tabBar = static_cast<TabBar*>(sender());
    if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(tabBar->parentWidget())) {
        // focus the view on clicking the tab bar
        if(TabPage* page = currentPage(viewFrame)) {
            page->folderView()->childView()->setFocus();
        }
    }
}

void MainWindow::tabContextMenu(const QPoint& pos) {
    TabBar* tabBar = static_cast<TabBar*>(sender());
    if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(tabBar->parentWidget())) {
        int tabNum = viewFrame->getTabBar()->count();
        if(tabNum <= 1) {
            return;
        }

        rightClickIndex_ = viewFrame->getTabBar()->tabAt(pos);
        if(rightClickIndex_ < 0) {
            return;
        }

        QMenu menu;
        if(rightClickIndex_ > 0) {
            menu.addAction(ui.actionCloseLeft);
        }
        if(rightClickIndex_ < tabNum - 1) {
            menu.addAction(ui.actionCloseRight);
            if(rightClickIndex_ > 0) {
                menu.addSeparator();
                menu.addAction(ui.actionCloseOther);
            }
        }
        menu.exec(viewFrame->getTabBar()->mapToGlobal(pos));
    }
}

void MainWindow::closeLeftTabs() {
    while(rightClickIndex_ > 0) {
        closeTab(rightClickIndex_ - 1);
        --rightClickIndex_;
    }
}

void MainWindow::closeRightTabs() {
    if(rightClickIndex_ < 0) {
        return;
    }
    while(rightClickIndex_ < activeViewFrame_->getTabBar()->count() - 1) {
        closeTab(rightClickIndex_ + 1);
    }
}

void MainWindow::focusPathEntry() {
    // use text entry for the path bar
    if(splitView_) {
        if(Fm::PathBar* pathBar = qobject_cast<Fm::PathBar*>(activeViewFrame_->getTopBar())) {
            pathBar->openEditor();
        }
        else if(Fm::PathEdit* pathEntry = qobject_cast<Fm::PathEdit*>(activeViewFrame_->getTopBar())) {
            pathEntry->setFocus();
            pathEntry->selectAll();
        }
    }
    else{
        if(pathEntry_ != nullptr) {
            pathEntry_->setFocus();
            pathEntry_->selectAll();
        }
        else if(pathBar_ != nullptr) {  // use button-style path bar
            pathBar_->openEditor();
        }
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if(event->mimeData()->hasFormat(QStringLiteral("application/pcmanfm-qt-tab"))
            // ensure that the tab drag source is ours (and not a root window, for example)
            && lastActive_ && lastActive_->isActiveWindow()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    if(event->mimeData()->hasFormat(QStringLiteral("application/pcmanfm-qt-tab"))) {
        dropTab();
    }
    event->acceptProposedAction();
}

void MainWindow::dropTab() {
    if(lastActive_ == nullptr // impossible
            || lastActive_ == this) { // don't drop on the same window
        activeViewFrame_->getTabBar()->finishMouseMoveEvent();
        return;
    }

    // close the tab in the first window and add
    // its page to a new tab in the second window
    TabPage* dropPage = lastActive_->currentPage();
    if(dropPage) {
        disconnect(dropPage, nullptr, lastActive_, nullptr);

        // release mouse before tab removal because otherwise, the source tabbar
        // might not be updated properly with tab reordering during a fast drag-and-drop
        lastActive_->activeViewFrame_->getTabBar()->releaseMouse();

        QWidget* page = lastActive_->activeViewFrame_->getStackedWidget()->currentWidget();
        lastActive_->activeViewFrame_->getStackedWidget()->removeWidget(page);
        int index = addTabWithPage(dropPage, activeViewFrame_);
        activeViewFrame_->getTabBar()->setCurrentIndex(index);
    }
    else {
        activeViewFrame_->getTabBar()->finishMouseMoveEvent(); // impossible
    }
}

void MainWindow::detachTab() {
    if (activeViewFrame_->getStackedWidget()->count() == 1) { // don't detach a single tab
        activeViewFrame_->getTabBar()->finishMouseMoveEvent();
        return;
    }

    // close the tab and move its page to a new window
    TabPage* dropPage = currentPage();
    if(dropPage) {
        disconnect(dropPage, nullptr, this, nullptr);

        activeViewFrame_->getTabBar()->releaseMouse(); // as in dropTab()

        QWidget* page = activeViewFrame_->getStackedWidget()->currentWidget();
        activeViewFrame_->getStackedWidget()->removeWidget(page);
        MainWindow* newWin = new MainWindow();
        newWin->addTabWithPage(dropPage, newWin->activeViewFrame_);
        newWin->show();
    }
    else {
        activeViewFrame_->getTabBar()->finishMouseMoveEvent(); // impossible
    }
}

void MainWindow::updateFromSettings(Settings& settings) {
    // apply settings

    // menu
    ui.actionDelete->setText(settings.useTrash() ? tr("&Move to Trash") : tr("&Delete"));
    ui.actionDelete->setIcon(settings.useTrash() ? QIcon::fromTheme(QStringLiteral("user-trash")) : QIcon::fromTheme(QStringLiteral("edit-delete")));

    // side pane
    ui.sidePane->setIconSize(QSize(settings.sidePaneIconSize(), settings.sidePaneIconSize()));

    // tabs
    for(int i = 0; i < ui.viewSplitter->count(); ++i) {
        if(ViewFrame* viewFrame = qobject_cast<ViewFrame*>(ui.viewSplitter->widget(i))) {
            viewFrame->getTabBar()->setTabsClosable(settings.showTabClose());
            viewFrame->getTabBar()->setVisible(settings.alwaysShowTabs() || (viewFrame->getTabBar()->count() > 1));

            // all tab pages
            int n = viewFrame->getStackedWidget()->count();

            for(int j = 0; j < n; ++j) {
                TabPage* page = static_cast<TabPage*>(viewFrame->getStackedWidget()->widget(j));
                page->updateFromSettings(settings);
            }
        }
    }
}

void MainWindow::on_actionOpenAsRoot_triggered() {
    TabPage* page = currentPage();

    if(page) {
        Application* app = static_cast<Application*>(qApp);
        Settings& settings = app->settings();

        if(!settings.suCommand().isEmpty()) {
            // run the su command
            // FIXME: it's better to get the filename of the current process rather than hard-code pcmanfm-qt here.
            QByteArray suCommand = settings.suCommand().toLocal8Bit();
            QByteArray programCommand = app->applicationFilePath().toLocal8Bit();
            programCommand += " %U";

            // if %s exists in the su command, substitute it with the program
            int substPos = suCommand.indexOf("%s");
            if(substPos != -1) {
                // replace %s with program
                suCommand.replace(substPos, 2, programCommand);
            }
            else {
                /* no %s found so just append to it */
                suCommand += programCommand;
            }

            Fm::GAppInfoPtr appInfo{g_app_info_create_from_commandline(suCommand.constData(), nullptr, GAppInfoCreateFlags(0), nullptr), false};

            if(appInfo) {
                auto cwd = page->path();
                Fm::GErrorPtr err;
                auto uri = cwd.uri();
                GList* uris = g_list_prepend(nullptr, uri.get());

                if(!g_app_info_launch_uris(appInfo.get(), uris, nullptr, &err)) {
                    QMessageBox::critical(this, tr("Error"), QString::fromUtf8(err->message));
                }

                g_list_free(uris);
            }
        }
        else {
            // show an error message and ask the user to set the command
            QMessageBox::critical(this, tr("Error"), tr("Switch user command is not set."));
            app->preferences(QStringLiteral("advanced"));
        }
    }
}

void MainWindow::on_actionFindFiles_triggered() {
    Application* app = static_cast<Application*>(qApp);
    auto selectedPaths = currentPage()->selectedFilePaths();
    QStringList paths;
    if(!selectedPaths.empty()) {
        for(auto& path: selectedPaths) {
            // FIXME: is it ok to use display name here?
            // This might be broken on filesystems with non-UTF-8 filenames.
            paths.append(QString::fromUtf8(path.displayName().get()));
        }
    }
    else {
        paths.append(currentPage()->pathName());
    }
    app->findFiles(paths);
}

void MainWindow::on_actionOpenTerminal_triggered() {
    TabPage* page = currentPage();
    if(page) {
        Application* app = static_cast<Application*>(qApp);
        app->openFolderInTerminal(page->path());
    }
}

void MainWindow::on_actionCopyFullPath_triggered() {
    TabPage* page = currentPage();
    if(page) {
        auto paths = page->selectedFilePaths();
        if(paths.size() == 1) {
            QApplication::clipboard()->setText(QString::fromUtf8(paths.front().toString().get()), QClipboard::Clipboard);
        }
    }
}

void MainWindow::onShortcutNextTab() {
    int current = activeViewFrame_->getTabBar()->currentIndex();
    if(current < activeViewFrame_->getTabBar()->count() - 1) {
        activeViewFrame_->getTabBar()->setCurrentIndex(current + 1);
    }
    else {
        activeViewFrame_->getTabBar()->setCurrentIndex(0);
    }
}

void MainWindow::onShortcutPrevTab() {
    int current = activeViewFrame_->getTabBar()->currentIndex();
    if(current > 0) {
        activeViewFrame_->getTabBar()->setCurrentIndex(current - 1);
    }
    else {
        activeViewFrame_->getTabBar()->setCurrentIndex(activeViewFrame_->getTabBar()->count() - 1);
    }
}

// Switch to nth tab when Alt+n or Ctrl+n is pressed
void MainWindow::onShortcutJumpToTab() {
    QShortcut* shortcut = reinterpret_cast<QShortcut*>(sender());
    QKeySequence seq = shortcut->key();
    int keyValue = seq[0];
    // See the source code of QKeySequence and refer to the method:
    // QString QKeySequencePrivate::encodeString(int key, QKeySequence::SequenceFormat format).
    // Then we know how to test if a key sequence contains a modifier.
    // It's a shame that Qt has no API for this task.

    if((keyValue & Qt::ALT) == Qt::ALT) { // test if we have Alt key pressed
        keyValue -= Qt::ALT;
    }
    else if((keyValue & Qt::CTRL) == Qt::CTRL) { // test if we have Ctrl key pressed
        keyValue -= Qt::CTRL;
    }

    // now keyValue should contains '0' - '9' only
    int index;
    if(keyValue == '0') {
        index = 9;
    }
    else {
        index = keyValue - '1';
    }
    if(index < activeViewFrame_->getTabBar()->count()) {
        activeViewFrame_->getTabBar()->setCurrentIndex(index);
    }
}

}
