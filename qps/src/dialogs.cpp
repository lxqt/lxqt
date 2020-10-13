/*
 * dialogs.cpp
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

#include "qps.h"
#include "dialogs.h"
#include <sched.h>

extern Qps *qps;

// center window a with respect to main application window
static void center_window(QWidget *a)
{
    QWidget *b = QApplication::activeWindow();
    // QWidget *b = qApp->mainWidget();
    a->move(b->x() + (b->width() - a->width()) / 2,
            b->y() + (b->height() - a->height()) / 2);
}

static void fix_size(QWidget *w) { w->setFixedSize(w->sizeHint()); }

// Modal dialog
IntervalDialog::IntervalDialog(const char *ed_txt, bool enabled) : QDialog()
{
    setWindowTitle( tr( "Change Update Period" ) );
    QVBoxLayout *tl = new QVBoxLayout;
    QHBoxLayout *h1 = new QHBoxLayout;
    setLayout(tl);
    tl->addLayout(h1);

    QLabel *label1 = new QLabel( tr( "New Update Period" ), this);
    h1->addWidget(label1);
    h1->addStretch(1);

    lined = new QLineEdit(this);
    lined->setMaxLength(7);
    lined->setText(ed_txt);
    if (enabled)
    {
        lined->selectAll();
        lined->setFocus();
    }
    // lined->setEnabled(enabled);
    lined->setEnabled(true);
    fix_size(lined);
    lined->setFixedWidth(64);
    h1->addWidget(lined);

    label = new QLabel(this);
    label->setFrameStyle(QFrame::Panel);
    label->setFrameShadow(QFrame::Sunken);
    label->setText("");
    label->setAlignment(Qt::AlignRight);
    tl->addWidget(label);

    /*
  toggle = new CrossBox("Dynamic Update (under devel)", this);
  fix_size(toggle);
  toggle->setChecked(enabled);
  //toggle->setChecked(false);
  toggle->setChecked(true);
  toggle->setEnabled(false);
  tl->addWidget(toggle);
    */
    QHBoxLayout *h2 = new QHBoxLayout;
    // h2->addStretch(1);
    cancel = new QPushButton( tr( "Cancel" ), this);
    h2->addWidget(cancel);
    ok = new QPushButton( tr( "OK" ), this);
    // ok->setFocus();
    h2->addWidget(ok);
    tl->addLayout(h2);

    connect(ok, SIGNAL(clicked()), SLOT(done_dialog()));
    connect(cancel, SIGNAL(clicked()), SLOT(reject()));
    connect(lined, SIGNAL(returnPressed()), SLOT(done_dialog()));
    connect(lined, SIGNAL(textChanged(const QString &)),
            SLOT(event_label_changed()));
    //    connect(toggle, SIGNAL(toggled(bool)), lined,
    //    SLOT(setEnabled(bool)));

    ok->setDefault(true);

    ///   Q3Accel *acc = new Q3Accel(this);
    /// acc->connectItem(acc->insertItem(Qt::Key_Escape), this,
    /// SLOT(reject()));
    //  tl->setSizeConstraint(QLayout::SetFixedSize);
    //    center_window(this);
}

void IntervalDialog::event_label_changed()
{
    QString txt;
    int i = 0;
    ed_result = lined->text();
    ed_result = ed_result.simplified();

    // if(toggle->isChecked())

    QString s = ed_result;
    if (s.length() == 0)
    {
        label->setText( tr( "No UPDATE" ) );
        return;
    }

    while ((s[i] >= '0' && s[i] <= '9') || s[i] == '.')
        i++;

    float period = (i > 0) ? s.leftRef(i).toFloat() : -1;

    s = s.mid(i, 3).simplified();
    if (s.length() == 0 || s == "s")
        period *= 1000;
    else if (s == "min")
        period *= 60000;
    else if (s != "ms")
        period = -1;
    if (period <= 0)
    {
        label->setText( tr( "Invalid value" ) );
        return;
    }

    txt = QString::asprintf("%d ms", (int)period);
    label->setText(txt);
}

