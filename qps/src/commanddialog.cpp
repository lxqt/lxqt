/*
 * commanddialog.cpp
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

#include "commanddialog.h"

#include "qps.h"
#include "command.h"
#include "commandutils.h"
#include "commandmodel.h"

#include <QListView>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QModelIndex>

#include <QLabel>
#include <QMessageBox>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>

extern QList<Command *> commands;

CommandDialog::CommandDialog()
{
    setWindowTitle( tr( "Edit Commands 0.1 alpha" ) );
    // setWindowFlags(Qt::WindowStaysOnTopHint);

    QHBoxLayout *hbox = new QHBoxLayout(this); // TOP
    CommandModel *cmdModel = new CommandModel(this);
    // item list
    listview = new QListView(this);
    listview->setModel(cmdModel);
    listview->setFixedWidth(fontMetrics().width("0") * 16);
    hbox->addWidget(listview);

    QVBoxLayout *vbox = new QVBoxLayout; // TOP-> RIGHT
    hbox->addLayout(vbox);

    QHBoxLayout *h1 = new QHBoxLayout;
    vbox->addLayout(h1);
    QLabel *l1 = new QLabel( tr( "Name:" ), this);
    h1->addWidget(l1);
    name = new QLineEdit(this);
    name->setMinimumWidth(170);
    name->setText("");
    h1->addWidget(name);

    QHBoxLayout *hbox2 = new QHBoxLayout;
    vbox->addLayout(hbox2);
    // qcheck1 = new QCheckBox (this);
    // qcheck1->setText("Toolbar");
    // qcheck1->setEnabled(false);
    // hbox2->addWidget(qcheck1);
    if (false)
    {
        qcheck2 = new QCheckBox(this);
        qcheck2->setText( tr( "Popup" ) );
        qcheck2->setEnabled(false);
        hbox2->addWidget(qcheck2);
    }

    QLabel *l2 = new QLabel( tr( "Command Line:" ), this);
    l2->setFixedHeight(l2->sizeHint().height());
    l2->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    vbox->addWidget(l2);

    cmdline = new QLineEdit(this);
    cmdline->setFixedHeight(cmdline->sizeHint().height());
    cmdline->setMinimumWidth(250);
    cmdline->setText("");
    vbox->addWidget(cmdline);

    QLabel *l3 = new QLabel( tr( "Substitutions:\n"
                                 "%p\tPID\n"
                                 "%c\tCOMMAND\n%C\tCMDLINE\n%u\tUSER\n"
                                 "%%\t%\n"
                                 "\n" ),
                            this);

    l3->setFrameStyle(QFrame::Panel);
    l3->setFrameShadow(QFrame::Sunken);
    l3->setAlignment(Qt::AlignVCenter | Qt::AlignLeft); // | Qt::ExpandTabs);
    vbox->addWidget(l3);

    QHBoxLayout *hl = new QHBoxLayout;
    vbox->addLayout(hl);
    new0 = new QPushButton( tr( "New..." ), this);
    hl->addWidget(new0);
    add = new QPushButton( tr( "Add..." ), this);
    hl->addWidget(add);
    del = new QPushButton( tr( "Delete" ), this);
    hl->addWidget(del);
    button_ok = new QPushButton( tr( "Close" ), this);
    hl->addWidget(button_ok);

    connect(listview, SIGNAL(clicked(const QModelIndex &)),
            SLOT(set_select(const QModelIndex &)));
    connect(new0, SIGNAL(clicked()), SLOT(new_cmd()));
    connect(add, SIGNAL(clicked()), SLOT(add_new()));
    connect(del, SIGNAL(clicked()), SLOT(del_current()));
    connect(button_ok, SIGNAL(clicked()), SLOT(close()));
    connect(name, SIGNAL(textChanged(const QString &)),
            SLOT(event_name_midified(const QString &)));
    connect(cmdline, SIGNAL(textChanged(const QString &)),
            SLOT(event_cmd_modified()));
    // connect(qcheck1, SIGNAL(toggled ( bool ) ),
    // SLOT(event_toolbar_checked(bool
    // )));

    (void) new TBloon(this);
    /// for(int i = 0; i < commands.size(); i++)
    /// listview->insertItem(commands[i]->name);
    /// listview->addItem(commands[i]->name);
    /// vbox->freeze();
}

// DEL
void CommandDialog::event_toolbar_checked(bool on)
{
    // name->text();
    int idx = find_command(name->text());
    if (idx >= 0)
        commands[idx]->toolbar = on;

    /// controlbar->update_bar();
}

void CommandDialog::event_name_midified(const QString &new_name)
{
    int idx;
    FUNC_START;
    // printf("debug:changed_description() start \n");
    idx = find_command(new_name);
    if (idx == -1)
    {
        add->setEnabled(true);
    }
    else
        add->setEnabled(false);

    // printf("debug:changed_description() end \n");
}

// if modified then call this function
void CommandDialog::event_cmd_modified()
{
    int idx;
    // if(name->text()=="") return;
    if (find_command(name->text()) < 0)
        return;

    idx = find_command(name->text());

    commands[idx]->name = name->text();
    commands[idx]->cmdline = cmdline->text();
    emit command_change();
}

//	set the description,cmdline  from current selected QListBox item
void CommandDialog::set_buttons(int index)
{
    if (index < 0)
    {
        new_cmd();
        return;
    }
    /*
    //bool sel = (lb->currentRow() >= 0);
    Command *c ;
    if(sel)
            //c = commands[find_command(lb->currentText())];
            c = commands[find_command(lb->currentText())];
    else
            c = commands[find_command(lb->text(index))];
    name->setText(c->name);
    cmdline->setText(c->cmdline);
    del->setEnabled(sel);
  */
}

//	called when clicked !
void CommandDialog::set_select(const QModelIndex &index)
{
    Command *c =
        static_cast<Command *>(index.internalPointer()); // never Null ?
                                                         /*
                                                                 if (item==NULL) return; // important
                                                                 Command *c = commands[find_command(item->text())];
                                                         */
    name->setText(c->name);
    cmdline->setText(c->cmdline);
    // DEL	qcheck1->setChecked(c->toolbar);
    //	qcheck2->setChecked(c->popup);

    //	bool sel = (listview->currentItem() >= 0);
    if (c->name == "Update")
        del->setEnabled(false);
    else
        del->setEnabled(true);
}

void CommandDialog::reset()
{
    listview->reset();
    name->setText("");
    cmdline->setText("");
    add->setText( tr( "Add..." ) );
    add->setEnabled(false);
    button_ok->setEnabled(true);
    listview->clearSelection();
}

void CommandDialog::new_cmd()
{
    reset();
    add->setEnabled(true);
    name->setFocus();
}

void CommandDialog::add_new()
{
    if (name->text() == "")
        return;

    // commands.add(new Command(name->text(),
    // cmdline->text(),qcheck1->isChecked
    // () ));
    commands.append(new Command(name->text(), cmdline->text(), false));
    check_commandAll(); // TEMP

    listview->reset();
    add->setEnabled(false);
    del->setEnabled(false);
    button_ok->setEnabled(true);

    emit command_change(); // notice to refresh Qps::make_command_menu()
                           //	control_bar->update_bar(); // ** important
}

void CommandDialog::del_current()
{
    int idx = find_command(name->text());
    if (idx >= 0)
    {
        // printf("del\n");
        commands.removeAt(idx);
        listview->reset(); // listview->reset();
        //		control_bar->update_bar();
        emit command_change(); // notice to refresh menu_commands
    }
}
