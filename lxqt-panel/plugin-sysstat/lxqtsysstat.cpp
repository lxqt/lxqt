/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#include "lxqtsysstat.h"
#include "lxqtsysstatutils.h"

#include <SysStat/CpuStat>
#include <SysStat/MemStat>
#include <SysStat/NetStat>

#include <QTimer>
#include <qmath.h>
#include <QPainter>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QCoreApplication>

LXQtSysStat::LXQtSysStat(const ILXQtPanelPluginStartupInfo &startupInfo):
    QObject(),
    ILXQtPanelPlugin(startupInfo),
    mWidget(new QWidget()),
    mFakeTitle(new LXQtSysStatTitle(mWidget)),
    mContent(new LXQtSysStatContent(this, mWidget))
{
    QVBoxLayout *borderLayout = new QVBoxLayout(mWidget);
    borderLayout->setContentsMargins(0, 0, 0, 0);
    borderLayout->setSpacing(0);
    borderLayout->addWidget(mContent);
    borderLayout->setStretchFactor(mContent, 1);

    mContent->setMinimumSize(2, 2);

    // qproperty of font type doesn't work with qss, so fake QLabel is used instead
    connect(mFakeTitle, SIGNAL(fontChanged(QFont)), mContent, SLOT(setTitleFont(QFont)));

    // has to be postponed to update the size first
    QTimer::singleShot(0, this, SLOT(lateInit()));
}

LXQtSysStat::~LXQtSysStat()
{
    delete mWidget;
}

void LXQtSysStat::lateInit()
{
    settingsChanged();
    mContent->setTitleFont(mFakeTitle->font());
    mSize = mContent->size();
}

QDialog *LXQtSysStat::configureDialog()
{
    return new LXQtSysStatConfiguration(settings(), mWidget);
}

void LXQtSysStat::realign()
{
    QSize newSize = mContent->size();
    if (mSize != newSize)
    {
        mContent->reset();
        mSize = newSize;
    }
}

void LXQtSysStat::settingsChanged()
{
    mContent->updateSettings(settings());
}

LXQtSysStatTitle::LXQtSysStatTitle(QWidget *parent):
    QLabel(parent)
{

}

LXQtSysStatTitle::~LXQtSysStatTitle()
{

}

bool LXQtSysStatTitle::event(QEvent *e)
{
    if (e->type() == QEvent::FontChange)
        emit fontChanged(font());

    return QLabel::event(e);
}

LXQtSysStatContent::LXQtSysStatContent(ILXQtPanelPlugin *plugin, QWidget *parent):
    QWidget(parent),
    mPlugin(plugin),
    mStat(nullptr),
    mUpdateInterval(0),
    mMinimalSize(0),
    mTitleFontPixelHeight(0),
    mUseThemeColours(true),
    mHistoryOffset(0)
{
    setObjectName(QStringLiteral("SysStat_Graph"));
}

LXQtSysStatContent::~LXQtSysStatContent()
{
}


// I don't like macros very much, but writing dozen similar functions is much much worse.

#undef QSS_GET_COLOUR
#define QSS_GET_COLOUR(GETNAME) \
QColor LXQtSysStatContent::GETNAME##Colour() const \
{ \
    return mThemeColours.GETNAME##Colour; \
}

#undef QSS_COLOUR
#define QSS_COLOUR(GETNAME, SETNAME) \
QSS_GET_COLOUR(GETNAME) \
void LXQtSysStatContent::SETNAME##Colour(QColor value) \
{ \
    mThemeColours.GETNAME##Colour = value; \
    if (mUseThemeColours) \
        mColours.GETNAME##Colour = mThemeColours.GETNAME##Colour; \
}

#undef QSS_NET_COLOUR
#define QSS_NET_COLOUR(GETNAME, SETNAME) \
QSS_GET_COLOUR(GETNAME) \
void LXQtSysStatContent::SETNAME##Colour(QColor value) \
{ \
    mThemeColours.GETNAME##Colour = value; \
    if (mUseThemeColours) \
    { \
        mColours.GETNAME##Colour = mThemeColours.GETNAME##Colour; \
        mixNetColours(); \
    } \
}

