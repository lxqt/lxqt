/*
 * misc.cpp
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 2014 dae hyun, yang <daehyun.yang@gmail.com>
 * Copyright 2015 Paulo Lieuthier <paulolieuthier@gmail.com>
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


#include "misc.h"
#include "proc.h"

extern SearchBox *search_box;
extern bool flag_show_thread;
extern int flag_thread_ok;
extern bool flag_devel;

#include "../icon/x1.xpm"
#include "../icon/x2.xpm"

#include <cstdio>
#include <ctime>
#include <cerrno>
#include <cstdarg>
#include <cstring>

// 300% faster than glibc (by fasthyun@magicn.com)
int x_atoi(const char *sstr)
{
    const char *str = sstr;
    int val = 0;
    while (*str)
    {
        val *= 10;
        val += *str - 48;
        str++;
    }
    return val;
}

#include <QApplication>
QWidget *getQpsWidget()
{
    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = 0; i < list.size(); ++i)
    {
        QWidget *w = list.at(i);
        if (w->objectName() == "qps")
            return w;
    }
    return nullptr;
}

/*
  ver 0.2
  A simplified vsscanf implementation from Internet

  %S can read  char_string_has_spaces.
        ex. "(%S)" ==  (foo bar) or (foo bar )


  %s %d %ld %u %x? %lu %*s %f
  %S: (%S)

  Only recognizes %s %f,%d, %u, %ld, %lu, %lg and  whitespace. ...terminates
  scanning.
  space* same tab*

 */

// Description:
//   	mini_sscanf() == strstr() + sscanf() + %S
//
//  there is test_sample_code in init_misc()
// 	fixed : invalid conversion from ‘const char*’ to ‘char*’
//
// TOO COMPLEX !!!!!!  -> simple code
char *strchar(const char *s, int k);
int mini_sscanf(const char *s1, const char *fmt, ...)
{

    va_list va;
    va_start(va, fmt);
    char *s = (char *)s1; // *** for gcc 4.4
    char *p;
    int k = 0;          // count
    while (*fmt and *s) //	if (*fmt==0 or *s==0) break;
    {
        if (*fmt == '%')
        {
            int n;
            if (fmt[1] == 'S')
            {
                // if(fmt[2]!=0)
                p = strchr(s, fmt[2]); // get the token
                int len = p - s;
                p = va_arg(va, char *);
                strncpy(p, s, len);
                p[len] = 0;
                s += len + 1;
                fmt += 3;
                k++;
            }
            else if (fmt[1] == 's') // extract string
            {
                // printf("%s : %%s =%s \n",__FUNCTION__,s);
                k += sscanf(s, "%s%n", va_arg(va, char *), &n);
                s += n;
                fmt += 2;
            }
            else if (fmt[1] == '*' && fmt[2] == 's') // skip
            {
                p = strchr(s, ' '); // 0x20,0x00,0x0A, 123 435 54054
                if (p == nullptr)
                {
                    //	printf("%s : %c [%s]
                    //\n",__FUNCTION__,fmt[1],s);
                    break;
                }
                s = p;
                fmt += 3;
            }
            else if (fmt[1] == 'c')
            {
                p = va_arg(va, char *);
                //	printf("%s : %c [%c]
                //\n",__FUNCTION__,fmt[1],*s);
                *p = *s;
                s++;
                k++;
                fmt += 2;
            }
            else if (fmt[1] == 'f')
            {
                k += sscanf(s, "%f%n", va_arg(va, float *), &n);
                s += n;
                fmt += 2;
            }
            else if (fmt[1] == 'd')
            {
                k += sscanf(s, "%d%n", va_arg(va, int *), &n);
                s += n;
                fmt += 2;
            }
            else if (fmt[1] == 'u')
            {
                k += sscanf(s, "%u%n", va_arg(va, int *), &n);
                s += n;
                fmt += 2;
            }
            else if (fmt[1] == 'l' && fmt[2] == 'd')
            {
                k += sscanf(s, "%ld%n", va_arg(va, long *), &n);
                s += n;
                fmt += 3;
            }
            else if (fmt[1] == 'l' && fmt[2] == 'u')
            {
                k += sscanf(s, "%lu%n", va_arg(va, long *), &n);
                s += n;
                fmt += 3;
            }
            else if (fmt[1] == 'l' && fmt[2] == 'g')
            {
                k += sscanf(s, "%lg%n", va_arg(va, double *), &n);
                s += n;
                fmt += 3;
            }
            else
            {
                printf("%s: unsupported"
                       " format '%c'",
                       __FUNCTION__, fmt[1]);
                break;
            }
        }
        else
        {
            if (isspace(*fmt)) // isblank
            {
                while (isspace(*s))
                    s++;
                fmt++;
            }
            else
            {
                char sstr[32];
                int n = strcspn(fmt, " %\n"); // find delimiters noSEGFAULT
                strncpy(sstr, fmt, n);
                sstr[n] = 0;
                p = strstr(s, sstr);
                if (p == nullptr)
                {
                    if (false and flag_devel)
                        printf("%s : can't found [%s] "
                               "in %s\n",
                               __FUNCTION__, sstr, s);
                    break;
                }
                s = p + n;
                fmt += n;
                // ddd at %s, at%s
                // printf("opp_vsscanf: unexpected ""char in
                // format '%s'",fmt);
                // break;
                // return k;
            }
        }

        if (*fmt == '\0' || *fmt == '#' or *s == 0)
            break;
    }
    va_end(va);
    return k;
}

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

