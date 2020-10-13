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

#ifndef LXQT_PLUGIN_MOUNT_MENUDISKITEM_H
#define LXQT_PLUGIN_MOUNT_MENUDISKITEM_H

#include <QFrame>
#include <QToolButton>
#include <Solid/Device>
#include <Solid/SolidNamespace>

class Popup;

class MenuDiskItem : public QFrame
{
    Q_OBJECT

public:
    explicit MenuDiskItem(Solid::Device device, Popup *popup);
    ~MenuDiskItem();

    QString deviceUdi() const { return mDevice.udi(); }
    void setMountStatus(bool mounted);

private:
    void updateMountStatus();
    Solid::Device opticalParent() const;

signals:
    void invalid(QString const & udi);

private slots:
    void diskButtonClicked();
    void ejectButtonClicked();

    void onMounted(Solid::ErrorType error,
                   QVariant resultData,
                   const QString &udi);
    void onUnmounted(Solid::ErrorType error,
                     QVariant resultData,
                     const QString &udi);

private:
    Popup *mPopup;
    Solid::Device mDevice;
    QToolButton *mDiskButton;
    QToolButton *mEjectButton;
    bool mDiskButtonClicked;
    bool mEjectButtonClicked;
};

#endif // MENUDISKITEM_H
