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

#include "popup.h"
#include "../panel/ilxqtpanelplugin.h"

#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QTimer>
#include <Solid/StorageAccess>
#include <Solid/StorageDrive>
#include <Solid/DeviceNotifier>

// Paulo: I'm not sure what this is for
static bool hasRemovableParent(Solid::Device device)
{
    // qDebug() << "acess:" << device.udi();
    for ( ; !device.udi().isEmpty(); device = device.parent())
    {
        Solid::StorageDrive* drive = device.as<Solid::StorageDrive>();
        if (drive && drive->isRemovable())
        {
            // qDebug() << "removable parent drive:" << device.udi();
            return true;
        }
    }
    return false;
}

Popup::Popup(ILXQtPanelPlugin * plugin, QWidget* parent):
    QDialog(parent,  Qt::Window | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::Popup | Qt::X11BypassWindowManagerHint),
    mPlugin(plugin),
    mPlaceholder(nullptr),
    mDisplayCount(0)
{
    setObjectName(QStringLiteral("LXQtMountPopup"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setLayout(new QVBoxLayout(this));
    layout()->setMargin(0);

    setAttribute(Qt::WA_AlwaysShowToolTips);

    mPlaceholder = new QLabel(tr("No devices are available"), this);
    mPlaceholder->setObjectName(QStringLiteral("NoDiskLabel"));
    layout()->addWidget(mPlaceholder);

    //Perform the potential long time operation after object construction
    //Note: can't use QTimer::singleShot with lambda in pre QT 5.4 code
    QTimer * aux_timer = new QTimer;
    connect(aux_timer, &QTimer::timeout, [this, aux_timer]
        {
            delete aux_timer; //cleanup
            const auto devices = Solid::Device::listFromType(Solid::DeviceInterface::StorageAccess);
            for (const Solid::Device& device : devices)
                if (hasRemovableParent(device))
                    addItem(device);
        });
    aux_timer->setSingleShot(true);
    aux_timer->start(0);

    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded,
            this, &Popup::onDeviceAdded);
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved,
            this, &Popup::onDeviceRemoved);
}

void Popup::showHide()
{
    if (isHidden())
    {
        mPlugin->willShowWindow(this);
        show();
    } else
        close();
}

void Popup::onDeviceAdded(QString const & udi)
{
    Solid::Device device(udi);
    if (device.is<Solid::StorageAccess>() && hasRemovableParent(device))
        addItem(device);
}

void Popup::onDeviceRemoved(QString const & udi)
{
    MenuDiskItem* item = nullptr;
    const int size = layout()->count() - 1;
    for (int i = size; 0 <= i; --i)
    {
        QWidget *w = layout()->itemAt(i)->widget();
        if (w == mPlaceholder)
            continue;

        MenuDiskItem *it = static_cast<MenuDiskItem *>(w);
        if (udi == it->deviceUdi())
        {
            item = it;
            break;
        }
    }

    if (item != nullptr)
    {
        layout()->removeWidget(item);
        item->deleteLater();

        --mDisplayCount;
        if (mDisplayCount == 0)
            mPlaceholder->show();

        emit deviceRemoved(Solid::Device{udi});
    }
}

void Popup::showEvent(QShowEvent *event)
{
    mPlaceholder->setVisible(mDisplayCount == 0);
    realign();
    setFocus();
    activateWindow();
    QWidget::showEvent(event);
    emit visibilityChanged(true);
}

void Popup::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    emit visibilityChanged(false);
}

void Popup::addItem(Solid::Device device)
{
    MenuDiskItem *item = new MenuDiskItem(device, this);
    connect(item, &MenuDiskItem::invalid, this, &Popup::onDeviceRemoved);
    item->setVisible(true);
    layout()->addWidget(item);

    mDisplayCount++;
    if (mDisplayCount != 0)
        mPlaceholder->hide();

    if (isVisible())
        realign();

    emit deviceAdded(device);
}

void Popup::realign()
{
    adjustSize();
    setGeometry(mPlugin->calculatePopupWindowPos(sizeHint()));
}
