/*
    Copyright (C) 2016  P.L. Lucas <selairi@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "brightnesssettings.h"
#include "outputwidget.h"
#include <QMessageBox>
#include <QPushButton>

BrightnessSettings::BrightnessSettings(QWidget *parent):QDialog(parent)
{
    ui = new Ui::BrightnessSettings();
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
    ui->buttonBox_2->button(QDialogButtonBox::Close)->setText(tr("Close"));
    mBrightness = new XRandrBrightness();
    mMonitors = mBrightness->getMonitorsInfo();
    mMonitorsInitial = mBrightness->getMonitorsInfo();
    mBacklight = new LXQt::Backlight(this);
    ui->headIconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("display-brightness-symbolic")).pixmap(32, 32));

    ui->backlightSlider->setEnabled(mBacklight->isBacklightAvailable() || mBacklight->isBacklightOff());
    ui->backlightGroupBox->setEnabled(mBacklight->isBacklightAvailable() || mBacklight->isBacklightOff());
    if(mBacklight->isBacklightAvailable()) {
        setBacklightSliderValue(mBacklight->getBacklight());

        mInitialBacklightValue = mLastBacklightValue = mBacklight->getBacklight();
        connect(ui->backlightSlider, &QSlider::valueChanged, this, &BrightnessSettings::setBacklight);

        connect(ui->backlightDownButton, &QToolButton::clicked,
                [this](bool){ ui->backlightSlider->setValue(ui->backlightSlider->value()-1); });
        connect(ui->backlightUpButton, &QToolButton::clicked,
                [this](bool){ ui->backlightSlider->setValue(ui->backlightSlider->value()+1); });
    }

    for(const MonitorInfo &monitor: qAsConst(mMonitors))
    {
        OutputWidget *output = new OutputWidget(monitor, this);
        ui->layout->addWidget(output);
        output->show();
        connect(output, SIGNAL(changed(MonitorInfo)), this, SLOT(monitorSettingsChanged(MonitorInfo)));
        connect(this, &BrightnessSettings::monitorReverted, output, &OutputWidget::setRevertedValues);
    }

    mConfirmRequestTimer.setSingleShot(true);
    mConfirmRequestTimer.setInterval(1000);
    connect(&mConfirmRequestTimer, &QTimer::timeout, this, &BrightnessSettings::requestConfirmation);

    connect(ui->buttonBox_2, &QDialogButtonBox::clicked,
            [this](QAbstractButton *button) {
        if(ui->buttonBox_1->button(QDialogButtonBox::Reset) == button) {
            revertValues();
        }
    } );
}

BrightnessSettings::~BrightnessSettings()
{
    delete ui;
    ui = nullptr;

    delete mBrightness;
    mBrightness = nullptr;
}

void BrightnessSettings::setBacklight()
{
    int value = ui->backlightSlider->value();
    // Set the minimum to 5% of the maximum to prevent a black screen
    int minBacklight = qMax(qRound((qreal)(mBacklight->getMaxBacklight())*0.05), 1);
    int maxBacklight = mBacklight->getMaxBacklight();
    int interval = maxBacklight - minBacklight;
    if(interval > 100)
        value = (value * maxBacklight) / 100;
    mBacklight->setBacklight(value);
    
    if (ui->confirmCB->isChecked())
        mConfirmRequestTimer.start();
}

void BrightnessSettings::monitorSettingsChanged(MonitorInfo monitor)
{
    mBrightness->setMonitorsSettings(QList<MonitorInfo>{}  << monitor);
    if (ui->confirmCB->isChecked())
        mConfirmRequestTimer.start();
    else {
        for (auto & m : mMonitors) {
            if (m.id() == monitor.id() && m.name() == monitor.name()) {
                m.setBacklight(monitor.backlight());
                m.setBrightness(monitor.brightness());
            }
        }
    }
}

void BrightnessSettings::requestConfirmation()
{
    QMessageBox msg{QMessageBox::Question, tr("Brightness settings changed")
                , tr("Confirmation required. Are the settings correct?")
                , QMessageBox::Yes | QMessageBox::No};
    int timeout = 5; // seconds
    QString no_text = msg.button(QMessageBox::No)->text();
    no_text += QStringLiteral("(%1)");
    msg.setButtonText(QMessageBox::No, no_text.arg(timeout));
    msg.setDefaultButton(QMessageBox::No);

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(false);
    timeoutTimer.setInterval(1000);
    connect(&timeoutTimer, &QTimer::timeout, [&] {
        msg.setButtonText(QMessageBox::No, no_text.arg(--timeout));
        if (timeout == 0)
        {
            timeoutTimer.stop();
            msg.reject();
        }
    });
    timeoutTimer.start();

    if (QMessageBox::Yes == msg.exec())
    {
        // re-read current values
        if(mBacklight->isBacklightAvailable())
            mLastBacklightValue = mBacklight->getBacklight();

        mMonitors = mBrightness->getMonitorsInfo();
    } else
    {
        // revert the changes
        if(mBacklight->isBacklightAvailable()) {
            disconnect(ui->backlightSlider, &QSlider::valueChanged, this, &BrightnessSettings::setBacklight);
            mBacklight->setBacklight(mLastBacklightValue);
            setBacklightSliderValue(mLastBacklightValue);
            connect(ui->backlightSlider, &QSlider::valueChanged, this, &BrightnessSettings::setBacklight);
        }

        mBrightness->setMonitorsSettings(mMonitors);
        for (const auto & monitor : qAsConst(mMonitors))
            emit monitorReverted(monitor);
    }
}

void BrightnessSettings::revertValues()
{
    if(mBacklight->isBacklightAvailable()) {
        disconnect(ui->backlightSlider, &QSlider::valueChanged, this, &BrightnessSettings::setBacklight);
        mBacklight->setBacklight(mInitialBacklightValue);
        setBacklightSliderValue(mInitialBacklightValue);
        connect(ui->backlightSlider, &QSlider::valueChanged, this, &BrightnessSettings::setBacklight);
    }

    mBrightness->setMonitorsSettings(mMonitorsInitial);
    for (const auto & monitor : qAsConst(mMonitorsInitial))
        emit monitorReverted(monitor);
}


void BrightnessSettings::setBacklightSliderValue(int value)
{
    // Set the minimum to 5% of the maximum to prevent a black screen
    int minBacklight = qMax(qRound((qreal)(mBacklight->getMaxBacklight())*0.05), 1);
    int maxBacklight = mBacklight->getMaxBacklight();
    int interval = maxBacklight - minBacklight;
    if(interval <= 100) {
        ui->backlightSlider->setMaximum(maxBacklight);
        ui->backlightSlider->setMinimum(minBacklight);
        ui->backlightSlider->setValue(value);
    } else {
        ui->backlightSlider->setMaximum(100);
        // Set the minimum to 5% of the maximum to prevent a black screen
        ui->backlightSlider->setMinimum(5);
        ui->backlightSlider->setValue( (value * 100) / maxBacklight);
    }
}
