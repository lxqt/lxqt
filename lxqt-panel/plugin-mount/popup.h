/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011-2013 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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

#ifndef LXQT_PLUGIN_MOUNT_POPUP_H
#define LXQT_PLUGIN_MOUNT_POPUP_H

#include "menudiskitem.h"

#include <QLabel>
#include <QDialog>
#include <Solid/Device>

class ILXQtPanelPlugin;

class Popup: public QDialog
{
    Q_OBJECT

public:
    explicit Popup(ILXQtPanelPlugin * plugin, QWidget* parent = nullptr);
    void realign();

public slots:
    void showHide();

private slots:
    void onDeviceAdded(QString const & udi);
    void onDeviceRemoved(QString const & udi);

signals:
    void visibilityChanged(bool visible);
    /*!
     * \brief Signal emitted when new device added into the popup
     * (device which we are interested in)
     */
    void deviceAdded(Solid::Device device);
    /*!
     * \brief Signal emitted when device is removed from the popup
     * (device which we are interested in)
     */
    void deviceRemoved(Solid::Device device);

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private:
    ILXQtPanelPlugin * mPlugin;
    QLabel *mPlaceholder;
    int mDisplayCount;

    void addItem(Solid::Device device);
};

#endif // POPUP_H
