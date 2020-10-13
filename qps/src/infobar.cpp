/*
 * infobar.cpp
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

#include <cstdio>
#include <sys/types.h>
#include <ctime>

#include "infobar.h"
#include "proc.h"
#include "qps.h"
#include "misc.h"
#include <QBitmap>

extern ControlBar *controlbar;
extern TFrame *infobox; // testing
extern Qps *qps;        // testing

int history_start_idx = 0; // testing

float cpu_total = 0;
float cpu_idle = 0;
float cpu_used = 0;

QMenu *m_popup;
subcpuRack *cpubar = nullptr; // DEL extra CPU_window

// ============================
gwidget *x_cpu;
gwidget *x_mem;
gwidget *x_swap;
gwidget *x_utime;
gwidget *x_load_avg;

class VCursor
{
  public:
    int px;
    int py;
    int idx;
    bool enable;
};
VCursor vcursor;

// DEL
void Infobar::hideEvent(QHideEvent * /*event*/)
{
    // printf("Infobar::hideEvent()\n");
    // cpubar->hide();
}

// return true if the swap meter is redlined
bool Infobar::swaplim_exceeded()
{
    /*
    if(Qps::swaplim_percent) {
            if(procview->swap_total > 0) {
                    int free_p = 100 * procview->swap_free /
    procview->swap_total;
                    return free_p < Qps::swaplimit;
            } else
                    return false;
    } else {
            return procview->swap_free < Qps::swaplimit;
    } */
    return false;
}

char rotate_str[] = "|/-\\|/-\\";
int rotate_idx = 0;
char rotate_char = '|';

// who call ?
// DEL
void Infobar::refresh()
{
    if (rotate_str[++rotate_idx] == 0)
        rotate_idx = 0;
    rotate_char = rotate_str[rotate_idx];

    if (Qps::show_load_graph)
        update();
}

// DEL
// public : add point,, update Pixmap(graph)
//  called by  Qps::refresh()
void Infobar::update_load()
{
    // add_history_point2(cpu_used);
    // NEED SOLUTION!!!!!!!! draw graph before exposed
    if (isVisible())
        drawGraphOnPixmap();
    // MouseCurosrCheck
}

void Infobar::drawGraphOnPixmap()
{
    //	if(isVisible()==false) return;
    if (size() != pixmap.size())
    {
        /// printf("icon_pm changed!!\n");
        pixmap = QPixmap(size());  // resize(w, h);
        pixmap.setMask(QBitmap()); // clear the mask for drawing
    }

    QPainter p(&pixmap);
    p.fillRect(rect(), QBrush(Qt::black)); // p.fill(Qt::black);

    for (int i = 0; i < wlist.size(); i++)
        wlist[i]->draw(&p);

    make_graph(width(), height(), &p);
}

// if p, then show A and hide B; otherwise do the opposite
// DEL?
void Infobar::showup()
{
    // if(cpubar->isVisible())	cpubar->raise();
}

// DEL?
void Infobar::show_and_hide(bool p, QWidget *a, QWidget *b)
{
    if (!p)
    {
        QWidget *c = a;
        a = b;
        b = c;
    }
    if (!a->isVisible())
    {
        b->hide();
        a->show();
    }
}

#define TOTAL_BAR 30
QColor total_color;
QColor part1_color;
QColor part2_color;
QColor part3_color;
// DRAFT CODE : DRAFT CODE !!
// return width
int drawSPECTRUM(QPainter *p, int x, int y, const char *name, int total,
                 int part1, int part2 = 0, int part3 = 0, int h = 0)
{
    int total_width;
    // bluelay  0,180,255
    // Move to Infor::Inforbar()
    total_color.setRgb(0, 65, 55);
    part1_color.setRgb(0, 255, 210);
    part2_color.setRgb(0, 185, 150);
    part3_color.setRgb(0, 160, 120);

    int i, w;
    int tx, ty, bar_xoffset, bar_h;
    int bar;

    char buff[32] = "NO";

    if (total == 0)
    {
        name = strcat(buff, name);
    }

    if (h == 0)
        h = pf_char_height();

    w = 2 + pf_write(p, x + 2, y + 2, name) + 3;
    bar_xoffset = x + w;

    if (total == 0)
        return w; //**

    // draw total_dark_null line
    bar_h = h - 1;
    ty = y + 2;
    tx = 0;
    p->setPen(total_color);
    for (i = 0; i < TOTAL_BAR; i++)
    {
        p->drawLine(bar_xoffset + tx, ty, bar_xoffset + tx, ty + bar_h);
        tx++;
        tx++;
    }
    total_width = w + tx;

    if (total == 0)
        total = 1;
    tx = 0;
    // draw part1
    p->setPen(part1_color);
    bar = part1 * TOTAL_BAR / total;
    if (bar == 0 and part1 != 0)
        bar = 1;
    for (i = 0; i < bar; i++)
    {
        p->drawLine(bar_xoffset + tx, ty, bar_xoffset + tx, ty + bar_h);
        tx += 2;
        // tx++;
    }

    if (part2 >= 0)
    {
        p->setPen(part2_color);
        bar = part2 * TOTAL_BAR / total;
        if (bar == 0 and part2 != 0)
            bar = 1;
        for (i = 0; i < bar; i++)
        {
            p->drawLine(bar_xoffset + tx, ty, bar_xoffset + tx, ty + bar_h);
            tx += 2;
            // tx++;
        }
    }
    if (part3 >= 0)
    {
        p->setPen(part3_color);
        bar = part3 * (TOTAL_BAR) / total;
        for (i = 0; i < bar; i++)
        {
            p->drawLine(bar_xoffset + tx, ty, bar_xoffset + tx, ty + bar_h);
            tx += 2;
        }
    }
    // this occured by reporter.
    // if(tx>TOTAL_BAR*2 ) printf("Error: %s total =%d, part1=%d, part2=%d
    // part3=%d\n",name,total,part1,part2,part3);
    return total_width;
}