QSS_COLOUR(grid,      setGrid)
QSS_COLOUR(title,     setTitle)
QSS_COLOUR(cpuSystem, setCpuSystem)
QSS_COLOUR(cpuUser,   setCpuUser)
QSS_COLOUR(cpuNice,   setCpuNice)
QSS_COLOUR(cpuOther,  setCpuOther)
QSS_COLOUR(frequency, setFrequency)
QSS_COLOUR(memApps,   setMemApps)
QSS_COLOUR(memBuffers,setMemBuffers)
QSS_COLOUR(memCached, setMemCached)
QSS_COLOUR(swapUsed,  setSwapUsed)

QSS_NET_COLOUR(netReceived,    setNetReceived)
QSS_NET_COLOUR(netTransmitted, setNetTransmitted)

#undef QSS_NET_COLOUR
#undef QSS_COLOUR
#undef QSS_GET_COLOUR

void LXQtSysStatContent::mixNetColours()
{
    QColor netReceivedColour_hsv = mColours.netReceivedColour.toHsv();
    QColor netTransmittedColour_hsv = mColours.netTransmittedColour.toHsv();
    qreal hue = (netReceivedColour_hsv.hueF() + netTransmittedColour_hsv.hueF()) / 2;
    if (qAbs(netReceivedColour_hsv.hueF() - netTransmittedColour_hsv.hueF()) > 0.5)
        hue += 0.5;
    mNetBothColour.setHsvF(
        hue,
        (netReceivedColour_hsv.saturationF() + netTransmittedColour_hsv.saturationF()) / 2,
        (netReceivedColour_hsv.valueF()      + netTransmittedColour_hsv.valueF()     ) / 2 );
}

void LXQtSysStatContent::setTitleFont(QFont value)
{
    mTitleFont = value;
    updateTitleFontPixelHeight();

    update();
}

void LXQtSysStatContent::updateTitleFontPixelHeight()
{
    if (mTitleLabel.isEmpty())
        mTitleFontPixelHeight = 0;
    else
    {
        QFontMetrics fm(mTitleFont);
        mTitleFontPixelHeight = fm.height() - 1;
    }
}

