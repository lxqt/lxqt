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


#include "lxqtsysstatconfiguration.h"
#include "ui_lxqtsysstatconfiguration.h"
#include "lxqtsysstatutils.h"
#include "lxqtsysstatcolours.h"
#include <QPushButton>
#include <SysStat/CpuStat>
#include <SysStat/MemStat>
#include <SysStat/NetStat>

//Note: strings can't actually be translated here (in static initialization time)
//      the QT_TR_NOOP here is just for qt translate tools to get the strings for translation
const QStringList LXQtSysStatConfiguration::msStatTypes = {
    QLatin1String(QT_TR_NOOP("CPU"))
    , QLatin1String(QT_TR_NOOP("Memory"))
    , QLatin1String(QT_TR_NOOP("Network"))
};

namespace
{
    //Note: workaround for making source strings translatable
    //  (no need to ever call this function)
    void localizationWorkaround();
    auto t = localizationWorkaround;//avoid unused function warning
    void localizationWorkaround()
    {
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu0"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu1"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu2"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu3"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu4"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu5"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu6"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu7"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu8"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu9"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu10"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu11"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu12"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu13"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu14"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu15"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu16"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu17"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu18"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu19"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu20"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu21"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu22"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "cpu23"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "memory"));
        static_cast<void>(QT_TRANSLATE_NOOP("LXQtSysStatConfiguration", "swap"));
        static_cast<void>(t);//avoid unused variable warning
    }
}

LXQtSysStatConfiguration::LXQtSysStatConfiguration(PluginSettings *settings, QWidget *parent) :
    LXQtPanelPluginConfigDialog(settings, parent),
    ui(new Ui::LXQtSysStatConfiguration),
    mStat(nullptr),
    mColoursDialog(nullptr)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("SysStatConfigurationWindow"));
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
    ui->buttonBox_2->button(QDialogButtonBox::Close)->setText(tr("Close"));
    //Note: translation is needed here in runtime (translator is attached already)
    for (auto const & type : msStatTypes)
        ui->typeCOB->addItem(tr(type.toStdString().c_str()), type);
    loadSettings();

    connect(ui->typeCOB, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LXQtSysStatConfiguration::saveSettings);
    connect(ui->intervalSB, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &LXQtSysStatConfiguration::saveSettings);
    connect(ui->sizeSB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &LXQtSysStatConfiguration::saveSettings);
    connect(ui->linesSB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &LXQtSysStatConfiguration::saveSettings);
    connect(ui->titleLE, &QLineEdit::editingFinished, this, &LXQtSysStatConfiguration::saveSettings);
    connect(ui->useFrequencyCB, &QCheckBox::toggled, this, &LXQtSysStatConfiguration::saveSettings);
    connect(ui->maximumHS, &QSlider::valueChanged, this, &LXQtSysStatConfiguration::saveSettings);
    connect(ui->logarithmicCB, &QCheckBox::toggled, this, &LXQtSysStatConfiguration::saveSettings);
    connect(ui->sourceCOB, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LXQtSysStatConfiguration::saveSettings);
    connect(ui->useThemeColoursRB, &QRadioButton::toggled, this, &LXQtSysStatConfiguration::saveSettings);
}

LXQtSysStatConfiguration::~LXQtSysStatConfiguration()
{
    delete ui;
}