// return /proc/* file size
int fsize(char *fname)
{
    int size = 0;
    if (false)
    {
        // !!! important !! not works with [/proc] , because [/proc/*]
        // always zero
        int fd = open(fname, O_RDONLY);
        if (fd < 0)
            return -1;
        size = lseek(fd, 0, SEEK_END);
        printf("size=%d\n", size);
        close(fd);
        return size;
    }

    int buf[1024];
    int r;
    int fd = open(fname, O_RDONLY);
    if (fd < 0)
        return -1;
    do
    {
        r = read(fd, buf, 1024);
        size += r;
        //	printf("r_size=%d\n",size);
    } while (r);
    // printf("size=%d\n",size);
    close(fd);
    return size;
}

// sleep for mili seconds
void msleep(long msec)
{
#ifdef LINUX
    timespec req;
    timeval tv;
    req.tv_sec = 0;
    req.tv_nsec = msec * 1000000;
    // int  	nanosleep(const struct timespec *req, struct timespec
    // *remain)
    /*while (nanosleep(&req, &req))
    {
            if (errno != EINTR)
                    break;
    } */

    tv.tv_sec = 0;
    tv.tv_usec = msec * 1000;
    select(0, nullptr, nullptr, nullptr, &tv);
    return;
#endif
}

void mem_string_k(int kbytes, char *buf)
{
    if (kbytes >= 1024)
    {
        int mb = kbytes >> 10;
        if (mb >= 1024 * 100)
            sprintf(buf, "%uGB", mb >> 10);
        else
            sprintf(buf, "%uMB", mb);
    }
    else
        sprintf(buf, "%uKB", kbytes);
}
void mem_string(int kbytes, char *buf)
{
    if (kbytes >= 1024)
    {
        int meg = kbytes >> 10;
        if (meg >= 1024 * 100)
            sprintf(buf, "%uGb", meg >> 10);
        else
            sprintf(buf, "%uMb", meg);
    }
    else
        sprintf(buf, "%uKb", kbytes);
}

// DEL
CheckMenu::CheckMenu(QWidget *parent) : QMenu(parent) {}

// ???
CrossBox::CrossBox(const char *text, QWidget *parent) : QCheckBox(text, parent)
{
}

void CrossBox::drawButton(QPainter * /*p*/)
{
    /////	QCheckBox::drawButton(p);
}

