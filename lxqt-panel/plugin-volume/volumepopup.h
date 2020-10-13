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

#ifndef VOLUMEPOPUP_H
#define VOLUMEPOPUP_H

#include <QDialog>

class QSlider;
class QPushButton;
class AudioDevice;

class VolumePopup : public QDialog
{
    Q_OBJECT
public:
    VolumePopup(QWidget* parent = nullptr);

    void openAt(QPoint pos, Qt::Corner anchor);
    void handleWheelEvent(QWheelEvent *event);

    QSlider *volumeSlider() const { return m_volumeSlider; }

    AudioDevice *device() const { return m_device; }
    void setDevice(AudioDevice *device);
    void setSliderStep(int step);

signals:
    void mouseEntered();
    void mouseLeft();

    // void volumeChanged(int value);
    void deviceChanged();
    void launchMixer();
    void stockIconChanged(const QString &iconName);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool event(QEvent * event) override;
    bool eventFilter(QObject * watched, QEvent * event) override;

private slots:
    void handleSliderValueChanged(int value);
    void handleMuteToggleClicked();
    void handleDeviceVolumeChanged(int volume);
    void handleDeviceMuteChanged(bool mute);

private:
    void realign();
    void updateStockIcon();

    QSlider *m_volumeSlider;
    QPushButton *m_mixerButton;
    QPushButton *m_muteToggleButton;
    QPoint m_pos;
    Qt::Corner m_anchor;
    AudioDevice *m_device;
};

#endif // VOLUMEPOPUP_H
