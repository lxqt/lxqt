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

#include "xdgmenu.h"
#include "xdgmenu_p.h"
#include "xdgmenureader.h"
#include "xmlhelper.h"
#include "xdgmenurules.h"
#include "xdgmenuapplinkprocessor.h"
#include "xdgdirs.h"
#include "xdgmenulayoutprocessor.h"

#include <QDebug>
#include <QtXml/QDomElement>
#include <QtXml/QDomNamedNodeMap>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QSettings>
#include <QDir>
#include <QHash>
#include <QLocale>
#include <QTranslator>
#include <QCoreApplication>
#include <QCryptographicHash>


// Helper functions prototypes
void installTranslation(const QString &name);
bool isParent(const QDomElement& parent, const QDomElement& child);


XdgMenu::XdgMenu(QObject *parent) :
    QObject(parent),
    d_ptr(new XdgMenuPrivate(this))
{
}


XdgMenu::~XdgMenu()
{
    Q_D(XdgMenu);
    delete d;
}


XdgMenuPrivate::XdgMenuPrivate(XdgMenu *parent):
    mOutDated(true),
    q_ptr(parent)
{
    mRebuildDelayTimer.setSingleShot(true);
    mRebuildDelayTimer.setInterval(REBUILD_DELAY);

    connect(&mRebuildDelayTimer, SIGNAL(timeout()), this, SLOT(rebuild()));
    connect(&mWatcher, SIGNAL(fileChanged(QString)), &mRebuildDelayTimer, SLOT(start()));
    connect(&mWatcher, SIGNAL(directoryChanged(QString)), &mRebuildDelayTimer, SLOT(start()));


    connect(this, SIGNAL(changed()), q_ptr, SIGNAL(changed()));
}


const QString XdgMenu::logDir() const
{
    Q_D(const XdgMenu);
    return d->mLogDir;
}


void XdgMenu::setLogDir(const QString& directory)
{
    Q_D(XdgMenu);
    d->mLogDir = directory;
}


const QDomDocument XdgMenu::xml() const
{
    Q_D(const XdgMenu);
    return d->mXml;
}


QString XdgMenu::menuFileName() const
{
    Q_D(const XdgMenu);
    return d->mMenuFileName;
}


QStringList XdgMenu::environments()
{
    Q_D(XdgMenu);
    return d->mEnvironments;
}


void XdgMenu::setEnvironments(const QStringList &envs)
{
    Q_D(XdgMenu);
    d->mEnvironments = envs;
}


void XdgMenu::setEnvironments(const QString &env)
{
    setEnvironments(QStringList() << env);
}


const QString XdgMenu::errorString() const
{
    Q_D(const XdgMenu);
    return d->mErrorString;
}


bool XdgMenu::read(const QString& menuFileName)
{
    Q_D(XdgMenu);

    d->mMenuFileName = menuFileName;

    d->clearWatcher();

    XdgMenuReader reader(this);
    if (!reader.load(d->mMenuFileName))
    {
        qWarning() << reader.errorString();
        d->mErrorString = reader.errorString();
        return false;
    }

    d->mXml = reader.xml();
    QDomElement root = d->mXml.documentElement();
    d->saveLog(QLatin1String("00-reader.xml"));

    d->simplify(root);
    d->saveLog(QLatin1String("01-simplify.xml"));

    d->mergeMenus(root);
    d->saveLog(QLatin1String("02-mergeMenus.xml"));

    d->moveMenus(root);
    d->saveLog(QLatin1String("03-moveMenus.xml"));

    d->mergeMenus(root);
    d->saveLog(QLatin1String("04-mergeMenus.xml"));

    d->deleteDeletedMenus(root);
    d->saveLog(QLatin1String("05-deleteDeletedMenus.xml"));

    d->processDirectoryEntries(root, QStringList());
    d->saveLog(QLatin1String("06-processDirectoryEntries.xml"));

    d->processApps(root);
    d->saveLog(QLatin1String("07-processApps.xml"));

    d->processLayouts(root);
    d->saveLog(QLatin1String("08-processLayouts.xml"));

    d->deleteEmpty(root);
    d->saveLog(QLatin1String("09-deleteEmpty.xml"));

    d->fixSeparators(root);
    d->saveLog(QLatin1String("10-fixSeparators.xml"));


    d->mOutDated = false;
    d->mHash = QCryptographicHash::hash(d->mXml.toByteArray(), QCryptographicHash::Md5);

    return true;
}


void XdgMenu::save(const QString& fileName)
{
    Q_D(const XdgMenu);

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        qWarning() << QString::fromLatin1("Cannot write file %1:\n%2.")
                .arg(fileName, file.errorString());
        return;
    }

    QTextStream ts(&file);
    d->mXml.save(ts, 2);

    file.close();
}


/************************************************
 For debug only
 ************************************************/
