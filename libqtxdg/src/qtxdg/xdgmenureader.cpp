/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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

#include "xdgmenureader.h"
#include "xdgmenu.h"
#include "xdgdirs.h"
#include "xmlhelper.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QtXml/QDomNamedNodeMap>
#include <QtXml/QDomNode>


XdgMenuReader::XdgMenuReader(XdgMenu* menu, XdgMenuReader*  parentReader, QObject *parent) :
    QObject(parent),
    mMenu(menu)
{
    mParentReader = parentReader;
    if (mParentReader)
        mBranchFiles << mParentReader->mBranchFiles;
}


XdgMenuReader::~XdgMenuReader()
{

}


bool XdgMenuReader::load(const QString& fileName, const QString& baseDir)
{
    if (fileName.isEmpty())
    {
        mErrorStr = QLatin1String("Menu file not defined.");
        return false;
    }

    QFileInfo fileInfo(QDir(baseDir), fileName);

    mFileName = fileInfo.canonicalFilePath();
    mDirName = fileInfo.canonicalPath();

    if (mBranchFiles.contains(mFileName))
        return false; // Recursive loop detected

    mBranchFiles << mFileName;

    QFile file(mFileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        mErrorStr = QString::fromLatin1("%1 not loading: %2").arg(fileName, file.errorString());
        return false;
    }
    //qDebug() << "Load file:" << mFileName;
    mMenu->addWatchPath(mFileName);

    QString errorStr;
    int errorLine;
    int errorColumn;

    if (!mXml.setContent(&file, true, &errorStr, &errorLine, &errorColumn))
    {
        mErrorStr = QString::fromLatin1("Parse error at line %1, column %2:\n%3")
                        .arg(errorLine)
                        .arg(errorColumn)
                        .arg(errorStr);
       return false;
    }

    QDomElement root = mXml.documentElement();

    QDomElement debugElement = mXml.createElement(QLatin1String("FileInfo"));
    debugElement.setAttribute(QLatin1String("file"), mFileName);
    if (mParentReader)
        debugElement.setAttribute(QLatin1String("parent"), mParentReader->fileName());

    QDomNode null;
    root.insertBefore(debugElement, null);

    processMergeTags(root);
    return true;
}


/************************************************
 Duplicate <MergeXXX> elements (that specify the same file) are handled as with
 duplicate <AppDir> elements (the last duplicate is used).
 ************************************************/
void XdgMenuReader::processMergeTags(QDomElement& element)
{
    QDomElement n = element.lastChildElement();
    QStringList mergedFiles;


    while (!n.isNull())
    {
        QDomElement next = n.previousSiblingElement();
        // MergeFile ..................
        if (n.tagName() == QLatin1String("MergeFile"))
        {
            processMergeFileTag(n, &mergedFiles);
            n.parentNode().removeChild(n);
        }

        // MergeDir ...................
        else if(n.tagName() == QLatin1String("MergeDir"))
        {
            processMergeDirTag(n, &mergedFiles);
            n.parentNode().removeChild(n);
        }

        // DefaultMergeDirs ...........
        else if (n.tagName() == QLatin1String("DefaultMergeDirs"))
        {
            processDefaultMergeDirsTag(n, &mergedFiles);
            n.parentNode().removeChild(n);
        }

        // AppDir ...................
        else if(n.tagName() == QLatin1String("AppDir"))
        {
            processAppDirTag(n);
            n.parentNode().removeChild(n);
        }

        // DefaultAppDirs .............
        else if(n.tagName() == QLatin1String("DefaultAppDirs"))
        {
            processDefaultAppDirsTag(n);
            n.parentNode().removeChild(n);
        }

        // DirectoryDir ...................
        else if(n.tagName() == QLatin1String("DirectoryDir"))
        {
            processDirectoryDirTag(n);
            n.parentNode().removeChild(n);
        }

        // DefaultDirectoryDirs ...........
        else if(n.tagName() == QLatin1String("DefaultDirectoryDirs"))
        {
            processDefaultDirectoryDirsTag(n);
            n.parentNode().removeChild(n);
        }


        // Menu .......................
        else if(n.tagName() == QLatin1String("Menu"))
        {
            processMergeTags(n);
        }

        n = next;
    }

}


