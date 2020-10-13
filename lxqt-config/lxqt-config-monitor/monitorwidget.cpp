/*
    Copyright (C) 2014  P.L. Lucas <selairi@gmail.com>
    Copyright (C) 2014  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "monitorwidget.h"

#include <QComboBox>
#include <QStringBuilder>
#include <QDialogButtonBox>
#include <KScreen/EDID>

#include <algorithm>

QString modeToString(KScreen::ModePtr mode)
{
    // mode->name() can be anything, not just widthxheight. eg if added with cvt.
    return QStringLiteral("%1x%2").arg(mode->size().width()).arg(mode->size().height());
}

KScreen::OutputPtr getOutputById(int id, KScreen::OutputList outputs)
{
    for (const KScreen::OutputPtr &output : outputs)
        if (output->id() == id)
            return output;

    return KScreen::OutputPtr(nullptr);
}

KScreen::ModePtr getModeById(QString id, KScreen::ModeList modes)
{
    for (const KScreen::ModePtr &mode : modes)
        if (mode->id() == id)
            return mode;

    return KScreen::ModePtr(nullptr);
}

static bool sizeBiggerThan(const KScreen::ModePtr &modeA, const KScreen::ModePtr &modeB)
{
    QSize sizeA = modeA->size();
    QSize sizeB = modeB->size();
    //return sizeA.width() * sizeA.height() > sizeB.width() * sizeB.height();
    return sizeA.width() > sizeB.width() || (sizeA.width() == sizeB.width() && sizeA.height() > sizeB.height());
}


MonitorWidget::MonitorWidget(KScreen::OutputPtr output, KScreen::ConfigPtr config, QWidget* parent) :
    QGroupBox(parent)
{
    this->output = output;
    this->config = config;

    ui.setupUi(this);

    ui.enabledCheckbox->setChecked(output->isEnabled());

    QList <KScreen::ModePtr> modeList = output->modes().values();

    // Remove duplicate sizes
    QMap<QString, KScreen::ModePtr> noDuplicateModes;
    for(const KScreen::ModePtr &mode : qAsConst(modeList)) {
        if( noDuplicateModes.keys().contains(modeToString(mode)) ) {
            KScreen::ModePtr actual = noDuplicateModes[modeToString(mode)];
            bool isActualPreferred = output->preferredModes().contains(actual->id());
            bool isModePreferred = output->preferredModes().contains(mode->id());
            if( ( mode->refreshRate() > actual->refreshRate() && !isActualPreferred ) || isModePreferred )
                noDuplicateModes[modeToString(mode)] = mode;
        }
        else
            noDuplicateModes[modeToString(mode)] = mode;
    }

    // Sort modes by size
    modeList = noDuplicateModes.values();
    std::sort(modeList.begin(), modeList.end(), sizeBiggerThan);

    // Add each mode to the list
    for (const KScreen::ModePtr &mode : qAsConst(modeList)) {
        ui.resolutionCombo->addItem(modeToString(mode), mode->id());
        if(output->preferredModes().contains(mode->id())) {
            // Make bold preferredModes
            QFont font = ui.resolutionCombo->font();
            font.setBold(true);
            ui.resolutionCombo->setItemData(ui.resolutionCombo->count()-1, font, Qt::FontRole);
        }
    }
    connect(ui.resolutionCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onResolutionChanged(int)));

    // Select actual mode in list
    if (output->currentMode()) {
        // Set the current mode in dropdown
        int idx = ui.resolutionCombo->findData(output->currentMode()->id());
        if (idx < 0) {
            // Select mode with same size
            for (const KScreen::ModePtr &mode : qAsConst(modeList)) {
                if( mode->size() == output->currentMode()->size() )
                    idx = ui.resolutionCombo->findData(output->currentMode()->id());
            }
        }
        if(idx < 0)
            idx = ui.resolutionCombo->findData(output->preferredMode()->id());
        if (idx >= 0)
            ui.resolutionCombo->setCurrentIndex(idx);
    }
    updateRefreshRates();


    // Update EDID information
    // KScreen doesn't make much public but that's ok...
    KScreen::Edid* edid = output->edid();
    if (edid && edid->isValid()) {
        ui.outputInfoLabel->setText(
            tr("Name: %1\n").arg(edid->name()) %
            tr("Vendor: %1\n").arg(edid->vendor()) %
            tr("Serial: %1\n").arg(edid->serial()) %
            tr("Display size: %1cm x %2cm\n").arg(edid->width()).arg(edid->height()) %
            tr("Serial number: %1\n").arg(edid->serial()) %
            tr("EISA device ID: %1\n").arg(edid->eisaId())
        );
    }

    if (config->connectedOutputs().count() == 1) {
        setOnlyMonitor(true);
        // There isn't always a primary output. Gross.
        output->setPrimary(true);
    }

    ui.xPosSpinBox->setValue(output->pos().x());
    ui.yPosSpinBox->setValue(output->pos().y());

    // Behavior chooser
    if (output->isPrimary())
        ui.behaviorCombo->setCurrentIndex(PrimaryDisplay);
    else
        ui.behaviorCombo->setCurrentIndex(ExtendDisplay);

    // Insert orientations
    ui.orientationCombo->addItem(tr("None"), KScreen::Output::None);
    ui.orientationCombo->addItem(tr("Left"), KScreen::Output::Left);
    ui.orientationCombo->addItem(tr("Right"), KScreen::Output::Right);
    ui.orientationCombo->addItem(tr("Inverted"), KScreen::Output::Inverted);
    switch(output->rotation()) {
    case KScreen::Output::None:
        ui.orientationCombo->setCurrentIndex(0);
        break;
    case KScreen::Output::Left:
        ui.orientationCombo->setCurrentIndex(1);
        break;
    case KScreen::Output::Right:
        ui.orientationCombo->setCurrentIndex(2);
        break;
    case KScreen::Output::Inverted:
        ui.orientationCombo->setCurrentIndex(3);
        break;
    }

    connect(ui.enabledCheckbox, SIGNAL(toggled(bool)), this, SLOT(onEnabledChanged(bool)));
    connect(ui.behaviorCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onBehaviorChanged(int)));
    connect(ui.xPosSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onPositionChanged(int)));
    connect(ui.yPosSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onPositionChanged(int)));
    connect(ui.orientationCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onOrientationChanged(int)));

    // Force update behavior visibility
    onBehaviorChanged(ui.behaviorCombo->currentIndex());
}

MonitorWidget::~MonitorWidget()
{
}

void MonitorWidget::onEnabledChanged(bool enabled)
{
    output->setEnabled(enabled);

    // If we're enabling a disabled output for the first time
    if (enabled && !output->currentMode()) {
        // order here matters
        onResolutionChanged(ui.resolutionCombo->currentIndex());
        onOrientationChanged(ui.orientationCombo->currentIndex());
        onBehaviorChanged(ui.behaviorCombo->currentIndex());
    }
}

void MonitorWidget::onOrientationChanged(int idx)
{
    output->setRotation((KScreen::Output::Rotation) ui.orientationCombo->currentData().toInt(0));
}

void MonitorWidget::onBehaviorChanged(int idx)
{
    // Behavior should match the index of the selected element
    ui.xPosSpinBox->setVisible(idx == ExtendDisplay);
    ui.yPosSpinBox->setVisible(idx == ExtendDisplay);

    output->setPrimary(idx == PrimaryDisplay);
    if(idx == PrimaryDisplay)
        emit primaryOutputChanged(this);
}

void MonitorWidget::onPositionChanged(int value)
{
    output->setPos(QPoint(ui.xPosSpinBox->value(), ui.yPosSpinBox->value()));
}

void MonitorWidget::onResolutionChanged(int index)
{
    output->setCurrentModeId(ui.resolutionCombo->currentData().toString());
    updateRefreshRates();
}

void MonitorWidget::onRateChanged(int index)
{
    output->setCurrentModeId(ui.rateCombo->currentData().toString());
}

void MonitorWidget::updateRefreshRates()
{
    disconnect(ui.rateCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onRateChanged(int)));
    ui.rateCombo->clear();

    if (output->modes().size() < 0)
        return;

    KScreen::ModePtr selectedMode = output->currentMode();
    if (selectedMode) {
        const auto modes = output->modes();
        for (const KScreen::ModePtr &mode : modes)
            if (mode->size() == selectedMode->size())
                ui.rateCombo->addItem(tr("%1 Hz").arg(mode->refreshRate()), mode->id());

        int idx = ui.rateCombo->findData(selectedMode->id());
        if (idx >= 0)
            ui.rateCombo->setCurrentIndex(idx);
    }

    connect(ui.rateCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onRateChanged(int)));
}

void MonitorWidget::setOnlyMonitor(bool isOnlyMonitor)
{
    ui.enabledCheckbox->setEnabled(!isOnlyMonitor);
    ui.behaviorCombo->setEnabled(!isOnlyMonitor);
    ui.xPosSpinBox->setVisible(!isOnlyMonitor);
    ui.yPosSpinBox->setVisible(!isOnlyMonitor);
    if(isOnlyMonitor) {
        ui.xPosSpinBox->setValue(0);
        ui.yPosSpinBox->setValue(0);
    }
}

void MonitorWidget::onPrimaryOutputChanged(MonitorWidget *widget)
{
    if(widget == this)
        return;
    ui.behaviorCombo->setCurrentIndex(ExtendDisplay);
}
