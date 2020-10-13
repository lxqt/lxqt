/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   Petr Vanek <petr@scribus.info>
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

#include "dialog.h"
#include "ui_dialog.h"
#include "widgets.h"
#include "commanditemmodel.h"
#include "configuredialog/configuredialog.h"

#include <LXQt/Globals>
#include <LXQt/Settings>
#include <LXQt/HtmlDelegate>
#include <LXQt/PowerManager>
#include <LXQt/ScreenSaver>
#include <LXQtGlobalKeys/Action>
#include <LXQtGlobalKeys/Client>

#include <QApplication>
#include <QDebug>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QProcess>
#include <QLineEdit>
#include <QAction>
#include <QMessageBox>
#include <QApplication>
#include <QMenu>
#include <QWindow>
#include <QScreen>
#include <QScrollBar>

#include <KWindowSystem/KWindowSystem>

#define DEFAULT_SHORTCUT "Alt+F2"

/************************************************

 ************************************************/
Dialog::Dialog(QWidget *parent) :
    QDialog(parent, Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint),
    ui(new Ui::Dialog),
    mSettings(new LXQt::Settings(QSL("lxqt-runner"), this)),
    mGlobalShortcut(0),
    mLockCascadeChanges(false),
    mDesktopChanged(false),
    mConfigureDialog(0)
{
    ui->setupUi(this);
    setWindowTitle(QSL("LXQt Runner"));
    setAttribute(Qt::WA_TranslucentBackground);

    connect(LXQt::Settings::globalSettings(), SIGNAL(iconThemeChanged()), this, SLOT(update()));
    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(hide()));
    connect(mSettings, SIGNAL(settingsChanged()), this, SLOT(applySettings()));

    mSearchTimer.setSingleShot(true);
    connect(&mSearchTimer, &QTimer::timeout, ui->commandEd, [this] {
        setFilter(ui->commandEd->text());
    });
    mSearchTimer.setInterval(350); // typing speed (not very fast)

    ui->commandEd->installEventFilter(this);

    connect(ui->commandEd, &QLineEdit::textChanged, [this] (QString const &) {
        mSearchTimer.start();
    });
    connect(ui->commandEd, &QLineEdit::returnPressed, this, &Dialog::runCommand);

    mCommandItemModel = new CommandItemModel(mSettings->value(QL1S("dialog/history_use"), true).toBool(), this);
    ui->commandList->installEventFilter(this);
    ui->commandList->setModel(mCommandItemModel);
    ui->commandList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->commandList, SIGNAL(clicked(QModelIndex)), this, SLOT(runCommand()));
    setFilter(QString());
    dataChanged();

    ui->commandList->setItemDelegate(new LXQt::HtmlDelegate(QSize(32, 32), ui->commandList));

    // Popup menu ...............................
    QAction *a = new QAction(QIcon::fromTheme(QSL("configure")), tr("Configure"), this);
    connect(a, &QAction::triggered, this, &Dialog::showConfigDialog);
    addAction(a);

    a = new QAction(QIcon::fromTheme(QSL("edit-clear-history")), tr("Clear History"), this);
    connect(a, &QAction::triggered, mCommandItemModel, &CommandItemModel::clearHistory);
    addAction(a);

    mPowerManager = new LXQt::PowerManager(this);
    addActions(mPowerManager->availableActions());
    mScreenSaver = new LXQt::ScreenSaver(this);
    addActions(mScreenSaver->availableActions());

    setContextMenuPolicy(Qt::ActionsContextMenu);

    QMenu *menu = new QMenu(this);
    menu->addActions(actions());
    ui->actionButton->setMenu(menu);
    // End of popup menu ........................

    applySettings();

    // screen updates
    connect(qApp, &QApplication::screenAdded, this, &Dialog::realign);
    connect(qApp, &QApplication::screenRemoved, this, &Dialog::realign);
    const auto primaryScreen = QGuiApplication::primaryScreen();
    if (primaryScreen != nullptr)
        connect(primaryScreen, &QScreen::availableGeometryChanged, this, &Dialog::realign);

    connect(mGlobalShortcut, &GlobalKeyShortcut::Action::activated, this, &Dialog::showHide);
    connect(mGlobalShortcut, &GlobalKeyShortcut::Action::shortcutChanged, this, &Dialog::shortcutChanged);
    connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, this, &Dialog::onActiveWindowChanged);
    connect(KWindowSystem::self(), &KWindowSystem::currentDesktopChanged, this, &Dialog::onCurrentDesktopChanged);

    resize(mSettings->value(QL1S("dialog/width"), 400).toInt(), size().height());

    // TEST
    connect(mCommandItemModel, &CommandItemModel::layoutChanged, this, &Dialog::dataChanged);
}


