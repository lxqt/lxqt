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

#include "volumepopup.h"

#include "audiodevice.h"

#include <XdgIcon>

#include <QSlider>
#include <QStyleOptionButton>
#include <QPushButton>
#include <QVBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QToolTip>
#include "audioengine.h"
#include <QDebug>
#include <QWheelEvent>

VolumePopup::VolumePopup(QWidget* parent):
    QDialog(parent, Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::Popup | Qt::X11BypassWindowManagerHint),
    m_pos(0,0),
    m_anchor(Qt::TopLeftCorner),
    m_device(0)
{
    m_mixerButton = new QPushButton(this);
    m_mixerButton->setObjectName(QStringLiteral("MixerLink"));
    m_mixerButton->setMinimumWidth(1);
    m_mixerButton->setToolTip(tr("Launch mixer"));
    m_mixerButton->setText(tr("Mi&xer"));
    m_mixerButton->setAutoDefault(false);

    m_volumeSlider = new QSlider(Qt::Vertical, this);
    m_volumeSlider->setTickPosition(QSlider::TicksBothSides);
    m_volumeSlider->setTickInterval(10);
    // the volume slider shows 0-100 and volumes of all devices
    // should be converted to percentages.
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->installEventFilter(this);

    m_muteToggleButton = new QPushButton(this);
    m_muteToggleButton->setIcon(XdgIcon::fromTheme(QLatin1String("audio-volume-muted-panel")));
    m_muteToggleButton->setCheckable(true);
    m_muteToggleButton->setAutoDefault(false);

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setSpacing(0);
    l->setMargin(0);

    l->addWidget(m_mixerButton, 0, Qt::AlignHCenter);
    l->addWidget(m_volumeSlider, 0, Qt::AlignHCenter);
    l->addWidget(m_muteToggleButton, 0, Qt::AlignHCenter);

    connect(m_mixerButton, SIGNAL(released()), this, SIGNAL(launchMixer()));
    connect(m_volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(handleSliderValueChanged(int)));
    connect(m_muteToggleButton, SIGNAL(clicked()), this, SLOT(handleMuteToggleClicked()));
}

bool VolumePopup::event(QEvent *event)
{
    if(event->type() == QEvent::WindowDeactivate)
    {
        // qDebug("QEvent::WindowDeactivate");
        hide();
    }
    return QDialog::event(event);
}

bool VolumePopup::eventFilter(QObject * watched, QEvent * event)
{
    if (watched == m_volumeSlider)
    {
        if (event->type() == QEvent::Wheel)
        {
            handleWheelEvent(dynamic_cast<QWheelEvent *>(event));
            return true;
        }
        return false;
    }
    return QDialog::eventFilter(watched, event);
}

void VolumePopup::enterEvent(QEvent * /*event*/)
{
    emit mouseEntered();
}

void VolumePopup::leaveEvent(QEvent * /*event*/)
{
    // qDebug("leaveEvent");
    emit mouseLeft();
}

void VolumePopup::handleSliderValueChanged(int value)
{
    if (!m_device)
        return;
    // qDebug("VolumePopup::handleSliderValueChanged: %d\n", value);
    m_device->setVolume(value);
    QTimer::singleShot(0, this, [this] { QToolTip::showText(QCursor::pos(), m_volumeSlider->toolTip()); });
}

void VolumePopup::handleMuteToggleClicked()
{
    if (!m_device)
        return;

    m_device->toggleMute();
}

void VolumePopup::handleDeviceVolumeChanged(int volume)
{
    // qDebug() << "handleDeviceVolumeChanged" << "volume" << volume << "max" << max;
    // calling m_volumeSlider->setValue will trigger
    // handleSliderValueChanged(), which set the device volume
    // again, so we have to block the signals to avoid recursive
    // signal emission.
    m_volumeSlider->blockSignals(true);
    m_volumeSlider->setValue(volume);
    m_volumeSlider->setToolTip(QStringLiteral("%1%").arg(volume));
    dynamic_cast<QWidget&>(*parent()).setToolTip(m_volumeSlider->toolTip()); //parent is the button on panel
    m_volumeSlider->blockSignals(false);

    // emit volumeChanged(percent);
    updateStockIcon();
}

void VolumePopup::handleDeviceMuteChanged(bool mute)
{
    m_muteToggleButton->setChecked(mute);
    updateStockIcon();
}

void VolumePopup::updateStockIcon()
{
    if (!m_device)
        return;

    QString iconName;
    if (m_device->volume() <= 0 || m_device->mute())
        iconName = QLatin1String("audio-volume-muted");
    else if (m_device->volume() <= 33)
        iconName = QLatin1String("audio-volume-low");
    else if (m_device->volume() <= 66)
        iconName = QLatin1String("audio-volume-medium");
    else
        iconName = QLatin1String("audio-volume-high");

    iconName.append(QLatin1String("-panel"));
    m_muteToggleButton->setIcon(XdgIcon::fromTheme(iconName));
    emit stockIconChanged(iconName);
}

void VolumePopup::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    realign();
}

void VolumePopup::openAt(QPoint pos, Qt::Corner anchor)
{
    m_pos = pos;
    m_anchor = anchor;
    realign();
    show();
}

void VolumePopup::handleWheelEvent(QWheelEvent *event)
{
    m_volumeSlider->setSliderPosition(m_volumeSlider->sliderPosition()
            + (event->angleDelta().y() / QWheelEvent::DefaultDeltasPerStep * m_volumeSlider->singleStep()));
}

void VolumePopup::setDevice(AudioDevice *device)
{
    if (device == m_device)
        return;

    // disconnect old device
    if (m_device)
        disconnect(m_device);

    m_device = device;

    if (m_device) {
        m_muteToggleButton->setChecked(m_device->mute());
        handleDeviceVolumeChanged(m_device->volume());
        connect(m_device, SIGNAL(volumeChanged(int)), this, SLOT(handleDeviceVolumeChanged(int)));
        connect(m_device, SIGNAL(muteChanged(bool)), this, SLOT(handleDeviceMuteChanged(bool)));
    }
    else
        updateStockIcon();
    emit deviceChanged();
}

void VolumePopup::setSliderStep(int step)
{
    m_volumeSlider->setSingleStep(step);
    m_volumeSlider->setPageStep(step * 10);
}

void VolumePopup::realign()
{
    QRect rect;
    rect.setSize(sizeHint());
    switch (m_anchor)
    {
    case Qt::TopLeftCorner:
        rect.moveTopLeft(m_pos);
        break;

    case Qt::TopRightCorner:
        rect.moveTopRight(m_pos);
        break;

    case Qt::BottomLeftCorner:
        rect.moveBottomLeft(m_pos);
        break;

    case Qt::BottomRightCorner:
        rect.moveBottomRight(m_pos);
        break;

    }

    QRect screen = QApplication::desktop()->availableGeometry(m_pos);

    if (rect.right() > screen.right())
        rect.moveRight(screen.right());

    if (rect.bottom() > screen.bottom())
        rect.moveBottom(screen.bottom());

    move(rect.topLeft());
}
