/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Aaron Lewis <the.warl0ck.1989@gmail.com>
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
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef LXQT_COLORPICKER_H
#define LXQT_COLORPICKER_H

#include "../panel/ilxqtpanelplugin.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QFrame>
#include <QFontMetrics>
#include <QLineEdit>
#include <QToolButton>
#include <XdgIcon>


class ColorPickerWidget: public QFrame
{
    Q_OBJECT
public:
    ColorPickerWidget(QWidget* parent = nullptr);
    ~ColorPickerWidget();

    QLineEdit *lineEdit() { return &mLineEdit; }
    QToolButton *button() { return &mButton; }


protected:
    void mouseReleaseEvent(QMouseEvent *event);

private slots:
    void captureMouse();

private:
    QLineEdit mLineEdit;
    QToolButton mButton;
    bool mCapturing;
};


class ColorPicker : public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT
public:
    ColorPicker(const ILXQtPanelPluginStartupInfo &startupInfo);
    ~ColorPicker();

    virtual QWidget *widget() { return &mWidget; }
    virtual QString themeId() const { return QStringLiteral("ColorPicker"); }

    bool isSeparate() const { return true; }

    void realign();

private:
    ColorPickerWidget mWidget;
};

class ColorPickerLibrary: public QObject, public ILXQtPanelPluginLibrary
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const
    {
        return new ColorPicker(startupInfo);
    }
};

#endif