int drawSPECTRUM2(QPainter *p, int x, int y, char *name, int total, int part1,
                  int part2 = 0, int part3 = 0, int bar_h = 9,
                  int bar_w = TOTAL_BAR)
{
    // Notice:
    // 		total=1, part1=1 occurs

    int total_width;
    // bluelay  0,180,255
    // Move to Infor::Inforbar()
    total_color.setRgb(0, 67, 75); // dark
    part1_color.setRgb(0, 255, 210);
    part2_color.setRgb(0, 220, 175);
    part3_color.setRgb(0, 180, 150);

    int i, w;
    int tx, ty, bar_xoffset;
    int bar;

    char buff[32] = "NO";

    if (total == 0)
    {
        // printf("total=0\n") ; //occur
        name = strcat(buff, name);
    }

    w = 2 + pf_write(p, x + 2, y + 2, name) + 2;
    bar_xoffset = x + w;

    if (total == 0)
        return w; //**

    // draw total_dark_null line
    bar_h = bar_h - 1; // if(h==0)  h=pf_char_height(); //9

    ty = y + 2;
    tx = 0;
    // draw Total_dark_bar
    p->setPen(total_color);
    for (i = 0; i < bar_w; i++)
    {
        p->drawLine(bar_xoffset + tx, ty, bar_xoffset + tx, ty + bar_h);
        tx += 2;
    }
    total_width = w + tx;

    if (total == 0)
        total = 1; // safer divide error.

    tx = 0;
    // draw part1
    p->setPen(part1_color);
    bar = part1 * bar_w / total;
    if (bar == 0 and part1 != 0)
        bar = 1;
    for (i = 0; i < bar; i++)
    {
        p->drawLine(bar_xoffset + tx, ty, bar_xoffset + tx, ty + bar_h);
        tx += 2;
        // tx++;
    }

    if (part2 >= 0)
    {
        p->setPen(part2_color);
        bar = part2 * bar_w / total;
        if (bar == 0 and part2 != 0)
            bar = 1;
        for (i = 0; i < bar; i++)
        {
            p->drawLine(bar_xoffset + tx, ty, bar_xoffset + tx, ty + bar_h);
            tx += 2;
            // tx++;
        }
    }
    if (part3 >= 0)
    {
        p->setPen(part3_color);
        bar = part3 * (bar_w) / total;
        for (i = 0; i < bar; i++)
        {
            p->drawLine(bar_xoffset + tx, ty, bar_xoffset + tx, ty + bar_h);
            tx += 2;
        }
    }
    // this occured by reporter.
    if (tx > TOTAL_BAR * 2)
    {
        printf("Error: TOTAL_BAR*2=%d, name= %s total =%d, part1=%d, "
               "part2=%d "
               "part3=%d\n",
               TOTAL_BAR * 2, name, total, part1, part2, part3);
        return -1;
    }
    //	if(part1<0){ printf("total=%d part1=%d
    // part2=%d\n",total,part1,part2);
    // return -1;}
    // printf("%s total =%d, part1=%d, part2=%d
    // part3=%d\n",name,total,part1,part2,part3);
    return total_width;
}

char str_buff[512];
#define TIMEDIFF(kind)                                                         \
    procview->cpu_times(cpu_id, Proc::kind) -                                  \
        procview->old_cpu_times(cpu_id, Proc::kind)
class w_cpu : public gwidget
{
  private:
    int cpu_n;
    long total, user, system, idle, nice, wait;

