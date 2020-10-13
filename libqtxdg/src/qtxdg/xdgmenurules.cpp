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

#include "xdgmenurules.h"
#include "xmlhelper.h"

#include <QDebug>
#include <QStringList>


/**
 * See: http://standards.freedesktop.org/desktop-entry-spec
 */

XdgMenuRule::XdgMenuRule(const QDomElement& element, QObject* parent) :
    QObject(parent)
{
    Q_UNUSED(element)
}


XdgMenuRule::~XdgMenuRule()
{
}


/************************************************
 The <Or> element contains a list of matching rules. If any of the matching rules
 inside the <Or> element match a desktop entry, then the entire <Or> rule matches
 the desktop entry.
 ************************************************/
XdgMenuRuleOr::XdgMenuRuleOr(const QDomElement& element, QObject* parent) :
    XdgMenuRule(element, parent)
{
    //qDebug() << "Create OR rule";
    DomElementIterator iter(element, QString());

    while(iter.hasNext())
    {
        QDomElement e = iter.next();

        if (e.tagName() == QLatin1String("Or"))
            mChilds.push_back(new XdgMenuRuleOr(e, this));

        else if (e.tagName() == QLatin1String("And"))
            mChilds.push_back(new XdgMenuRuleAnd(e, this));

        else if (e.tagName() == QLatin1String("Not"))
            mChilds.push_back(new XdgMenuRuleNot(e, this));

        else if (e.tagName() == QLatin1String("Filename"))
            mChilds.push_back(new XdgMenuRuleFileName(e, this));

        else if (e.tagName() == QLatin1String("Category"))
            mChilds.push_back(new XdgMenuRuleCategory(e, this));

        else if (e.tagName() == QLatin1String("All"))
            mChilds.push_back(new XdgMenuRuleAll(e, this));

        else
            qWarning() << QString::fromLatin1("Unknown rule") << e.tagName();
    }

}


bool XdgMenuRuleOr::check(const QString& desktopFileId, const XdgDesktopFile& desktopFile)
{
    for (std::list<XdgMenuRule*>::const_iterator i=mChilds.cbegin(); i!=mChilds.cend(); ++i)
        if ((*i)->check(desktopFileId, desktopFile))  return true;

    return false;
}


/************************************************
 The <And> element contains a list of matching rules. If each of the matching rules
 inside the <And> element match a desktop entry, then the entire <And> rule matches
 the desktop entry.
 ************************************************/
XdgMenuRuleAnd::XdgMenuRuleAnd(const QDomElement& element, QObject *parent) :
    XdgMenuRuleOr(element, parent)
{
//    qDebug() << "Create AND rule";
}


bool XdgMenuRuleAnd::check(const QString& desktopFileId, const XdgDesktopFile& desktopFile)
{
    for (std::list<XdgMenuRule*>::const_iterator i=mChilds.cbegin(); i!=mChilds.cend(); ++i)
        if (!(*i)->check(desktopFileId, desktopFile))  return false;

    //FIXME: Doon't use implicit casts
    return mChilds.size();
}


/************************************************
 The <Not> element contains a list of matching rules. If any of the matching rules
 inside the <Not> element matches a desktop entry, then the entire <Not> rule does
 not match the desktop entry. That is, matching rules below <Not> have a logical OR
 relationship.
 ************************************************/
XdgMenuRuleNot::XdgMenuRuleNot(const QDomElement& element, QObject *parent) :
    XdgMenuRuleOr(element, parent)
{
//    qDebug() << "Create NOT rule";
}


bool XdgMenuRuleNot::check(const QString& desktopFileId, const XdgDesktopFile& desktopFile)
{
    return ! XdgMenuRuleOr::check(desktopFileId, desktopFile);
}


/************************************************
 The <Filename> element is the most basic matching rule. It matches a desktop entry
 if the desktop entry has the given desktop-file id. See Desktop-File Id.
 ************************************************/
XdgMenuRuleFileName::XdgMenuRuleFileName(const QDomElement& element, QObject *parent) :
    XdgMenuRule(element, parent),
    mId(element.text())
{
    //qDebug() << "Create FILENAME rule";
}


bool XdgMenuRuleFileName::check(const QString& desktopFileId, const XdgDesktopFile& desktopFile)
{
    Q_UNUSED(desktopFile)
    //qDebug() << "XdgMenuRuleFileName:" << desktopFileId << mId;
    return desktopFileId == mId;
}


/************************************************
 The <Category> element is another basic matching predicate. It matches a desktop entry
 if the desktop entry has the given category in its Categories field.
 ************************************************/
XdgMenuRuleCategory::XdgMenuRuleCategory(const QDomElement& element, QObject *parent) :
    XdgMenuRule(element, parent),
    mCategory(element.text())
{
}


bool XdgMenuRuleCategory::check(const QString& desktopFileId, const XdgDesktopFile& desktopFile)
{
    Q_UNUSED(desktopFileId)
    QStringList cats = desktopFile.categories();
    return cats.contains(mCategory);
}


/************************************************
 The <All> element is a matching rule that matches all desktop entries.
 ************************************************/
XdgMenuRuleAll::XdgMenuRuleAll(const QDomElement& element, QObject *parent) :
    XdgMenuRule(element, parent)
{
}


bool XdgMenuRuleAll::check(const QString& desktopFileId, const XdgDesktopFile& desktopFile)
{
    Q_UNUSED(desktopFileId)
    Q_UNUSED(desktopFile)
    return true;
}


XdgMenuRules::XdgMenuRules(QObject* parent) :
    QObject(parent)
{
}


XdgMenuRules::~XdgMenuRules()
{
}


void XdgMenuRules::addInclude(const QDomElement& element)
{
    mIncludeRules.push_back(new XdgMenuRuleOr(element, this));
}


void XdgMenuRules::addExclude(const QDomElement& element)
{
    mExcludeRules.push_back(new XdgMenuRuleOr(element, this));
}


bool XdgMenuRules::checkInclude(const QString& desktopFileId, const XdgDesktopFile& desktopFile)
{
    for (std::list<XdgMenuRule*>::const_iterator i=mIncludeRules.cbegin(); i!=mIncludeRules.cend(); ++i)
        if ((*i)->check(desktopFileId, desktopFile))  return true;

    return false;
}


bool XdgMenuRules::checkExclude(const QString& desktopFileId, const XdgDesktopFile& desktopFile)
{
    for (std::list<XdgMenuRule*>::const_iterator i=mExcludeRules.cbegin(); i!=mExcludeRules.cend(); ++i)
        if ((*i)->check(desktopFileId, desktopFile))  return true;

    return false;
}
