/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *   Palo Kisa <palo.kisa@gmail.com>
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

#ifndef SUDO_H
#define SUDO_H

#include <QObject>
#include <QScopedPointer>
#include <QStringList>

class PasswordDialog;

class Sudo : public QObject
{
    Q_OBJECT

public:
    enum backend_t
    {
        BACK_NONE
            , BACK_SUDO
            , BACK_SU
    };

public:
    Sudo();
    ~Sudo();
    int main();

    /// A name of the backend for the purpose of the display to the user
    QString backendName() {
        return backendName(mBackend);
    }
    /// A static version of previous one.
    /// @returns a backend name for the given \p backEnd.
    static QString backendName (backend_t backEnd);
    ///
    /// @returns a squashed and quoted string of arguments suitable to passsed to shell
    /// @arg userFriendly  -  if true, performes a smarter quoting (e.g. no quoting for
    ///                       string with no special characters)
    QString squashedArgs (bool userFriendly=0) const;

private:
    //parent methods
    int parent();
    void stopChild();

    //child methods
    void child();

private:
    QScopedPointer<PasswordDialog> mDlg;
    QStringList mArgs;
    backend_t mBackend;

    int mChildPid;
    int mPwdFd;
    int mRet;
};

#endif //SUDO_H