TBloon::TBloon(QWidget *parent) : QLabel(parent)
{

    return;
    paren = parent;
    setStyleSheet("QLabel { "
                  //"border-width: 1px; border-style: solid;  border-color:
                  // rgb(150,45,100); border-radius: 5px ;"
                  "border-width: 1px; border-style: solid;  border-color: "
                  "rgb(150,180,180); border-radius: 5px ;"
                  "background-color : rgba(0,0,0,48%); padding: 3px; color: "
                  "rgb(255,120,60); }");
    // COLOR orange FF5d00

    setText( tr( " This is unstable Alpha feature\n You maybe see a SEGFAULT..." ) );
    resize(sizeHint());
    //	parent->installEventFilter(this);
    //	parent->setMouseTracking(true);
    hide();

    // Construct a 1-second timeline with a frame range of 0 - 100
    timeLine = new QTimeLine(100000, this);
    timeLine->setFrameRange(0, 20000);
    timeLine->setCurveShape(QTimeLine::EaseOutCurve);
    //	timeLine->set(0, 20000);
    connect(timeLine, SIGNAL(frameChanged(int)), this, SLOT(update(int)));
}

/*
void TBloon::start_ani()
{

} */

void TBloon::update(int /*val*/)
{
    //`QPoint p=QCursor::pos();
    // qDebug("pos %d %d", p.x(),p.y());
    QPoint p2 = paren->mapFromGlobal(QCursor::pos());

    // qDebug("wpos %d %d", p2.x(),p2.y());
    int tx = p2.x();
    int ty = p2.y();
    int sx = pos().x();
    int sy = pos().y();
    float dx = tx - sx;
    float dy = ty - sy;
    dx = dx / 1.35;
    dy = dy / 1.35;
    move(tx + dx + 8, ty + dy + 14);
}

bool TBloon::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Enter)
    {
        QPoint p = paren->mapFromGlobal(QCursor::pos());
        move(p);
        //	qDebug("Enter parent");
        timeLine->start();
        show();
        return true;
    }
    else if (event->type() == QEvent::Leave)
    {
        //	qDebug("Leave parent");
        timeLine->stop();
        hide();
        return true;
    }
    else if (event->type() == QEvent::MouseMove)
    {
        // QMouseEvent *me = static_cast<QMouseEvent *>(event);
        //	qDebug("Ate key press %d %d", me->x(),me->y());
        //	move(me->x()+5,me->y()+4);
        return false;
    }
    else
    {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}

/*
void TBloon::paintEvent(  QPaintEvent * event )
{
        QLabel::paintEvent(event);
} */

TFrame::TFrame(QWidget *parent) : QLabel(parent)
// TFrame::TFrame(QWidget *parent):QFrame(parent)
{
    text = tr( "this is Tframe widget" );
    // setAutoFillBackground(false);
    // setGeometry(50,50,100,100);
    // setAttribute(Qt::WA_OpaquePaintEvent);
    // setFrameShape(QFrame::StyledPanel);
    // setFrameShadow(QFrame::Sunken);
    // setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    // setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    // setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
    // setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    //"border-width: 1px; border-style: solid;  border-color:
    // rgb(150,45,100);
    // border-radius: 5px ;"
    setStyleSheet("QLabel { "
                  "border-width: 1px; border-style: solid;  border-color: "
                  "rgb(210,50,130); border-radius: 5px ;"
                  "background-color : rgba(0,0,0,69%); padding: 3px; color: "
                  "rgb(0,255,150); }");
    // 	setStyleSheet(  "QLabel { " "border-width: 1px; border-style:
    // solid;
    // border-radius: 5px ; padding: 3px; }");
    hide();

    timeLine = new QTimeLine(2000, this);
    timeLine->setFrameRange(0, 100);
    connect(timeLine, SIGNAL(frameChanged(int)), this, SLOT(setValue(int)));
}

/*
void TFrame::showText(QPoint glob_pos,QString str)
{


}


void TFrame::move(int x,int y)
{


} */

// QToolTip::showText(e->globalPos(), s);
void TFrame::showText(QPoint /*glob_pos*/, QString /*str*/) {}

void TFrame::setText(QString str)
{
    text = str;

    QLabel::setText(str);

    // resize(minimumSizeHint());
    resize(sizeHint()); // **** before show !!  *****

    if (str.size() == 0)
    {
        hide();
        return;
    }
    show();
}