void LXQtSysStatContent::updateSettings(const PluginSettings *settings)
{
    double old_updateInterval = mUpdateInterval;
    int old_minimalSize = mMinimalSize;
    QString old_dataType = mDataType;
    QString old_dataSource = mDataSource;
    bool old_useFrequency = mUseFrequency;
    bool old_logarithmicScale = mLogarithmicScale;
    int old_logScaleSteps = mLogScaleSteps;

    mUseThemeColours = settings->value(QStringLiteral("graph/useThemeColours"), true).toBool();
    mUpdateInterval = settings->value(QStringLiteral("graph/updateInterval"), 1.0).toDouble();
    mMinimalSize = settings->value(QStringLiteral("graph/minimalSize"), 30).toInt();

    mGridLines = settings->value(QStringLiteral("grid/lines"), 1).toInt();

    mTitleLabel = settings->value(QStringLiteral("title/label"), QString()).toString();

    // default to CPU monitoring
    mDataType = settings->value(QStringLiteral("data/type"), LXQtSysStatConfiguration::msStatTypes[0]).toString();

    mDataSource = settings->value(QStringLiteral("data/source"), QStringLiteral("cpu")).toString();

    mUseFrequency = settings->value(QStringLiteral("cpu/useFrequency"), true).toBool();

    mNetMaximumSpeed = PluginSysStat::netSpeedFromString(settings->value(QStringLiteral("net/maximumSpeed"), QStringLiteral("1 MB/s")).toString());
    mLogarithmicScale = settings->value(QStringLiteral("net/logarithmicScale"), true).toBool();

    mLogScaleSteps = settings->value(QStringLiteral("net/logarithmicScaleSteps"), 4).toInt();
    mLogScaleMax = static_cast<qreal>(static_cast<int64_t>(1) << mLogScaleSteps);

    mNetRealMaximumSpeed = static_cast<qreal>(static_cast<int64_t>(1) << mNetMaximumSpeed);


    mSettingsColours.gridColour = QColor(settings->value(QStringLiteral("grid/colour"), QStringLiteral("#c0c0c0")).toString());

    mSettingsColours.titleColour = QColor(settings->value(QStringLiteral("title/colour"), QStringLiteral("#ffffff")).toString());

    mSettingsColours.cpuSystemColour = QColor(settings->value(QStringLiteral("cpu/systemColour"),    QStringLiteral("#800000")).toString());
    mSettingsColours.cpuUserColour   = QColor(settings->value(QStringLiteral("cpu/userColour"),      QStringLiteral("#000080")).toString());
    mSettingsColours.cpuNiceColour   = QColor(settings->value(QStringLiteral("cpu/niceColour"),      QStringLiteral("#008000")).toString());
    mSettingsColours.cpuOtherColour  = QColor(settings->value(QStringLiteral("cpu/otherColour"),     QStringLiteral("#808000")).toString());
    mSettingsColours.frequencyColour = QColor(settings->value(QStringLiteral("cpu/frequencyColour"), QStringLiteral("#808080")).toString());

    mSettingsColours.memAppsColour    = QColor(settings->value(QStringLiteral("mem/appsColour"),    QStringLiteral("#000080")).toString());
    mSettingsColours.memBuffersColour = QColor(settings->value(QStringLiteral("mem/buffersColour"), QStringLiteral("#008000")).toString());
    mSettingsColours.memCachedColour  = QColor(settings->value(QStringLiteral("mem/cachedColour"),  QStringLiteral("#808000")).toString());
    mSettingsColours.swapUsedColour   = QColor(settings->value(QStringLiteral("mem/swapColour"),    QStringLiteral("#800000")).toString());

    mSettingsColours.netReceivedColour    = QColor(settings->value(QStringLiteral("net/receivedColour"),    QStringLiteral("#000080")).toString());
    mSettingsColours.netTransmittedColour = QColor(settings->value(QStringLiteral("net/transmittedColour"), QStringLiteral("#808000")).toString());


    if (mUseThemeColours)
        mColours = mThemeColours;
    else
        mColours = mSettingsColours;

    mixNetColours();

    updateTitleFontPixelHeight();


    bool minimalSizeChanged      = old_minimalSize      != mMinimalSize;
    bool updateIntervalChanged   = old_updateInterval   != mUpdateInterval;
    bool dataTypeChanged         = old_dataType         != mDataType;
    bool dataSourceChanged       = old_dataSource       != mDataSource;
    bool useFrequencyChanged     = old_useFrequency     != mUseFrequency;
    bool logScaleStepsChanged    = old_logScaleSteps    != mLogScaleSteps;
    bool logarithmicScaleChanged = old_logarithmicScale != mLogarithmicScale;

    bool needReconnecting    = dataTypeChanged || dataSourceChanged || useFrequencyChanged;
    bool needTimerRestarting = needReconnecting || updateIntervalChanged;
    bool needFullReset       = needTimerRestarting || minimalSizeChanged || logScaleStepsChanged || logarithmicScaleChanged;


    if (mStat)
    {
        if (needTimerRestarting)
            mStat->stopUpdating();

        if (needReconnecting)
            mStat->disconnect(this);
    }

    if (dataTypeChanged)
    {
        if (mStat)
        {
            mStat->deleteLater();
            mStat = nullptr;
        }

        if (mDataType == QLatin1String("CPU"))
            mStat = new SysStat::CpuStat(this);
        else if (mDataType == QLatin1String("Memory"))
            mStat = new SysStat::MemStat(this);
        else if (mDataType == QLatin1String("Network"))
            mStat = new SysStat::NetStat(this);
    }

    if (mStat)
    {
        if (needReconnecting)
        {
            if (mDataType == QLatin1String("CPU"))
            {
                if (mUseFrequency)
                {
                    qobject_cast<SysStat::CpuStat*>(mStat)->setMonitoring(SysStat::CpuStat::LoadAndFrequency);
                    connect(qobject_cast<SysStat::CpuStat*>(mStat), SIGNAL(update(float, float, float, float, float, uint)), this, SLOT(cpuUpdate(float, float, float, float, float, uint)));
                }
                else
                {
                    qobject_cast<SysStat::CpuStat*>(mStat)->setMonitoring(SysStat::CpuStat::LoadOnly);
                    connect(qobject_cast<SysStat::CpuStat*>(mStat), SIGNAL(update(float, float, float, float)), this, SLOT(cpuUpdate(float, float, float, float)));
                }
            }
            else if (mDataType == QLatin1String("Memory"))
            {
                if (mDataSource == QLatin1String("memory"))
                    connect(qobject_cast<SysStat::MemStat*>(mStat), SIGNAL(memoryUpdate(float, float, float)), this, SLOT(memoryUpdate(float, float, float)));
                else
                    connect(qobject_cast<SysStat::MemStat*>(mStat), SIGNAL(swapUpdate(float)), this, SLOT(swapUpdate(float)));
            }
            else if (mDataType == QLatin1String("Network"))
            {
                connect(qobject_cast<SysStat::NetStat*>(mStat), SIGNAL(update(unsigned, unsigned)), this, SLOT(networkUpdate(unsigned, unsigned)));
            }

            mStat->setMonitoredSource(mDataSource);
        }

        if (needTimerRestarting)
            mStat->setUpdateInterval(static_cast<int>(mUpdateInterval * 1000.0));
    }

    if (needFullReset)
        reset();
    else
        update();
}

