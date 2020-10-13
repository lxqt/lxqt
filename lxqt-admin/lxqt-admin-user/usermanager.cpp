/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2016 LXQt team
 * Authors:
 *   Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#include "usermanager.h"

#include <LXQt/Globals>

#include <QDebug>
#include <algorithm>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QProcess>
#include <QFile>
#include <QMessageBox>
#include <QLatin1String>
#include <unistd.h>

static const QString PASSWD_FILE = QStringLiteral("/etc/passwd");
static const QString GROUP_FILE = QStringLiteral("/etc/group");
static const QString LOGIN_DEFS_FILE = QStringLiteral("/etc/login.defs");

UserManager::UserManager(QObject *parent):
    QObject(parent),
    mWatcher(new QFileSystemWatcher(QStringList() << PASSWD_FILE << GROUP_FILE, this))
{
    loadUsersAndGroups();
    connect(mWatcher, &QFileSystemWatcher::fileChanged, this, &UserManager::onFileChanged);
}

UserManager::~UserManager() {
    qDeleteAll(mUsers);
    qDeleteAll(mGroups);
}

void UserManager::loadUsersAndGroups()
{
    // Note: getpwent(), getgrent() makes no attempt to suppress duplicate information
    // if multiple sources are specified in nsswitch.conf(5).

    // load groups
    setgrent();
    struct group * grp;
    while((grp = getgrent())) {
        if (mGroups.cend() != std::find_if(mGroups.cbegin(), mGroups.cend(), [grp] (const GroupInfo * g) -> bool { return g->gid() == grp->gr_gid; }))
            continue;
        GroupInfo* group = new GroupInfo(grp);
        mGroups.append(group);
        // add members of this group
        for(char** member_name = grp->gr_mem; *member_name; ++member_name) {
            group->addMember(QString::fromLatin1(*member_name));
        }
    }
    endgrent();
    std::sort(mGroups.begin(), mGroups.end(), [](GroupInfo* g1, GroupInfo* g2) {
        return g1->name() < g2->name();
    });

    // load users
    setpwent();
    struct passwd * pw;
    while((pw = getpwent())) {
        if (mUsers.cend() != std::find_if(mUsers.cbegin(), mUsers.cend(), [pw] (const UserInfo * u) -> bool { return u->uid() == pw->pw_uid; }))
            continue;
        UserInfo* user = new UserInfo(pw);
        mUsers.append(user);
        // add groups to this user
        for(const GroupInfo* group: qAsConst(mGroups)) {
            if(group->hasMember(user->name())) {
                user->addGroup(group->name());
            }
        }
    }
    endpwent();
    std::sort(mUsers.begin(), mUsers.end(), [](UserInfo*& u1, UserInfo*& u2) {
        return u1->name() < u2->name();
    });
}

// load settings from /etc/login.defs
void UserManager::loadLoginDefs() {
    // FIXME: parse /etc/login.defs to get max UID, max system UID...etc.
    QFile file(LOGIN_DEFS_FILE);
    if(file.open(QIODevice::ReadOnly)) {
        while(!file.atEnd()) {
            QByteArray line = file.readLine().trimmed();
            if(line.isEmpty() || line.startsWith('#'))
                continue;
            QStringList parts = QString::fromUtf8(line).split(QRegExp(QSL("\\s")), QString::SkipEmptyParts);
            if(parts.length() >= 2) {
                QString& key = parts[0];
                QString& val = parts[1];
                if(key == QLatin1String("SYS_UID_MIN")) {
                }
                else if(key == QLatin1String("SYS_UID_MAX")) {
                }
                else if(key == QLatin1String("UID_MIN")) {
                }
                else if(key == QLatin1String("UID_MAX")) {
                }
                else if(key == QLatin1String("SYS_GID_MIN")) {
                }
                else if(key == QLatin1String("SYS_GID_MAX")) {
                }
                else if(key == QLatin1String("GID_MIN")) {
                }
                else if(key == QLatin1String("GID_MAX")) {
                }
            }
        }
        file.close();
    }
}


UserInfo* UserManager::findUserInfo(const char* name) {
    auto it = std::find_if(mUsers.begin(), mUsers.end(), [name](const UserInfo* user) {
        return user->name() == QString::fromUtf8(name);
    });
    return it != mUsers.end() ? *it : nullptr;
}

UserInfo* UserManager::findUserInfo(QString name) {
    auto it = std::find_if(mUsers.begin(), mUsers.end(), [name](const UserInfo* user) {
        return user->name() == name;
    });
    return it != mUsers.end() ? *it : nullptr;
}

UserInfo* UserManager::findUserInfo(uid_t uid) {
    auto it = std::find_if(mUsers.begin(), mUsers.end(), [uid](const UserInfo* user) {
        return user->uid() == uid;
    });
    return it != mUsers.end() ? *it : nullptr;
}