// to avoid ~
void TFrame::draw(QPainter &p)
{
    //	if(!isVisible()) 	return;
    return;
    int h = fontMetrics().height() + 3;
    int w = fontMetrics().width(text) + 9;
    setFixedSize(w, h);
    //	QFont font("Adobe Helvetica"); // helvetica & No-Antialias
    // 	font.setPixelSize(10);
    //	setFont(font);
    //	p.drawRoundRect(rect(),10,10);
    //	p.fillRect ( cr,QColor(0,0,0));
    QColor bg = QColor(0, 0, 0, 120);
#if QT_VERSION < 0x040400
    p.fillRect(rect(), QColor(0, 0, 0, 120));
#else
    // p.fillRect(rect(),QColor(0,0,0,90));

    //	p.setPen(QColor(0,255,155)); // less visually obtrusive than
    // black
    //	p.setBrush(QBrush(bg));
    //	p.drawRoundedRect(0,0,w,h, 4, 4);
    p.fillRect(rect(), bg);

#endif

    p.setPen(QColor(0, 255, 155)); // less visually obtrusive than black
    p.drawText(0, 0, w, h, Qt::AlignVCenter | Qt::AlignHCenter, text);
}

void TFrame::setPos()
{
    QWidget *parent = QWidget::parentWidget();

    QPoint p = parent->mapFromGlobal(QCursor::pos());

    setPos(p.x(), p.y());
}

void TFrame::setPos(int x, int y)
{
    QWidget *parent = QWidget::parentWidget();
    int pwidth = parent->width();

    if (width() + x > pwidth)
    {
        x = pwidth - width();
    }
    move(x + 16, y + 4);
}

void TFrame::moveEvent(QMoveEvent * /*e*/)
{
    // int x=e->pos().x();
    // printf("pos (%d %d) \n",e->pos().x(),e->pos().y());
    // event->oldPos();
}

void TFrame::setValue(int val)
{
    opacity += ((float)val / 100);
    update();
}

void TFrame::showEvent(QShowEvent * /*event*/)
{
    opacity = 0.2;
    //	timeLine->start();
}

// Maybe SEGFAULT
void setPaletteBlend(QPalette &p)
{
    for (int i = 0; i < (QPalette::NoRole); i++)
    {
        QColor c = p.color(QPalette::ColorRole(i));
        c.alpha();
    }
}

void TFrame::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
    return;

    QCursor::pos();
    //	if(x()+width() >  ) ;
    QPalette np = palette();
    setPaletteBlend(np);

    QPainter p(this); // why?

    //	if(opacity>1) timeLine->stop(); //
    //	p.setOpacity(opacity);
    //	p.setOpacity(0.01);
    //	setWindowOpacity (0.1 );
    //	QFrame::paintEvent(event);
    //    p.scale(0.5,0.5);

    // draw(p);
    // p.setBackgroundMode (Qt::TransparentMode);
    // p.setRenderHint(QPainter::Antialiasing);
    // painter.translate(width() / 2, height() / 2);
}

UFrame::UFrame(QWidget *parent) : QFrame(parent)
// TFrame::TFrame(QWidget *parent):QFrame(parent)
{
    //	setAttribute(Qt::WA_OpaquePaintEvent);
    //	setFrameShape(QFrame::StyledPanel);
    //	setFrameShadow(QFrame::Sunken);
    //	setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);

    //"border-width: 1px; border-style: solid;  border-color:
    // rgb(150,45,100);
    // border-radius: 5px ;"

    // setStyleSheet(  "QLabel { "   "border-width: 1px; border-style:
    // solid;
    // border-color: rgb(210,50,130); border-radius: 5px ;"
    // "background-color :
    // rgba(0,0,0,70%); padding: 3px; color: rgb(0,255,150); }");
    hide();

    QVBoxLayout *vlayout = new QVBoxLayout;
    QLabel *label = new QLabel( tr( "title" ) );
    vlayout->addWidget(label);

    setLayout(vlayout);
    QWidget *qps = getQpsWidget();
    if (qps)
    {
        int w = qps->width();
        int h = qps->height();
        printf("qps w=%d h=%d\n", qps->width(), qps->height());
        move(w / 2, h / 2);
    }
}



void UFrame::setTitle(QString str)
{
    //	QLabel::setText(str);
    title = str;
}

void UFrame::paintEvent(QPaintEvent * /*event*/) {}

QPixmap *letters;
int pf_height = 9;
int pf_width = 6;

void init_xpm()
{
    letters = new QPixmap(":/icon/letters.png");
    pf_height = 9;
    pf_width = 6;
}

