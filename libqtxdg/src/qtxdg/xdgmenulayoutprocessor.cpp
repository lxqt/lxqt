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

#include "xdgmenulayoutprocessor.h"
#include "xmlhelper.h"
#include <QDebug>
#include <QMap>


// Helper functions prototypes
QDomElement findLastElementByTag(const QDomElement element, const QString tagName);
int childsCount(const QDomElement& element);


QDomElement findLastElementByTag(const QDomElement element, const QString tagName)
{
    QDomNodeList l = element.elementsByTagName(tagName);
    if (l.isEmpty())
        return QDomElement();

    return l.at(l.length()-1).toElement();
}


/************************************************
 If no default-layout has been specified then the layout as specified by
 the following elements should be assumed:
 <DefaultLayout
     show_empty="false"
     inline="false"
     inline_limit="4"
     inline_header="true"
     inline_alias="false">
     <Merge type="menus"/>
     <Merge type="files"/>
 </DefaultLayout>
 ************************************************/
XdgMenuLayoutProcessor::XdgMenuLayoutProcessor(QDomElement& element):
    mElement(element),
    mDefaultLayout(findLastElementByTag(element, QLatin1String("DefaultLayout")))
{
    mDefaultParams.mShowEmpty = false;
    mDefaultParams.mInline = false;
    mDefaultParams.mInlineLimit = 4;
    mDefaultParams.mInlineHeader = true;
    mDefaultParams.mInlineAlias = false;

    if (mDefaultLayout.isNull())
    {
        // Create DefaultLayout node
        QDomDocument doc = element.ownerDocument();
        mDefaultLayout = doc.createElement(QLatin1String("DefaultLayout"));

        QDomElement menus = doc.createElement(QLatin1String("Merge"));
        menus.setAttribute(QLatin1String("type"), QLatin1String("menus"));
        mDefaultLayout.appendChild(menus);

        QDomElement files = doc.createElement(QLatin1String("Merge"));
        files.setAttribute(QLatin1String("type"), QLatin1String("files"));
        mDefaultLayout.appendChild(files);

        mElement.appendChild(mDefaultLayout);
    }

    setParams(mDefaultLayout, &mDefaultParams);

    // If a menu does not contain a <Layout> element or if it contains an empty <Layout> element
    // then the default layout should be used.
    mLayout = findLastElementByTag(element, QLatin1String("Layout"));
    if (mLayout.isNull() || !mLayout.hasChildNodes())
        mLayout = mDefaultLayout;
}


XdgMenuLayoutProcessor::XdgMenuLayoutProcessor(QDomElement& element, XdgMenuLayoutProcessor *parent):
	mDefaultParams(parent->mDefaultParams),
    mElement(element)
{
    // DefaultLayout ............................
    QDomElement defaultLayout = findLastElementByTag(element, QLatin1String("DefaultLayout"));

    if (defaultLayout.isNull())
        mDefaultLayout = parent->mDefaultLayout;
    else
        mDefaultLayout = defaultLayout;

    setParams(mDefaultLayout, &mDefaultParams);

    // If a menu does not contain a <Layout> element or if it contains an empty <Layout> element
    // then the default layout should be used.
    mLayout = findLastElementByTag(element, QLatin1String("Layout"));
    if (mLayout.isNull() || !mLayout.hasChildNodes())
        mLayout = mDefaultLayout;

}


void XdgMenuLayoutProcessor::setParams(QDomElement defaultLayout, LayoutParams *result)
{
    if (defaultLayout.hasAttribute(QLatin1String("show_empty")))
        result->mShowEmpty = defaultLayout.attribute(QLatin1String("show_empty")) == QLatin1String("true");

    if (defaultLayout.hasAttribute(QLatin1String("inline")))
        result->mInline = defaultLayout.attribute(QLatin1String("inline")) == QLatin1String("true");

    if (defaultLayout.hasAttribute(QLatin1String("inline_limit")))
        result->mInlineLimit = defaultLayout.attribute(QLatin1String("inline_limit")).toInt();

    if (defaultLayout.hasAttribute(QLatin1String("inline_header")))
        result->mInlineHeader = defaultLayout.attribute(QLatin1String("inline_header")) == QLatin1String("true");

    if (defaultLayout.hasAttribute(QLatin1String("inline_alias")))
        result->mInlineAlias = defaultLayout.attribute(QLatin1String("inline_alias")) == QLatin1String("true");
}


