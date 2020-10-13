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

#include "menudiskitem.h"
#include "popup.h"

#include <QDesktopServices>
#include <QEvent>
#include <QHBoxLayout>
#include <QUrl>
#include <QTimer>
#include <XdgIcon>
#include <Solid/StorageAccess>
#include <Solid/OpticalDrive>
#include <LXQt/Notification>
#include <QDebug>

MenuDiskItem::MenuDiskItem(Solid::Device device, Popup *popup):
    QFrame(popup),
    mPopup(popup),
    mDevice(device),
    mDiskButton(nullptr),
    mEjectButton(nullptr),
    mDiskButtonClicked(false),
    mEjectButtonClicked(false)
{
    Solid::StorageAccess * const iface = device.as<Solid::StorageAccess>();
    Q_ASSERT(nullptr != iface);

    mDiskButton = new QToolButton(this);
    mDiskButton->setObjectName(QStringLiteral("DiskButton"));
    mDiskButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mDiskButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    connect(mDiskButton, &QToolButton::clicked, this, &MenuDiskItem::diskButtonClicked);

    mEjectButton = new QToolButton(this);
    mEjectButton->setObjectName(QStringLiteral("EjectButton"));
    mEjectButton->setIcon(XdgIcon::fromTheme(QStringLiteral("media-eject")));
    connect(mEjectButton, &QToolButton::clicked, this, &MenuDiskItem::ejectButtonClicked);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(mDiskButton);
    layout->addWidget(mEjectButton);
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);

    connect(iface, &Solid::StorageAccess::setupDone, this, &MenuDiskItem::onMounted);
    connect(iface, &Solid::StorageAccess::teardownDone, this, &MenuDiskItem::onUnmounted);
    connect(iface, &Solid::StorageAccess::accessibilityChanged, [this] (bool /*accessible*/, QString const &) {
        updateMountStatus();
    });

    updateMountStatus();
}

MenuDiskItem::~MenuDiskItem()
{
}

void MenuDiskItem::setMountStatus(bool mounted)
{
    mEjectButton->setEnabled(mounted);
}

void MenuDiskItem::updateMountStatus()
{
    //Note: don't use the QStringLiteral here as it is causing a SEGFAULT in static finalization time
    //(the string is released upon our *.so removal, but the reference is still in held in libqtxdg...)
    static const QIcon icon = XdgIcon::fromTheme(mDevice.icon(), QLatin1String("drive-removable-media"));

    if (mDevice.isValid())
    {
        mDiskButton->setIcon(icon);
        mDiskButton->setText(mDevice.description());

        setMountStatus(mDevice.as<Solid::StorageAccess>()->isAccessible() || !opticalParent().udi().isEmpty());
    }
    else
        emit invalid(mDevice.udi());
}

Solid::Device MenuDiskItem::opticalParent() const
{
    Solid::Device it;
    if (mDevice.isValid())
    {
        it = mDevice;
        // search for parent drive
        for (; !it.udi().isEmpty(); it = it.parent())
            if (it.is<Solid::OpticalDrive>())
                break;
    }
    return it;
}

void MenuDiskItem::diskButtonClicked()
{
    mDiskButtonClicked = true;
    Solid::StorageAccess* di = mDevice.as<Solid::StorageAccess>();
    if (!di->isAccessible())
        di->setup();
    else
        onMounted(Solid::NoError, QString(), mDevice.udi());

    mPopup->hide();
}

void MenuDiskItem::ejectButtonClicked()
{
    mEjectButtonClicked = true;
    Solid::StorageAccess* di = mDevice.as<Solid::StorageAccess>();
    if (di->isAccessible())
        di->teardown();
    else
        onUnmounted(Solid::NoError, QString(), mDevice.udi());

    mPopup->hide();
}

void MenuDiskItem::onMounted(Solid::ErrorType error, QVariant resultData, const QString & /*udi*/)
{
    if (mDiskButtonClicked)
    {
        mDiskButtonClicked = false;

        if (Solid::NoError == error)
            QDesktopServices::openUrl(QUrl(mDevice.as<Solid::StorageAccess>()->filePath()));
        else
        {
            QString errorMsg = tr("Mounting of <b><nobr>\"%1\"</nobr></b> failed: %2");
            errorMsg = errorMsg.arg(mDevice.description()).arg(resultData.toString());
            LXQt::Notification::notify(tr("Removable media/devices manager"), errorMsg, mDevice.icon());
        }
    }
}

void MenuDiskItem::onUnmounted(Solid::ErrorType error, QVariant resultData, const QString & /*udi*/)
{
    if (mEjectButtonClicked)
    {
        mEjectButtonClicked = false;

        if (Solid::NoError == error)
        {
            Solid::Device opt_parent = opticalParent();
            if (!opt_parent.udi().isEmpty())
                opt_parent.as<Solid::OpticalDrive>()->eject();
        }
        else
        {
            QString errorMsg = tr("Unmounting of <strong><nobr>\"%1\"</nobr></strong> failed: %2");
            errorMsg = errorMsg.arg(mDevice.description()).arg(resultData.toString());
            LXQt::Notification::notify(tr("Removable media/devices manager"), errorMsg, mDevice.icon());
        }
    }
}
