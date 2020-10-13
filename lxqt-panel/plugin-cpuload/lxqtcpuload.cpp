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

#include "lxqtcpuload.h"
#include "../panel/ilxqtpanelplugin.h"
#include "../panel/pluginsettings.h"
#include <QtCore>
#include <QPainter>
#include <QLinearGradient>
#include <QHBoxLayout>

extern "C" {
#include <statgrab.h>
}

#ifdef __sg_public
// since libstatgrab 0.90 this macro is defined, so we use it for version check
#define STATGRAB_NEWER_THAN_0_90 	1
#endif

#define BAR_ORIENT_BOTTOMUP "bottomUp"
#define BAR_ORIENT_TOPDOWN "topDown"
#define BAR_ORIENT_LEFTRIGHT "leftRight"
#define BAR_ORIENT_RIGHTLEFT "rightLeft"


LXQtCpuLoad::LXQtCpuLoad(ILXQtPanelPlugin* plugin, QWidget* parent):
    QFrame(parent),
    mPlugin(plugin),
    m_avg(0),
    m_showText(false),
    m_barWidth(20),
    m_barOrientation(TopDownBar),
    m_timerID(-1)
{
    setObjectName(QStringLiteral("LXQtCpuLoad"));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(&m_stuff);

    /* Initialise statgrab */
#ifdef STATGRAB_NEWER_THAN_0_90
    sg_init(0);
#else
    sg_init();
#endif

    /* Drop setuid/setgid privileges. */
    if (sg_drop_privileges() != 0) {
        perror("Error. Failed to drop privileges");
    }

    m_font.setPointSizeF(8);

    settingsChanged();
}

LXQtCpuLoad::~LXQtCpuLoad()
{
  sg_shutdown();
}

void LXQtCpuLoad::setSizes()
{
    if (m_barOrientation == RightToLeftBar || m_barOrientation == LeftToRightBar)
    {
        m_stuff.setFixedHeight(m_barWidth);
        m_stuff.setMinimumWidth(24);
    }
    else
    {
        m_stuff.setFixedWidth(m_barWidth);
        m_stuff.setMinimumHeight(24);
    }
}

void LXQtCpuLoad::resizeEvent(QResizeEvent *)
{
    setSizes();
    update();
}


double LXQtCpuLoad::getLoadCpu() const
{
#ifdef STATGRAB_NEWER_THAN_0_90
    size_t count;
    sg_cpu_percents* cur = sg_get_cpu_percents(&count);
#else
    sg_cpu_percents* cur = sg_get_cpu_percents();
#endif
    return (cur->user + cur->kernel + cur->nice);
}

void LXQtCpuLoad::timerEvent(QTimerEvent * /*event*/)
{
    double avg = getLoadCpu();
    if ( qAbs(m_avg-avg)>1 )
    {
        m_avg = avg;
        setToolTip(tr("CPU load %1%").arg(m_avg));
        update();
    }
}

void LXQtCpuLoad::paintEvent ( QPaintEvent * )
{
    QPainter p(this);
    QPen pen;
    pen.setWidth(2);
    p.setPen(pen);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.setFont(m_font);
    QRectF r = rect();

    QRectF r1;
    QLinearGradient shade(0,0,1,1);
    if (m_barOrientation == RightToLeftBar || m_barOrientation == LeftToRightBar)
    {
        float vo = (r.height() - static_cast<double>(m_barWidth))/2.0;
        float ho = r.width()*(1-m_avg*0.01);

        if (m_barOrientation == RightToLeftBar)
        {
            r1.setRect(r.left()+ho, r.top()+vo, r.width()-ho, r.height()-2*vo );
        }
        else // LeftToRightBar
        {
            r1.setRect(r.left(), r.top()+vo, r.width()-ho, r.height()-2*vo );
        }
        shade.setFinalStop(0, r1.height());
    }
    else // BottomUpBar || TopDownBar
    {
        float vo = r.height()*(1-m_avg*0.01);
        float ho = (r.width() - static_cast<double>(m_barWidth) )/2.0;

        if (m_barOrientation == TopDownBar)
        {
            r1.setRect(r.left()+ho, r.top(), r.width()-2*ho, r.height()-vo );
        }
        else // BottomUpBar
        {
            r1.setRect(r.left()+ho, r.top()+vo, r.width()-2*ho, r.height()-vo );
        }
        shade.setFinalStop(r1.width(), 0);
    }

    shade.setSpread(QLinearGradient::ReflectSpread);
    shade.setColorAt(0, QColor(0, 196, 0, 128));
    shade.setColorAt(0.5, QColor(0, 128, 0, 255) );
    shade.setColorAt(1, QColor(0, 196, 0 , 128));

    p.fillRect(r1, shade);

    if (m_showText)
    {
        p.setPen(fontColor);
        p.drawText(rect(), Qt::AlignCenter, QString::number(m_avg));
    }
}


void LXQtCpuLoad::settingsChanged()
{
    if (m_timerID != -1)
        killTimer(m_timerID);

    m_showText = mPlugin->settings()->value(QStringLiteral("showText"), false).toBool();
    m_barWidth = mPlugin->settings()->value(QStringLiteral("barWidth"), 20).toInt();
    m_updateInterval = mPlugin->settings()->value(QStringLiteral("updateInterval"), 1000).toInt();

    QString barOrientation = mPlugin->settings()->value(QStringLiteral("barOrientation"), QStringLiteral(BAR_ORIENT_BOTTOMUP)).toString();
    if (barOrientation == QLatin1String(BAR_ORIENT_RIGHTLEFT))
        m_barOrientation = RightToLeftBar;
    else if (barOrientation == QLatin1String(BAR_ORIENT_LEFTRIGHT))
        m_barOrientation = LeftToRightBar;
    else if (barOrientation == QLatin1String(BAR_ORIENT_TOPDOWN))
        m_barOrientation = TopDownBar;
    else
        m_barOrientation = BottomUpBar;

    m_timerID = startTimer(m_updateInterval);
    setSizes();
    update();
}