QDomElement XdgMenuLayoutProcessor::searchElement(const QString &tagName, const QString &attributeName, const QString &attributeValue) const
{
    DomElementIterator it(mElement, tagName);
    while (it.hasNext())
    {
        QDomElement e = it.next();
        if (e.attribute(attributeName) == attributeValue)
        {
            return e;
        }
    }

    return QDomElement();
}


int childsCount(const QDomElement& element)
{
    int count = 0;
    DomElementIterator it(element);
    while (it.hasNext())
    {
        QString tag = it.next().tagName();
        if (tag == QLatin1String("AppLink") || tag == QLatin1String("Menu") || tag == QLatin1String("Separator"))
            count ++;
    }

    return count;
}


void XdgMenuLayoutProcessor::run()
{
    QDomDocument doc = mLayout.ownerDocument();
    mResult = doc.createElement(QLatin1String("Result"));
    mElement.appendChild(mResult);

    // Process childs menus ...............................
    {
        DomElementIterator it(mElement, QLatin1String("Menu"));
        while (it.hasNext())
        {
            QDomElement e = it.next();
            XdgMenuLayoutProcessor p(e, this);
            p.run();
        }
    }


    // Step 1 ...................................
    DomElementIterator it(mLayout);
    it.toFront();
    while (it.hasNext())
    {
        QDomElement e = it.next();

        if (e.tagName() == QLatin1String("Filename"))
            processFilenameTag(e);

        else if (e.tagName() == QLatin1String("Menuname"))
            processMenunameTag(e);

        else if (e.tagName() == QLatin1String("Separator"))
            processSeparatorTag(e);

        else if (e.tagName() == QLatin1String("Merge"))
        {
            QDomElement merge = mResult.ownerDocument().createElement(QLatin1String("Merge"));
            merge.setAttribute(QLatin1String("type"), e.attribute(QLatin1String("type")));
            mResult.appendChild(merge);
        }
    }

    // Step 2 ...................................
    {
        MutableDomElementIterator ri(mResult, QLatin1String("Merge"));
        while (ri.hasNext())
        {
            processMergeTag(ri.next());
        }
    }

    // Move result cilds to element .............
    MutableDomElementIterator ri(mResult);
    while (ri.hasNext())
    {
        mElement.appendChild(ri.next());
    }

    // Final ....................................
    mElement.removeChild(mResult);

    if (mLayout.parentNode() == mElement)
        mElement.removeChild(mLayout);

    if (mDefaultLayout.parentNode() == mElement)
        mElement.removeChild(mDefaultLayout);

}


/************************************************
 The <Filename> element is the most basic matching rule.
 It matches a desktop entry if the desktop entry has the given desktop-file id
 ************************************************/
void XdgMenuLayoutProcessor::processFilenameTag(const QDomElement &element)
{
    QString id = element.text();

    QDomElement appLink = searchElement(QLatin1String("AppLink"), QLatin1String("id"), id);
    if (!appLink.isNull())
        mResult.appendChild(appLink);
}


/************************************************
 Its contents references an immediate sub-menu of the current menu, as such it should never contain
 a slash. If no such sub-menu exists the element should be ignored.
 This element may have various attributes, the default values are taken from the DefaultLayout key.

 show_empty [ bool ]
    defines whether a menu that contains no desktop entries and no sub-menus
    should be shown at all.

 inline [ bool ]
    If the inline attribute is "true" the menu that is referenced may be copied into the current
    menu at the current point instead of being inserted as sub-menu of the current menu.

 inline_limit [ int ]
    defines the maximum number of entries that can be inlined. If the sub-menu has more entries
    than inline_limit, the sub-menu will not be inlined. If the inline_limit is 0 (zero) there
    is no limit.

 inline_header [ bool ]
    defines whether an inlined menu should be preceded with a header entry listing the caption of
    the sub-menu.

 inline_alias [ bool ]
    defines whether a single inlined entry should adopt the caption of the inlined menu. In such
    case no additional header entry will be added regardless of the value of the inline_header
    attribute.

 Example: if a menu has a sub-menu titled "WordProcessor" with a single entry "OpenOffice 4.2", and
 both inline="true" and inline_alias="true" are specified then this would result in the
 "OpenOffice 4.2" entry being inlined in the current menu but the "OpenOffice 4.2" caption of the
 entry would be replaced with "WordProcessor".
 ************************************************/