/************************************************
 Any number of <MergeFile> elements may be listed below a <Menu> element, giving
 the name of another menu file to be merged into this one.
 If fileName is not an absolute path then the file to be merged should be located
 relative to the location of this menu.

 If the type attribute is missing or set to "path" then the contents of the
 <MergeFile> element indicates the file to be merged.

 If the type attribute is set to "parent" and the file that contains this
 <MergeFile> element is located under one of the paths specified by
 $XDG_CONFIG_DIRS, the contents of the element should be ignored and the remaining
 paths specified by $XDG_CONFIG_DIRS are searched for a file with the same relative
 filename. The first file encountered should be merged. There should be no merging
 at all if no matching file is found. ( Libmenu additional scans ~/.config/menus.)
 ************************************************/
void XdgMenuReader::processMergeFileTag(QDomElement& element, QStringList* mergedFiles)
{
    //qDebug() << "Process " << element;// << "in" << mFileName;

    if (element.attribute(QLatin1String("type")) != QLatin1String("parent"))
    {
        mergeFile(element.text(), element, mergedFiles);
    }

    else
    {
        QString relativeName;
        QStringList configDirs = XdgDirs::configDirs();

        for (const QString &configDir : qAsConst(configDirs))
        {
            if (mFileName.startsWith(configDir))
            {
                relativeName = mFileName.mid(configDir.length());
                configDirs.removeAll(configDir);
                break;
            }
        }


        if (relativeName.isEmpty())
        {
            QString configHome = XdgDirs::configHome();
            if (mFileName.startsWith(configHome))
                relativeName = mFileName.mid(configHome.length());
        }

        if (relativeName.isEmpty())
            return;

        for (const QString &configDir : qAsConst(configDirs))
        {
            if (QFileInfo::exists(configDir + relativeName))
            {
                mergeFile(configDir + relativeName, element, mergedFiles);
                return;
            }
        }
    }
}


/************************************************
 A <MergeDir> contains the name of a directory. Each file in the given directory
 which ends in the ".menu" extension should be merged in the same way that a
 <MergeFile> would be. If the filename given as a <MergeDir> is not an absolute
 path, it should be located relative to the location of the menu file being parsed.
 The files inside the merged directory are not merged in any specified order.

 Duplicate <MergeDir> elements (that specify the same directory) are handled as with
 duplicate <AppDir> elements (the last duplicate is used).

 KDE additional scans ~/.config/menus.
 ************************************************/
void XdgMenuReader::processMergeDirTag(QDomElement& element, QStringList* mergedFiles)
{
    //qDebug() << "Process " << element;// << "in" << mFileName;

    mergeDir(element.text(), element, mergedFiles);
    element.parentNode().removeChild(element);
}


/************************************************
 The element has no content. The element should be treated as if it were a list of
 <MergeDir> elements containing the default merge directory locations. When expanding
 <DefaultMergeDirs> to a list of <MergeDir>, the default locations that are earlier
 in the search path go later in the <Menu> so that they have priority.

 Note that a system that uses either gnome-applications.menu or kde-applications.menu
 depending on the desktop environment in use must still use applications-merged as the
 default merge directory in both cases.

 Implementations may chose to use .menu files with names other than application.menu
 for tasks or menus other than the main application menu. In that case the first part
 of the name of the default merge directory is derived from the name of the .menu file.
 ************************************************/
void XdgMenuReader::processDefaultMergeDirsTag(QDomElement& element, QStringList* mergedFiles)
{
    //qDebug() << "Process " << element;// << "in" << mFileName;

    QString menuBaseName = QFileInfo(mMenu->menuFileName()).baseName();
    int n = menuBaseName.lastIndexOf(QLatin1Char('-'));
    if (n>-1)
        menuBaseName = menuBaseName.mid(n+1);

    QStringList dirs = XdgDirs::configDirs();
    dirs << XdgDirs::configHome();

    for (const QString &dir : qAsConst(dirs))
    {
        mergeDir(QString::fromLatin1("%1/menus/%2-merged").arg(dir, menuBaseName), element, mergedFiles);
    }

    if (menuBaseName == QLatin1String("applications"))
        mergeFile(QString::fromLatin1("%1/menus/applications-kmenuedit.menu").arg(XdgDirs::configHome()), element, mergedFiles);
}


/************************************************
 If the filename given as an <AppDir> is not an absolute path, it should be located
 relative to the location of the menu file being parsed.
 ************************************************/
