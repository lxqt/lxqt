/*
 * misc.h
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

#ifndef MISC_H
#define MISC_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
#include <QAbstractButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QStatusBar>
#include <QPainter>

#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QToolButton>

#include <QTimerEvent>
#include <QKeyEvent>
#include <QBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QFrame>
#include <QResizeEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QTimeLine>
#include <QTabWidget>

char *userName(int uid, int euid);
char *groupName(int gid, int egid);

void setQpsTheme();

int fsize(char *fname);
void msleep(long msec);
void mem_string(int kbytes, char *buf);
void mem_string_k(int kbytes, char *buf);

void init_xpm();
void init_misc(QWidget *main);
int pf_write(QPainter *p, int x, int y, const char *str);
int pf_str_width(char *str);
int pf_char_height();
void check_qps_running();

int QPS_PROCVIEW_CPU_NUM();
void AddLog(QString str);

class CrossBox : public QCheckBox
{
  public:
    CrossBox(const char *text, QWidget *parent);

  protected:
    virtual void drawButton(QPainter *paint);
};

class CheckMenu : public QMenu
{
  public:
    CheckMenu(QWidget *parent = nullptr);
};

class TFrame : public QLabel
{
    Q_OBJECT
  public:
    TFrame(QWidget *parent);
    void setText(QString str);
    void draw(QPainter &p);
    void showText(QPoint pos, QString str);
    void setPos(int x, int y);
    void setPos();

  protected slots:
    //    		void refresh();
    //  		void update(int n);
    //		QToolButton *button,*button2,*button3;
    //		void event_cursor_moved(QMouseEvent *e);
    void setValue(int val);

  protected:
    void paintEvent(QPaintEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void showEvent(QShowEvent *event) override;

  private:
    QString text;
    QTimeLine *timeLine;
    float opacity;
};

class TBloon : public QLabel
{
    Q_OBJECT
  public:
    TBloon(QWidget *parent);
    //	void setText(QString str);
    //	void draw( QPainter &p );
    //	void showText(QPoint pos,QString str);

    bool eventFilter(QObject *watched, QEvent *event) override;
  protected slots:
    void update(int val);
    //    		void refresh();
    //  		void update(int n);
    //		QToolButton *button,*button2,*button3;
    //		void event_cursor_moved(QMouseEvent *e);
  protected:
    //	virtual void paintEvent(  QPaintEvent * event );
    //	virtual void moveEvent (QMoveEvent * event );
  private:
    QWidget *paren;
    QString text;
    QTimeLine *timeLine;
};

class UFrame : public QFrame
{
    Q_OBJECT
  public:
    UFrame(QWidget *parent);
    void setTitle(QString str);
  protected slots:
    //    		void refresh();
    //  		void update(int n);
    //		QToolButton *button,*button2,*button3;
    //		void event_cursor_moved(QMouseEvent *e);
  protected:
    void paintEvent(QPaintEvent *event) override;

  private:
    QString title;
    QString stylesheet;
};

class XButton : public QAbstractButton
{
    Q_OBJECT
  public:
    XButton(QWidget *parent);
  protected slots:
    //    		void refresh();
    //  		void update(int n);
    //		QToolButton *button,*button2,*button3;
    //		void event_cursor_moved(QMouseEvent *e);
  protected:
    // virtual void drawButton 3( QPainter * ) ;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *p) override;
};

class SearchBox : public QLineEdit
{
    Q_OBJECT
  public:
    SearchBox(QWidget *parent);
    void event_cursor_moved(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e) override;
  protected slots:
    void onClearButtonClicked();
    void event_xbutton_clicked();
};

class LogBox : public QLabel
{
    Q_OBJECT
  public:
    LogBox(QWidget *w);
    QTextEdit *text;
    // QLabel *label,*label2,*label3;
};

class StatusBar : public QStatusBar
{
    Q_OBJECT
  public:
    StatusBar(QWidget *parent);
    void update(int n);
    QLabel *label;
};

class ControlBar : public QFrame
{
    Q_OBJECT
  public:
    QComboBox *view;
    ControlBar(QWidget *parent);
    void setMode(bool treemode);

    QToolButton *pauseButton;

signals:
    void modeChange(bool treemode);
    void viewChange(QAction *);
    void need_refresh();

  public slots:
    void linear_clicked();
    void view_changed(int idx);
    void tree_clicked();
    void show_thread_clicked();
    void setPaused(bool);

  private:
    QRadioButton *b_tree, *b_linear, *b_treeT;
    QCheckBox *check_thread;
    QBoxLayout *layout;
};

class QTabWidgetX : public QTabWidget
{
    Q_OBJECT
  public:
    QTabWidgetX(QWidget *parent);
    void showTab(bool);
};

// class ServerAdaptor: public QDBusAbstractAdaptor
class ServerAdaptor : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.trolltech.Examples.CarInterface")
    Q_CLASSINFO("D-Bus Introspection",
                ""
                "  <interface name=\"com.trolltech.Examples.CarInterface\" >\n"
                "    <method name=\"accelerate\" />\n"
                "    <method name=\"decelerate\" />\n"
                "    <method name=\"turnLeft\" />\n"
                "    <method name=\"turnRight\" />\n"
                "    <signal name=\"crashed\" />\n"
                "  </interface>\n"
                "")
  public:
    //    ServerAdaptor(QObject *parent);
    ServerAdaptor(){};
    ~ServerAdaptor() override{};

  public:         // PROPERTIES
  public Q_SLOTS: // METHODS
    void accelerate();
Q_SIGNALS: // SIGNALS
    void crashed();
};

#endif // MISC_H
