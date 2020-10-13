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

#include "backlight.h"
#include <QEvent>

LXQtBacklight::LXQtBacklight(const ILXQtPanelPluginStartupInfo &startupInfo):
        QObject(),
        ILXQtPanelPlugin(startupInfo)
{
    m_backlightButton = new QToolButton();
    // use our own icon
    m_backlightButton->setIcon(QIcon::fromTheme(QStringLiteral("brightnesssettings")));

    connect(m_backlightButton, SIGNAL(clicked(bool)), this, SLOT(showSlider(bool)));

    m_backlightSlider = nullptr;
}


LXQtBacklight::~LXQtBacklight()
{
    delete m_backlightButton;
}


QWidget *LXQtBacklight::widget()
{
    return m_backlightButton;
}

void LXQtBacklight::deleteSlider()
{
    if(m_backlightSlider) {
        m_backlightSlider->deleteLater();
    }
    m_backlightSlider = nullptr;
    //printf("Deleted\n");
}

void LXQtBacklight::showSlider(bool)
{
    if(! m_backlightSlider) {
        m_backlightSlider = new SliderDialog(m_backlightButton);
        connect(m_backlightSlider, SIGNAL(dialogClosed()), this, SLOT(deleteSlider()));
        //printf("New Slider\n");
    }
    QSize size = m_backlightSlider->sizeHint();
    QRect rect = calculatePopupWindowPos(size);
    m_backlightSlider->setGeometry(rect);
    m_backlightSlider->updateBacklight();
    m_backlightSlider->show();
    m_backlightSlider->setFocus();
}



