/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *  Balázs Béla <balazsbela[at]gmail.com>
 *  Paulo Lieuthier <paulolieuthier@gmail.com>
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
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "statusnotifierbutton.h"

#include <QDir>
#include <QFile>
#include <dbusmenu-qt5/dbusmenuimporter.h>
#include "../panel/ilxqtpanelplugin.h"
#include "sniasync.h"
#include <XdgIcon>

namespace
{
    /*! \brief specialized DBusMenuImporter to correctly create actions' icons based
     * on name
     */
    class MenuImporter : public DBusMenuImporter
    {
    public:
        using DBusMenuImporter::DBusMenuImporter;

    protected:
        virtual QIcon iconForName(const QString & name) override
        {
            return XdgIcon::fromTheme(name);
        }
    };
}

StatusNotifierButton::StatusNotifierButton(QString service, QString objectPath, ILXQtPanelPlugin* plugin, QWidget *parent)
    : QToolButton(parent),
    mMenu(nullptr),
    mStatus(Passive),
    mFallbackIcon(QIcon::fromTheme(QLatin1String("application-x-executable"))),
    mPlugin(plugin),
    mAutoHide(false)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoRaise(true);
    interface = new SniAsync(service, objectPath, QDBusConnection::sessionBus(), this);

    connect(interface, &SniAsync::NewIcon, this, &StatusNotifierButton::newIcon);
    connect(interface, &SniAsync::NewOverlayIcon, this, &StatusNotifierButton::newOverlayIcon);
    connect(interface, &SniAsync::NewAttentionIcon, this, &StatusNotifierButton::newAttentionIcon);
    connect(interface, &SniAsync::NewToolTip, this, &StatusNotifierButton::newToolTip);
    connect(interface, &SniAsync::NewStatus, this, &StatusNotifierButton::newStatus);

    // get the title only at the start because that title is used
    // for deciding about (auto-)hiding
    interface->propertyGetAsync(QLatin1String("Title"), [this] (QString value) {
        mTitle = value;
        QTimer::singleShot(0, this, [this]() { // wait for the c-tor
            Q_EMIT titleFound(mTitle);
        });
    });

    interface->propertyGetAsync(QLatin1String("Menu"), [this] (QDBusObjectPath path) {
        if (!path.path().isEmpty())
        {
            mMenu = (new MenuImporter{interface->service(), path.path(), this})->menu();
            mMenu->setObjectName(QLatin1String("StatusNotifierMenu"));
        }
    });

    interface->propertyGetAsync(QLatin1String("Status"), [this] (QString status) {
        newStatus(status);
    });

    interface->propertyGetAsync(QLatin1String("IconThemePath"), [this] (QString value) {
        //do the logic of icons after we've got the theme path
        refetchIcon(Active, value);
        refetchIcon(Passive, value);
        refetchIcon(NeedsAttention, value);
    });

    newToolTip();

    // The timer that hides an auto-hiding button after it gets attention:
    mHideTimer.setSingleShot(true);
    mHideTimer.setInterval(300000);
    connect(&mHideTimer, &QTimer::timeout, this, [this] {
        hide();
        Q_EMIT attentionChanged();
    });
}

StatusNotifierButton::~StatusNotifierButton()
{
    delete interface;
}

void StatusNotifierButton::newIcon()
{
    if (!icon().isNull() && icon().name() != QLatin1String("application-x-executable"))
        onNeedingAttention();

    interface->propertyGetAsync(QLatin1String("IconThemePath"), [this] (QString value) {
        refetchIcon(Passive, value);
    });
}

void StatusNotifierButton::newOverlayIcon()
{
    onNeedingAttention();

    interface->propertyGetAsync(QLatin1String("IconThemePath"), [this] (QString value) {
        refetchIcon(Active, value);
    });
}

void StatusNotifierButton::newAttentionIcon()
{
    onNeedingAttention();

    interface->propertyGetAsync(QLatin1String("IconThemePath"), [this] (QString value) {
        refetchIcon(NeedsAttention, value);
    });
}

