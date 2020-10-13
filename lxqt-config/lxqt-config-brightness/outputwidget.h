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

#ifndef __OUTPUT_WIDGET_H__
#define __OUTPUT_WIDGET_H__

#include <QGroupBox>
#include <QMouseEvent>
#include "monitorinfo.h"
#include "ui_outputwidget.h"

class OutputWidget: public QWidget
{
Q_OBJECT
public:
    OutputWidget(MonitorInfo monitor, QWidget *parent);
    ~OutputWidget();

signals:
    void changed(MonitorInfo info);

public slots:
    void brightnessChanged(int value);
    void setRevertedValues(const MonitorInfo & monitor);
protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    MonitorInfo mMonitor;
    Ui::OutputWidget *ui;
};

#endif