  public:
    void draw(QPainter *p) override
    {
        x = 0;
        char buff[32];
        int cpu_id;
        int cpu_n;
        int gheight = 10;
        int mw;

        user = 0, system = 0, idle = 0, nice = 0, wait = 0;

        cpu_n = procview->num_cpus;

        width = 0;

        // if(cpubar->isVisible() and cpu_n>3)
        if (true)
        {
            // show Total CPU
            cpu_id = cpu_n;
            user = TIMEDIFF(CPUTIME_USER);
#ifdef LINUX
            nice = TIMEDIFF(CPUTIME_NICE);
#endif
            system = TIMEDIFF(CPUTIME_SYSTEM);
#ifdef SOLARIS
            wait = TIMEDIFF(CPUTIME_WAIT);
#endif
            idle = TIMEDIFF(CPUTIME_IDLE);
            total = user + system + wait + nice + idle;
            width = drawSPECTRUM(p, 0, 0, "CPU", total, user, system, nice,
                                 gheight - 1);
            //	width+=5;
        }

        for (cpu_id = 0; cpu_id < cpu_n; cpu_id++)
        {

            //	if(procview->num_cpus == procview->old_num_cpus)
            user = TIMEDIFF(CPUTIME_USER);
            idle = TIMEDIFF(CPUTIME_IDLE);
            system = TIMEDIFF(CPUTIME_SYSTEM);
#ifdef LINUX
            nice = TIMEDIFF(CPUTIME_NICE);
#endif
#ifdef SOLARIS
            wait = TIMEDIFF(CPUTIME_WAIT);
#endif

#ifdef LINUX
            total = user + system + nice + idle;
#endif
#ifdef SOLARIS
            total = user + system + wait + idle;
#endif

            if (cpu_n > 1 and cpu_n < 129) // 9~16
            {
                int bar_w = 0;
                if (cpu_n <= 2)
                    bar_w = TOTAL_BAR; // default
                else if (cpu_n < 9)
                    bar_w = TOTAL_BAR / 2;
                else // if(cpu_n<17)
                    bar_w = TOTAL_BAR / 3;
                // else bar_w=13;

                int r, c;
                c = cpu_id / 2;
                r = cpu_id % 2;
                sprintf(buff, "%d", cpu_id);
                mw = 4 * 3 + (bar_w * 2 + 8 * 2 - 1) * c;
                if (c >= 1)
                    mw -= 6;
                if (c >= 2)
                    mw -= 6;

                if (false)
                {
                    printf("cpuid %d , user %d , %d \n", 1,
                           procview->cpu_times(1, Proc::CPUTIME_USER),
                           procview->old_cpu_times(0, Proc::CPUTIME_USER));
                }

#ifdef LINUX
                (void) drawSPECTRUM2(p, mw, gheight + 1 + r * (gheight - 1),
                                      buff, total, user, system, nice,
                                      gheight - 2, bar_w);
#endif

#ifdef SOLARIS
                (void) drawSPECTRUM2(p, mw, gheight + 1 + r * (gheight - 1),
                                      buff, total, user, system, wait,
                                      gheight - 2, bar_w);
// drawSPECTRUM(p,0,cpu_id*10,buff,total,user,system,wait);
#endif
            }
        }
        p->fillRect(0, gheight + 2, 50 + mw + 100, gheight * 4,
                    QColor(0, 0, 0, 130));
        height = gheight * 1;
    }

    char *info() override
    {
        float f_user, f_nice, f_system, f_wait;

        f_user = (float)user / total * 100;
        f_nice = (float)nice / total * 100;
        f_wait = (float)wait / total * 100;
        f_system = (float)system / total * 100;
#ifdef LINUX
        sprintf(str_buff, "user: %1.1f%%  system:%1.1f%%  nice:%1.1f%% ",
                f_user, f_system, f_nice);
#endif

#ifdef SOLARIS
        sprintf(str_buff, "user: %1.1f%%  system:%1.1f%%  nice:%1.1f%% ",
                f_user, f_system, f_wait);
#endif
        return str_buff;
    };
};

class w_mem : public gwidget
{
  private:
    int used;

  public:
    void draw(QPainter *p) override
    {
        height = pf_char_height() + 4;
        x = x_cpu->xpluswidth() + 10;
#ifdef LINUX
        used = procview->mem_total - procview->mem_free -
               procview->mem_buffers - procview->mem_cached;
        width = drawSPECTRUM(p, x, 0, "MEM", procview->mem_total, used,
                             procview->mem_cached, procview->mem_buffers);
#endif

#ifdef SOLARIS
        used = procview->mem_total - procview->mem_free;
        width = drawSPECTRUM(p, x, 0, "MEM", procview->mem_total, used);
#endif
    }
    char *info() override
    {
        char str[80];

        strcpy(str_buff, "Total: ");
        mem_string(procview->mem_total, str);
        strcat(str_buff, str);

        strcat(str_buff, "  used: ");
        mem_string(used, str);
        strcat(str_buff, str);

#ifdef LINUX
        strcat(str_buff, "  cached: ");
        mem_string(procview->mem_cached, str);
        strcat(str_buff, str);
        strcat(str_buff, "  buffer: ");
        mem_string(procview->mem_buffers, str);
        strcat(str_buff, str);
#endif

        //	sprintf(str_buff,"Total: %dKb , cache: %dKb , buffer:
        //%dKb",
        //		procview->mem_total,procview->mem_cached,procview->mem_buffers);
        return str_buff;
    };
};

