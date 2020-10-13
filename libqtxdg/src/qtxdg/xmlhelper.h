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

#ifndef QTXDG_XMLHELPER_H
#define QTXDG_XMLHELPER_H

#include "xdgmacros.h"
#include <QDebug>
#include <QtXml/QDomElement>
#include <QtXml/QDomNode>

class QTXDG_API DomElementIterator
{
public:
    explicit DomElementIterator(const QDomNode& parentNode, const QString& tagName = QString())
        : mTagName(tagName),
          mParent(parentNode)
    {
        toFront();
    }

    void toFront()
    {
        mNext = mParent.firstChildElement(mTagName);
    }

    bool hasNext()
    {
        return (!mNext.isNull());
    }

    const QDomElement& next()
    {
        mCur = mNext;
        mNext = mNext.nextSiblingElement(mTagName);
        return mCur;
    }


    void toBack()
    {
        mNext = mParent.lastChildElement(mTagName);
    }


    bool hasPrevious()
    {
        return (!mNext.isNull());
    }

    const QDomElement& previous()
    {
        mCur = mNext;
        mNext = mNext.previousSiblingElement(mTagName);
        return mCur;
    }

    const QDomElement& current() const
    {
        return mCur;
    }


private:
    QString  mTagName;
    QDomNode mParent;
    QDomElement mCur;
    QDomElement mNext;
};

class MutableDomElementIterator
{
public:
    explicit MutableDomElementIterator(QDomNode& parentNode, const QString& tagName = QString())
        : mTagName(tagName),
          mParent(parentNode)
    {
        toFront();
    }

    void toFront()
    {
        mNext = mParent.firstChildElement(mTagName);
    }

    bool hasNext()
    {
        return (!mNext.isNull());
    }

    QDomElement& next()
    {
        mCur = mNext;
        mNext = mNext.nextSiblingElement(mTagName);
        return mCur;
    }


    void toBack()
    {
        mNext = mParent.lastChildElement(mTagName);
    }


    bool hasPrevious()
    {
        return (!mNext.isNull());
    }

    QDomElement& previous()
    {
        mCur = mNext;
        mNext = mNext.previousSiblingElement(mTagName);
        return mCur;
    }

    QDomElement& current()
    {
        return mCur;
    }


private:
    QString  mTagName;
    QDomNode mParent;
    QDomElement mCur;
    QDomElement mNext;
};





QDebug operator<<(QDebug dbg, const QDomElement &el);


#endif // QTXDG_XMLHELPER_H