void XdgMenuPrivate::load(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        qWarning() << QString::fromLatin1("%1 not loading: %2").arg(fileName, file.errorString());
        return;
    }
    mXml.setContent(&file, true);
}


void XdgMenuPrivate::saveLog(const QString& logFileName)
{
    Q_Q(XdgMenu);
    if (!mLogDir.isEmpty())
        q->save(mLogDir + QLatin1Char('/') + logFileName);
}


void XdgMenuPrivate::mergeMenus(QDomElement& element)
{
    QHash<QString, QDomElement> menus;

    MutableDomElementIterator it(element, QLatin1String("Menu"));

    it.toFront();
    while(it.hasNext())
    {
        it.next();
        menus[it.current().attribute(QLatin1String("name"))] = it.current();
    }


    it.toBack();
    while (it.hasPrevious())
    {
        QDomElement src = it.previous();
        QDomElement dest = menus[src.attribute(QLatin1String("name"))];
        if (dest != src)
        {
            prependChilds(src, dest);
            element.removeChild(src);
        }
    }


    QDomElement n = element.firstChildElement(QLatin1String("Menu"));
    while (!n.isNull())
    {
        mergeMenus(n);
        n = n.nextSiblingElement(QLatin1String("Menu"));
    }

    it.toFront();
    while(it.hasNext())
        mergeMenus(it.next());
}


void XdgMenuPrivate::simplify(QDomElement& element)
{
    MutableDomElementIterator it(element);
    //it.toFront();
    while(it.hasNext())
    {
        QDomElement n = it.next();

        if (n.tagName() == QLatin1String("Name"))
        {
            // The <Name> field must not contain the slash character ("/");
            // implementations should discard any name containing a slash.
            element.setAttribute(QLatin1String("name"), n.text().remove(QLatin1Char('/')));
            n.parentNode().removeChild(n);
        }

        // ......................................
        else if(n.tagName() == QLatin1String("Deleted"))
        {
            element.setAttribute(QLatin1String("deleted"), true);
            n.parentNode().removeChild(n);
        }
        else if(n.tagName() == QLatin1String("NotDeleted"))
        {
            element.setAttribute(QLatin1String("deleted"), false);
            n.parentNode().removeChild(n);
        }

        // ......................................
        else if(n.tagName() == QLatin1String("OnlyUnallocated"))
        {
            element.setAttribute(QLatin1String("onlyUnallocated"), true);
            n.parentNode().removeChild(n);
        }
        else if(n.tagName() == QLatin1String(QLatin1String("NotOnlyUnallocated")))
        {
            element.setAttribute(QLatin1String("onlyUnallocated"), false);
            n.parentNode().removeChild(n);
        }

        // ......................................
        else if(n.tagName() == QLatin1String("FileInfo"))
        {
            n.parentNode().removeChild(n);
        }

        // ......................................
        else if(n.tagName() == QLatin1String("Menu"))
        {
            simplify(n);
        }

    }

}


void XdgMenuPrivate::prependChilds(QDomElement& srcElement, QDomElement& destElement)
{
    MutableDomElementIterator it(srcElement);

    it.toBack();
    while(it.hasPrevious())
    {
        QDomElement n = it.previous();
        destElement.insertBefore(n, destElement.firstChild());
    }

    if (srcElement.attributes().contains(QLatin1String("deleted")) &&
        !destElement.attributes().contains(QLatin1String("deleted"))
       )
        destElement.setAttribute(QLatin1String("deleted"), srcElement.attribute(QLatin1String("deleted")));

    if (srcElement.attributes().contains(QLatin1String("onlyUnallocated")) &&
        !destElement.attributes().contains(QLatin1String("onlyUnallocated"))
       )
        destElement.setAttribute(QLatin1String("onlyUnallocated"), srcElement.attribute(QLatin1String("onlyUnallocated")));
}


void XdgMenuPrivate::appendChilds(QDomElement& srcElement, QDomElement& destElement)
{
    MutableDomElementIterator it(srcElement);

    while(it.hasNext())
        destElement.appendChild(it.next());

    if (srcElement.attributes().contains(QLatin1String("deleted")))
        destElement.setAttribute(QLatin1String("deleted"), srcElement.attribute(QLatin1String("deleted")));

    if (srcElement.attributes().contains(QLatin1String("onlyUnallocated")))
        destElement.setAttribute(QLatin1String("onlyUnallocated"), srcElement.attribute(QLatin1String("onlyUnallocated")));
}


/************************************************
 Search item by path. The path can be absolute or relative. If the element not
 found, the behavior depends on a parameter "createNonExisting." If it's true, then
 the missing items will be created, otherwise the function returns 0.
 ************************************************/