GroupInfo* UserManager::findGroupInfo(const char* name) {
    auto it = std::find_if(mGroups.begin(), mGroups.end(), [name](const GroupInfo* group) {
        return group->name() == QString::fromUtf8(name);
    });
    return it != mGroups.end() ? *it : nullptr;
}

GroupInfo* UserManager::findGroupInfo(QString name) {
    auto it = std::find_if(mGroups.begin(), mGroups.end(), [name](const GroupInfo* group) {
        return group->name() == name;
    });
    return it != mGroups.end() ? *it : nullptr;
}

GroupInfo* UserManager::findGroupInfo(gid_t gid) {
    auto it = std::find_if(mGroups.begin(), mGroups.end(), [gid](const GroupInfo* group) {
        return group->gid() == gid;
    });
    return it != mGroups.end() ? *it : nullptr;
}

void UserManager::reload() {
    mWatcher->addPath(PASSWD_FILE);
    mWatcher->addPath(GROUP_FILE);

    qDeleteAll(mUsers);  // free the old UserInfo objects
    mUsers.clear();

    qDeleteAll(mGroups);  // free the old GroupInfo objects
    mGroups.clear();

    loadUsersAndGroups();
    Q_EMIT changed();
}

void UserManager::onFileChanged(const QString &path) {
    // QFileSystemWatcher is very broken and has a ridiculous design.
    // we get "fileChanged()" when the file is deleted or modified,
    // but there is no way to distinguish them. If the file is deleted,
    // the QFileSystemWatcher stop working silently. Hence we workaround
    // this by remove the paths from the watcher and add them back again
    // to force the creation of new notifiers.
    mWatcher->removePath(PASSWD_FILE);
    mWatcher->removePath(GROUP_FILE);
    QTimer::singleShot(500, this, &UserManager::reload);
}

bool UserManager::pkexec(const QStringList& command, const QByteArray& stdinData) {
    Q_ASSERT(!command.isEmpty());
    QProcess process;
    qDebug() << command;
    QStringList args;
    args << QStringLiteral("--disable-internal-agent")
        << QStringLiteral("lxqt-admin-user-helper")
        << command;
    process.start(QStringLiteral("pkexec"), args);
    if(!stdinData.isEmpty()) {
        process.waitForStarted();
        process.write(stdinData);
        process.waitForBytesWritten();
        process.closeWriteChannel();
    }
    process.waitForFinished(-1);
    QByteArray pkexec_error = process.readAllStandardError();
    qDebug() << pkexec_error;
    const bool succeeded = process.exitCode() == 0;
    if (!succeeded)
    {
        QMessageBox * msg = new QMessageBox{QMessageBox::Critical, tr("lxqt-admin-user")
            , tr("<strong>Action (%1) failed:</strong><br/><pre>%2</pre>").arg(command[0], QString::fromUtf8(pkexec_error))};
        msg->setAttribute(Qt::WA_DeleteOnClose, true);
        msg->show();
    }
    return succeeded;
}

bool UserManager::addUser(UserInfo* user) {
    if(!user || user->name().isEmpty())
        return false;
    QStringList command;
    command << QStringLiteral("useradd");
    if(user->uid() != 0) {
        command << QStringLiteral("-u") << QString::number(user->uid());
    }
    if(!user->homeDir().isEmpty()) {
        command << QStringLiteral("-d") << user->homeDir();
        command << QStringLiteral("-m"); // create the user's home directory if it does not exist.
    }
    if(!user->shell().isEmpty()) {
        command << QStringLiteral("-s") << user->shell();
    }
    if(!user->fullName().isEmpty()) {
        command << QStringLiteral("-c") << user->fullName();
    }
    if(user->gid() != 0) {
        command << QStringLiteral("-g") << QString::number(user->gid());
    }
    if(!user->groups().isEmpty()) {  // set group membership
        command << QStringLiteral("-G") << user->groups().join(QL1C(','));
    }
#ifdef Q_OS_FREEBSD
    command << QStringLiteral("-n");
#endif
    command << user->name();
    return pkexec(command);
}