class w_swap : public gwidget
{
  private:
    int used;

  public:
    void draw(QPainter *p) override
    {
        x = x_mem->xpluswidth() + 10;
        used = procview->swap_total - procview->swap_free;
        width = drawSPECTRUM(p, x, 0, "SWAP", procview->swap_total, used);
        height = pf_char_height() + 4;
    }
    char *info() override
    {
        char str[80];

        strcpy(str_buff, "Total: ");
        mem_string(procview->swap_total, str);
        strcat(str_buff, str);
        strcat(str_buff, "  Free: ");
        mem_string(procview->swap_free, str);
        strcat(str_buff, str);
        strcat(str_buff, "  Used: ");
        mem_string(used, str);
        strcat(str_buff, str);

        // sprintf(str_buff,"Total: %d Kbyte , used %d Kbyte",
        //		procview->swap_total,procview->swap_free);
        return str_buff;
    };
};

// DRAFT CODE  !!
int drawUTIME(QPainter *p, int x, int y, long boot_time)
{
    char buff[1024];
    // printf("size of long=%d, size of time_t=%d
    // \n",sizeof(long),sizeof(time_t));
    long u = (long)time(nullptr) - (long)boot_time;
    int up_days = u / (3600 * 24);
    u %= (3600 * 24);
    int up_hrs = u / 3600;
    u %= 3600;
    int up_mins = u / 60;
    int sec = u % 60;
    if (up_days == 0)
    {
        if (up_hrs == 0)
            sprintf(buff, "UPTIME %d:%02d", up_mins, sec);
        else
            sprintf(buff, "UPTIME %d:%02d:%02d", up_hrs, up_mins, sec);
    }
    else
        sprintf(buff, "UPTIME %dDAY%s,%d:%02d:%02d", up_days,
                (up_days == 1) ? "" : "s", up_hrs, up_mins, sec);
    return pf_write(p, x, y, buff);
}

class w_utime : public gwidget
{
  public:
    void draw(QPainter *p) override
    {
        height = pf_char_height() + 4;
        x = x_swap->xpluswidth() + 10;
        width = drawUTIME(p, x, 2, procview->boot_time);
    }
    const char *info() override { return "passed time after system booting"; };
};

class w_load_avg : public gwidget
{
    void draw(QPainter *p) override
    {
        char buff[64];
        // printf("w_load_avg\n");
        // sprintf(buff,"QPS %3.02f%%", Procinfo::loadQps);
        sprintf(buff, " 1m:%1.02f 5m:%1.02f 15m:%1.02f", procview->loadavg[0],
                procview->loadavg[1], procview->loadavg[2]);

        width = pf_str_width(buff);

        x = parent->width() - width - 6;

        int w = x_utime->xpluswidth() + 15;
        if (x < w)
            x = w;

        pf_write(p, x, 2, buff);

        //
        x = parent->width() - 8;
        y = parent->height() - 9;

        char str[2] = {0, 0};
        str[0] = rotate_char;
        pf_write(p, x, y, str);
    }
    const char *info() override
    {
        return "Average CPU%% each 1, 5 ,15 minutes";
    };
};

GraphBase::GraphBase(QWidget * /*parent*/, Procview *pv)
{
    procview = pv;
    // setCursor ( QCursor(Qt::CrossCursor) ) ;
    npoints = 0, peak = 0, h_index = 0, dirty = true;
    official_height = 39;

    hist_size = 1280;
    history = new float[hist_size];

    setMinimumHeight(24);

    QWidget::setMouseTracking(true);
}

IO_Graph::IO_Graph(QWidget *parent, Procview *pv) : GraphBase(parent, pv)
{
    setMinimumHeight(22);
}

void GraphBase::make_graph(int w, int h, QPainter *p)
{
    float ratio = h;

    int hsize = procview->history.size();
    int start = hsize - w;

    if (start < 0)
        start = 0;

    QPolygon pa(hsize - start); // QVector pa(npts);

    int idx = 0;
    for (int i = start; i < procview->history.size(); i++, idx++)
    {
        SysHistory *hist = procview->history[i];
        // printf("[%d] hist =%f\n",i,hist->load_cpu);
        pa[idx] = QPoint(idx, h - 1 - (int)(hist->load_cpu * ratio));
    }

    if (h == official_height)
    {
        history_start_idx = start; // for MousePointer!!
        npoints = idx;             // printf("x npoints=%d \n",npoints);
    }

    // draw scale lines
    p->setPen(QColor(0, 210, 100)); // p.setPen(QColor(0,70,54));
    p->drawPolyline(pa);

    dirty = false;
}

