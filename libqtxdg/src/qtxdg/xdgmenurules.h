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

#ifndef QTXDG_XDGMENURULES_H
#define QTXDG_XDGMENURULES_H

#include <QObject>
#include <QtXml/QDomElement>

#include <list>

#include "xdgdesktopfile.h"


/**
 * See: http://standards.freedesktop.org/desktop-entry-spec
 */

class XdgMenuRule : public QObject
{
    Q_OBJECT
public:
    explicit XdgMenuRule(const QDomElement& element, QObject* parent = nullptr);
    ~XdgMenuRule() override;

    virtual bool check(const QString& desktopFileId, const XdgDesktopFile& desktopFile) = 0;
};


class XdgMenuRuleOr : public XdgMenuRule
{
    Q_OBJECT
public:
    explicit XdgMenuRuleOr(const QDomElement& element, QObject* parent = nullptr);

    bool check(const QString& desktopFileId, const XdgDesktopFile& desktopFile) override;

protected:
    std::list<XdgMenuRule*> mChilds;
};


class XdgMenuRuleAnd : public XdgMenuRuleOr
{
    Q_OBJECT
public:
    explicit XdgMenuRuleAnd(const QDomElement& element, QObject* parent = nullptr);
    bool check(const QString& desktopFileId, const XdgDesktopFile& desktopFile) override;
};


class XdgMenuRuleNot : public XdgMenuRuleOr
{
    Q_OBJECT
public:
    explicit XdgMenuRuleNot(const QDomElement& element, QObject* parent = nullptr);
    bool check(const QString& desktopFileId, const XdgDesktopFile& desktopFile) override;
};


class XdgMenuRuleFileName : public XdgMenuRule
{
    Q_OBJECT
public:
    explicit XdgMenuRuleFileName(const QDomElement& element, QObject* parent = nullptr);
    bool check(const QString& desktopFileId, const XdgDesktopFile& desktopFile) override;
private:
    QString mId;
};


class XdgMenuRuleCategory : public XdgMenuRule
{
    Q_OBJECT
public:
    explicit XdgMenuRuleCategory(const QDomElement& element, QObject* parent = nullptr);
    bool check(const QString& desktopFileId, const XdgDesktopFile& desktopFile) override;
private:
    QString mCategory;
};


class XdgMenuRuleAll : public XdgMenuRule
{
    Q_OBJECT
public:
    explicit XdgMenuRuleAll(const QDomElement& element, QObject* parent = nullptr);
    bool check(const QString& desktopFileId, const XdgDesktopFile& desktopFile) override;
};



class XdgMenuRules : public QObject
{
    Q_OBJECT
public:
    explicit XdgMenuRules(QObject* parent = nullptr);
    ~XdgMenuRules() override;

    void addInclude(const QDomElement& element);
    void addExclude(const QDomElement& element);

    bool checkInclude(const QString& desktopFileId, const XdgDesktopFile& desktopFile);
    bool checkExclude(const QString& desktopFileId, const XdgDesktopFile& desktopFile);

protected:
    std::list<XdgMenuRule*> mIncludeRules;
    std::list<XdgMenuRule*> mExcludeRules;
};

#endif // QTXDG_XDGMENURULES_H