void IntervalDialog::done_dialog()
{
    int i = 0;

    ed_result = lined->text();
    ed_result = ed_result.simplified();

    // if(toggle->isChecked())
    QString s = ed_result;
    while ((s[i] >= '0' && s[i] <= '9') || s[i] == '.')
        i++;

    float period = (i > 0) ? s.leftRef(i).toFloat() : -1;

    s = s.mid(i, 3).simplified();
    if (s.length() == 0 || s == "s")
        period *= 1000;
    else if (s == "min")
        period *= 60000;
    else if (s != "ms")
        period = -1;
    if (period < 0)
        return;

    qps->set_update_period((int)period);
    qps->update_timer();

    accept();
}

SliderDialog::SliderDialog(int defaultval, int minval, int maxval) : QDialog()
{
    setWindowTitle( tr( "Renice Process" ) );
    QVBoxLayout *tl = new QVBoxLayout;
    QHBoxLayout *h1 = new QHBoxLayout;
    setLayout(tl);
    tl->addLayout(h1);

    label = new QLabel( tr( "New nice value:" ), this);
    h1->addWidget(label);

    h1->addStretch(1);

    lined = new QLineEdit(this);
    lined->setMaxLength(3);
    lined->setText(QString::number(defaultval));
    lined->setFocus();
    lined->setFixedWidth(64);
    h1->addWidget(lined);

    slider = new QSlider(Qt::Horizontal, this);
    slider->setMaximum(maxval);
    slider->setMinimum(minval);
    slider->setTickInterval(10);
    slider->setTickPosition(QSlider::TicksBelow);
    slider->setValue(defaultval);
    tl->addWidget(slider);

    QHBoxLayout *h2 = new QHBoxLayout;
    tl->addLayout(h2);

    // decorate slider
    QLabel *left = new QLabel(this);
    QLabel *mid = new QLabel(this);
    QLabel *right = new QLabel(this);
    left->setNum(minval);
    mid->setNum((minval + maxval) / 2);
    right->setNum(maxval);
    h2->addWidget(left);
    h2->addStretch(1);
    h2->addWidget(mid);
    h2->addStretch(1);
    h2->addWidget(right);

    QHBoxLayout *h3 = new QHBoxLayout;
    tl->addLayout(h3);

    h3->addStretch(1);
    cancel = new QPushButton( tr( "Cancel" ), this);
    // fix_size(cancel);
    h3->addWidget(cancel);

    ok = new QPushButton( tr( "OK" ), this);
    ok->setFixedSize(cancel->sizeHint());
    h3->addWidget(ok);

    connect(ok, SIGNAL(clicked()), SLOT(done_dialog()));
    ok->setDefault(true);
    connect(cancel, SIGNAL(clicked()), SLOT(reject()));
    connect(lined, SIGNAL(returnPressed()), SLOT(done_dialog()));
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(slider_change(int)));
    // Q3Accel *acc = new Q3Accel(this);
    // acc->connectItem(acc->insertItem(Qt::Key_Escape), this,
    // SLOT(reject()));
    // tl->freeze();

    center_window(this);
}

void SliderDialog::done_dialog()
{
    ed_result = lined->text();
    ed_result = ed_result.simplified();
    accept();
}

void SliderDialog::slider_change(int val)
{
    QString s;
    s.setNum(val);
    lined->setText(s);
    lined->selectAll();
}

// DRAFT CODE,
PermissionDialog::PermissionDialog(QString msg, QString /*passwd*/) : QDialog()
{
    setWindowTitle( tr( "Permission" ) );
    QVBoxLayout *vbox = new QVBoxLayout;
    label = new QLabel(msg, this);
    vbox->addWidget(label);

    setLayout(vbox);

    QHBoxLayout *hbox = new QHBoxLayout;
    vbox->addLayout(hbox);
    label = new QLabel( tr( "Root password" ), this);
    hbox->addWidget(label);
    lined = new QLineEdit(this);
    hbox->addWidget(lined);

    hbox = new QHBoxLayout;
    vbox->addLayout(hbox);
    QPushButton *cancel = new QPushButton( tr( "Cancel" ), this);
    hbox->addWidget(cancel);

    QPushButton *ok = new QPushButton( tr( "OK" ), this);
    hbox->addWidget(ok);

    connect(ok, SIGNAL(clicked()), SLOT(accept()));
    connect(cancel, SIGNAL(clicked()), SLOT(reject()));
}