void IO_Graph::make_graph(int w, int h, QPainter *p)
{
    // p->fillRect(0,0,w,h,QBrush(Qt::black));
    // p->fill(Qt::black);
    float ratio = 1.3; // test

    int hsize = procview->history.size();
    int start = hsize - w;

    if (start < 0)
        start = 0;

    p->setPen(QColor(80, 90, 254)); // BLUE color

    int idx = 0;
    for (int i = start; i < procview->history.size(); i++, idx++)
    {
        SysHistory *hist = procview->history[i];
        // printf("[%d] hist =%f\n",i,hist->load_cpu);
        p->drawLine(idx, h - 1, idx, h - 1 - (int)(hist->load_io * ratio));
    }

    // if(h==official_height)  // jump not init!!!
    {
        history_start_idx = start; // for MousePointer!!
        npoints = idx;             // printf("x npoints=%d \n",npoints);
    }
}

void GraphBase::drawGraphOnPixmap()
{
    //
    if (rotate_str[++rotate_idx] == 0)
        rotate_idx = 0;
    rotate_char = rotate_str[rotate_idx];

    //	if(isVisible()==false) return;
    if (size() != pixmap.size())
    {
        /// printf("icon_pm changed!!\n");
        pixmap = QPixmap(size());  // resize(w, h);
        pixmap.setMask(QBitmap()); // clear the mask for drawing
    }

    QPainter p(&pixmap);
    p.fillRect(rect(), QBrush(Qt::black)); // p.fill(Qt::black);

    make_graph(width(), height(), &p);
}

// DRAFT CODE  !!!
void Infobar::paintEvent(QPaintEvent *e)
{
    // printf("Infobar()::paintEvent\n");
    QRect ur = e->rect(); // update rectangle
    QPainter p(this);

    // full re-draw! by update(); ???
    // if( ur.width()==width() and ur.height()==height() )
    {
        ///	drawGraphOnPixmap();
    }

    p.drawPixmap(ur, pixmap, ur);

    return;
    // drww VCursor
    if (vcursor.enable)
    {
        /// int px = vcursor.px;
        ///	p.setPen(QColor(80,195,80));
        ///	p.drawLine (px,0,px,height());
    }
}

void GraphBase::paintEvent(QPaintEvent *e)
{
    QRect ur = e->rect(); // update rectangle
    QPainter p(this);

    // full re-draw! by update();
    if (ur.width() == width() and ur.height() == height())
    {
        drawGraphOnPixmap();
    }

    p.drawPixmap(ur, pixmap, ur);

    return;

    // if(vcursor.enable);
    // ------- Cursor testing ---------
    p.setPen(QColor(250, 159, 5));
    int rel_x = x();
    p.drawLine(vcursor.px - rel_x, 0, vcursor.px - rel_x, height());
}

/* DEL
void IO_Graph::paintEvent ( QPaintEvent *e )
{
        GraphBase::paintEvent (e);
} */

// TODO: 1.sort 2. time(?)
QString doHistory(SysHistory *sysh)
{
    QString str;

    char buf[128];
    // sprintf(buf,"miniHistory /* %.02f%%",sysh->load_cpu*100);
    sprintf(buf, "miniHistory CPU");
    str += QString::fromLatin1(buf);

    // void linearize_tree(QVector<Procinfo *> *ps, int level, int prow,
    // bool
    // hide)
    // qsort(ps->data(), ps->size(), sizeof(Procinfo *),(compare_func)
    // compare_backwards);

    for (const auto *p : qAsConst(sysh->procs))
    {
        if (p->pcpu == 0)
            continue;
        sprintf(buf, " (%.01f%%)", p->pcpu);
        str += "\n" + p->command + QString::fromLatin1(buf);
    }

    return str;
}

// TODO: name change!
QString GraphBase::doHistoryTXT(SysHistory *sysh)
{
    QString str;

    char buf[128];
    // sprintf(buf,"miniHistory /* %.02f%%",sysh->load_cpu*100);
    sprintf(buf, "%%CPU miniHistory test");
    str += QString::fromLatin1(buf);
    for (const auto *p : qAsConst(sysh->procs))
    {
        if (p->pcpu == 0)
            continue;
        sprintf(buf, " (%.02f%%)", p->pcpu);
        str += "\n" + p->command + QString::fromLatin1(buf);
    }
    return str;
}