void LXQtSysStatContent::resizeEvent(QResizeEvent * /*event*/)
{
    reset();
}

void LXQtSysStatContent::reset()
{
    setMinimumSize(mPlugin->panel()->isHorizontal() ? mMinimalSize : 2,
                   mPlugin->panel()->isHorizontal() ? 2 : mMinimalSize);

    mHistoryOffset = 0;
    mHistoryImage = QImage(width(), 100, QImage::Format_ARGB32);
    mHistoryImage.fill(Qt::transparent);
    update();
}

template <typename T>
T clamp(const T &value, const T &min, const T &max)
{
    return qMin(qMax(value, min), max);
}

// QPainter.drawLine with pen set to Qt::transparent doesn't clear anything
void LXQtSysStatContent::clearLine()
{
    QRgb bg = QColor(Qt::transparent).rgba();
    for (int i = 0; i < 100; ++i)
        reinterpret_cast<QRgb*>(mHistoryImage.scanLine(i))[mHistoryOffset] = bg;
}

void LXQtSysStatContent::cpuUpdate(float user, float nice, float system, float other, float frequencyRate, uint)
{
    int y_system = static_cast<int>(system * 100.0 * frequencyRate);
    int y_user   = static_cast<int>(user   * 100.0 * frequencyRate);
    int y_nice   = static_cast<int>(nice   * 100.0 * frequencyRate);
    int y_other  = static_cast<int>(other  * 100.0 * frequencyRate);
    int y_freq   = static_cast<int>(         100.0 * frequencyRate);

    toolTipInfo(tr("system: %1%<br>user: %2%<br>nice: %3%<br>other: %4%<br>freq: %5%", "CPU tooltip information")
            .arg(y_system).arg(y_user).arg(y_nice).arg(y_other).arg(y_freq));

    y_system = clamp(y_system, 0, 99);
    y_user   = clamp(y_user + y_system, 0, 99);
    y_nice   = clamp(y_nice + y_user  , 0, 99);
    y_other  = clamp(y_other, 0, 99);
    y_freq   = clamp(y_freq, 0, 99);

    clearLine();
    QPainter painter(&mHistoryImage);
    if (y_system != 0)
    {
        painter.setPen(mColours.cpuSystemColour);
        painter.drawLine(mHistoryOffset, y_system, mHistoryOffset, 0);
    }
    if (y_user != y_system)
    {
        painter.setPen(mColours.cpuUserColour);
        painter.drawLine(mHistoryOffset, y_user, mHistoryOffset, y_system);
    }
    if (y_nice != y_user)
    {
        painter.setPen(mColours.cpuNiceColour);
        painter.drawLine(mHistoryOffset, y_nice, mHistoryOffset, y_user);
    }
    if (y_other != y_nice)
    {
        painter.setPen(mColours.cpuOtherColour);
        painter.drawLine(mHistoryOffset, y_other, mHistoryOffset, y_nice);
    }
    if (y_freq != y_other)
    {
        painter.setPen(mColours.frequencyColour);
        painter.drawLine(mHistoryOffset, y_freq, mHistoryOffset, y_other);
    }

    mHistoryOffset = (mHistoryOffset + 1) % width();

    update(0, mTitleFontPixelHeight, width(), height() - mTitleFontPixelHeight);
}

