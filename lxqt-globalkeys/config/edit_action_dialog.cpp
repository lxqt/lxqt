/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
 * Authors:
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#include "edit_action_dialog.h"
#include "actions.h"
#include <QPushButton>
EditActionDialog::EditActionDialog(Actions *actions, QWidget *parent)
    : QDialog(parent)
    , mActions(actions)
{
    setupUi(this);
    buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    shortcut_SS->setActions(mActions);
    connect(this, SIGNAL(accepted()), SLOT(when_accepted()));
}
void EditActionDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type())
    {
    case QEvent::LanguageChange:
        retranslateUi(this);
        break;
    default:
        break;
    }
}

static QString joinCommandLine(const QString &command, QStringList arguments)
{
    arguments.prepend(command);
    int m = arguments.length();
    for (int i = 0; i < m; ++i)
    {
        QString &item = arguments[i];
        if (item.contains(QRegExp(QStringLiteral("[ \r\n\t\"']"))))
        {
            item.prepend(QLatin1Char('\'')).append(QLatin1Char('\''));
        }
        else if (item.isEmpty())
        {
            item = QString::fromLatin1("''");
        }
    }
    return arguments.join(QLatin1Char(' '));
}

static QStringList splitCommandLine(QString commandLine)
{
    commandLine.prepend(QLatin1Char(' ')).append(QLatin1Char(' '));
    QStringList result;
    QRegExp spacePattern(QStringLiteral("\\s+"));
    QRegExp itemPattern(QStringLiteral("([^ \r\n\t\"']+)|((\"([^\"]|\\\")*\")|('([^']|\\')*'))(?=\\s)"));

    for (int pos = 0; ;)
    {
        if (commandLine.indexOf(spacePattern, pos) != pos)
        {
            return QStringList();
        }
        pos += spacePattern.matchedLength();

        if (pos == commandLine.length())
        {
            break;
        }

        if (commandLine.indexOf(itemPattern, pos) != pos)
        {
            return QStringList();
        }
        pos += itemPattern.matchedLength();

        QString item = itemPattern.cap(2);
        if (item.length())
        {
            result << item.mid(1, item.length() - 2);
        }
        else
        {
            result << itemPattern.cap(1);
        }
    }
    return result;
}

void EditActionDialog::when_accepted()
{
    if (mId)
    {
        if (command_RB->isChecked())
        {
            QStringList commandLine = splitCommandLine(command_PTE->toPlainText());
            if (!commandLine.length())
            {
                return;
            }
            mActions->modifyCommandAction(mId, commandLine[0], commandLine.mid(1), description_LE->text());
        }
        else if (dbus_method_RB->isChecked())
        {
            mActions->modifyMethodAction(mId, service_LE->text(), QDBusObjectPath(path_LE->text()), interface_LE->text(), method_LE->text(), description_LE->text());
        }
        mActions->changeShortcut(mId, mShortcut);
        mActions->enableAction(mId, enabled_CB->isChecked());
    }
    else
    {
        QPair<QString, qulonglong> result = qMakePair(QString(), 0ull);
        if (command_RB->isChecked())
        {
            QStringList commandLine = splitCommandLine(command_PTE->toPlainText());
            if (!commandLine.length())
            {
                return;
            }
            result = mActions->addCommandAction(mShortcut, commandLine[0], commandLine.mid(1), description_LE->text());
        }
        else if (dbus_method_RB->isChecked())
        {
            result = mActions->addMethodAction(mShortcut, service_LE->text(), QDBusObjectPath(path_LE->text()), interface_LE->text(), method_LE->text(), description_LE->text());
        }
        if (result.second && !enabled_CB->isChecked())
        {
            mActions->enableAction(result.second, false);
        }
    }
}

bool EditActionDialog::load(qulonglong id)
{
    mId = id;

    if (mId)
    {
        QPair<bool, GeneralActionInfo> info = mActions->actionById(id);
        if (!info.first)
        {
            return false;
        }

        bool canEdit = ((info.second.type == QLatin1String("command")) || (info.second.type == QLatin1String("method")));

        mShortcut = info.second.shortcut;
        shortcut_SS->setText(mShortcut);
        description_LE->setText(info.second.description);
        enabled_CB->setChecked(info.second.enabled);
        command_RB->setChecked(info.second.type == QLatin1String("command"));
        dbus_method_RB->setChecked(info.second.type == QLatin1String("method"));
        action_SW->setCurrentWidget((info.second.type == QLatin1String("method")) ? dbus_method_P : command_P);
        if (info.second.type == QLatin1String("command"))
        {
            QPair<bool, CommandActionInfo> commandInfo = mActions->commandActionInfoById(id);
            if (!commandInfo.first)
            {
                return false;
            }
            command_PTE->setPlainText(joinCommandLine(commandInfo.second.command, commandInfo.second.arguments));
        }
        else if (info.second.type == QLatin1String("method"))
        {
            QPair<bool, MethodActionInfo> methodInfo = mActions->methodActionInfoById(id);
            if (!methodInfo.first)
            {
                return false;
            }
            service_LE->setText(methodInfo.second.service);
            path_LE->setText(methodInfo.second.path.path());
            interface_LE->setText(methodInfo.second.interface);
            method_LE->setText(methodInfo.second.method);
        }
        else
        {
            command_PTE->clear();

            // To disable all radio boxes, once one has been selected, we need to turn exclusivity off temporarily
            command_RB->setAutoExclusive(false);
            dbus_method_RB->setAutoExclusive(false);
            command_RB->setChecked(false);
            dbus_method_RB->setChecked(false);
            command_RB->setAutoExclusive(true);
            dbus_method_RB->setAutoExclusive(true);
        }

        description_LE->setEnabled(canEdit);
        command_RB->setEnabled(false);
        dbus_method_RB->setEnabled(false);
        action_SW->setEnabled(canEdit);
    }
    else
    {
        shortcut_SS->setText(QString());
        description_LE->clear();
        enabled_CB->setChecked(true);
        action_SW->setCurrentWidget(command_P);
        command_PTE->clear();
        service_LE->clear();
        path_LE->clear();
        interface_LE->clear();
        method_LE->clear();

        // To disable all radio boxes, once one has been selected, we need to turn exclusivity off temporarily
        command_RB->setAutoExclusive(false);
        dbus_method_RB->setAutoExclusive(false);
        command_RB->setChecked(false);
        dbus_method_RB->setChecked(false);
        command_RB->setAutoExclusive(true);
        dbus_method_RB->setAutoExclusive(true);

        description_LE->setEnabled(true);
        command_RB->setEnabled(true);
        dbus_method_RB->setEnabled(true);
        action_SW->setEnabled(false);
    }

    return true;
}

void EditActionDialog::on_shortcut_SS_shortcutGrabbed(const QString &shortcut)
{
    mShortcut = shortcut;
    shortcut_SS->setText(shortcut);
}

void EditActionDialog::on_command_RB_clicked(bool checked)
{
    if (checked)
    {
        action_SW->setCurrentWidget(command_P);
        action_SW->setEnabled(true);
    }
}

void EditActionDialog::on_dbus_method_RB_clicked(bool checked)
{
    if (checked)
    {
        action_SW->setCurrentWidget(dbus_method_P);
        action_SW->setEnabled(true);
    }
}
