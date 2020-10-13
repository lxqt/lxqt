/*
* Copyright (c) 2014, 2015 Christian Surlykke, Paulo Lieuthier
*
* This file is part of the LXQt project. <https://lxqt.org>
* It is distributed under the LGPL 2.1 or later license.
* Please refer to the LICENSE file for a copy of the license.
*/
#include "iconproducer.h"

#include <LXQt/Globals>
#include <LXQt/Settings>
#include <XdgIcon>
#include <QDebug>
#include <QtSvg/QSvgRenderer>
#include <QPainter>
#include <cmath>

IconProducer::IconProducer(Solid::Battery *battery, QObject *parent) : QObject(parent)
{
    connect(battery, &Solid::Battery::chargeStateChanged, this, &IconProducer::updateState);
    connect(battery, &Solid::Battery::chargePercentChanged, this, &IconProducer::updateChargePercent);
    connect(&mSettings, SIGNAL(settingsChanged()), this, SLOT(update()));

    mChargePercent = battery->chargePercent();
    mState = battery->chargeState();
    themeChanged();
}

IconProducer::IconProducer(QObject *parent):  QObject(parent)
{
    themeChanged();
    update();
}


void IconProducer::updateChargePercent(int newChargePercent)
{
    mChargePercent = newChargePercent;

    update();
}

void IconProducer::updateState(int newState)
{
    mState = (Solid::Battery::ChargeState) newState;

    update();
}

void IconProducer::update()
{
    QString newIconName;

    if (mSettings.isUseThemeIcons())
    {
        QMap<float, QString> *levelNameMap = (mState == Solid::Battery::Discharging ? &mLevelNameMapDischarging : &mLevelNameMapCharging);
        const auto levels = levelNameMap->keys();
        for (const float level : levels)
        {
            if (level >= mChargePercent)
            {
                newIconName = levelNameMap->value(level);
                break;
            }
        }
    }

    if (mSettings.isUseThemeIcons() && newIconName == mIconName)
        return;

    mIconName = newIconName;
    mIcon = mSettings.isUseThemeIcons() ? QIcon::fromTheme(mIconName) : circleIcon();
    emit iconChanged();
}

void IconProducer::themeChanged()
{
    /*
     * We maintain specific charge-level-to-icon-name mappings for Oxygen and Awoken and
     * asume all other themes adhere to the freedesktop standard.
     * This is certainly not true, and as bug reports come in stating that
     * this and that theme is not working we will probably have to add new
     * mappings beside Oxygen and Awoken
     */
    mLevelNameMapDischarging.clear();
    mLevelNameMapCharging.clear();

    if (QIcon::themeName() == QL1S("oxygen"))
    {
        // Means:
        // Use 'battery-low' for levels up to 10
        //  -  'battery-caution' for levels between 10 and 20
        //  -  'battery-040' for levels between 20 and 40, etc..
        mLevelNameMapDischarging[10] = QL1S("battery-low");
        mLevelNameMapDischarging[20] = QL1S("battery-caution");
        mLevelNameMapDischarging[40] = QL1S("battery-040");
        mLevelNameMapDischarging[60] = QL1S("battery-060");
        mLevelNameMapDischarging[80] = QL1S("battery-080");
        mLevelNameMapDischarging[101] = QL1S("battery-100");
        mLevelNameMapCharging[10] = QL1S("battery-charging-low");
        mLevelNameMapCharging[20] = QL1S("battery-charging-caution");
        mLevelNameMapCharging[40] = QL1S("battery-charging-040");
        mLevelNameMapCharging[60] = QL1S("battery-charging-060");
        mLevelNameMapCharging[80] = QL1S("battery-charging-080");
        mLevelNameMapCharging[101] = QL1S("battery-charging");
    }
    else if (QIcon::themeName().startsWith(QL1S("AwOken"))) // AwOken, AwOkenWhite, AwOkenDark
    {
        mLevelNameMapDischarging[5] = QL1S("battery-000");
        mLevelNameMapDischarging[30] = QL1S("battery-020");
        mLevelNameMapDischarging[50] = QL1S("battery-040");
        mLevelNameMapDischarging[70] = QL1S("battery-060");
        mLevelNameMapDischarging[95] = QL1S("battery-080");
        mLevelNameMapDischarging[101] = QL1S("battery-100");
        mLevelNameMapCharging[5] = QL1S("battery-000-charging");
        mLevelNameMapCharging[30] = QL1S("battery-020-charging");
        mLevelNameMapCharging[50] = QL1S("battery-040-charging");
        mLevelNameMapCharging[70] = QL1S("battery-060-charging");
        mLevelNameMapCharging[95] = QL1S("battery-080-charging");
        mLevelNameMapCharging[101] = QL1S("battery-100-charging");
    }
    else // As default we fall back to the freedesktop scheme.
    {
        mLevelNameMapDischarging[3] = QL1S("battery-empty");
        mLevelNameMapDischarging[10] = QL1S("battery-caution");
        mLevelNameMapDischarging[50] = QL1S("battery-low");
        mLevelNameMapDischarging[90] = QL1S("battery-good");
        mLevelNameMapDischarging[101] = QL1S("battery-full");
        mLevelNameMapCharging[3] = QL1S("battery-empty");
        mLevelNameMapCharging[10] = QL1S("battery-caution-charging");
        mLevelNameMapCharging[50] = QL1S("battery-low-charging");
        mLevelNameMapCharging[90] = QL1S("battery-good-charging");
        mLevelNameMapCharging[101] = QL1S("battery-full-charging");
    }

    update();
}

