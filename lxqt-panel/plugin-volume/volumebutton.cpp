/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Johannes Zellner <webmaster@nebulon.de>
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

#include "volumebutton.h"

#include "volumepopup.h"
#include "audiodevice.h"

#include <QSlider>
#include <QMouseEvent>
#include <QProcess>
#include <QToolTip>

#include <XdgIcon>
#include "../panel/ilxqtpanel.h"
#include "../panel/ilxqtpanelplugin.h"

VolumeButton::VolumeButton(ILXQtPanelPlugin *plugin, QWidget* parent):
        QToolButton(parent),
        mPlugin(plugin),
        m_panel(plugin->panel()),
        m_showOnClick(true),
        m_muteOnMiddleClick(true)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoRaise(true);
    setMouseTracking(true);
    // initial icon for button. It will be replaced after devices scan.
    // In the worst case - no soundcard/pulse - is found it remains
    // in the button but at least the button is not blank ("invisible")
    handleStockIconChanged(QStringLiteral("dialog-error"));
    m_volumePopup = new VolumePopup(this);

    m_popupHideTimer.setInterval(1000);
    connect(this, SIGNAL(clicked()), this, SLOT(toggleVolumeSlider()));
    connect(&m_popupHideTimer, SIGNAL(timeout()), this, SLOT(hideVolumeSlider()));

    connect(m_volumePopup, SIGNAL(mouseEntered()), &m_popupHideTimer, SLOT(stop()));
    connect(m_volumePopup, SIGNAL(mouseLeft()), &m_popupHideTimer, SLOT(start()));

    connect(m_volumePopup, SIGNAL(launchMixer()), this, SLOT(handleMixerLaunch()));
    connect(m_volumePopup, SIGNAL(stockIconChanged(QString)), this, SLOT(handleStockIconChanged(QString)));
}

VolumeButton::~VolumeButton()
{
}

void VolumeButton::setShowOnClicked(bool state)
{
    if (m_showOnClick == state)
        return;

    m_showOnClick = state;
}

void VolumeButton::setMuteOnMiddleClick(bool state)
{
    m_muteOnMiddleClick = state;
}

void VolumeButton::setMixerCommand(const QString &command)
{
    m_mixerCommand = command;
}

void VolumeButton::enterEvent(QEvent * /*event*/)
{
    if (!m_showOnClick)
        showVolumeSlider();

    m_popupHideTimer.stop();
}

void VolumeButton::mouseMoveEvent(QMouseEvent *event)
{
    // moving the tooltip must be achieved by chaging the text
    // (hide/show - won't work because of the internal hide delay)
    QString tooltip = toolTip();
    if (!tooltip.isEmpty())
    {
        *(tooltip.rbegin()) = QLatin1Char('X');
        QToolTip::showText(event->globalPos(), tooltip);
        QToolTip::showText(event->globalPos(), toolTip());
    }
    QToolButton::mouseMoveEvent(event);
}

void VolumeButton::leaveEvent(QEvent * /*event*/)
{
    m_popupHideTimer.start();
}

void VolumeButton::wheelEvent(QWheelEvent *event)
{
    m_volumePopup->handleWheelEvent(event);
}

void VolumeButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MidButton && m_muteOnMiddleClick) {
        if (m_volumePopup->device()) {
            m_volumePopup->device()->toggleMute();
            return;
        }
    }

    QToolButton::mouseReleaseEvent(event);
}

void VolumeButton::toggleVolumeSlider()
{
    if (m_volumePopup->isVisible()) {
        hideVolumeSlider();
    } else {
        showVolumeSlider();
    }
}

void VolumeButton::showVolumeSlider()
{

    if (m_volumePopup->isVisible())
        return;

    m_popupHideTimer.stop();
    m_volumePopup->updateGeometry();
    m_volumePopup->adjustSize();
    QRect pos = mPlugin->calculatePopupWindowPos(m_volumePopup->size());
    mPlugin->willShowWindow(m_volumePopup);
    m_volumePopup->openAt(pos.topLeft(), Qt::TopLeftCorner);
    m_volumePopup->activateWindow();
}

void VolumeButton::hideVolumeSlider()
{
    // qDebug() << "hideVolumeSlider";
    m_popupHideTimer.stop();
    m_volumePopup->hide();
}

void VolumeButton::handleMixerLaunch()
{
    QProcess::startDetached(m_mixerCommand);
}

void VolumeButton::handleStockIconChanged(const QString &iconName)
{
    setIcon(XdgIcon::fromTheme(iconName));
}
