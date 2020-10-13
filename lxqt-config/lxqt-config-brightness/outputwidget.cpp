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

#include "outputwidget.h"

OutputWidget::OutputWidget(MonitorInfo monitor, QWidget *parent):QWidget(parent), mMonitor(monitor)
{
    ui = new Ui::OutputWidget();
    ui->setupUi(this);

    ui->label->setText(QStringLiteral("<b>")+monitor.name()+QStringLiteral(":</b>"));
    
    ui->brightnessSlider->setMinimum(0);
    ui->brightnessSlider->setMaximum(200);
    ui->brightnessSlider->setValue(monitor.brightness()*100);

    connect(ui->brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(brightnessChanged(int)));
    connect(ui->brightnessDownButton, &QToolButton::clicked, 
        [this](bool){ ui->brightnessSlider->setValue(ui->brightnessSlider->value()-1); });
    connect(ui->brightnessUpButton, &QToolButton::clicked,
        [this](bool){ ui->brightnessSlider->setValue(ui->brightnessSlider->value()+1); });
}

OutputWidget::~OutputWidget()
{
    delete ui;
    ui = nullptr;
}

void OutputWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if ( event->button() == Qt::RightButton && ui->brightnessSlider->underMouse() ) {
        ui->brightnessSlider->setValue(100);
    }
}



void OutputWidget::brightnessChanged(int value)
{
    mMonitor.setBrightness((float)value/100.0);
    emit changed(mMonitor);
}

void OutputWidget::setRevertedValues(const MonitorInfo & monitor)
{
    if (mMonitor.id() == monitor.id() && mMonitor.name() == monitor.name()) {
        ui->brightnessSlider->blockSignals(true);
        ui->brightnessSlider->setValue(monitor.brightness()*100);
        ui->brightnessSlider->blockSignals(false);
    }
}