void LXQtSysStatContent::cpuUpdate(float user, float nice, float system, float other)
{
    int y_system = static_cast<int>(system * 100.0);
    int y_user   = static_cast<int>(user   * 100.0);
    int y_nice   = static_cast<int>(nice   * 100.0);
    int y_other  = static_cast<int>(other  * 100.0);

    toolTipInfo(tr("system: %1%<br>user: %2%<br>nice: %3%<br>other: %4%<br>freq: n/a", "CPU tooltip information")
            .arg(y_system).arg(y_user).arg(y_nice).arg(y_other));

    y_system = clamp(y_system, 0, 99);
    y_user   = clamp(y_user + y_system, 0, 99);
    y_nice   = clamp(y_nice + y_user, 0, 99);
    y_other  = clamp(y_other + y_nice, 0, 99);

    clearLine();
    QPainter painter(&mHistoryImage);
    if (y_system != 0)
    {
        painter.setPen(mColours.cpuSystemColour);
        painter.drawLine(mHistoryOffset, y_system, mHistoryOffset, 0);
    }
    if (y_user != y_system)
    {
        painter.setPen(mColours.cpuUserColour);
        painter.drawLine(mHistoryOffset, y_user, mHistoryOffset, y_system);
    }
    if (y_nice != y_user)
    {
        painter.setPen(mColours.cpuNiceColour);
        painter.drawLine(mHistoryOffset, y_nice, mHistoryOffset, y_user);
    }
    if (y_other != y_nice)
    {
        painter.setPen(mColours.cpuOtherColour);
        painter.drawLine(mHistoryOffset, y_other, mHistoryOffset, y_nice);
    }

    mHistoryOffset = (mHistoryOffset + 1) % width();

    update(0, mTitleFontPixelHeight, width(), height() - mTitleFontPixelHeight);
}

void LXQtSysStatContent::memoryUpdate(float apps, float buffers, float cached)
{
    int y_apps    = static_cast<int>(apps    * 100.0);
    int y_buffers = static_cast<int>(buffers * 100.0);
    int y_cached  = static_cast<int>(cached  * 100.0);

    toolTipInfo(tr("apps: %1%<br>buffers: %2%<br>cached: %3%", "Memory tooltip information")
        .arg(y_apps).arg(y_buffers).arg(y_cached));

    y_apps    = clamp(y_apps, 0, 99);
    y_buffers = clamp(y_buffers + y_apps, 0, 99);
    y_cached  = clamp(y_cached + y_buffers, 0, 99);

    clearLine();
    QPainter painter(&mHistoryImage);
    if (y_apps != 0)
    {
        painter.setPen(mColours.memAppsColour);
        painter.drawLine(mHistoryOffset, y_apps, mHistoryOffset, 0);
    }
    if (y_buffers != y_apps)
    {
        painter.setPen(mColours.memBuffersColour);
        painter.drawLine(mHistoryOffset, y_buffers, mHistoryOffset, y_apps);
    }
    if (y_cached != y_buffers)
    {
        painter.setPen(mColours.memCachedColour);
        painter.drawLine(mHistoryOffset, y_cached, mHistoryOffset, y_buffers);
    }

    mHistoryOffset = (mHistoryOffset + 1) % width();

    update(0, mTitleFontPixelHeight, width(), height() - mTitleFontPixelHeight);
}

void LXQtSysStatContent::swapUpdate(float used)
{
    int y_used = static_cast<int>(used * 100.0);

    toolTipInfo(tr("used: %1%", "Swap tooltip information").arg(y_used));

    y_used = clamp(y_used, 0, 99);

    clearLine();
    QPainter painter(&mHistoryImage);
    if (y_used != 0)
    {
        painter.setPen(mColours.swapUsedColour);
        painter.drawLine(mHistoryOffset, y_used, mHistoryOffset, 0);
    }

    mHistoryOffset = (mHistoryOffset + 1) % width();

    update(0, mTitleFontPixelHeight, width(), height() - mTitleFontPixelHeight);
}