void GraphBase::mouseMoveEvent(QMouseEvent *e)
{
    int half_height = height() / 2;
    int dy;

    px = e->pos().x(); // x in Infobar
    py = e->pos().y(); // y in Infobar

    dy = py - half_height; //
    dy /= 2;

    // printf("px=%d py=%d  y=%d h=%d , dy=%d\n",px,py,y,height(),dy);
    // gap=infobox->width() + px  - width();

    // printf("procview npoints=%d  px=%d\n",npoints,px);
    QString text;
    int idx = px + history_start_idx;
    // if(px<npoints) //h_index, npoints
    if (idx < procview->history.size() and idx >= 0)
    {
        //	printf("procview idx=%d px=%d\n",idx,px);
        text += doHistoryTXT(procview->history[idx]);
    }
    else
    {
        // text ="xxx";
    }

    infobox->setText(text);

    QPoint p = mapTo(qps, e->pos()); //??

    // infobox->move(a.x()+16,a.y()+4);
    // infobox->setPos(p.x()+16,p.y()+4);
    infobox->setPos();

    // PROBLEM : leaving old vcursor ...
    vcursor.px = p.x();
    vcursor.py = p.y();
    int rel_x = GraphBase::x();
    update(p.x() - 5 - rel_x, 0, p.x() + 5 - rel_x,
           height()); // only Vcursor update
}

// TODO: 1.sort 2. time
QString IO_Graph::doHistoryTXT(SysHistory *sysh)
{
    QString str;

    char buf[64], mem_str[64];
    // sprintf(buf,"miniHistory /* %.02f%%",sysh->load_cpu*100);
    sprintf(buf, "miniHistory IO");
    str += QString::fromLatin1(buf);

    for (const auto *p : qAsConst(sysh->procs))
    {
        if (p->io_read_KBps == 0 and p->io_write_KBps == 0)
            continue;
        buf[0] = 0;

        str += "\n" + p->command;
        // str+="\n"+ p->command + QString::fromLatin1(buf);
        str += " (";

        if (p->io_read_KBps)
        {
            mem_string_k(p->io_read_KBps, mem_str);
            sprintf(buf, "%s/s read", mem_str);
            str += QString::fromLatin1(buf);
        }

        if (p->io_write_KBps)
        {
            mem_string_k(p->io_write_KBps, mem_str);
            sprintf(buf, " %s/s write", mem_str);
            str += QString::fromLatin1(buf);
        }
        str += ")";
    }
    return str;
}

void GraphBase::leaveEvent(QEvent *) { infobox->hide(); }

/*
void IO_Graph::mouseMoveEvent ( QMouseEvent *e ) {
        GraphBase::mouseMoveEvent(e);
} */

void GraphBase::mousePressEvent(QMouseEvent * /*e*/) {}

Infobar::Infobar(QWidget *parent, Procview *pv) : QFrame(parent)
{
    procview = pv;
    official_height = 32;

    // setCursor ( QCursor(Qt::CrossCursor) ) ;
    {
        npoints = 0, peak = 0, h_index = 0, dirty = true; //

        hist_size = 1280;
        history = new float[hist_size];

        // setBackgroundRole (QPalette::WindowText);
        setAutoFillBackground(false);
        ////check setAttribute(Qt::WA_OpaquePaintEvent);
        // setFrameShape(QFrame::Panel);
        setFrameShadow(QFrame::Sunken);
        setMinimumHeight(official_height);
        // setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Fixed);
        // setStyleSheet("QFrame { background-color: yellow }");
        x_cpu = new w_cpu();
        x_cpu->setParent(this, pv);
        x_mem = new w_mem();
        x_mem->setParent(this, pv);
        x_swap = new w_swap();
        x_swap->setParent(this, pv);
        x_utime = new w_utime();
        x_utime->setParent(this, pv);
        x_load_avg = new w_load_avg();
        x_load_avg->setParent(this, pv);

        wlist.append(x_cpu);
        wlist.append(x_mem);
        wlist.append(x_swap);
        wlist.append(x_utime);
        wlist.append(x_load_avg);

        QWidget::setMouseTracking(true);
    } //

    // is_vertical = Qps::vertical_cpu_bar; //DEL

    //	cpubar=new subcpuRack(parent,pv);

    /*	m_popup = new QMenu("popup",this);
            QAction *act=new QAction("Under Development",this);
            act->setDisabled(true);
            m_popup->addAction(act);
    */
}

Infobar::~Infobar() { delete[] history; }