void XdgMenuLayoutProcessor::processMenunameTag(const QDomElement &element)
{
    QString id = element.text();
    QDomElement menu = searchElement(QLatin1String("Menu"), QLatin1String("name"), id);
    if (menu.isNull())
        return;

    LayoutParams params = mDefaultParams;
    setParams(element, &params);

    int count = childsCount(menu);

    if (count == 0)
    {
        if (params.mShowEmpty)
        {
            menu.setAttribute(QLatin1String("keep"), QLatin1String("true"));
            mResult.appendChild(menu);
        }
        return;
    }


    bool doInline = params.mInline &&
                    (!params.mInlineLimit || params.mInlineLimit > count);

    bool doAlias = params.mInlineAlias &&
                   doInline && (count == 1);

    bool doHeader = params.mInlineHeader &&
                    doInline && !doAlias;


    if (!doInline)
    {
        mResult.appendChild(menu);
        return;
    }


    // Header ....................................
    if (doHeader)
    {
        QDomElement header = mLayout.ownerDocument().createElement(QLatin1String("Header"));

        QDomNamedNodeMap attrs = menu.attributes();
        for (int i=0; i < attrs.count(); ++i)
        {
            header.setAttributeNode(attrs.item(i).toAttr());
        }

        mResult.appendChild(header);
    }

    // Alias .....................................
    if (doAlias)
    {
        menu.firstChild().toElement().setAttribute(QLatin1String("title"), menu.attribute(QLatin1String("title")));
    }

    // Inline ....................................
    MutableDomElementIterator it(menu);
    while (it.hasNext())
    {
        mResult.appendChild(it.next());
    }

}


/************************************************
 It indicates a suggestion to draw a visual separator at this point in the menu.
 <Separator> elements at the start of a menu, at the end of a menu or that directly
 follow other <Separator> elements may be ignored.
 ************************************************/
void XdgMenuLayoutProcessor::processSeparatorTag(const QDomElement &element)
{
    QDomElement separator = element.ownerDocument().createElement(QLatin1String("Separator"));
    mResult.appendChild(separator);
}


/************************************************
 It indicates the point where desktop entries and sub-menus that are not explicitly mentioned
 within the <Layout> or <DefaultLayout> element are to be inserted.
 It has a type attribute that indicates which elements should be inserted:

 type="menus"
    means that all sub-menus that are not explicitly mentioned should be inserted in alphabetical
    order of their visual caption at this point.

 type="files" means that all desktop entries contained in this menu that are not explicitly
    mentioned should be inserted in alphabetical order of their visual caption at this point.

 type="all" means that a mix of all sub-menus and all desktop entries that are not explicitly
    mentioned should be inserted in alphabetical order of their visual caption at this point.

 ************************************************/
void XdgMenuLayoutProcessor::processMergeTag(const QDomElement &element)
{
    QString type = element.attribute(QLatin1String("type"));
    QMap<QString, QDomElement> map;
    MutableDomElementIterator it(mElement);

    while (it.hasNext())
    {
        QDomElement e = it.next();
        if (
            ((type == QLatin1String("menus") || type == QLatin1String("all")) && e.tagName() == QLatin1String("Menu" )) ||
            ((type == QLatin1String("files") || type == QLatin1String("all")) && e.tagName() == QLatin1String("AppLink"))
           )
            map.insert(e.attribute(QLatin1String("title")), e);
    }

    QMapIterator<QString, QDomElement> mi(map);
    while (mi.hasNext()) {
        mi.next();
        mResult.insertBefore(mi.value(), element);
    }

    mResult.removeChild(element);
}
