/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2020 LXQt team
 * Authors:
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


#ifndef LXQTBACKLIGHT_H
#define LXQTBACKLIGHT_H

#include <QToolButton>
#include "../panel/ilxqtpanelplugin.h"
#include "sliderdialog.h"

namespace LXQt {
class Notification;
}
namespace GlobalKeyShortcut
{
class Action;
}



class LXQtBacklight : public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT
public:
    LXQtBacklight(const ILXQtPanelPluginStartupInfo &startupInfo);
    ~LXQtBacklight();

    virtual QWidget *widget();
    virtual QString themeId() const { return QStringLiteral("Backlight"); }
    virtual ILXQtPanelPlugin::Flags flags() const { return PreferRightAlignment ; }

protected Q_SLOTS:
    void showSlider(bool);
    void deleteSlider();

private:
    QToolButton *m_backlightButton;
    SliderDialog *m_backlightSlider;
};


class LXQtBacklightPluginLibrary: public QObject, public ILXQtPanelPluginLibrary
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const
    {
        return new LXQtBacklight(startupInfo);
    }
};

class Slider: public QSlider
{
public:
    Slider(QWidget *parent);
protected:
    bool event(QEvent * event) override;
};

#endif // LXQTBACKLIGHT_H
