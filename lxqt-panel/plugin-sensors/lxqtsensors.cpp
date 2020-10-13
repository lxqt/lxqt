/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Łukasz Twarduś <ltwardus@gmail.com>
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

#include "lxqtsensors.h"
#include "lxqtsensorsconfiguration.h"
#include "../panel/ilxqtpanelplugin.h"
#include "../panel/ilxqtpanel.h"
#include <QBoxLayout>
#include <QDebug>
#include <QMessageBox>
#include <QPalette>

static constexpr double DEFAULT_MAX = 200; // 200 Celsius

LXQtSensors::LXQtSensors(ILXQtPanelPlugin *plugin, QWidget* parent):
    QFrame(parent),
    mPlugin(plugin),
    mSettings(plugin->settings())
{

    mDetectedChips = mSensors.getDetectedChips();

    /**
     * We have all needed data to initialize default settings, we have to do it here as later
     * we are using them.
     */
    initDefaultSettings();

    // Add GUI elements
    ProgressBar* pg = nullptr;

    mLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    mLayout->setSpacing(0);
    mLayout->setContentsMargins(0, 0, 0, 0);

    QString chipFeatureLabel;

    mSettings->beginGroup(QStringLiteral("chips"));

    for (int i = 0; i < mDetectedChips.size(); ++i)
    {
        mSettings->beginGroup(mDetectedChips[i].getName());
        const QList<Feature>& features = mDetectedChips[i].getFeatures();

        for (int j = 0; j < features.size(); ++j)
        {
            if (features[j].getType() == SENSORS_FEATURE_TEMP)
            {
                chipFeatureLabel = features[j].getLabel();
                mSettings->beginGroup(chipFeatureLabel);

                pg = new ProgressBar(this);
                pg->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

                // Hide progress bar if it is not enabled
                if (!mSettings->value(QStringLiteral("enabled")).toBool())
                {
                    pg->hide();
                }

                pg->setToolTip(chipFeatureLabel);
                pg->setTextVisible(false);
                pg->setSensorColor(mSettings->value(QStringLiteral("color")).toString());

                mTemperatureProgressBars.push_back(pg);
                mLayout->addWidget(pg);

                mSettings->endGroup();
            }
        }
        mSettings->endGroup();
    }

    mSettings->endGroup();

    // Fit plugin to current panel
    realign();

    // Updated sensors readings to display actual values at start
    updateSensorReadings();

    // Run timer that will be updating sensor readings
    connect(&mUpdateSensorReadingsTimer, SIGNAL(timeout()), this, SLOT(updateSensorReadings()));
    mUpdateSensorReadingsTimer.start(mSettings->value(QStringLiteral("updateInterval")).toInt() * 1000);

    // Run timer that will be showin warning
    mWarningAboutHighTemperatureTimer.setInterval(500);
    connect(&mWarningAboutHighTemperatureTimer, SIGNAL(timeout()), this,
            SLOT(warningAboutHighTemperature()));
    if (mSettings->value(QStringLiteral("warningAboutHighTemperature")).toBool())
    {
        mWarningAboutHighTemperatureTimer.start();
    }
}


LXQtSensors::~LXQtSensors()
{
}


