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

#include "fbsdtimedatectl.h"
#include <LXQt/Globals>
#include <QProcess>
#include <QDebug>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QMessageBox>


FBSDTimeDateCtl::FBSDTimeDateCtl()
{
}

FBSDTimeDateCtl::~FBSDTimeDateCtl()
{

}

QString FBSDTimeDateCtl::timeZone() const
{
    QFile tzFile(QSL("/var/db/zoneinfo"));
         if (!tzFile.open(QFile::ReadOnly | QFile::Text)) return QSL("Unknown");
         QTextStream in(&tzFile);
             QString lastTZone = in.readLine();
                 return lastTZone;

}

bool FBSDTimeDateCtl::setTimeZone(QString timeZone, QString& errorMessage)
{
    mHelperArgs << QSL("-t") << timeZone;
    return true;
}

bool FBSDTimeDateCtl::setDateTime(QDateTime dateTime, QString& errorMessage)
{
    qint64 epochsec = dateTime.toSecsSinceEpoch();
    mHelperArgs << QSL("-d") << QString::number(epochsec);
    mHelperArgs << QString::number(QDateTime::currentDateTime().toSecsSinceEpoch());
    return true;
}

bool FBSDTimeDateCtl::useNtp() const
{
    QProcess process;
    QStringList args = QStringList();
    args  << QSL("/etc/rc.d/ntpd") << QSL("rcvar");
    process.start(QSL("/bin/sh"),args);
    process.waitForFinished(-1);
    QString t = QString::fromLatin1(process.readAllStandardOutput());
    QStringList o = t.split(QSL("\n"));
    for (QStringList::iterator it = o.begin();
         it != o.end(); ++it) {
        QString current = *it;
        if(!current.startsWith(QSL("#")) && current.contains(QSL("ntpd_enable"),Qt::CaseInsensitive) && current.contains(QSL("yes"),Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool FBSDTimeDateCtl::setUseNtp(bool value, QString& errorMessage)
{
    mHelperArgs << QSL("-n") << (value ? QSL("true") : QSL("false"));
    return true;
}

bool FBSDTimeDateCtl::localRtc() const
{
    return QFileInfo::exists(QSL("/etc/wall_cmos_clock"));
}

bool FBSDTimeDateCtl::setLocalRtc(bool value, QString& errorMessage)
{
    mHelperArgs << QSL("-l") << (value ? QSL("true") : QSL("false"));
    return true;
}
bool FBSDTimeDateCtl::pkexec()
{
    if(mHelperArgs.isEmpty()) {
        return true;
    }
    QProcess process;
    QStringList args;
    args << QSL("--disable-internal-agent")
        << QSL("/usr/local/bin/lxqt-admin-time-helper")
        << mHelperArgs;
    process.start(QSL("pkexec"), args);
    process.waitForFinished(-1);
    QByteArray pkexec_error = process.readAllStandardError();
    qDebug() << pkexec_error;
    const bool succeeded = process.exitCode() == 0;
    if (!succeeded)
    {
        QMessageBox * msg = new QMessageBox{QMessageBox::Critical, QObject::tr("lxqt-admin-time-helper")
            , QObject::tr("<strong>Action (%1) failed:</strong><br/><pre>%2</pre>").arg(args[0]).arg(QString::fromLatin1(pkexec_error))};
        msg->setAttribute(Qt::WA_DeleteOnClose, true);
        msg->show();
    }
    return succeeded;
}
