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

#ifndef QTXDG_XDGMENULAYOUTPROCESSOR_H
#define QTXDG_XDGMENULAYOUTPROCESSOR_H

#include <QtXml/QDomElement>
#include <QList>

struct LayoutItem
{
    enum Type{
        Filename,
        Menuname,
        Separator,
        MergeMenus,
        MergeFiles,
        MergeAll,
    };

    Type type;
    bool showEmpty;
    bool isInline;
    bool inlineLimit;
    bool inlineHeader;
    bool inlineAlias;
    QString fileId;
};

//class Layout: public QList<LayoutItem>
//{
//public:
/*    Layout() {}


    bool showEmpty() { return mShowEmpty; }
    void setShowEmpty(bool value) { mShowEmpty = value; }

    bool isInline() { return mInline; }
    void setInline(bool value) { mInline = value; }

    int inlineLimit() { return mInlineLimit; }
    void setInlineLimit(int value) { mInlineLimit = value; }

    bool inlineHeader() {return mInlineHeader; }
    void setInlineHeader(bool value) { mInlineHeader = value; }

    bool inlineAlias() { return mInlineAlias; }
    void setInlineAlias(bool value) { mInlineAlias = value; }



private:
*/

struct LayoutParams
{
    bool mShowEmpty;
    bool mInline;
    int mInlineLimit;
    bool mInlineHeader;
    bool mInlineAlias;
};


class XdgMenuLayoutProcessor
{
public:
    XdgMenuLayoutProcessor(QDomElement& element);
    void run();

protected:
    XdgMenuLayoutProcessor(QDomElement& element, XdgMenuLayoutProcessor *parent);

private:
    void setParams(QDomElement defaultLayout, LayoutParams *result);
    QDomElement searchElement(const QString &tagName, const QString &attributeName, const QString &attributeValue) const;
    void processFilenameTag(const QDomElement &element);
    void processMenunameTag(const QDomElement &element);
    void processSeparatorTag(const QDomElement &element);
    void processMergeTag(const QDomElement &element);

    LayoutParams mDefaultParams;
    QDomElement& mElement;
    QDomElement mDefaultLayout;
    QDomElement mLayout;
    QDomElement mResult;
};

#endif // QTXDG_XDGMENULAYOUTPROCESSOR_H
