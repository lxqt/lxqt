/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
 * Authors:
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

#include <QButtonGroup>
#include <QWheelEvent>
#include <QtDebug>
#include <QSignalMapper>
#include <QTimer>
#include <lxqt-globalkeys.h>
#include <LXQt/GridLayout>
#include <KWindowSystem/KWindowSystem>
#include <QX11Info>
#include <cmath>

#include "desktopswitch.h"
#include "desktopswitchbutton.h"
#include "desktopswitchconfiguration.h"

static const QString DEFAULT_SHORTCUT_TEMPLATE(QStringLiteral("Control+F%1"));

DesktopSwitch::DesktopSwitch(const ILXQtPanelPluginStartupInfo &startupInfo) :
    QObject(),
    ILXQtPanelPlugin(startupInfo),
    m_pSignalMapper(new QSignalMapper(this)),
    m_desktopCount(KWindowSystem::numberOfDesktops()),
    mRows(-1),
    mShowOnlyActive(false),
    mDesktops(new NETRootInfo(QX11Info::connection(), NET::NumberOfDesktops | NET::CurrentDesktop | NET::DesktopNames, NET::WM2DesktopLayout)),
    mLabelType(static_cast<DesktopSwitchButton::LabelType>(-1))
{
    m_buttons = new QButtonGroup(this);
    connect (m_pSignalMapper, SIGNAL(mapped(int)), this, SLOT(setDesktop(int)));

    mLayout = new LXQt::GridLayout(&mWidget);
    mWidget.setLayout(mLayout);

    settingsChanged();

    onCurrentDesktopChanged(KWindowSystem::currentDesktop());
    QTimer::singleShot(0, this, SLOT(registerShortcuts()));

    connect(m_buttons, SIGNAL(buttonClicked(int)), this, SLOT(setDesktop(int)));

    connect(KWindowSystem::self(), SIGNAL(numberOfDesktopsChanged(int)), SLOT(onNumberOfDesktopsChanged(int)));
    connect(KWindowSystem::self(), SIGNAL(currentDesktopChanged(int)), SLOT(onCurrentDesktopChanged(int)));
    connect(KWindowSystem::self(), SIGNAL(desktopNamesChanged()), SLOT(onDesktopNamesChanged()));

    connect(KWindowSystem::self(), static_cast<void (KWindowSystem::*)(WId, NET::Properties, NET::Properties2)>(&KWindowSystem::windowChanged),
            this, &DesktopSwitch::onWindowChanged);
}

void DesktopSwitch::registerShortcuts()
{
    // Register shortcuts to change desktop
    GlobalKeyShortcut::Action * gshortcut;
    QString path;
    QString description;
    for (int i = 0; i < 12; ++i)
    {
        path = QStringLiteral("/panel/%1/desktop_%2").arg(settings()->group()).arg(i + 1);
        description = tr("Switch to desktop %1").arg(i + 1);

        gshortcut = GlobalKeyShortcut::Client::instance()->addAction(QString(), path, description, this);
        if (nullptr != gshortcut)
        {
            m_keys << gshortcut;
            connect(gshortcut, &GlobalKeyShortcut::Action::registrationFinished, this, &DesktopSwitch::shortcutRegistered);
            connect(gshortcut, SIGNAL(activated()), m_pSignalMapper, SLOT(map()));
            m_pSignalMapper->setMapping(gshortcut, i);
        }
    }
}

void DesktopSwitch::shortcutRegistered()
{
    GlobalKeyShortcut::Action * const shortcut = qobject_cast<GlobalKeyShortcut::Action*>(sender());

    disconnect(shortcut, &GlobalKeyShortcut::Action::registrationFinished, this, &DesktopSwitch::shortcutRegistered);

    const int i = m_keys.indexOf(shortcut);
    Q_ASSERT(-1 != i);

    if (shortcut->shortcut().isEmpty())
    {
        shortcut->changeShortcut(DEFAULT_SHORTCUT_TEMPLATE.arg(i + 1));
    }
}