/************************************************

 ************************************************/
Dialog::~Dialog()
{
    delete ui;
}


/************************************************

 ************************************************/
void Dialog::closeEvent(QCloseEvent *event)
{
    hide();
    event->accept();
}


/************************************************

 ************************************************/
QSize Dialog::sizeHint() const
{
    QSize size = QDialog::sizeHint();
    size.setWidth(this->size().width());
    return size;
}


/************************************************

 ************************************************/
void Dialog::resizeEvent(QResizeEvent * /*event*/)
{
    mSettings->setValue(QL1S("dialog/width"), size().width());
}


/************************************************

 ************************************************/
bool Dialog::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (object == ui->commandEd)
            return editKeyPressEvent(keyEvent);

        if (object == ui->commandList)
            return listKeyPressEvent(keyEvent);
    }

    return QDialog::eventFilter(object, event);
}


/************************************************
 eventFilter for ui->commandEd
 ************************************************/
bool Dialog::editKeyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_N:
        if (event->modifiers().testFlag(Qt::ControlModifier))
        {
            QKeyEvent ev(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
            editKeyPressEvent(&ev);
            return true;
        }
            return false;

    case Qt::Key_P:
        if (event->modifiers().testFlag(Qt::ControlModifier))
        {
            QKeyEvent ev(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
            editKeyPressEvent(&ev);
            return true;
        }
            return false;

    case Qt::Key_Up:
    case Qt::Key_PageUp:
        if (ui->commandEd->text().isEmpty() &&
            ui->commandList->isVisible() &&
            ui->commandList->currentIndex().row() == 0
           )
        {
            setFilter(QString(), false);
            return true;
        }
        qApp->sendEvent(ui->commandList, event);
        return true;

    case Qt::Key_Down:
    case Qt::Key_PageDown:
        if (ui->commandEd->text().isEmpty() &&
            ui->commandList->isHidden()
           )
        {
            setFilter(QString(), true);

            // set focus to the list so that it highlights the first item correctly,
            // and then set it back to the textfield, where it belongs
            ui->commandList->setFocus();
            ui->commandEd->setFocus();
            return true;
        }
        qApp->sendEvent(ui->commandList, event);
        return true;

    case Qt::Key_Tab:
        const CommandProviderItem *command = mCommandItemModel->command(ui->commandList->currentIndex());
        if (command)
            ui->commandEd->setText(command->title());
        return true;
    }

    return QDialog::eventFilter(ui->commandList, event);
}


/************************************************
 eventFilter for ui->commandList
 ************************************************/
bool Dialog::listKeyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Up:
    case Qt::Key_PageUp:
        if (ui->commandList->currentIndex().row() == 0)
        {
            ui->commandEd->setFocus();
            return true;
        }
        break;

    case Qt::Key_Left:
    case Qt::Key_Right:
        ui->commandEd->setFocus();
        qApp->sendEvent(ui->commandEd, event);
        return true;

    default:
        // Alphabetical or number key ...........
        if (!event->text().isEmpty())
        {
            ui->commandEd->setFocus();
            qApp->sendEvent(ui->commandEd, event);
            return true;
        }
    }

    return QDialog::eventFilter(ui->commandEd, event);
}


/************************************************

 ************************************************/
void Dialog::showHide()
{
    // Using KWindowSystem to detect the active window since
    // QWidget::isActiveWindow is not working reliably.
    if (isVisible() && (KWindowSystem::activeWindow() == winId()))
    {
        hide();
    }
    else
    {
        realign();
        show();
        KWindowSystem::forceActiveWindow(winId());
        ui->commandEd->setFocus();
    }
}


/************************************************

 ************************************************/
void Dialog::realign()
{
    QRect desktop;

    int screenNumber = mMonitor;
    const auto screens = QGuiApplication::screens();
    if (mMonitor < 0 || mMonitor > screens.size() - 1) {
        const auto screen = QGuiApplication::screenAt(QCursor::pos());
        screenNumber = screen ? screens.indexOf(screen) : 0;
    }

    desktop = screens.at(screenNumber)->availableGeometry().intersected(KWindowSystem::workArea(screenNumber));

    QRect rect = this->geometry();
    rect.moveCenter(desktop.center());

    if (mShowOnTop)
        rect.moveTop(desktop.top());
    else
        rect.moveTop(desktop.center().y() - ui->panel->sizeHint().height());

    setGeometry(rect);
}