// a System's Info bar
Infobar2::Infobar2(QWidget * /*parent*/, Procview *pv)
{
    // procview=pv; //***
    official_height = 35;
    // setAutoFillBackground ( false );
    // setAttribute(Qt::WA_OpaquePaintEvent);
    // setFrameShape(QFrame::Panel);
    setFrameShadow(QFrame::Sunken);
    // setMinimumHeight(official_height);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    //	if(layout()) delete layout();

    QBoxLayout *layout = new QHBoxLayout();

    setLayout(layout);

    layout->setSpacing(1); // betweeen gap
    layout->setMargin(0);

    // BasicInfo *basic=new BasicInfo(this,pv);
    //	layout->addWidget(basic);

    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->setSpacing(1); // betweeen gap
    vlayout->setMargin(0);

    layout->addLayout(vlayout);
    // layout->setContentsMargins (1,1,1,1);
    // layout->setContentsMargins ( int left, int top, int right, int bottom
    // );

    // GraphBase *gp=new GraphBase(this,pv);  // %CPU
    basic = new Infobar(this, pv);
    vlayout->addWidget(basic);

    // IO_Graph
    io_graph = new IO_Graph(this, pv); // %IO
    vlayout->addWidget(io_graph);
    // basic-hide();
    // io_graph->hide();
    m_popup = new QMenu("popup", this);
    QAction *act = new QAction("Under Development", this);
    act->setDisabled(true);
    m_popup->addAction(act);
}

void Infobar2::updatePixmap()
{
    // NEED SOLUTION!!!!!!!! draw graph before exposed?
    if (isVisible())
    {
        basic->drawGraphOnPixmap();
        io_graph->drawGraphOnPixmap();
    }
}

// old style : relative method
// add value to the history, updating peak.
// USING
void Infobar::add_history_point(unsigned int value)
{
    // simul circular buffer
    history[h_index++] = value; // float
    if (h_index >= hist_size)
        h_index = 0;
    if (npoints < hist_size)
        npoints++;
}

// BACKUP: add value to the history, updating peak.
void Infobar::add_history_point2(float value)
{
    static float last_val = 0;

    float f;
    f = last_val - value;
    // if (f > 0)
    //	value+=f/1.15;  // slow up ,slow down
    // else
    value += f / 1.5; // slow up ,slow down
    history[h_index++] = value;
    if (h_index >= hist_size)
        h_index = 0;
    if (npoints < hist_size)
        npoints++;
    if (value > peak)
        peak = value;
    else
    {
        peak = 0; // no negative values
        for (int i = 0; i < npoints; i++)
            if (history[i] > peak)
                peak = history[i];
    }
    // printf("hist_size=%d h_index=%d  val=%f\n",hist_size,h_index,value);

    last_val = value;
}

// return updated pixmap for use as an icon
QPixmap *Infobar::make_icon(int w, int h)
{
    if (w != icon_pm.width() or h != icon_pm.height())
    {
        /// printf("icon_pm changed!!!!!!!!!!!!!!!!!!11\n");
        icon_pm = QPixmap(w, h);    // pm.resize(w, h);
        icon_pm.setMask(QBitmap()); // remove the mask for drawing
    }
    QPainter pt(&icon_pm);
    pt.fillRect(0, 0, w, h, QBrush(Qt::black));

    if (false)
    {
        /*	int thick=h/10;
                int bottom=h/4;

                pt.setClipRect();
                pt.translate(); */
    }

    make_graph(w, h, &pt, true);
    return &icon_pm;
}

void Infobar::mousePressEvent(QMouseEvent *e)
{
    emit clicked();
    if (e->button() == Qt::LeftButton)
    {
        vcursor.enable = true;
        return;
    }

    if (e->button() == Qt::RightButton)
        m_popup->popup(e->globalPos());
}

// only works if mouse cursor in this area
void Infobar::mouseMoveEvent(QMouseEvent *e)
{
    int half_height = height() / 2;
    int dy;
    // int gap;

    px = e->pos().x(); // x in Infobar
    py = e->pos().y(); // y in Infobar

    dy = py - half_height; //
    dy /= 2;

    // printf("px=%d py=%d  y=%d h=%d , dy=%d\n",px,py,y,height(),dy);
    // gap=infobox->width() + px  - width();

    int i;
    int setinfo = 0;
    for (i = 0; i < wlist.size(); i++)
        if (wlist[i]->intersect(px, py))
        {
            setinfo = 1;
            infobox->setText(wlist[i]->info());
            break;
        }

    // printf("procview npoints=%d  px=%d\n",npoints,px);
    QString text;
    int idx = px + history_start_idx;
    // if(px<npoints) //h_index, npoints
    if (idx < procview->history.size() and idx >= 0)
    {
        //	printf("procview idx=%d px=%d\n",idx,px);
        text += doHistory(procview->history[idx]);
    }
    else
    {
        text = "";
    }

    if (setinfo == 0)
        infobox->setText(text);

    // QPoint p = mapTo(qps, e->pos()); //??

    // infobox->move(a.x()+16,a.y()+4);
    // infobox->setPos(p.x()+16,p.y()+4);
    infobox->setPos();

    //	vcursor.px=p.x();
    //	update(p.x()-5,0,p.x()+5,height());
}

void Infobar::enterEvent(QEvent *)
{
    //	infobox->hide();
}