void DesktopSwitch::onWindowChanged(WId id, NET::Properties properties, NET::Properties2 /*properties2*/)
{
    if (properties.testFlag(NET::WMState) && isWindowHighlightable(id))
    {
        KWindowInfo info = KWindowInfo(id, NET::WMDesktop | NET::WMState);
        int desktop = info.desktop();
        if (!info.valid() || info.onAllDesktops())
            return;
        else
        {
            DesktopSwitchButton *button = static_cast<DesktopSwitchButton *>(m_buttons->button(desktop - 1));
            if(button)
                button->setUrgencyHint(id, info.hasState(NET::DemandsAttention));
        }
    }
}

void DesktopSwitch::refresh()
{
    const QList<QAbstractButton*> btns = m_buttons->buttons();

    int i = 0;
    const int current_desktop = KWindowSystem::currentDesktop();
    const int current_cnt = btns.count();
    const int border = qMin(btns.count(), m_desktopCount);
    //update existing buttons
    for ( ; i < border; ++i)
    {
        DesktopSwitchButton * button = qobject_cast<DesktopSwitchButton*>(btns[i]);
        button->update(i, mLabelType,
                       KWindowSystem::desktopName(i + 1).isEmpty() ?
                       tr("Desktop %1").arg(i + 1) :
                       KWindowSystem::desktopName(i + 1));
        button->setVisible(!mShowOnlyActive || i + 1 == current_desktop);
    }

    //create new buttons (if neccessary)
    QAbstractButton *b;
    for ( ; i < m_desktopCount; ++i)
    {
        b = new DesktopSwitchButton(&mWidget, i, mLabelType,
                KWindowSystem::desktopName(i+1).isEmpty() ?
                tr("Desktop %1").arg(i+1) :
                KWindowSystem::desktopName(i+1));
        mWidget.layout()->addWidget(b);
        m_buttons->addButton(b, i);
        b->setVisible(!mShowOnlyActive || i + 1 == current_desktop);
    }

    //delete unneeded buttons (if neccessary)
    for ( ; i < current_cnt; ++i)
    {
        b = m_buttons->buttons().constLast();
        m_buttons->removeButton(b);
        mWidget.layout()->removeWidget(b);
        delete b;
    }
}

bool DesktopSwitch::isWindowHighlightable(WId window)
{
    // this method was borrowed from the taskbar plugin
    QFlags<NET::WindowTypeMask> ignoreList;
    ignoreList |= NET::DesktopMask;
    ignoreList |= NET::DockMask;
    ignoreList |= NET::SplashMask;
    ignoreList |= NET::ToolbarMask;
    ignoreList |= NET::MenuMask;
    ignoreList |= NET::PopupMenuMask;
    ignoreList |= NET::NotificationMask;

    KWindowInfo info(window, NET::WMWindowType | NET::WMState, NET::WM2TransientFor);
    if (!info.valid())
        return false;

    if (NET::typeMatchesMask(info.windowType(NET::AllTypesMask), ignoreList))
        return false;

    if (info.state() & NET::SkipTaskbar)
        return false;

    // WM_TRANSIENT_FOR hint not set - normal window
    WId transFor = info.transientFor();
    if (transFor == 0 || transFor == window || transFor == (WId) QX11Info::appRootWindow())
        return true;

    info = KWindowInfo(transFor, NET::WMWindowType);

    QFlags<NET::WindowTypeMask> normalFlag;
    normalFlag |= NET::NormalMask;
    normalFlag |= NET::DialogMask;
    normalFlag |= NET::UtilityMask;

    return !NET::typeMatchesMask(info.windowType(NET::AllTypesMask), normalFlag);
}

DesktopSwitch::~DesktopSwitch()
{
}

void DesktopSwitch::setDesktop(int desktop)
{
    KWindowSystem::setCurrentDesktop(desktop + 1);
}