bool UserManager::modifyUser(UserInfo* user, UserInfo* newSettings) {
    if(!user || user->name().isEmpty() || !newSettings)
        return false;

    bool isDirty = false;
    QStringList command;
    command << QStringLiteral("usermod");
    if(newSettings->uid() != user->uid()) {
        command << QStringLiteral("-u") << QString::number(newSettings->uid());
	isDirty=true;
    }
    if(newSettings->homeDir() != user->homeDir()) {
        command << QStringLiteral("-d") << newSettings->homeDir();
	isDirty=true;
    }
    if(newSettings->shell() != user->shell()) {
        command << QStringLiteral("-s") << newSettings->shell();
	isDirty=true;
    }

    if(newSettings->fullName() != user->fullName()) {
        command << QStringLiteral("-c") << newSettings->fullName();
	isDirty=true;
    }
    if(newSettings->gid() != user->gid()) {
        command << QStringLiteral("-g") << QString::number(newSettings->gid());
	isDirty=true;
    }
    if(newSettings->name() != user->name()) {	  // change login name
        command << QStringLiteral("-l") << newSettings->name();
	isDirty=true;
    }
    if(newSettings->groups() != user->groups()) {  // change group membership
        command << QStringLiteral("-G") << newSettings->groups().join(QL1C(','));
	isDirty=true;
    }
#ifdef Q_OS_FREEBSD
    command << QStringLiteral("-n");
#endif
    command << user->name();
    if(isDirty) {
        return pkexec(command);
    }
    return true; //No changes
}

bool UserManager::deleteUser(UserInfo* user) {
    if(!user || user->name().isEmpty())
        return false;

    QStringList command;
    command << QStringLiteral("userdel");
    command << user->name();
    return pkexec(command);
}

bool UserManager::changePassword(UserInfo* user, QByteArray newPasswd) {
    // In theory, the current user should be able to use "passwd" to
    // reset his/her own password without root permission, but...
    // /usr/bin/passwd is a setuid program running as root and QProcess
    // does not seem to capture its stdout... So... requires root for now.
    if(geteuid() == user->uid()) {
        // FIXME: there needs to be a way to let a user change his/her own password.
        // Maybe we can use our pkexec helper script to achieve this.
    }
    QStringList command;
    command << QStringLiteral("passwd");
    command << user->name();

    // we need to type the new password for two times.
    QByteArray stdinData;
    stdinData += newPasswd;
    stdinData += "\n";
    stdinData += newPasswd;
    stdinData += "\n";
    return pkexec(command, stdinData);
}

bool UserManager::addGroup(GroupInfo* group) {
    if(!group || group->name().isEmpty())
        return false;

    QStringList command;
    command << QStringLiteral("groupadd");
    if(group->gid() != 0) {
        command << QStringLiteral("-g") << QString::number(group->gid());
    }
    command << group->name();
    return pkexec(command);
}

bool UserManager::modifyGroup(GroupInfo* group, GroupInfo* newSettings) {
    if(!group || group->name().isEmpty() || !newSettings)
        return false;
    QStringList command;
    bool isDirty = false;
    command << QStringLiteral("groupmod");
    if(newSettings->gid() != group->gid()) {
        command << QStringLiteral("-g") << QString::number(newSettings->gid());
        isDirty = true;
       }
    if(newSettings->name() != group->name()) {
        isDirty = true;
#ifdef Q_OS_FREEBSD
        command << QStringLiteral("-l");
#else
        command << QStringLiteral("-n");
#endif
        command << newSettings->name();
    }
#ifdef Q_OS_FREEBSD
    if(newSettings->members() != group->members()) {
        isDirty = true;
        command << QStringLiteral("-M");  // Set the list of group members.
        command << newSettings->members().join(QL1C(','));
    }
    command << QStringLiteral("-n");
#endif
    command << group->name();

    if(isDirty && !pkexec(command))
        return false;

    // if group members are changed, use gpasswd to reset members on linux
#ifndef Q_OS_FREEBSD //This is already done with pw groupmod -M earlier.
    if(newSettings->members() != group->members()) {
        command.clear();
        command << QStringLiteral("gpasswd");
        command << QStringLiteral("-M");  // Set the list of group members.
        command << newSettings->members().join(QL1C(','));
        //if the group name changed the group->name() is still the old setting.
        if(newSettings->name() != group->name()) {
           command << newSettings->name();
           } else {
           command << group->name();
        }

        return pkexec(command);
    }
#endif
    return true;
}

bool UserManager::deleteGroup(GroupInfo* group) {
    if(!group || group->name().isEmpty())
        return false;
    QStringList command;
    command << QStringLiteral("groupdel");
    command << group->name();
    return pkexec(command);
}

bool UserManager::changePassword(GroupInfo* group, QByteArray newPasswd) {
    QStringList command;
    command << QStringLiteral("gpasswd");
    command << group->name();

    // we need to type the new password for two times.
    QByteArray stdinData = newPasswd;
    stdinData += "\n";
    stdinData += newPasswd;
    stdinData += "\n";
    return pkexec(command, stdinData);
}

const QStringList& UserManager::availableShells() {
    if(mAvailableShells.isEmpty()) {
        QFile file(QSL("/etc/shells"));
        if(file.open(QIODevice::ReadOnly)) {
            while(!file.atEnd()) {
                QByteArray line = file.readLine().trimmed();
                if(line.isEmpty() || line.startsWith('#'))
                    continue;
                mAvailableShells.append(QString::fromLocal8Bit(line));
            }
            file.close();
        }
    }
    return mAvailableShells;
}