void Infobar::leaveEvent(QEvent *)
{

    infobox->hide();

    return;
    /*
    if(controlbar and controlbar->isHidden())
    {
            // controlbar->show();
            setMinimumHeight(official_height);
    } */
}

// DRAFT CODE
// draw the load graph on the internal pixmap, if needed
// called by
// 		1.make_icon(int w, int h)
// 		2.paintEvent()
// 		3.update_load()
void Infobar::make_graph(int w, int h, QPainter *p, bool test)
{
    // QPainter p(this);
    // p.setBackgroundMode (Qt::OpaqueMode);
    // p.setBackground(QBrush(Qt::black));
    // p->fillRect(0,0,w,h,QBrush(Qt::black));
    // p->fill(Qt::black);
    float ratio = h;
    int idx = 0;

    if (h == official_height) // tmp
    {
        ratio = h - 11;
    }

    int hsize = procview->history.size();
    int start = hsize - w;

    if (start < 0)
        start = 0;
    p->setPen(QColor(0, 210, 100)); // p.setPen(QColor(0,70,54));

    if (test == false)
    {
        QPolygon pa(hsize - start); // QVector pa(npts);
        //	QPolygon point_array_io(hsize-start);

        for (int i = start; i < procview->history.size(); i++, idx++)
        {
            SysHistory *hist = procview->history[i];
            // printf("[%d] hist =%f\n",i,hist->load_cpu);
            pa[idx] = QPoint(idx, h - 1 - (int)(hist->load_cpu * ratio));
            ///	point_array_io[idx] = QPoint(idx, h - 1 -
            ///(int)(hist->load_io *
            /// ratio));
        }
        //	p->setPen(QColor(100,100,250));
        ////p.setPen(QColor(0,70,54));
        //	p->drawPolyline(point_array_io);

        // draw scale lines
        p->drawPolyline(pa);
    }
    else
    {
        int idx = 0;
        for (int i = start; i < procview->history.size(); i++, idx++)
        {
            SysHistory *hist = procview->history[i];
            p->drawLine(idx, h - 1, idx, h - 1 - (int)(hist->load_cpu * ratio));
        }
    }

    //		if(h==official_height)
    {
        history_start_idx = start; // for MousePointer!!
        npoints = idx;             // printf("x npoints=%d \n",npoints);
    }

    dirty = false;
}

void Infobar::resizeEvent(QResizeEvent * /*e*/)
{
    // static int first=0; if(first==0){	drawPixmap(); first=1;}
    drawGraphOnPixmap();
}
subcpuRack::subcpuRack(QWidget *p, Procview *pv) : QWidget(p)
{
    parent = p;
    procview = pv;
    // setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);
    // setWindowFlags(Qt::FramelessWindowHint |Qt::Tool );
    setAttribute(Qt::WA_OpaquePaintEvent);
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(0, 0, 0));
    setPalette(pal);
    QWidget::setMouseTracking(true);
    setMinimumHeight(12);
}

void subcpuRack::refresh() {}

void subcpuRack::mousePressEvent(QMouseEvent *e)
{
    m_popup->popup(e->globalPos());
    // hide();
}

// DRAFT CODE  !!!
void subcpuRack::paintEvent(QPaintEvent * /*e*/)
{

    // static QPaint *p=// QPainter *p=new QPainter(this);
    QPainter p(this);
    int w;
    // QRect cr = contentsRect();
    // QRect cr = p->viewport();
//    QRect cr = p.window(); // rect.

    p.fillRect(rect(), QBrush(Qt::black));

    // p->setPen(lineColor);
    p.setPen(QColor(50, 50, 50));
    p.drawLine(0, 0, QWidget::width(), 0);
    // p.fillRect(cr,QBrush(QColor(255,255,255,50)));

    // if(procview->num_cpus>=4) // temporaly...
    // else
    w = 2 + pf_write(&p, 2, 2, "SUB CPU");
}

// GraphDisplay, miniDisplay

PDisplay::PDisplay(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setMargin(0);
    vlayout->setSpacing(1);
    setLayout(vlayout);

    // return;
    // ** setBackgroundColor(color); **
    QPalette pal;
    pal = palette();
    setAutoFillBackground(true);
    // setBackgroundRole (QPalette::WindowText);
    pal.setColor(backgroundRole(), QColor(80, 80, 80));
    // palette.setColor(QPalette::Window, QColor(0,0,100));
    setPalette(pal);
}
// a BAR in a RACK
//

Infobar2 *PDisplay::addSystem(Procview *pv)
{
    //	Infobar* bar= new Infobar(this,pv);
    Infobar2 *bar2 = new Infobar2(this, pv);
    layout()->addWidget(bar2);

    /// pv->read_system(); //

    if (false and pv->num_cpus > 9)
    {
        QWidget *rack;
        rack = new subcpuRack(this, pv);
        layout()->addWidget(rack);
    }
    return bar2; //->basic;
}