QDomElement XdgMenu::findMenu(QDomElement& baseElement, const QString& path, bool createNonExisting)
{
    Q_D(XdgMenu);
    // Absolute path ..................
    if (path.startsWith(QLatin1Char('/')))
    {
        QDomElement root = d->mXml.documentElement();
        return findMenu(root, path.section(QLatin1Char('/'), 2), createNonExisting);
    }

    // Relative path ..................
    if (path.isEmpty())
        return baseElement;


    QString name = path.section(QLatin1Char('/'), 0, 0);
    MutableDomElementIterator it(baseElement);
    while(it.hasNext())
    {
        QDomElement n = it.next();
        if (n.attribute(QLatin1String("name")) == name)
            return findMenu(n, path.section(QLatin1Char('/'), 1), createNonExisting);
    }



    // Not found ......................
    if (!createNonExisting)
        return QDomElement();


    const QStringList names = path.split(QLatin1Char('/'), QString::SkipEmptyParts);
    QDomElement el = baseElement;
    for (const QString &name : names)
    {
        QDomElement p = el;
        el = d->mXml.createElement(QLatin1String("Menu"));
        p.appendChild(el);
        el.setAttribute(QLatin1String("name"), name);
    }
    return el;

}


bool isParent(const QDomElement& parent, const QDomElement& child)
{
    QDomNode  n = child;
    while (!n.isNull())
    {
        if (n == parent)
            return true;
        n = n.parentNode();
    }
    return false;
}


/************************************************
 Recurse to resolve <Move> elements for each menu starting with any child menu before
 handling the more top level menus. So the deepest menus have their <Move> operations
 performed first. Within each <Menu>, execute <Move> operations in the order that
 they appear.
 If the destination path does not exist, simply relocate the origin <Menu> element,
 and change its <Name> field to match the destination path.
 If the origin path does not exist, do nothing.
 If both paths exist, take the origin <Menu> element, delete its <Name> element, and
 prepend its remaining child elements to the destination <Menu> element.
 ************************************************/
void XdgMenuPrivate::moveMenus(QDomElement& element)
{
    Q_Q(XdgMenu);

    {
        MutableDomElementIterator i(element, QLatin1String("Menu"));
        while(i.hasNext())
            moveMenus(i.next());
    }

    MutableDomElementIterator i(element, QLatin1String("Move"));
    while(i.hasNext())
    {
        i.next();
        QString oldPath = i.current().lastChildElement(QLatin1String("Old")).text();
        QString newPath = i.current().lastChildElement(QLatin1String("New")).text();

        element.removeChild(i.current());

        if (oldPath.isEmpty() || newPath.isEmpty())
            continue;

        QDomElement oldMenu = q->findMenu(element, oldPath, false);
        if (oldMenu.isNull())
            continue;

        QDomElement newMenu = q->findMenu(element, newPath, true);

        if (isParent(oldMenu, newMenu))
            continue;

        appendChilds(oldMenu, newMenu);
        oldMenu.parentNode().removeChild(oldMenu);
    }
}


/************************************************
 For each <Menu> containing a <Deleted> element which is not followed by a
 <NotDeleted> element, remove that menu and all its child menus.

 Kmenuedit create .hidden menu entry, delete it too.
 ************************************************/
void XdgMenuPrivate::deleteDeletedMenus(QDomElement& element)
{
    MutableDomElementIterator i(element, QLatin1String("Menu"));
    while(i.hasNext())
    {
        QDomElement e = i.next();
        if (e.attribute(QLatin1String("deleted")) == QLatin1String("1") ||
            e.attribute(QLatin1String("name")) == QLatin1String(".hidden")
            )
            element.removeChild(e);
        else
            deleteDeletedMenus(e);
    }

}


void XdgMenuPrivate::processDirectoryEntries(QDomElement& element, const QStringList& parentDirs)
{
    QStringList dirs;
    QStringList files;

    element.setAttribute(QLatin1String("title"), element.attribute(QLatin1String("name")));

    MutableDomElementIterator i(element, QString());
    i.toBack();
    while(i.hasPrevious())
    {
        QDomElement e = i.previous();

        if (e.tagName() == QLatin1String("Directory"))
        {
            files << e.text();
            element.removeChild(e);
        }

        else if (e.tagName() == QLatin1String("DirectoryDir"))
        {
            dirs << e.text();
            element.removeChild(e);
        }
    }

    dirs << parentDirs;

    bool found = false;
    for (const QString &file : qAsConst(files)){
        if (file.startsWith(QLatin1Char('/')))
            found = loadDirectoryFile(file, element);
        else
        {
            for (const QString &dir : qAsConst(dirs))
            {
                found = loadDirectoryFile(dir + QLatin1Char('/') + file, element);
                if (found) break;
            }
        }
        if (found) break;
    }


    MutableDomElementIterator it(element, QLatin1String("Menu"));
    while(it.hasNext())
    {
        QDomElement e = it.next();
        processDirectoryEntries(e, dirs);
    }

}