/************************************************

 ************************************************/
void Dialog::applySettings()
{
    if (mLockCascadeChanges)
        return;

    // Shortcut .................................
    QString shortcut = mSettings->value(QL1S("dialog/shortcut"), QL1S(DEFAULT_SHORTCUT)).toString();
    if (shortcut.isEmpty())
        shortcut = QL1S(DEFAULT_SHORTCUT);

    if (!mGlobalShortcut)
        mGlobalShortcut = GlobalKeyShortcut::Client::instance()->addAction(shortcut, QSL("/runner/show_hide_dialog"), tr("Show/hide runner dialog"), this);
    else if (mGlobalShortcut->shortcut() != shortcut)
        mGlobalShortcut->changeShortcut(shortcut);

    mShowOnTop = mSettings->value(QL1S("dialog/show_on_top"), true).toBool();

    mMonitor = mSettings->value(QL1S("dialog/monitor"), -1).toInt();

    mCommandItemModel->setUseHistory(mSettings->value(QL1S("dialog/history_use"), true).toBool());
    mCommandItemModel->showHistoryFirst(mSettings->value(QL1S("dialog/history_first"), true).toBool());
    ui->commandList->setShownCount(mSettings->value(QL1S("dialog/list_shown_items"), 4).toInt());

    realign();
    mSettings->sync();
}


/************************************************

 ************************************************/
void Dialog::shortcutChanged(const QString &/*oldShortcut*/, const QString &newShortcut)
{
    if (!newShortcut.isEmpty())
    {
        mLockCascadeChanges = true;

        mSettings->setValue(QL1S("dialog/shortcut"), newShortcut);
        mSettings->sync();

        mLockCascadeChanges = false;
    }
}


/************************************************

 ************************************************/
void Dialog::onActiveWindowChanged(WId id)
{
    if (isVisible() && 0 != id && id != winId())
    {
        if (mDesktopChanged)
        {
            mDesktopChanged = false;
            KWindowSystem::forceActiveWindow(winId());
        } else
        {
            hide();
        }
    }
}


/************************************************

 ************************************************/
void Dialog::onCurrentDesktopChanged(int screen)
{
    if (isVisible())
    {
        KWindowSystem::setOnDesktop(winId(), screen);
        KWindowSystem::forceActiveWindow(winId());
        //Note: workaround for changing desktop while runner is shown
        // The KWindowSystem::forceActiveWindow may fail to correctly activate runner if there
        // are any other windows on the new desktop (probably because of the sequence while WM
        // changes the virtual desktop (change desktop and activate any of the windows on it))
        mDesktopChanged = true;
    }
}


/************************************************

 ************************************************/
void Dialog::setFilter(const QString &text, bool onlyHistory)
{
    if (mCommandItemModel->isOutDated())
        mCommandItemModel->rebuild();

    QString trimmedText = text.simplified();
    mCommandItemModel->setCommand(trimmedText);
    mCommandItemModel->showOnlyHistory(onlyHistory);
    mCommandItemModel->setFilterRegExp(trimmedText);
    mCommandItemModel->invalidate();

    // tidy up layout and select first item
    ui->commandList->doItemsLayout();
    ui->commandList->setCurrentIndex(mCommandItemModel->index(0, 0));
}

/************************************************

 ************************************************/
void Dialog::dataChanged()
{
    if (mCommandItemModel->rowCount())
    {
       ui->commandList->setCurrentIndex(mCommandItemModel->appropriateItem(mCommandItemModel->command()));
        ui->commandList->scrollTo(ui->commandList->currentIndex());
        ui->commandList->show();
    }
    else
    {
        ui->commandList->hide();
    }

    adjustSize();

}


/************************************************

 ************************************************/
void Dialog::runCommand()
{
    bool res =false;
    const CommandProviderItem *command = mCommandItemModel->command(ui->commandList->currentIndex());

    if (command)
        res = command->run();

    if (res)
    {
        hide();
        ui->commandEd->clear();
    }

}


/************************************************

 ************************************************/
void Dialog::showConfigDialog()
{
    if (!mConfigureDialog)
        mConfigureDialog = new ConfigureDialog(mSettings, QL1S(DEFAULT_SHORTCUT), this);
    mConfigureDialog->exec();
}

#undef DEFAULT_SHORTCUT