void LXQtSensors::updateSensorReadings()
{
    QString tooltip;

    double critTemp = 0;
    double maxTemp = 0;
    double minTemp = 0;
    double curTemp = 0;
    bool highTemperature = false;

    // Iterator for temperature progress bars
    QList<ProgressBar*>::iterator temperatureProgressBarsIt =
        mTemperatureProgressBars.begin();
    const bool use_fahrenheit = mSettings->value(QStringLiteral("useFahrenheitScale")).toBool();
    const bool warn_high = mSettings->value(QStringLiteral("warningAboutHighTemperature")).toBool();
    const double default_max = use_fahrenheit ? celsiusToFahrenheit(DEFAULT_MAX) : DEFAULT_MAX;

    for (int i = 0; i < mDetectedChips.size(); ++i)
    {
        const QList<Feature>& features = mDetectedChips[i].getFeatures();

        for (int j = 0; j < features.size(); ++j)
        {
            if (features[j].getType() == SENSORS_FEATURE_TEMP)
            {
                tooltip = features[j].getLabel() + QStringLiteral(" (") + QChar(0x00B0);

                critTemp = features[j].getValue(SENSORS_SUBFEATURE_TEMP_CRIT);
                maxTemp = features[j].getValue(SENSORS_SUBFEATURE_TEMP_MAX);
                minTemp = features[j].getValue(SENSORS_SUBFEATURE_TEMP_MIN);
                curTemp = features[j].getValue(SENSORS_SUBFEATURE_TEMP_INPUT);

                double temp_to_check = maxTemp == 0.0 ? critTemp : maxTemp;

                // Check if temperature is too high
                if (temp_to_check != 0.0 && curTemp >= temp_to_check)
                {
                    if (warn_high)
                    {
                        // Add current progress bar to the "warning container"
                        mHighTemperatureProgressBars.insert(*temperatureProgressBarsIt);
                    }

                    highTemperature = true;
                }
                else
                {
                    mHighTemperatureProgressBars.remove(*temperatureProgressBarsIt);

                    highTemperature = false;
                }

                if (use_fahrenheit)
                {
                    critTemp = celsiusToFahrenheit(critTemp);
                    maxTemp = celsiusToFahrenheit(maxTemp);
                    minTemp = celsiusToFahrenheit(minTemp);
                    curTemp = celsiusToFahrenheit(curTemp);

                    tooltip += QLatin1String("F)");
                }
                else
                {
                    tooltip += QLatin1String("C)");
                }


                // Set maximum temperature
                (*temperatureProgressBarsIt)->setMaximum(critTemp == 0.0 ? default_max : critTemp);
                // Set minimum temperature
                (*temperatureProgressBarsIt)->setMinimum(minTemp);
                // Set current temperature
                (*temperatureProgressBarsIt)->setValue(curTemp);

                tooltip += QLatin1String("<br><br>Crit: ");
                tooltip += QString::number((*temperatureProgressBarsIt)->maximum());
                tooltip += QLatin1String("<br>Max: ");
                tooltip += QString::number(int(maxTemp));
                tooltip += QLatin1String("<br>Cur: ");

                // Mark high temperature in the tooltip
                if (highTemperature)
                {
                    tooltip += QLatin1String("<span style=\"font-size:8pt; font-weight:600; color:#FF0000;\">");
                    tooltip += QString::number((*temperatureProgressBarsIt)->value());
                    tooltip += QLatin1String(" !</span>");
                }
                else
                {
                    tooltip += QString::number((*temperatureProgressBarsIt)->value());
                }

                tooltip += QLatin1String("<br>Min: ");
                tooltip += QString::number((*temperatureProgressBarsIt)->minimum());
                (*temperatureProgressBarsIt)->setToolTip(tooltip);

                // Go to the next temperature progress bar
                ++temperatureProgressBarsIt;
            }
        }
    }

    update();
}


void LXQtSensors::warningAboutHighTemperature()
{
    // Iterator for temperature progress bars
    QSet<ProgressBar*>::iterator temperatureProgressBarsIt =
        mHighTemperatureProgressBars.begin();

    int curValue;
    int maxValue;

    for (; temperatureProgressBarsIt != mHighTemperatureProgressBars.end();
         ++temperatureProgressBarsIt)
    {
        curValue = (*temperatureProgressBarsIt)->value();
        maxValue = (*temperatureProgressBarsIt)->maximum();

        if (maxValue > curValue)
        {
            (*temperatureProgressBarsIt)->setValue(maxValue);
        }
        else
        {
            (*temperatureProgressBarsIt)->setValue((*temperatureProgressBarsIt)->minimum());
        }

    }
    update();
}


void LXQtSensors::settingsChanged()
{
    mUpdateSensorReadingsTimer.setInterval(mSettings->value(QStringLiteral("updateInterval")).toInt() * 1000);

    // Iterator for temperature progress bars
    QList<ProgressBar*>::iterator temperatureProgressBarsIt =
        mTemperatureProgressBars.begin();

    mSettings->beginGroup(QStringLiteral("chips"));

    for (int i = 0; i < mDetectedChips.size(); ++i)
    {
        mSettings->beginGroup(mDetectedChips[i].getName());
        const QList<Feature>& features = mDetectedChips[i].getFeatures();

        for (int j = 0; j < features.size(); ++j)
        {
            if (features[j].getType() == SENSORS_FEATURE_TEMP)
            {
                mSettings->beginGroup(features[j].getLabel());

                if (mSettings->value(QStringLiteral("enabled")).toBool())
                {
                    (*temperatureProgressBarsIt)->show();
                }
                else
                {
                    (*temperatureProgressBarsIt)->hide();
                }

                (*temperatureProgressBarsIt)->setSensorColor(mSettings->value(QStringLiteral("color")).toString());

                mSettings->endGroup();

                // Go to the next temperature progress bar
                ++temperatureProgressBarsIt;
            }
        }

        mSettings->endGroup();
    }

    mSettings->endGroup();


    if (mSettings->value(QStringLiteral("warningAboutHighTemperature")).toBool())
    {
        // Update sensors readings to get the list of high temperature progress bars
        updateSensorReadings();

        if (!mWarningAboutHighTemperatureTimer.isActive())
            mWarningAboutHighTemperatureTimer.start();
    }
    else if (mWarningAboutHighTemperatureTimer.isActive())
    {
        mWarningAboutHighTemperatureTimer.stop();

        // Update sensors readings to set progress bar values to "normal" height
        updateSensorReadings();
    }

    realign();
    update();
}