bool XdgMenuPrivate::loadDirectoryFile(const QString& fileName, QDomElement& element)
{
    XdgDesktopFile file;
    file.load(fileName);

    if (!file.isValid())
        return false;


    element.setAttribute(QLatin1String("title"), file.localizedValue(QLatin1String("Name")).toString());
    element.setAttribute(QLatin1String("comment"), file.localizedValue(QLatin1String("Comment")).toString());
    element.setAttribute(QLatin1String("icon"), file.value(QLatin1String("Icon")).toString());

    Q_Q(XdgMenu);
    q->addWatchPath(QFileInfo(file.fileName()).absolutePath());
    return true;
}


void XdgMenuPrivate::processApps(QDomElement& element)
{
    Q_Q(XdgMenu);
    XdgMenuApplinkProcessor processor(element, q);
    processor.run();
}


void XdgMenuPrivate::deleteEmpty(QDomElement& element)
{
    MutableDomElementIterator it(element, QLatin1String("Menu"));
    while(it.hasNext())
        deleteEmpty(it.next());

    if (element.attribute(QLatin1String("keep")) == QLatin1String("true"))
        return;

    QDomElement childMenu = element.firstChildElement(QLatin1String("Menu"));
    QDomElement childApps = element.firstChildElement(QLatin1String("AppLink"));

    if (childMenu.isNull() && childApps.isNull())
    {
        element.parentNode().removeChild(element);
    }
}


void XdgMenuPrivate::processLayouts(QDomElement& element)
{
    XdgMenuLayoutProcessor proc(element);
    proc.run();
}


void XdgMenuPrivate::fixSeparators(QDomElement& element)
{

    MutableDomElementIterator it(element, QLatin1String("Separator"));
    while(it.hasNext())
    {
        QDomElement s = it.next();
        if (s.previousSiblingElement().tagName() == QLatin1String("Separator"))
            element.removeChild(s);
    }


    QDomElement first = element.firstChild().toElement();
    if (first.tagName() == QLatin1String("Separator"))
        element.removeChild(first);

    QDomElement last = element.lastChild().toElement();
    if (last.tagName() == QLatin1String("Separator"))
        element.removeChild(last);


    MutableDomElementIterator mi(element, QLatin1String("Menu"));
    while(mi.hasNext())
        fixSeparators(mi.next());
}


/************************************************
 $XDG_CONFIG_DIRS/menus/${XDG_MENU_PREFIX}applications.menu
 The first file found in the search path should be used; other files are ignored.
 ************************************************/
QString XdgMenu::getMenuFileName(const QString& baseName)
{
    const QStringList configDirs = XdgDirs::configDirs();
    QString menuPrefix = QString::fromLocal8Bit(qgetenv("XDG_MENU_PREFIX"));

    for (const QString &configDir : configDirs)
    {
        QFileInfo file(QString::fromLatin1("%1/menus/%2%3").arg(configDir, menuPrefix, baseName));
        if (file.exists())
            return file.filePath();
    }

    QStringList wellKnownFiles;
    // razor- is a priority for us
    wellKnownFiles << QLatin1String("razor-applications.menu");
    // the "global" menu file name on suse and fedora
    wellKnownFiles << QLatin1String("applications.menu");
    // rest files ordered by priority (descending)
    wellKnownFiles << QLatin1String("kde4-applications.menu");
    wellKnownFiles << QLatin1String("kde-applications.menu");
    wellKnownFiles << QLatin1String("gnome-applications.menu");
    wellKnownFiles << QLatin1String("lxde-applications.menu");

    for (const QString &configDir : configDirs)
    {
        for (const QString &f : qAsConst(wellKnownFiles))
        {
            QFileInfo file(QString::fromLatin1("%1/menus/%2").arg(configDir, f));
            if (file.exists())
                return file.filePath();
        }
    }


    return QString();
}


void XdgMenu::addWatchPath(const QString &path)
{
    Q_D(XdgMenu);

    if (d->mWatcher.files().contains(path))
        return;

    if (d->mWatcher.directories().contains(path))
        return;

    d->mWatcher.addPath(path);
}


bool XdgMenu::isOutDated() const
{
    Q_D(const XdgMenu);
    return d->mOutDated;
}


void XdgMenuPrivate::rebuild()
{
    Q_Q(XdgMenu);
    QByteArray prevHash = mHash;
    q->read(mMenuFileName);

    if (prevHash != mHash)
    {
        mOutDated = true;
        Q_EMIT changed();
    }
}


void XdgMenuPrivate::clearWatcher()
{
    QStringList sl;
    sl << mWatcher.files();
    sl << mWatcher.directories();
    if (sl.length())
        mWatcher.removePaths(sl);
}
