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


#ifndef DESKTOPSWITCH_H
#define DESKTOPSWITCH_H

#include "../panel/ilxqtpanelplugin.h"
#include <QFrame>
#include <QScopedPointer>
#include <KWindowSystem/NETWM>

#include "desktopswitchbutton.h"

class QSignalMapper;
class QButtonGroup;
class NETRootInfo;
namespace LXQt {
class GridLayout;
}

class DesktopSwitchWidget: public QFrame
{
    Q_OBJECT
public:
    DesktopSwitchWidget();

private:
    int m_mouseWheelThresholdCounter;

protected:
    void wheelEvent(QWheelEvent* e);
};

/**
 * @brief Desktop switcher. A very simple one...
 */
class DesktopSwitch : public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT
public:
    DesktopSwitch(const ILXQtPanelPluginStartupInfo &startupInfo);
    ~DesktopSwitch();

    QString themeId() const { return QStringLiteral("DesktopSwitch"); }
    QWidget *widget() { return &mWidget; }
    bool isSeparate() const { return true; }
    void realign();

    virtual ILXQtPanelPlugin::Flags flags() const { return HaveConfigDialog; }
    QDialog *configureDialog();

private:
    QButtonGroup * m_buttons;
    QList<GlobalKeyShortcut::Action*> m_keys;
    QSignalMapper* m_pSignalMapper;
    int m_desktopCount;
    DesktopSwitchWidget mWidget;
    LXQt::GridLayout *mLayout;
    int mRows;
    bool mShowOnlyActive;
    QScopedPointer<NETRootInfo> mDesktops;
    DesktopSwitchButton::LabelType mLabelType;

    void refresh();
    bool isWindowHighlightable(WId window);

private slots:
    void setDesktop(int desktop);
    void onNumberOfDesktopsChanged(int);
    void onCurrentDesktopChanged(int);
    void onDesktopNamesChanged();
    virtual void settingsChanged();
    void registerShortcuts();
    void shortcutRegistered();
    void onWindowChanged(WId id, NET::Properties properties, NET::Properties2 properties2);
};

class DesktopSwitchPluginLibrary: public QObject, public ILXQtPanelPluginLibrary
{
    Q_OBJECT
    // Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const { return new DesktopSwitch(startupInfo);}
};

#endif
