/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
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

#include "lxqtnetworkmonitor.h"
#include "lxqtnetworkmonitorconfiguration.h"
#include "../panel/ilxqtpanelplugin.h"

#include <QEvent>
#include <QPainter>
#include <QPixmap>
#include <QLinearGradient>
#include <QHBoxLayout>

extern "C" {
#include <statgrab.h>
}

#ifdef __sg_public
// since libstatgrab 0.90 this macro is defined, so we use it for version check
#define STATGRAB_NEWER_THAN_0_90 	1
#endif

LXQtNetworkMonitor::LXQtNetworkMonitor(ILXQtPanelPlugin *plugin, QWidget* parent):
    QFrame(parent),
    mPlugin(plugin)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(&m_stuff);
    setLayout(layout);
    /* Initialise statgrab */
#ifdef STATGRAB_NEWER_THAN_0_90
    sg_init(0);
#else
    sg_init();
#endif

    m_iconList << QStringLiteral("modem") << QStringLiteral("monitor")
               << QStringLiteral("network") << QStringLiteral("wireless");

    startTimer(800);

    settingsChanged();
}

LXQtNetworkMonitor::~LXQtNetworkMonitor()
{
}

void LXQtNetworkMonitor::resizeEvent(QResizeEvent *)
{
    m_stuff.setMinimumWidth(m_pic.width() + 2);
    m_stuff.setMinimumHeight(m_pic.height() + 2);

    update();
}


void LXQtNetworkMonitor::timerEvent(QTimerEvent * /*event*/)
{
    bool matched = false;

#ifdef STATGRAB_NEWER_THAN_0_90
    size_t num_network_stats;
    size_t x;
#else
    int num_network_stats;
    int x;
#endif
    sg_network_io_stats *network_stats = sg_get_network_io_stats_diff(&num_network_stats);

    for (x = 0; x < num_network_stats; x++)
    {
        if (m_interface == QString::fromLocal8Bit(network_stats->interface_name))
        {
            if (network_stats->rx != 0 && network_stats->tx != 0)
            {
                m_pic.load(iconName(QStringLiteral("transmit-receive")));
            }
            else if (network_stats->rx != 0 && network_stats->tx == 0)
            {
                m_pic.load(iconName(QStringLiteral("receive")));
            }
            else if (network_stats->rx == 0 && network_stats->tx != 0)
            {
                m_pic.load(iconName(QStringLiteral("transmit")));
            }
            else
            {
                m_pic.load(iconName(QStringLiteral("idle")));
            }

            matched = true;

            break;
        }

        network_stats++;
    }

    if (!matched)
    {
        m_pic.load(iconName(QStringLiteral("error")));
    }

    update();
}

void LXQtNetworkMonitor::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    QRectF r = rect();

    int leftOffset = (r.width() - m_pic.width() + 2) / 2;
    int topOffset = (r.height() - m_pic.height() + 2) / 2;

    p.drawPixmap(leftOffset, topOffset, m_pic);
}

bool LXQtNetworkMonitor::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip)
    {
#ifdef STATGRAB_NEWER_THAN_0_90
        size_t num_network_stats;
        size_t x;
#else
        int num_network_stats;
        int x;
#endif
        sg_network_io_stats *network_stats = sg_get_network_io_stats(&num_network_stats);

        for (x = 0; x < num_network_stats; x++)
        {
            if (m_interface == QString::fromLocal8Bit(network_stats->interface_name))
            {
                setToolTip(tr("Network interface <b>%1</b>").arg(m_interface) + QStringLiteral("<br>")
                           + tr("Transmitted %1").arg(convertUnits(network_stats->tx)) + QStringLiteral("<br>")
                           + tr("Received %1").arg(convertUnits(network_stats->rx))
                          );
            }
            network_stats++;
        }
    }
    return QFrame::event(event);
}

//void LXQtNetworkMonitor::showConfigureDialog()
//{
//    LXQtNetworkMonitorConfiguration *confWindow =
//        this->findChild<LXQtNetworkMonitorConfiguration*>("LXQtNetworkMonitorConfigurationWindow");

//    if (!confWindow)
//    {
//        confWindow = new LXQtNetworkMonitorConfiguration(settings(), this);
//    }

//    confWindow->show();
//    confWindow->raise();
//    confWindow->activateWindow();
//}

void LXQtNetworkMonitor::settingsChanged()
{
    m_iconIndex = mPlugin->settings()->value(QStringLiteral("icon"), 1).toInt();
    m_interface = mPlugin->settings()->value(QStringLiteral("interface")).toString();
    if (m_interface.isEmpty())
    {
#ifdef STATGRAB_NEWER_THAN_0_90
        size_t count;
#else
        int count;
#endif
        sg_network_iface_stats* stats = sg_get_network_iface_stats(&count);
        if (count > 0)
            m_interface = QString(QLatin1String(stats[0].interface_name));
    }

    m_pic.load(iconName(QStringLiteral("error")));
}

QString LXQtNetworkMonitor::convertUnits(double num)
{
    QString unit = tr("B");
    QStringList units = QStringList(tr("KiB")) << tr("MiB") << tr("GiB") << tr("TiB") << tr("PiB");
    for (QStringListIterator iter(units); num >= 1024 && iter.hasNext();)
    {
        num /= 1024;
        unit = iter.next();
    }
    return QString::number(num, 'f', 2) + QLatin1Char(' ') + unit;
}
