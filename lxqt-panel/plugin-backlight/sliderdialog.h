/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2020 LXQt team
 * Authors:
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


#ifndef SLIDERDIALOG_H
#define SLIDERDIALOG_H

#include <QDialog>
#include <QSlider>
#include <QToolButton>
#include <LXQt/lxqtbacklight.h>


class SliderDialog: public QDialog
{
    Q_OBJECT
    
public:
    SliderDialog(QWidget *parent);
    void updateBacklight();

Q_SIGNALS:
    void dialogClosed();

protected:
    bool event(QEvent *event) override;
    
private:
    QSlider *m_slider;
    QToolButton *m_upButton, *m_downButton;
    LXQt::Backlight *m_backlight;
    
private Q_SLOTS:
    void sliderValueChanged(int value);
    void downButtonClicked(bool);
    void upButtonClicked(bool);
    
};

#endif // SLIDERDIALOG_H