#ifdef LINUX
#include <sys/prctl.h>
#include <QThread>
char strong_buff[128];
class MyThread : public QThread
{
  public:
    void run() override
    {
        prctl(PR_SET_NAME, "thread_test123456789");

        while (true)
        {
            for (int i = 0; i < 20000; i++)
            {
                int p = rand() % 120;
                strong_buff[p] = p;
            }
            msleep(40);
        }
        exec();
    }
};
class MyThread2 : public QThread
{
  public:
    void run() override
    {
        prctl(PR_SET_NAME, "thread_test987654321");

        while (true)
        {
            int m = 0;
            int i;
            for (i = 0; i < 100000; i++)
            {
                int p = rand() % 120;
                // strong_buff[p]=p;
                m = i * p;
            }
            m += i;
            msleep(60);
        }
        exec();
    }
};
#endif

int pf_str_width(char *str)
{
    int len;
    len = strlen(str) * pf_width;
    return len;
}
int pf_char_height() { return pf_height; }

int pf_write(QPainter *p, int x, int y, const char *str)
{
    int i, len, n;
    int ch;
    int x0 = x;
    len = strlen(str);
    for (i = 0; i < len; i++)
    {
        ch = str[i];

        if (ch == '\n')
        {
            // y+=pf_height;
            continue;
        }
        // if(ch>96) ch-=(97-65);
        n = ch - 32;
        p->drawPixmap(x, y, *letters, n * pf_width, 0, pf_width + 1, pf_height);
        x += pf_width;
    }
    return x - x0;
}

QPixmap *x1, *x2;
XButton::XButton(QWidget *parent) : QAbstractButton(parent)
{
    setFocusPolicy(Qt::NoFocus);
    x1 = new QPixmap((const char **)x1_xpm);
    x2 = new QPixmap((const char **)x2_xpm);
    int w = x1->width();
    setGeometry(0, 0, w, w);
}

void XButton::resizeEvent(QResizeEvent * /*p*/)
{
    int i, h;
    i = height() % 2;

    if (i != 0)
        h = height() + 1;
    else
        h = height();
    setFixedSize(h, h);
}

void XButton::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);

    if (isDown())
        p.drawPixmap(0, 0, *x2);
    else
        p.drawPixmap(0, 0, *x1);

    return;
}

SearchBox::SearchBox(QWidget *parent) : QLineEdit(parent)
{
    setToolTip( tr( "PID, COMMAND, USER..." ) );
    setPlaceholderText( tr("Filter"));
    setClearButtonEnabled(true);
    setMinimumWidth(100);

    setFocus(Qt::ActiveWindowFocusReason);

    QList<QToolButton*> list = findChildren<QToolButton*>();
    if (!list.isEmpty())
    {
        QToolButton *clearButton = list.at(0);
        if (clearButton)
            connect (clearButton, &QAbstractButton::clicked, this, &SearchBox::onClearButtonClicked);
    }
}

void SearchBox::event_cursor_moved(QMouseEvent *e) { move(e->x(), e->y()); }

void SearchBox::event_xbutton_clicked()
{
    QKeyEvent ke(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier); // temp..
    keyPressEvent(&ke);
}

LogBox::LogBox(QWidget *p) : QLabel(p)
{
    text = new QTextEdit(p);
    text->setFocusPolicy(Qt::NoFocus);
}

// Location: bottom
StatusBar::StatusBar(QWidget *parent) : QStatusBar(parent)
{
    setSizeGripEnabled(true);
    label = new QLabel(this);
    label->setFrameShape(QFrame::NoFrame);
    addWidget(label);
}

extern int num_opened_files;
void StatusBar::update(int total_n)
{
    label->setText(tr( "Process count: %1" ).arg( total_n ) );
}