SchedDialog::SchedDialog(int policy, int prio) : QDialog()
{
    setWindowTitle( tr( "Change scheduling" ) );
    QVBoxLayout *vl = new QVBoxLayout;
    setLayout(vl);

    bgrp = new QGroupBox( tr( "Scheduling Policy" ), this);
    vl->addWidget(bgrp); // bgrp->setCheckable(1);
    rb_other = new QRadioButton( tr( "SCHED_OTHER (time-sharing)" ), bgrp);
    rb_fifo = new QRadioButton( tr( "SCHED_FIFO (real-time)" ), bgrp);
    rb_rr = new QRadioButton( tr( "SCHED_RR (real-time)" ), bgrp);

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(rb_other);
    vbox->addWidget(rb_fifo);
    vbox->addWidget(rb_rr);
    bgrp->setLayout(vbox);

    connect(rb_other, SIGNAL(clicked(bool)), SLOT(button_clicked(bool)));
    connect(rb_fifo, SIGNAL(clicked(bool)), SLOT(button_clicked(bool)));
    connect(rb_rr, SIGNAL(clicked(bool)), SLOT(button_clicked(bool)));

    QHBoxLayout *hbox1 = new QHBoxLayout;
    QPushButton *ok, *cancel;
    ok = new QPushButton( tr( "OK" ), this);
    ok->setDefault(true);
    cancel = new QPushButton( tr( "Cancel" ), this);
    hbox1->addWidget(ok);
    hbox1->addWidget(cancel);
    vl->addLayout(hbox1);

    connect(ok, SIGNAL(clicked()), SLOT(done_dialog()));
    connect(cancel, SIGNAL(clicked()), SLOT(reject()));

    int active = 0;
    QRadioButton *rb;
    switch (policy)
    {
    case SCHED_OTHER:
        active = 0;
        rb = rb_other;
        break;
    case SCHED_FIFO:
        active = 1;
        rb = rb_fifo;
        break;
    case SCHED_RR:
        active = 2;
        rb = rb_rr;
        break;
    }
    rb->setChecked(true);
    out_policy = policy;
    out_prio = prio;

    QHBoxLayout *hbox = new QHBoxLayout;
    lbl = new QLabel( tr( "Priority (1-99):" ), this);
    lined = new QLineEdit(this);
    hbox->addWidget(lbl);
    hbox->addWidget(lined);
    vbox->addLayout(hbox);
    QFont f = font();
    f.setBold(false);
    lined->setFont(f);
    lined->resize(60, lined->sizeHint().height());
    lined->setMaxLength(4);
    QString s;
    s.setNum(prio);
    lined->setText(s);
    button_clicked(active);

    // make sure return and escape work as accelerators
    connect(lined, SIGNAL(returnPressed()), SLOT(done_dialog()));
}

void SchedDialog::done_dialog()
{
    if (rb_rr->isChecked())
        out_policy = SCHED_RR;
    else if (rb_fifo->isChecked())
        out_policy = SCHED_FIFO;
    else
        out_policy = SCHED_OTHER;
    QString s(lined->text());
    bool ok;
    out_prio = s.toInt(&ok);
    if (out_policy != SCHED_OTHER && (!ok || out_prio < 1 || out_prio > 99))
    {
        QMessageBox::warning( this
                            , tr( "Invalid Input" )
                            , tr( "The priority must be in the range 1..99" ) );
    }
    else
        accept();
}

void SchedDialog::button_clicked(bool /*val*/)
{
    //	printf("SchedDialog::checked()\n");
    if (rb_other->isChecked())
    {
        lbl->setEnabled(false);
        lined->setEnabled(false);
    }
    else
    {
        QString s(lined->text());
        bool ok;
        int n = s.toInt(&ok);
        if (ok && n == 0)
            lined->setText("1");
        lbl->setEnabled(true);
        lined->setEnabled(true);
        lined->setFocus();
        lined->selectAll();
    }
}