void XdgMenuReader::processAppDirTag(QDomElement& element)
{
    //qDebug() << "Process " << element;
    addDirTag(element, QLatin1String("AppDir"), element.text());
}


/************************************************
 The element has no content. The element should be treated as if it were a list of
 <AppDir> elements containing the default app dir locations
 ($XDG_DATA_DIRS/applications/).

 menu-cache additional prepends $XDG_DATA_HOME/applications.
 ************************************************/
void XdgMenuReader::processDefaultAppDirsTag(QDomElement& element)
{
    //qDebug() << "Process " << element;
    QStringList dirs = XdgDirs::dataDirs();
    dirs.prepend(XdgDirs::dataHome(false));

    for (const QString &dir : qAsConst(dirs))
    {
        //qDebug() << "Add AppDir: " << dir + "/applications/";
        addDirTag(element, QLatin1String("AppDir"), dir + QLatin1String("/applications/"));
    }
}

/************************************************
 If the filename given as a <DirectoryDir> is not an absolute path, it should be
 located relative to the location of the menu file being parsed.
 ************************************************/
void XdgMenuReader::processDirectoryDirTag(QDomElement& element)
{
    //qDebug() << "Process " << element;
    addDirTag(element, QLatin1String("DirectoryDir"), element.text());
}


/************************************************
 The element has no content. The element should be treated as if it were a list of
 <DirectoryDir> elements containing the default desktop dir locations
 ($XDG_DATA_DIRS/desktop-directories/).

 menu-cache additional prepends $XDG_DATA_HOME/applications.
 ************************************************/
void XdgMenuReader::processDefaultDirectoryDirsTag(QDomElement& element)
{
    //qDebug() << "Process " << element;
    QStringList dirs = XdgDirs::dataDirs();
    dirs.prepend(XdgDirs::dataHome(false));

    for (const QString &dir : qAsConst(dirs))
        addDirTag(element, QLatin1String("DirectoryDir"), dir + QLatin1String("/desktop-directories/"));
}

/************************************************

 ************************************************/
void XdgMenuReader::addDirTag(QDomElement& previousElement, const QString& tagName, const QString& dir)
{
    QFileInfo dirInfo(mDirName, dir);
    if (dirInfo.isDir())
    {
//        qDebug() << "\tAdding " + dirInfo.canonicalFilePath();
        QDomElement element = mXml.createElement(tagName);
        element.appendChild(mXml.createTextNode(dirInfo.canonicalFilePath()));
        previousElement.parentNode().insertBefore(element, previousElement);
    }
}

/*
 If fileName is not an absolute path then the file to be merged should be located
 relative to the location of this menu file.
 ************************************************/
void XdgMenuReader::mergeFile(const QString& fileName, QDomElement& element, QStringList* mergedFiles)
{
    XdgMenuReader reader(mMenu, this);
    QFileInfo fileInfo(QDir(mDirName), fileName);

    if (!fileInfo.exists())
        return;

    if (mergedFiles->contains(fileInfo.canonicalFilePath()))
    {
        //qDebug() << "\tSkip: allredy merged";
        return;
    }

    //qDebug() << "Merge file: " << fileName;
    mergedFiles->append(fileInfo.canonicalFilePath());

    if (reader.load(fileName, mDirName))
    {
        //qDebug() << "\tOK";
        QDomElement n = reader.xml().firstChildElement().firstChildElement();
        while (!n.isNull())
        {
            // As a special exception, remove the <Name> element from the root
            // element of each file being merged.
            if (n.tagName() != QLatin1String("Name"))
            {
                QDomNode imp = mXml.importNode(n, true);
                element.parentNode().insertBefore(imp, element);
            }

            n = n.nextSiblingElement();
        }
    }
}


void XdgMenuReader::mergeDir(const QString& dirName, QDomElement& element, QStringList* mergedFiles)
{
    QFileInfo dirInfo(mDirName, dirName);

    if (dirInfo.isDir())
    {
        //qDebug() << "Merge dir: " << dirInfo.canonicalFilePath();
        QDir dir = QDir(dirInfo.canonicalFilePath());
        const QFileInfoList files = dir.entryInfoList(QStringList() << QLatin1String("*.menu"), QDir::Files | QDir::Readable);

        for (const QFileInfo &file : files)
            mergeFile(file.canonicalFilePath(), element, mergedFiles);
    }
}