ControlBar::ControlBar(QWidget *parent) : QFrame(parent)
{
    setFrameStyle(QFrame::NoFrame);
    layout = new QHBoxLayout(this);
    layout->addSpacing(4); // left gap
    layout->setMargin(0);

    b_linear = new QRadioButton(tr( "Linear" ), this);
    b_linear->setFocusPolicy(Qt::NoFocus);

    b_tree = new QRadioButton( tr( "Tree" ), this);
    b_tree->setFocusPolicy(Qt::NoFocus);

    search_box = new SearchBox(this);
    connect(b_linear, SIGNAL(clicked()), SLOT(linear_clicked()));
    connect(b_tree, SIGNAL(clicked()), SLOT(tree_clicked()));

    if (flag_thread_ok)
    {
        check_thread = new QCheckBox( ( "Thread" ), this);
        check_thread->setFocusPolicy(Qt::NoFocus);
        connect(check_thread, SIGNAL(clicked()), SLOT(show_thread_clicked()));
        check_thread->setChecked(flag_show_thread);
    }

    view = new QComboBox(this);
    view->insertItem(0, tr( "All Processes" ), Procview::ALL);
    view->insertItem(1, tr( "Your Processes" ), Procview::OWNED);
    view->insertItem(2, tr( "Non-Root Processes" ), Procview::NROOT);
    view->insertItem(3, tr( "Running Processes" ), Procview::RUNNING);
    connect(view, SIGNAL(activated(int)), SLOT(view_changed(int)));
    view->setFocusPolicy(Qt::NoFocus);

    layout->addWidget(b_linear);
    layout->addWidget(b_tree);
    if (flag_thread_ok)
        layout->addWidget(check_thread);
    layout->addWidget(search_box);
    layout->addWidget(view);

    layout->addStretch();

    pauseButton = new QToolButton;
    pauseButton->setCheckable(true);
    pauseButton->setIcon(QIcon::fromTheme("media-playback-pause"));
    pauseButton->setToolTip(tr("Pause (Ctrl+Space)"));
    pauseButton->setFocusPolicy(Qt::NoFocus);
    pauseButton->setAutoRaise(true);
    connect(pauseButton, SIGNAL(clicked(bool)), SLOT(setPaused(bool)));

    layout->addWidget(pauseButton);

    setLayout(layout);
}

void PSTABLE_SETTREEMODE(bool mode);
void ControlBar::setMode(bool treemode) // just.. interface function
{
    b_linear->setChecked(!treemode);
    b_tree->setChecked(treemode);

    PSTABLE_SETTREEMODE(treemode); //	pstable->setTreeMode(treemode);
}

#include <QAction>
void ControlBar::view_changed(int idx)
{
    QAction act(this);
    act.setData(QVariant(idx));
    emit viewChange(&act);
}

extern bool flag_refresh;
void ControlBar::setPaused(bool b)
{
    flag_refresh = !b;
}

void ControlBar::linear_clicked()
{
    setMode(false);
}

void ControlBar::tree_clicked()
{
    setMode(true);
}

void ControlBar::show_thread_clicked()
{
    flag_show_thread = check_thread->isChecked();
    emit need_refresh();
}

void ServerAdaptor::accelerate() {}

char *read_proc_file(const char *fname, int pid = -1, int tgid = -1); // Temp
void init_misc(QWidget * /*main*/)
{

    init_xpm(); // xpm image file load

    if (true) // TESTING
    {
    }

#ifdef LINUX
    if (false or flag_devel) // thread's Name Change test
    {
        QThread *task = new MyThread();
        task->start();
        task = new MyThread2();
        task->start();
    }
#endif

    if (false) // test mini_sscanf()
    {
        char *buf;
        int val;
        char sstr[32] = "pu%s";
        char buffer[128];

        if ((buf = read_proc_file("stat", 1)) == nullptr)
        {
            printf("cant open [%s]\n", "sss");
        }
        if (mini_sscanf(buf, sstr, &val) != 0)
            printf("found [%s] val=%d \n", sstr, val);
        if (mini_sscanf(buf, "(%s)", &buffer) != 0)
            printf("found  str=%s \n", buffer);

        abort();
    }
}

QTabWidgetX::QTabWidgetX(QWidget * /*parent*/) {}

void QTabWidgetX::showTab(bool flag)
{

    QTabBar *tb = tabBar();
    //	tb->setDrawBase(false); //??

    if (flag)
        tb->show();
    else
        tb->hide();
// if flag_useTabView == false
#if QT_VERSION > 0x040500
//	setDocumentMode (true); // QT 4.5
#endif
}
