/*
    Copyright (C) 2014  P.L. Lucas <selairi@gmail.com>

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

#ifndef _MONITORPICTURE_H_
#define _MONITORPICTURE_H_

#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QSvgRenderer>
#include <QGraphicsSvgItem>
#include <QDialog>
#include "monitor.h"
#include "ui_monitorpicture.h"
#include "monitorwidget.h"

class MonitorPicture;

class MonitorPictureDialog : public QDialog
{
    Q_OBJECT

public:
    MonitorPictureDialog(KScreen::ConfigPtr config, QWidget * parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    void setScene(QList<MonitorWidget*> monitors);
    void updateMonitorWidgets(QString primaryMonitor);
    void moveMonitorPictureToNearest(MonitorPicture* monitorPicture);
    void updateScene();

protected:
    void showEvent(QShowEvent * event) override;

private:
    Ui::MonitorPictureDialog ui;
    QList<MonitorPicture*> pictures;
    bool updatingOk;
    KScreen::ConfigPtr mConfig;
    bool firstShownOk;
    int maxMonitorSize;
};

class MonitorPicture : public QGraphicsRectItem
{
public:
    MonitorPicture(QGraphicsItem *parent,
                   MonitorWidget *monitorWidget,
                   MonitorPictureDialog *monitorPictureDialog);
    void setMonitorPosition(int x, int y);
    void adjustNameSize();

    MonitorWidget *monitorWidget;
    int originX, originY;

    void updateSize(QSize size);

private:
    QGraphicsTextItem *textItem;
    QGraphicsSvgItem *svgItem;    
    MonitorPictureDialog *monitorPictureDialog;


protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant & value) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event) override;
};

class MonitorPictureProxy: public QObject
{
    Q_OBJECT
    public:
        MonitorPictureProxy(QObject *parent, MonitorPicture *monitorPicture);
        MonitorPicture *monitorPicture;
        
    public slots:
        void updateSize();
        void updatePosition();
        
};


#endif // _MONITORPICTURE_H_