void StatusNotifierButton::refetchIcon(Status status, const QString& themePath)
{
    QString nameProperty, pixmapProperty;
    if (status == Active)
    {
        nameProperty = QLatin1String("OverlayIconName");
        pixmapProperty = QLatin1String("OverlayIconPixmap");
    }
    else if (status == NeedsAttention)
    {
        nameProperty = QLatin1String("AttentionIconName");
        pixmapProperty = QLatin1String("AttentionIconPixmap");
    }
    else // status == Passive
    {
        nameProperty = QLatin1String("IconName");
        pixmapProperty = QLatin1String("IconPixmap");
    }

    interface->propertyGetAsync(nameProperty, [this, status, pixmapProperty, themePath] (QString iconName) {
        QIcon nextIcon;
        if (!iconName.isEmpty())
        {
            if (QIcon::hasThemeIcon(iconName))
                nextIcon = QIcon::fromTheme(iconName);
            else
            {
                QDir themeDir(themePath);
                if (themeDir.exists())
                {
                    if (themeDir.exists(iconName + QStringLiteral(".png")))
                        nextIcon.addFile(themeDir.filePath(iconName + QStringLiteral(".png")));

                    if (themeDir.cd(QStringLiteral("hicolor")) || (themeDir.cd(QStringLiteral("icons")) && themeDir.cd(QStringLiteral("hicolor"))))
                    {
                        const QStringList sizes = themeDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
                        for (const QString &dir : sizes)
                        {
                            const QStringList dirs = QDir(themeDir.filePath(dir)).entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
                            for (const QString &innerDir : dirs)
                            {
                                QString file = themeDir.absolutePath() + QLatin1Char('/') + dir + QLatin1Char('/') + innerDir + QLatin1Char('/') + iconName + QStringLiteral(".png");
                                if (QFile::exists(file))
                                    nextIcon.addFile(file);
                            }
                        }
                    }
                }
            }

            switch (status)
            {
                case Active:
                    mOverlayIcon = nextIcon;
                    break;
                case NeedsAttention:
                    mAttentionIcon = nextIcon;
                    break;
                case Passive:
                    mIcon = nextIcon;
                    break;
            }

            resetIcon();
        }
        else
        {
            interface->propertyGetAsync(pixmapProperty, [this, status, pixmapProperty] (IconPixmapList iconPixmaps) {
                if (iconPixmaps.empty())
                    return;

                QIcon nextIcon;

                for (IconPixmap iconPixmap: iconPixmaps)
                {
                    if (!iconPixmap.bytes.isNull())
                    {
                        QImage image((uchar*) iconPixmap.bytes.data(), iconPixmap.width,
                                     iconPixmap.height, QImage::Format_ARGB32);

                        const uchar *end = image.constBits() + image.sizeInBytes();
                        uchar *dest = reinterpret_cast<uchar*>(iconPixmap.bytes.data());
                        for (const uchar *src = image.constBits(); src < end; src += 4, dest += 4)
                            qToUnaligned(qToBigEndian<quint32>(qFromUnaligned<quint32>(src)), dest);

                        nextIcon.addPixmap(QPixmap::fromImage(image));
                    }
                }

                switch (status)
                {
                    case Active:
                        mOverlayIcon = nextIcon;
                        break;
                    case NeedsAttention:
                        mAttentionIcon = nextIcon;
                        break;
                    case Passive:
                        mIcon = nextIcon;
                        break;
                }

                resetIcon();
            });
        }
    });
}

void StatusNotifierButton::newToolTip()
{
    interface->propertyGetAsync(QLatin1String("ToolTip"), [this] (ToolTip tooltip) {
        QString toolTipTitle = tooltip.title;
        if (!toolTipTitle.isEmpty())
            setToolTip(toolTipTitle);
        else
            interface->propertyGetAsync(QLatin1String("Title"), [this] (QString title) {
                // we should get here only in case the ToolTip.title was empty
                if (!title.isEmpty())
                    setToolTip(title);
            });
    });
}

void StatusNotifierButton::newStatus(QString status)
{
    Status newStatus;
    if (status == QLatin1String("Passive"))
        newStatus = Passive;
    else if (status == QLatin1String("Active"))
        newStatus = Active;
    else
        newStatus = NeedsAttention;

    if (mStatus == newStatus)
        return;

    mStatus = newStatus;
    if (mStatus == NeedsAttention)
        onNeedingAttention();
    resetIcon();
}

void StatusNotifierButton::contextMenuEvent(QContextMenuEvent* /*event*/)
{
    //XXX: avoid showing of parent's context menu, we are (optionally) providing context menu on mouseReleaseEvent
    //QWidget::contextMenuEvent(event);
}

void StatusNotifierButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        interface->Activate(QCursor::pos().x(), QCursor::pos().y());
    else if (event->button() == Qt::MidButton)
        interface->SecondaryActivate(QCursor::pos().x(), QCursor::pos().y());
    else if (Qt::RightButton == event->button())
    {
        if (mMenu)
        {
            mPlugin->willShowWindow(mMenu);
            mMenu->popup(mPlugin->panel()->calculatePopupWindowPos(QCursor::pos(), mMenu->sizeHint()).topLeft());
        } else
            interface->ContextMenu(QCursor::pos().x(), QCursor::pos().y());
    }

    QToolButton::mouseReleaseEvent(event);
}

void StatusNotifierButton::wheelEvent(QWheelEvent *event)
{
    interface->Scroll(event->delta(), QStringLiteral("vertical"));
}

void StatusNotifierButton::resetIcon()
{
    if (mStatus == Active && !mOverlayIcon.isNull())
        setIcon(mOverlayIcon);
    else if (mStatus == NeedsAttention && !mAttentionIcon.isNull())
        setIcon(mAttentionIcon);
    else if (!mIcon.isNull()) // mStatus == Passive
        setIcon(mIcon);
    else if (!mOverlayIcon.isNull())
        setIcon(mOverlayIcon);
    else if (!mAttentionIcon.isNull())
        setIcon(mAttentionIcon);
    else
        setIcon(mFallbackIcon);
}

void StatusNotifierButton::setAutoHide(bool autoHide, int minutes, bool forcedVisible)
{
    if (autoHide)
        mHideTimer.setInterval(qBound(1, minutes, 60) * 60000);
    if (mAutoHide != autoHide)
    {
        mAutoHide = autoHide;
        setVisible(!mAutoHide || forcedVisible);
        if (!mAutoHide)
            mHideTimer.stop();
    }
}

void StatusNotifierButton::onNeedingAttention()
{
    if (mAutoHide)
    {
        show();
        mHideTimer.start();
        Q_EMIT attentionChanged();
    }
}

bool StatusNotifierButton::hasAttention() const
{
    return mHideTimer.isActive();
}