void LXQtSysStatConfiguration::loadSettings()
{
    ui->intervalSB->setValue(settings().value(QStringLiteral("graph/updateInterval"), 1.0).toDouble());
    ui->sizeSB->setValue(settings().value(QStringLiteral("graph/minimalSize"), 30).toInt());

    ui->linesSB->setValue(settings().value(QStringLiteral("grid/lines"), 1).toInt());

    ui->titleLE->setText(settings().value(QStringLiteral("title/label"), QString()).toString());

    int typeIndex = ui->typeCOB->findData(settings().value(QStringLiteral("data/type"), msStatTypes[0]));
    ui->typeCOB->setCurrentIndex((typeIndex >= 0) ? typeIndex : 0);
    on_typeCOB_currentIndexChanged(ui->typeCOB->currentIndex());

    int sourceIndex = ui->sourceCOB->findData(settings().value(QStringLiteral("data/source"), QString()));
    ui->sourceCOB->setCurrentIndex((sourceIndex >= 0) ? sourceIndex : 0);

    ui->useFrequencyCB->setChecked(settings().value(QStringLiteral("cpu/useFrequency"), true).toBool());
    ui->maximumHS->setValue(PluginSysStat::netSpeedFromString(settings().value(QStringLiteral("net/maximumSpeed"), QStringLiteral("1 MB/s")).toString()));
    on_maximumHS_valueChanged(ui->maximumHS->value());
    ui->logarithmicCB->setChecked(settings().value(QStringLiteral("net/logarithmicScale"), true).toBool());
    ui->logScaleSB->setValue(settings().value(QStringLiteral("net/logarithmicScaleSteps"), 4).toInt());

    bool useThemeColours = settings().value(QStringLiteral("graph/useThemeColours"), true).toBool();
    ui->useThemeColoursRB->setChecked(useThemeColours);
    ui->useCustomColoursRB->setChecked(!useThemeColours);
    ui->customColoursB->setEnabled(!useThemeColours);
}

void LXQtSysStatConfiguration::saveSettings()
{
    settings().setValue(QStringLiteral("graph/useThemeColours"), ui->useThemeColoursRB->isChecked());
    settings().setValue(QStringLiteral("graph/updateInterval"), ui->intervalSB->value());
    settings().setValue(QStringLiteral("graph/minimalSize"), ui->sizeSB->value());

    settings().setValue(QStringLiteral("grid/lines"), ui->linesSB->value());

    settings().setValue(QStringLiteral("title/label"), ui->titleLE->text());

    //Note:
    // need to make a realy deep copy of the msStatTypes[x] because of SEGFAULTs
    // occuring in static finalization time (don't know the real reason...maybe ordering of static finalizers/destructors)
    QString type = QString::fromUtf8(ui->typeCOB->itemData(ui->typeCOB->currentIndex(), Qt::UserRole).toString().toStdString().c_str());
    settings().setValue(QStringLiteral("data/type"), type);
    settings().setValue(QStringLiteral("data/source"), ui->sourceCOB->itemData(ui->sourceCOB->currentIndex(), Qt::UserRole));

    settings().setValue(QStringLiteral("cpu/useFrequency"), ui->useFrequencyCB->isChecked());

    settings().setValue(QStringLiteral("net/maximumSpeed"), PluginSysStat::netSpeedToString(ui->maximumHS->value()));
    settings().setValue(QStringLiteral("net/logarithmicScale"), ui->logarithmicCB->isChecked());
    settings().setValue(QStringLiteral("net/logarithmicScaleSteps"), ui->logScaleSB->value());
}

void LXQtSysStatConfiguration::on_typeCOB_currentIndexChanged(int index)
{
    if (mStat)
        mStat->deleteLater();
    switch (index)
    {
    case 0:
        mStat = new SysStat::CpuStat(this);
        break;

    case 1:
        mStat = new SysStat::MemStat(this);
        break;

    case 2:
        mStat = new SysStat::NetStat(this);
        break;
    }

    ui->sourceCOB->blockSignals(true);
    ui->sourceCOB->clear();
    const auto sources = mStat->sources();
    for (auto const & s : sources)
        ui->sourceCOB->addItem(tr(s.toStdString().c_str()), s);
    ui->sourceCOB->blockSignals(false);
    ui->sourceCOB->setCurrentIndex(0);
}

void LXQtSysStatConfiguration::on_maximumHS_valueChanged(int value)
{
    ui->maximumValueL->setText(PluginSysStat::netSpeedToString(value));
}