void DesktopSwitch::onNumberOfDesktopsChanged(int count)
{
    qDebug() << "Desktop count changed from" << m_desktopCount << "to" << count;
    m_desktopCount = count;
    refresh();
}

void DesktopSwitch::onCurrentDesktopChanged(int current)
{
    if (mShowOnlyActive)
    {
        int i = 1;
        const auto buttons = m_buttons->buttons();
        for (const auto button : buttons)
        {
            if (current == i)
            {
                button->setChecked(true);
                button->setVisible(true);
            } else
            {
                button->setVisible(false);
            }
            ++i;
        }
    } else
    {
        QAbstractButton *button = m_buttons->button(current - 1);
        if (button)
            button->setChecked(true);
    }
}

void DesktopSwitch::onDesktopNamesChanged()
{
    refresh();
}

void DesktopSwitch::settingsChanged()
{
    const int rows = settings()->value(QStringLiteral("rows"), 1).toInt();
    const bool show_only_active = settings()->value(QStringLiteral("showOnlyActive"), false).toBool();
    const int label_type = settings()->value(QStringLiteral("labelType"), DesktopSwitchButton::LABEL_TYPE_NUMBER).toInt();

    const bool need_realign = mRows != rows || show_only_active != mShowOnlyActive;
    const bool need_refresh = mLabelType != static_cast<DesktopSwitchButton::LabelType>(label_type) || show_only_active != mShowOnlyActive;

    mRows = rows;
    mShowOnlyActive = show_only_active;
    mLabelType = static_cast<DesktopSwitchButton::LabelType>(label_type);
    if (need_realign)
    {
        // WARNING: Changing the desktop layout may call "LXQtPanel::realign", which calls
        // "DesktopSwitch::realign()". Therefore, the desktop layout should not be changed
        // inside the latter method.
        int columns = static_cast<int>(ceil(static_cast<float>(m_desktopCount) / mRows));
        if (panel()->isHorizontal())
        {
            mDesktops->setDesktopLayout(NET::OrientationHorizontal, columns, mRows, mWidget.isRightToLeft() ? NET::DesktopLayoutCornerTopRight : NET::DesktopLayoutCornerTopLeft);
        }
        else
        {
            mDesktops->setDesktopLayout(NET::OrientationHorizontal, mRows, columns, mWidget.isRightToLeft() ? NET::DesktopLayoutCornerTopRight : NET::DesktopLayoutCornerTopLeft);
        }
        realign(); // in case it isn't called when the desktop layout changes
    }
    if (need_refresh)
        refresh();
}

void DesktopSwitch::realign()
{
    mLayout->setEnabled(false);
    if (panel()->isHorizontal())
    {
        mLayout->setRowCount(mShowOnlyActive ? 1 : mRows);
        mLayout->setColumnCount(0);
    }
    else
    {
        mLayout->setColumnCount(mShowOnlyActive ? 1 : mRows);
        mLayout->setRowCount(0);
    }
    mLayout->setEnabled(true);
}

QDialog *DesktopSwitch::configureDialog()
{
    return new DesktopSwitchConfiguration(settings());
}

DesktopSwitchWidget::DesktopSwitchWidget():
    QFrame(),
    m_mouseWheelThresholdCounter(0)
{
}

void DesktopSwitchWidget::wheelEvent(QWheelEvent *e)
{
    // Without some sort of threshold which has to be passed, scrolling is too sensitive
    m_mouseWheelThresholdCounter -= e->delta();
    // If the user hasn't scrolled far enough in one direction (positive or negative): do nothing
    if(abs(m_mouseWheelThresholdCounter) < 100)
        return;

    int max = KWindowSystem::numberOfDesktops();
    int delta = e->delta() < 0 ? 1 : -1;
    int current = KWindowSystem::currentDesktop() + delta;

    if (current > max){
        current = 1;
    }
    else if (current < 1)
        current = max;

    m_mouseWheelThresholdCounter = 0;
    KWindowSystem::setCurrentDesktop(current);
}
