/*
 * prefs.cpp
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

#include "prefs.h"
#include "proc.h"
#include "qps.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <qstring.h>
#include <QtGui>
#include <QApplication>
#include <QFontComboBox>

//#include <qstylefactory.h>
extern Qps *qps;

extern QFontComboBox *font_cb;
// class that validates input for the "swap warning limit" field
class Swapvalid : public QValidator
{
  public:
    Swapvalid(QWidget *parent) : QValidator(parent) {}
    State validate(QString &s, int &) const override;
};

struct Boxvar
{
Q_DECLARE_TR_FUNCTIONS(Boxvar)
public:
    const QString text;
    bool *variable;
    QCheckBox *cb;

    Boxvar() : text( QString() ), variable( static_cast<  bool * >( nullptr ) ), cb( static_cast< QCheckBox *>( nullptr ) ) {}
    Boxvar( const QString t, bool *v, QCheckBox *c ) : text( t ), variable( v ), cb( c ) {}

    static QVector< Boxvar > *general_boxes()
    {
        static QVector< Boxvar > boxes( { { tr( "Exit on closing" ), &Qps::flag_exit, nullptr}
                                        , { tr( "Remember Position" ), &Qps::save_pos, nullptr}
                                        } );
        return &boxes;
    }

//#ifdef LINUX
    static QVector< Boxvar > *sockinfo_boxes()
    {
        static QVector< Boxvar > boxes( { { tr( "Host Name Lookup" ), &Qps::hostname_lookup, nullptr}
                                        , { tr( "Service Name Lookup" ), &Qps::service_lookup, nullptr} } );
        return &boxes;
    }
//#endif
    static QVector< Boxvar > *tree_boxes()
    {
        static QVector< Boxvar > boxes( { { tr( "Disclosure Triangles" ), &Qps::tree_gadgets, nullptr}
                                        , { tr( "Branch Lines" ), &Qps::tree_lines, nullptr} } );
        return &boxes;
    }

    static QVector< Boxvar > *misc_boxes()
    {
        static QVector< Boxvar > boxes( { { tr( "Auto Save Settings on Exit" ), &Qps::auto_save_options, nullptr}
                                        , { tr( "Selection: Copy PIDs to Clipboard" ), &Qps::pids_to_selection, nullptr}
#ifdef SOLARIS
                                        , { tr( "Normalize NICE" ), &Qps::normalize_nice, 0}
                                        , { tr( "Use pmap for Map Names" ), &Qps::use_pmap, 0}
#endif
                                        } );
        return &boxes;
    }

};

struct Cbgroup
{
Q_DECLARE_TR_FUNCTIONS(Cbgroup)
public:
    const QString caption;
    QVector< Boxvar > *boxvar;

    Cbgroup() : caption( QString() ), boxvar( static_cast< QVector< Boxvar > * >( nullptr ) ) {}
    Cbgroup( const QString c, QVector< Boxvar > *b ) : caption( c ), boxvar( b ) {}

    static QVector< Cbgroup > &groups()
    {
        static QVector< Cbgroup > groups( { { tr( "General" ), Boxvar::general_boxes() }
                                          } );
        return groups;
    }

};


void find_fontsets();
// dual use function: both validate and apply changes
QValidator::State Swapvalid::validate(QString &s, int &) const
{
    // only accept /^[0-9]*[%kKmM]?$/
    int len = s.length();
    int i = 0;
    while (i < len && s[i] >= '0' && s[i] <= '9')
        i++;
    if ((i < len && QString("kKmM%").contains(s[i]) == 0) || i < len - 1)
        return Invalid;
    if (s[i] == 'k')
        s[i] = 'K';
    if (s[i] == 'm')
        s[i] = 'M';
    // int val = atoi((const char *)s);
    int val = s.toInt();
    bool percent;
    if (s[i] == '%')
    {
        percent = true;
    }
    else
    {
        percent = false;
        if (s[i] == 'M')
            val <<= 10;
    }
    Qps::swaplimit = val;
    Qps::swaplim_percent = percent;
    return Acceptable;
}

Preferences::Preferences(QWidget *parent) : QDialog(parent)
{
    int flag_test = 0;
    setWindowTitle( tr( "Preferences" ) );
    QVBoxLayout *v_layout = new QVBoxLayout;

    if (flag_test)
    {
        QTabWidget *tbar = new QTabWidget(this);
        QWidget *w = new QWidget(this);
        tbar->addTab(w, tr( "Setting" ) );
        w->setLayout(v_layout);
    }
    else
        setLayout(v_layout);

    v_layout->setSpacing(1);
    // v_layout->setSpacing(1);

    QVector< Cbgroup >::iterator endItG = Cbgroup::groups().end();
    for( QVector< Cbgroup >::iterator itG = Cbgroup::groups().begin(); itG != endItG; ++ itG )
    {
        QGroupBox *grp = new QGroupBox( itG->caption, this );
        QVBoxLayout *vbox = new QVBoxLayout;
        if ( itG->boxvar )
        {
            QVector< Boxvar >::iterator endItB = itG->boxvar->end();
            for( QVector< Boxvar >::iterator itB = itG->boxvar->begin(); itB != endItB; ++ itB )
            {
                itB->cb = new QCheckBox( itB->text, grp );
                vbox->addWidget( itB->cb );
                connect( itB->cb, SIGNAL(clicked()), SLOT(update_reality()));
            }
        }
        grp->setLayout(vbox);
        v_layout->addWidget(grp);
    }

    update_boxes();

    rb_totalcpu = nullptr; // tmp

    if (QPS_PROCVIEW_CPU_NUM() > 1)
    {
        QGroupBox *grp_cpu = new QGroupBox( tr( "%CPU divided by" ), this);
        QVBoxLayout *vboxlayout = new QVBoxLayout;
        QHBoxLayout *hbox = new QHBoxLayout;
        vboxlayout->addLayout(hbox);

        // num_cpus

        rb_totalcpu = new QRadioButton( tr( "Total cpu: %1" ).arg( QPS_PROCVIEW_CPU_NUM() ), grp_cpu);
        QRadioButton *rb2 = new QRadioButton( tr( "Single cpu: 1" ), grp_cpu);
        if (!Procview::flag_pcpu_single)
            rb_totalcpu->setChecked(true);
        else
            rb2->setChecked(true);

        rb_totalcpu->setToolTip( tr( "default" ) );
        rb2->setToolTip( tr( "for developer" ) );
        hbox->addWidget(rb_totalcpu);
        hbox->addWidget(rb2);
        grp_cpu->setLayout(vboxlayout);
        v_layout->addWidget(grp_cpu);

        connect(rb_totalcpu, SIGNAL(clicked()), this, SLOT(update_config()));
        connect(rb2, SIGNAL(clicked()), this, SLOT(update_config()));
    }

    // Appearance ====================================
    if (font_cb == nullptr)
    {
        font_cb = new QFontComboBox(this); // preload
        font_cb->setWritingSystem(QFontDatabase::Latin);
        font_cb->setCurrentFont(QApplication::font());

        // remove Some Ugly Font : hershey...
        for (int i = 0; i < font_cb->count();)
        {
            QString name = font_cb->itemText(i);
            if (name.contains("hershey", Qt::CaseInsensitive) == true)
            {
                font_cb->removeItem(i);
            }
            else
                i++;
        }
    }

    if (font_cb)
    {
        font_cb->show();

        QGroupBox *gbox = new QGroupBox( tr( "Appearance" ), this);
        QVBoxLayout *vbox = new QVBoxLayout;
        QHBoxLayout *hbox = new QHBoxLayout();

        psizecombo = new QComboBox(this);
        hbox->addWidget(font_cb);
        hbox->addWidget(psizecombo);
        vbox->addLayout(hbox);
        gbox->setLayout(vbox);
        v_layout->addWidget(gbox);

        connect(font_cb, SIGNAL(activated(int)), this, SLOT(font_changed(int)));
        connect(psizecombo, SIGNAL(activated(int)), SLOT(font_changed(int)));

        // add to font size
        init_font_size();
    }

    QPushButton *saveButton = new QPushButton("OK", this);
    // saveButton->setFocusPolicy(QWidget::NoFocus);
    saveButton->setFocus();
    saveButton->setDefault(true);
    v_layout->addWidget(saveButton);
    // v_layout->freeze();

    connect(saveButton, SIGNAL(clicked()), SLOT(closed()));
}

//
void Preferences::init_font_size()
{
    psizecombo->clear();
    int i, idx = 0;
    for (i = 5; i < 24; i++)
        psizecombo->insertItem(idx++, QString::number(i));

    // find current font size
    i = 0;
    for (int psize = QApplication::font().pointSize(); i < psizecombo->count();
         ++i)
    {
        const int sz = psizecombo->itemText(i).toInt();
        if (sz == psize)
        {
            psizecombo->setCurrentIndex(i);
            break;
        }
    }
}

// slot: update check boxes to reflect current status
void Preferences::update_boxes()
{
    QVector< Cbgroup >::iterator endItG = Cbgroup::groups().end();
    for( QVector< Cbgroup >::iterator itG = Cbgroup::groups().begin(); itG != endItG; ++ itG )
    {
        if ( ! itG->boxvar )
        {
            continue;
        }
        QVector< Boxvar >::iterator endItB = itG->boxvar->end();
        for( QVector< Boxvar >::iterator itB = itG->boxvar->begin(); itB != endItB; ++ itB )
        {
            itB->cb->setChecked( *( itB->variable ) );
        }
    }
}

// slot: update flags and repaint to reflect state of check boxes
void Preferences::update_reality()
{
    QVector< Cbgroup >::iterator endItG = Cbgroup::groups().end();
    for( QVector< Cbgroup >::iterator itG = Cbgroup::groups().begin(); itG != endItG; ++ itG )
    {
        if ( ! itG->boxvar )
        {
            continue;
        }
        QVector< Boxvar >::iterator endItB = itG->boxvar->end();
        for( QVector< Boxvar >::iterator itB = itG->boxvar->begin(); itB != endItB; ++ itB )
        {
            *( itB->variable ) = itB->cb->isChecked();
        }
    }
    emit prefs_change();
}

void Preferences::update_config()
{
    if (rb_totalcpu and rb_totalcpu->isChecked() == true)
        Procview::flag_pcpu_single = false;
    else
        Procview::flag_pcpu_single = true;
}

void Preferences::closed()
{
    update_config();
    hide();
    emit prefs_change();
}

void Preferences::closeEvent(QCloseEvent *event)
{
    // since we never close the dialog, we hide it here explicitly
    hide();
    event->ignore();
}

// work
void Preferences::font_changed(int /*i*/)
{
    int size = psizecombo->currentText().toInt();
    QFont font = font_cb->currentFont();
    font.setPointSize(size);

    QApplication::setFont(font);
}

// DRAFT CODE:
void Preferences::fontset_changed(int /*i*/) {}