void LXQtSysStatConfiguration::coloursChanged()
{
    const LXQtSysStatColours::Colours &colours = mColoursDialog->colours();

    settings().setValue(QStringLiteral("grid/colour"),  colours[QStringLiteral("grid")].name());
    settings().setValue(QStringLiteral("title/colour"), colours[QStringLiteral("title")].name());

    settings().setValue(QStringLiteral("cpu/systemColour"),    colours[QStringLiteral("cpuSystem")].name());
    settings().setValue(QStringLiteral("cpu/userColour"),      colours[QStringLiteral("cpuUser")].name());
    settings().setValue(QStringLiteral("cpu/niceColour"),      colours[QStringLiteral("cpuNice")].name());
    settings().setValue(QStringLiteral("cpu/otherColour"),     colours[QStringLiteral("cpuOther")].name());
    settings().setValue(QStringLiteral("cpu/frequencyColour"), colours[QStringLiteral("cpuFrequency")].name());

    settings().setValue(QStringLiteral("mem/appsColour"),    colours[QStringLiteral("memApps")].name());
    settings().setValue(QStringLiteral("mem/buffersColour"), colours[QStringLiteral("memBuffers")].name());
    settings().setValue(QStringLiteral("mem/cachedColour"),  colours[QStringLiteral("memCached")].name());
    settings().setValue(QStringLiteral("mem/swapColour"),    colours[QStringLiteral("memSwap")].name());

    settings().setValue(QStringLiteral("net/receivedColour"),    colours[QStringLiteral("netReceived")].name());
    settings().setValue(QStringLiteral("net/transmittedColour"), colours[QStringLiteral("netTransmitted")].name());
}

void LXQtSysStatConfiguration::on_customColoursB_clicked()
{
    if (!mColoursDialog)
    {
        mColoursDialog = new LXQtSysStatColours(this);
        connect(mColoursDialog, SIGNAL(coloursChanged()), SLOT(coloursChanged()));
    }

    LXQtSysStatColours::Colours colours;

    const LXQtSysStatColours::Colours &defaultColours = mColoursDialog->defaultColours();

    colours[QStringLiteral("grid")]  = QColor(settings().value(QStringLiteral("grid/colour"),  defaultColours[QStringLiteral("grid")] .name()).toString());
    colours[QStringLiteral("title")] = QColor(settings().value(QStringLiteral("title/colour"), defaultColours[QStringLiteral("title")].name()).toString());

    colours[QStringLiteral("cpuSystem")]    = QColor(settings().value(QStringLiteral("cpu/systemColour"),    defaultColours[QStringLiteral("cpuSystem")]   .name()).toString());
    colours[QStringLiteral("cpuUser")]      = QColor(settings().value(QStringLiteral("cpu/userColour"),      defaultColours[QStringLiteral("cpuUser")]     .name()).toString());
    colours[QStringLiteral("cpuNice")]      = QColor(settings().value(QStringLiteral("cpu/niceColour"),      defaultColours[QStringLiteral("cpuNice")]     .name()).toString());
    colours[QStringLiteral("cpuOther")]     = QColor(settings().value(QStringLiteral("cpu/otherColour"),     defaultColours[QStringLiteral("cpuOther")]    .name()).toString());
    colours[QStringLiteral("cpuFrequency")] = QColor(settings().value(QStringLiteral("cpu/frequencyColour"), defaultColours[QStringLiteral("cpuFrequency")].name()).toString());

    colours[QStringLiteral("memApps")]    = QColor(settings().value(QStringLiteral("mem/appsColour"),    defaultColours[QStringLiteral("memApps")]   .name()).toString());
    colours[QStringLiteral("memBuffers")] = QColor(settings().value(QStringLiteral("mem/buffersColour"), defaultColours[QStringLiteral("memBuffers")].name()).toString());
    colours[QStringLiteral("memCached")]  = QColor(settings().value(QStringLiteral("mem/cachedColour"),  defaultColours[QStringLiteral("memCached")] .name()).toString());
    colours[QStringLiteral("memSwap")]    = QColor(settings().value(QStringLiteral("mem/swapColour"),    defaultColours[QStringLiteral("memSwap")]   .name()).toString());

    colours[QStringLiteral("netReceived")]    = QColor(settings().value(QStringLiteral("net/receivedColour"),    defaultColours[QStringLiteral("netReceived")]   .name()).toString());
    colours[QStringLiteral("netTransmitted")] = QColor(settings().value(QStringLiteral("net/transmittedColour"), defaultColours[QStringLiteral("netTransmitted")].name()).toString());

    mColoursDialog->setColours(colours);

    mColoursDialog->exec();
}
