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

#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include <QStringList>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

class QFileSystemWatcher;

class UserInfo
{
public:
    explicit UserInfo():mUid(0), mGid(0) {
    }
    explicit UserInfo(struct passwd* pw):
        mUid(pw->pw_uid),
        mGid(pw->pw_gid),
        mName(QString::fromLatin1(pw->pw_name)),
        mFullName(QString::fromUtf8(pw->pw_gecos)),
        mShell(QString::fromLocal8Bit(pw->pw_shell)),
        mHomeDir(QString::fromLocal8Bit(pw->pw_dir))
    {
    }

    uid_t uid()const {
        return mUid;
    }
    void setUid(uid_t uid) {
        mUid = uid;
    }

    gid_t gid() const {
        return mGid;
    }

    void setGid(gid_t gid) {
        mGid = gid;
    }

    QString name() const {
        return mName;
    }

    void setName(const QString& name) {
        mName = name;
    }

    QString fullName() const {
        return mFullName;
    }
    void setFullName(const QString& fullName) {
        mFullName = fullName;
    }

    QString shell() const {
        return mShell;
    }
    void setShell(const QString& shell) {
        mShell = shell;
    }

    QString homeDir() const {
        return mHomeDir;
    }
    void setHomeDir(const QString& homeDir) {
        mHomeDir = homeDir;
    }

    const QStringList& groups() const {
        return mGroups;
    }

    void addGroup(const QString& group) {
        mGroups.append(group);
    }

    void removeGroup(const QString& group) {
        mGroups.removeOne(group);
    }

    void removeAllGroups() {
        mGroups.clear();
    }

    bool hasGroup(const QString& group) {
        return mGroups.contains(group);
    }

private:
    uid_t mUid;
    gid_t mGid;
    QString mName;
    QString mFullName;
    QString mShell;
    QString mHomeDir;
    QStringList mGroups;
};

class GroupInfo
{
public:
    explicit GroupInfo(): mGid(0) {
    }
    explicit GroupInfo(struct group* grp):
        mGid(grp->gr_gid),
        mName(QString::fromUtf8(grp->gr_name))
    {
    }

    gid_t gid() const {
        return mGid;
    }
    void setGid(gid_t gid) {
        mGid = gid;
    }

    QString name() const {
        return mName;
    }
    void setName(const QString& name) {
        mName = name;
    }

    const QStringList& members() const {
        return mMembers;
    }

    void setMembers(const QStringList& members) {
        mMembers = members;
    }

    void addMember(const QString& userName) {
        mMembers.append(userName);
    }

    void removeMember(const QString& userName) {
        mMembers.removeOne(userName);
    }

    void removeAllMemberss() {
        mMembers.clear();
    }

    bool hasMember(const QString& userName) const {
        return mMembers.contains(userName);
    }

private:
    gid_t mGid;
    QString mName;
    QStringList mMembers;
};

class UserManager : public QObject
{
    Q_OBJECT
public:
    explicit UserManager(QObject *parent = 0);
    ~UserManager();

    const QList<UserInfo*>& users() const {
        return mUsers;
    }

    const QList<GroupInfo*>& groups() const {
        return mGroups;
    }

    const QStringList& availableShells();

    bool addUser(UserInfo* user);
    bool modifyUser(UserInfo* user, UserInfo* newSettings);
    bool deleteUser(UserInfo* user);
    bool changePassword(UserInfo* user, QByteArray newPasswd);

    bool addGroup(GroupInfo* group);
    bool modifyGroup(GroupInfo* group, GroupInfo* newSettings);
    bool deleteGroup(GroupInfo* group);
    bool changePassword(GroupInfo* group, QByteArray newPasswd);

    // FIXME: add APIs to change group membership with "gpasswd"

    UserInfo* findUserInfo(const char* name);
    UserInfo* findUserInfo(QString name);
    UserInfo* findUserInfo(uid_t uid);

    GroupInfo* findGroupInfo(const char* name);
    GroupInfo* findGroupInfo(QString name);
    GroupInfo* findGroupInfo(gid_t gid);

private:
    void loadUsersAndGroups();
    void loadLoginDefs();
    bool pkexec(const QStringList &command, const QByteArray &stdinData = QByteArray());

Q_SIGNALS:
    void changed();

protected Q_SLOTS:
    void onFileChanged(const QString &path);
    void reload();

private:
    QList<UserInfo*> mUsers;
    QList<GroupInfo*> mGroups;
    QFileSystemWatcher* mWatcher;
    QStringList mAvailableShells;
};

#endif // USERMANAGER_H