QIcon& IconProducer::circleIcon()
{
    static QMap<Solid::Battery::ChargeState, QMap<int, QIcon> > cache;

    int chargeLevelAsInt = (int) (mChargePercent + 0.49);

    if (!cache[mState].contains(chargeLevelAsInt))
        cache[mState][chargeLevelAsInt] = buildCircleIcon(mState, mChargePercent);

    return cache[mState][chargeLevelAsInt];
}

QIcon IconProducer::buildCircleIcon(Solid::Battery::ChargeState state, int chargeLevel)
{
    static QString svg_template = QL1S(
        "<svg\n"
        "    xmlns:dc='http://purl.org/dc/elements/1.1/'\n"
        "    xmlns:cc='http://creativecommons.org/ns#'\n"
        "    xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'\n"
        "    xmlns:svg='http://www.w3.org/2000/svg'\n"
        "    xmlns='http://www.w3.org/2000/svg'\n"
        "    version='1.1'\n"
        "    width='200'\n"
        "    height='200'>\n"
        "\n"
        "<defs>\n"
        "    <linearGradient id='greenGradient' x1='0%' y1='0%' x2='100%' y2='100%'>\n"
        "        <stop offset='0%' style='stop-color:rgb(125,255,125);stop-opacity:1' />\n"
        "        <stop offset='150%' style='stop-color:rgb(15,125,15);stop-opacity:1' />\n"
        "    </linearGradient>\n"
        "</defs>\n"
        "\n"
        "<circle cx='100' cy='100' r='99' style='fill:#FFFFFF;stroke:none; opacity:0.2;'/>\n"
        "<path d='M 100,20 A80,80 0, LARGE_ARC_FLAG, SWEEP_FLAG, END_X,END_Y' style='fill:none; stroke:url(#greenGradient); stroke-width:38;' />\n"
        "<path d='M 100,20 A80,80 0, LARGE_ARC_FLAG, SWEEP_FLAG, END_X,END_Y' style='fill:none; stroke:red; stroke-width:38; opacity:RED_OPACITY' />\n"
        "\n"
        " STATE_MARKER\n"
        "\n"
        "</svg>");

    static QString filledCircle   = QL1S("<circle cx='100' cy='100' r='35'/>");
    static QString plus           = QL1S("<path d='M 60,100 L140,100 M100,60 L100,140' style='stroke:black; stroke-width:30;'/>");
    static QString minus          = QL1S("<path d='M 60,100 L140,100' style='stroke:black; stroke-width:30;'/>");
    static QString hollowCircle   = QL1S("<circle cx='100' cy='100' r='30' style='fill:none;stroke:black;stroke-width:10'/>");

    QString svg = svg_template;

    if (chargeLevel > 99)
        chargeLevel = 99;

    double angle;
    QString sweepFlag;
    if (state == Solid::Battery::Discharging)
    {
        angle = M_PI_2 + 2 * M_PI * chargeLevel/100;
        sweepFlag = QL1C('0');
    }
    else
    {
        angle = M_PI_2 - 2 *M_PI * chargeLevel/100;
        sweepFlag = QL1C('1');
    }
    double circle_endpoint_x = 80.0 * cos(angle) + 100;
    double circle_endpoint_y = -80.0 * sin(angle) + 100;

    QString largeArgFlag = chargeLevel > 50 ? QL1S("1") : QL1S("0");

    svg.replace(QL1S("END_X"), QString::number(circle_endpoint_x));
    svg.replace(QL1S("END_Y"), QString::number(circle_endpoint_y));
    svg.replace(QL1S("LARGE_ARC_FLAG"), largeArgFlag);
    svg.replace(QL1S("SWEEP_FLAG"), sweepFlag);

    switch (state)
    {
    case Solid::Battery::FullyCharged:
        svg.replace(QL1S("STATE_MARKER"), filledCircle);
        break;
    case Solid::Battery::Charging:
        svg.replace(QL1S("STATE_MARKER"), plus);
        break;
    case Solid::Battery::Discharging:
        svg.replace(QL1S("STATE_MARKER"), minus);
        break;
    default:
        svg.replace(QL1S("STATE_MARKER"), hollowCircle);
    }

    if (state != Solid::Battery::FullyCharged && state != Solid::Battery::Charging &&  chargeLevel < mSettings.getPowerLowLevel() + 30)
    {
        if (chargeLevel <= mSettings.getPowerLowLevel() + 10)
            svg.replace(QL1S("RED_OPACITY"), QL1S("1"));
        else
            svg.replace(QL1S("RED_OPACITY"), QString::number((mSettings.getPowerLowLevel() + 30 - chargeLevel)/20.0));
    }
    else
        svg.replace(QL1S("RED_OPACITY"), QL1S("0"));

    // qDebug() << svg;

    // Paint the svg on a pixmap and create an icon from that.
    QSvgRenderer render(svg.toLatin1());
    QPixmap pixmap(render.defaultSize());
    pixmap.fill(QColor(0,0,0,0));
    QPainter painter(&pixmap);
    render.render(&painter);
    return QIcon(pixmap);
}
