/*
 * infobar.h
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 1997-1999 Mattias Engdegård
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef INFOBAR_H
#define INFOBAR_H

#include "proc.h"

#include <QString>
#include <QPixmap>
#include <QResizeEvent>
#include <QFrame>
#include <QEvent>
#include <QMouseEvent>

class subcpuRack : public QWidget
{
    Q_OBJECT
  public:
    subcpuRack(QWidget *parent, Procview *p);
    void refresh();
    //   void update_load();
    //   void show_and_hide(bool, QWidget *, QWidget *);
    // signals:  void config_change();
  protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;

  private:
    //	List<gwidget*>  wlist;
    QWidget *parent;
    Procview *procview; // temp
    bool is_vertical;
};

class gwidget
{
  public:
    gwidget() { x = 0, y = 0, width = 0, height = 0; };
    virtual ~gwidget() = default;
    gwidget(QWidget *p) { parent = p; };
    virtual void draw(QPainter */*p*/) { printf("gwidget\n"); };
    virtual void setPosition(int /*parent_width*/, int /*parent_height*/){};
    virtual const char *info() { return nullptr; };
    void setParent(QWidget *p, Procview *procv)
    {
        parent = p;
        procview = procv;
    };
    void resize(int x_, int y_, int w, int h)
    {
        x = x_;
        y = y_;
        width = w;
        height = h;
    };
    bool intersect(int cx, int cy)
    {
        cx = cx - x;
        cy = cy - y;
        if (cx > 0 and cx < width)
            if (cy > 0 and cy < height)
                return true;
        return false;
    }

    int xpluswidth() { return x + width; };
    Procview *procview; // temp

  protected:
    int x;
    int y;
    int width;
    int height;
    QRect rect;
    QWidget *parent;
};

class Infobar : public QFrame
{
    Q_OBJECT
  public:
    Infobar(){};

    Infobar(QWidget *parent, Procview *);
    ~Infobar() override;

    void configure(); // respond to configuration change

    QPixmap *make_icon(int w, int h);
    // DEL QPixmap *load_icon(int w, int h) { return make_icon(w, h); }
    void refresh(); // update();
    void update_load();
    void drawGraphOnPixmap();
    bool swaplim_exceeded();
    void show_and_hide(bool, QWidget *, QWidget *);
    void showup();
signals:
    void clicked();
    void config_change();

  protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;
    void enterEvent(QEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *e) override;

    void add_history_point(unsigned value);
    void add_history_point2(float value);
    void make_graph(int w, int h, QPainter *p, bool test = false);
    Procview *procview;

  private:
    // history points are stored in fixed point, scaled by history_scale
    static const unsigned int history_scale = 256;
    static const int time_dist = 60; // time between vertical lines (seconds)

    QList<gwidget *> wlist;
    int hist_size;  // history buffer size DEL
    int npoints;    // nr of history points currently remembered
    float *history; // (circular) history buffer DEL
    int h_index;    // next history index to use
    float peak;     // largest value in history

    bool dirty; // history changed since pixmap was drawn
    QPixmap pixmap;
    QPixmap icon_pm;

    int px, py; //	pointer position

    bool is_vertical;
    int official_height;
};

class GraphBase : public QWidget
{
    Q_OBJECT
  public:
    GraphBase(QWidget *parent, Procview *);
    void drawGraphOnPixmap();

  protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;
    //	virtual	void enterEvent ( QEvent * event ) ;
    //	void resizeEvent ( QResizeEvent * e );

    virtual void make_graph(int w, int h, QPainter *p);
    virtual QString doHistoryTXT(SysHistory *sysh);

    Procview *procview;
    int official_height;
    int npoints; // nr of history points currently remembered
  private:
    // history points are stored in fixed point, scaled by history_scale
    static const unsigned int history_scale = 256;

    int hist_size;  // history buffer size DEL
    float *history; // (circular) history buffer DEL
    int h_index;    // next history index to use
    float peak;     // largest value in history

    bool dirty; // history changed since pixmap was drawn
    QPixmap pixmap;
    QPixmap icon_pm;

    int px, py; //	pointer position
};

class IO_Graph : public GraphBase
{
    Q_OBJECT
  public:
    IO_Graph(QWidget *parent, Procview *);

  protected:
    void make_graph(int w, int h, QPainter *p) override;
    QString doHistoryTXT(SysHistory *sysh) override;

  private:
};

// class Infobar2 : public Infobar
class Infobar2 : public QFrame
{
    Q_OBJECT
  public:
    Infobar2(QWidget *parent, Procview *);
    void updatePixmap(); // TEMP
    QPixmap *load_icon(int w, int h) { return basic->make_icon(w, h); }
    void refresh(); // update();
    Infobar *basic;
    IO_Graph *io_graph;

  private:
    int official_height;
};

class PDisplay : public QWidget
{
    Q_OBJECT
  public:
    PDisplay(QWidget *parent);
    Infobar2 *addSystem(Procview *p);
};

#endif // INFOBAR_H