void LXQtSysStatContent::networkUpdate(unsigned received, unsigned transmitted)
{
    qreal min_value = qMin(qMax(static_cast<qreal>(qMin(received, transmitted)) / mNetRealMaximumSpeed, static_cast<qreal>(0.0)), static_cast<qreal>(1.0));
    qreal max_value = qMin(qMax(static_cast<qreal>(qMax(received, transmitted)) / mNetRealMaximumSpeed, static_cast<qreal>(0.0)), static_cast<qreal>(1.0));
    if (mLogarithmicScale)
    {
        min_value = qLn(min_value * (mLogScaleMax - 1.0) + 1.0) / qLn(2.0) / static_cast<qreal>(mLogScaleSteps);
        max_value = qLn(max_value * (mLogScaleMax - 1.0) + 1.0) / qLn(2.0) / static_cast<qreal>(mLogScaleSteps);
    }

    int y_min_value = static_cast<int>(min_value * 100.0);
    int y_max_value = static_cast<int>(max_value * 100.0);

    toolTipInfo(tr("min: %1%<br>max: %2%", "Network tooltip information").arg(y_min_value).arg(y_max_value));

    y_min_value = clamp(y_min_value, 0, 99);
    y_max_value = clamp(y_max_value + y_min_value, 0, 99);

    clearLine();
    QPainter painter(&mHistoryImage);
    if (y_min_value != 0)
    {
        painter.setPen(mNetBothColour);
        painter.drawLine(mHistoryOffset, y_min_value, mHistoryOffset, 0);
    }
    if (y_max_value != y_min_value)
    {
        painter.setPen((received > transmitted) ? mColours.netReceivedColour : mColours.netTransmittedColour);
        painter.drawLine(mHistoryOffset, y_max_value, mHistoryOffset, y_min_value);
    }

    mHistoryOffset = (mHistoryOffset + 1) % width();

    update(0, mTitleFontPixelHeight, width(), height() - mTitleFontPixelHeight);
}

void LXQtSysStatContent::paintEvent(QPaintEvent *event)
{
    QPainter p(this);

    qreal graphTop = 0;
    qreal graphHeight = height();

    bool hasTitle = !mTitleLabel.isEmpty();

    if (hasTitle)
    {
        graphTop = mTitleFontPixelHeight;
        graphHeight -= graphTop;

        if (event->region().intersects(QRect(0, 0, width(), graphTop)))
        {
            p.setPen(mColours.titleColour);
            p.setFont(mTitleFont);
            p.drawText(QRectF(0, 0, width(), graphTop), Qt::AlignHCenter | Qt::AlignVCenter, mTitleLabel);
        }
    }

    if (graphHeight < 1)
        graphHeight = 1;

    p.scale(1.0, -1.0);

    p.drawImage(QRect(0, -height(), width() - mHistoryOffset, graphHeight), mHistoryImage, QRect(mHistoryOffset, 0, width() - mHistoryOffset, 100));
    if (mHistoryOffset)
        p.drawImage(QRect(width() - mHistoryOffset, -height(), mHistoryOffset, graphHeight), mHistoryImage, QRect(0, 0, mHistoryOffset, 100));

    p.resetTransform();

    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(mColours.gridColour);
    qreal w = static_cast<qreal>(width());
    if (hasTitle)
        p.drawLine(QPointF(0.0, graphTop + 0.5), QPointF(w, graphTop + 0.5)); // 0.5 looks better with antialiasing
    for (int l = 0; l < mGridLines; ++l)
    {
        qreal y = graphTop + static_cast<qreal>(l + 1) * graphHeight / (static_cast<qreal>(mGridLines + 1));
        p.drawLine(QPointF(0.0, y), QPointF(w, y));
    }
}

void LXQtSysStatContent::toolTipInfo(QString const & tooltip)
{
    setToolTip(QStringLiteral("<b>%1(%2)</b><br>%3")
            .arg(QCoreApplication::translate("LXQtSysStatConfiguration", mDataType.toStdString().c_str()))
            .arg(QCoreApplication::translate("LXQtSysStatConfiguration", mDataSource.toStdString().c_str()))
            .arg(tooltip));
}