void LXQtSensors::realign()
{
    // Default values for LXQtPanel::PositionBottom or LXQtPanel::PositionTop
    Qt::Orientation cur_orient = Qt::Vertical;
    Qt::LayoutDirection cur_layout_dir = Qt::LeftToRight;

    if (mPlugin->panel()->isHorizontal())
    {
        mLayout->setDirection(QBoxLayout::LeftToRight);
    }
    else
    {
        mLayout->setDirection(QBoxLayout::TopToBottom);
    }

    switch (mPlugin->panel()->position())
    {
    case ILXQtPanel::PositionLeft:
        cur_orient = Qt::Horizontal;
        break;

    case ILXQtPanel::PositionRight:
        cur_orient = Qt::Horizontal;
        cur_layout_dir = Qt::RightToLeft;
        break;

    default:
        break;
    }

    for (int i = 0; i < mTemperatureProgressBars.size(); ++i)
    {
        mTemperatureProgressBars[i]->setOrientation(cur_orient);
        mTemperatureProgressBars[i]->setLayoutDirection(cur_layout_dir);

        if (mPlugin->panel()->isHorizontal())
        {
            mTemperatureProgressBars[i]->setFixedWidth(mPlugin->settings()->value(QStringLiteral("tempBarWidth")).toInt());
            mTemperatureProgressBars[i]->setFixedHeight(QWIDGETSIZE_MAX);
        }
        else
        {
            mTemperatureProgressBars[i]->setFixedHeight(mPlugin->settings()->value(QStringLiteral("tempBarWidth")).toInt());
            mTemperatureProgressBars[i]->setFixedWidth(QWIDGETSIZE_MAX);
        }
    }
}


double LXQtSensors::celsiusToFahrenheit(double celsius)
{
    // Fahrenheit = 32 * (9/5) * Celsius
    return 32 + 1.8 * celsius;
}


void LXQtSensors::initDefaultSettings()
{
    if (!mSettings->contains(QStringLiteral("updateInterval")))
    {
        mSettings->setValue(QStringLiteral("updateInterval"), 1);
    }

    if (!mSettings->contains(QStringLiteral("tempBarWidth")))
    {
        mSettings->setValue(QStringLiteral("tempBarWidth"), 8);
    }

    if (!mSettings->contains(QStringLiteral("useFahrenheitScale")))
    {
        mSettings->setValue(QStringLiteral("useFahrenheitScale"), false);
    }

    mSettings->beginGroup(QStringLiteral("chips"));

    // Initialize default sensors settings
    for (int i = 0; i < mDetectedChips.size(); ++i)
    {
        mSettings->beginGroup(mDetectedChips[i].getName());
        const QList<Feature>& features = mDetectedChips[i].getFeatures();

        for (int j = 0; j < features.size(); ++j)
        {
            if (features[j].getType() == SENSORS_FEATURE_TEMP)
            {
                mSettings->beginGroup(features[j].getLabel());
                if (!mSettings->contains(QStringLiteral("enabled")))
                {
                    mSettings->setValue(QStringLiteral("enabled"), true);
                }

                if (!mSettings->contains(QStringLiteral("color")))
                {
                    // This is the default from QtDesigner
                    mSettings->setValue(QStringLiteral("color"), QColor(qRgb(98, 140, 178)).name());
                }
                mSettings->endGroup();
            }
        }
        mSettings->endGroup();
    }

    mSettings->endGroup();

    if (!mSettings->contains(QStringLiteral("warningAboutHighTemperature")))
    {
        mSettings->setValue(QStringLiteral("warningAboutHighTemperature"), true);
    }
}


ProgressBar::ProgressBar(QWidget *parent):
    QProgressBar(parent)
{
}


QSize ProgressBar::sizeHint() const
{
    return QSize(20, 20);
}

void ProgressBar::setSensorColor(const QString &colorName)
{
    // NOTE: Only a style sheet guarantees that custom colors are applied
    // because not all widget styles use palettes to draw progress bars.
    static const QString ss = QStringLiteral("QProgressBar{background-color: %1; border-radius: 2px; border: 1px solid %2;}"
                                             "QProgressBar::chunk{background-color: %3; border-radius: 1px;}");
    QPalette pal = palette();
    QColor base = pal.color(QPalette::Base);
    QColor outline = pal.color(QPalette::Text);
    outline.setRgbF(0.5 * base.redF()   + 0.5 * outline.redF(),
                    0.5 * base.greenF() + 0.5 * outline.greenF(),
                    0.5 * base.blueF()  + 0.5 * outline.blueF());
    setStyleSheet(ss.arg(base.name(), outline.name(), colorName));
}
