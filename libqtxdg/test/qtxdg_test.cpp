/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013~2015 LXQt team
 * Authors:
 *   Christian Surlykke <christian@surlykke.dk>
 *   Jerome Leclanche <jerome@leclan.ch>
 *   Luís Pereira <luis.artur.pereira@gmail.com>
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


#include "qtxdg_test.h"

#include "xdgdesktopfile.h"
#include "xdgdesktopfile_p.h"
#include "xdgdirs.h"
#include "xdgmimeapps.h"

#include <QtTest>

#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include <QDebug>
#include <QSettings>

void QtXdgTest::testDefaultApp()
{
    QStringList mimedirs = XdgDirs::dataDirs();
    mimedirs.prepend(XdgDirs::dataHome(false));
    for (const QString &mimedir : qAsConst(mimedirs))
    {
        QDir dir(mimedir + QStringLiteral("/mime"));
        qDebug() << dir.path();
        QStringList filters = (QStringList() << QStringLiteral("*.xml"));
        const QFileInfoList &mediaDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &mediaDir : mediaDirs)
        {
            qDebug() << "    " << mediaDir.fileName();
            const QStringList mimeXmlFileNames = QDir(mediaDir.absoluteFilePath()).entryList(filters, QDir::Files);
            for (const QString &mimeXmlFileName : mimeXmlFileNames)
            {
                QString mimetype = mediaDir.fileName() + QLatin1Char('/') + mimeXmlFileName.left(mimeXmlFileName.length() - 4);
                QString xdg_utils_default = xdgUtilDefaultApp(mimetype);
                QString desktop_file_default = xdgDesktopFileDefaultApp(mimetype);

                if (xdg_utils_default != desktop_file_default)
                {
                    qDebug() << mimetype;
                    qDebug() << "xdg-mime query default:" << xdg_utils_default;
                    qDebug() << "xdgdesktopfile default:" << desktop_file_default;
                }
            }
        }

    }
}

void QtXdgTest::compare(QString mimetype)
{
    QString xdgUtilDefault = xdgUtilDefaultApp(mimetype);
    QString xdgDesktopDefault = xdgDesktopFileDefaultApp(mimetype);
    if (xdgUtilDefault != xdgDesktopDefault)
    {
        qDebug() << mimetype;
        qDebug() << "xdg-mime default:" << xdgUtilDefault;
        qDebug() << "xdgDesktopfile default:" << xdgDesktopDefault;
    }
}


void QtXdgTest::testTextHtml()
{
    compare(QStringLiteral("text/html"));
}

void QtXdgTest::testMeldComparison()
{
    compare(QStringLiteral("application/x-meld-comparison"));
}

void QtXdgTest::testCustomFormat()
{
    QSettings::Format desktopFormat = QSettings::registerFormat(QStringLiteral("list"), readDesktopFile, writeDesktopFile);
    QFile::remove(QStringLiteral("/tmp/test.list"));
    QFile::remove(QStringLiteral("/tmp/test2.list"));
    QSettings test(QStringLiteral("/tmp/test.list"), desktopFormat);
    test.beginGroup(QStringLiteral("Default Applications"));
    test.setValue(QStringLiteral("text/plain"), QStringLiteral("gvim.desktop"));
    test.setValue(QStringLiteral("text/html"), QStringLiteral("firefox.desktop"));
    test.endGroup();
    test.beginGroup(QStringLiteral("Other Applications"));
    test.setValue(QStringLiteral("application/pdf"), QStringLiteral("qpdfview.desktop"));
    test.setValue(QStringLiteral("image/svg+xml"), QStringLiteral("inkscape.desktop"));
    test.sync();

    QFile::copy(QStringLiteral("/tmp/test.list"), QStringLiteral("/tmp/test2.list"));

    QSettings test2(QStringLiteral("/tmp/test2.list"), desktopFormat);
    QVERIFY(test2.allKeys().size() == 4);

    test2.beginGroup(QStringLiteral("Default Applications"));
//    qDebug() << test2.value("text/plain");
    QVERIFY(test2.value(QStringLiteral("text/plain")) == QLatin1String("gvim.desktop"));

//    qDebug() << test2.value("text/html");
    QVERIFY(test2.value(QStringLiteral("text/html")) == QLatin1String("firefox.desktop"));
    test2.endGroup();

    test2.beginGroup(QStringLiteral("Other Applications"));
//    qDebug() << test2.value("application/pdf");
    QVERIFY(test2.value(QStringLiteral("application/pdf")) == QLatin1String("qpdfview.desktop"));

//    qDebug() << test2.value("image/svg+xml");
    QVERIFY(test2.value(QStringLiteral("image/svg+xml")) == QStringLiteral("inkscape.desktop"));
    test2.endGroup();
}


QString QtXdgTest::xdgDesktopFileDefaultApp(QString mimetype)
{
    XdgMimeApps appsDb;
    XdgDesktopFile *defaultApp = appsDb.defaultApp(mimetype);
    QString defaultAppS;
    if (defaultApp)
    {
        defaultAppS = QFileInfo(defaultApp->fileName()).fileName();
    }
    return defaultAppS;
}



QString QtXdgTest::xdgUtilDefaultApp(QString mimetype)
{
    QProcess xdg_mime;
    QString program = QStringLiteral("xdg-mime");
    QStringList args = (QStringList() << QStringLiteral("query") << QStringLiteral("default") << mimetype);
    qDebug() << "running" << program << args.join(QLatin1Char(' '));
    xdg_mime.start(program, args);
    xdg_mime.waitForFinished(1000);
    return QString::fromUtf8(xdg_mime.readAll()).trimmed();
}

#if 0
int main(int argc, char** args)
{
//    QtXdgTest().testDefaultApp();
//      qDebug() << "Default for text/html:" << QtXdgTest().xdgDesktopFileDefaultApp("text/html");
//    QtXdgTest().testMeldComparison();
    qDebug() << QtXdgTest().testCustomFormat();
};
#endif // 0

QTEST_MAIN(QtXdgTest)
