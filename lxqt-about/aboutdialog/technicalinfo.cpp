/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
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


#include "technicalinfo.h"
#include <LXQt/Translator>

#include <XdgDirs>

using namespace LXQt;

class TechInfoTable
{
public:
    TechInfoTable(const QString &title);
    void add(const QString &name, const QVariant &value);
    QString html() const;
    QString text(int nameFieldWidth) const;
    int maxNameLength() const;
private:
    QString mTitle;
    QList<QPair<QString,QString> > mRows;
};



TechInfoTable::TechInfoTable(const QString &title)
{
    mTitle = title;
}

void TechInfoTable::add(const QString &name, const QVariant &value)
{
    QPair<QString,QString> row;
    row.first = name;
    row.second = value.toString();
    mRows.append(row);
}

QString TechInfoTable::html() const
{
    QString res;

    res = QStringLiteral("<style TYPE='text/css'> "
            ".techInfoKey { white-space: nowrap ; margin: 0 20px 0 16px; } "
          "</style>");

    res += QStringLiteral("<b>%1</b>").arg(mTitle);
    res += QLatin1String("<table width='100%'>");
    QPair<QString,QString> row;
    for(const auto& row : qAsConst(mRows))
    {
        res += QStringLiteral("<tr>"
                       "<td class=techInfoTd width='1%'>"
                            "<div class=techInfoKey>%1</div>"
                        "</td>"
                        "<td>%2</td>"
                       "</tr>").arg(row.first, row.second);
    }

    res += QLatin1String("</table>");
    return res;
}

QString TechInfoTable::text(int nameFieldWidth) const
{
    QString res;
    res += QStringLiteral("%1\n").arg(mTitle);

    QPair<QString,QString> row;
    for(const auto& row : qAsConst(mRows))
    {
        res += QStringLiteral("  %1  %2\n")
                .arg(row.first + QStringLiteral(":"), -nameFieldWidth)
                .arg(row.second);
    }
    return res;
}

int TechInfoTable::maxNameLength() const
{
    int res = 0;
    QPair<QString,QString> row;
    for(const auto& row : qAsConst(mRows))
        res = qMax(res, row.first.length());

    return res;
}


QString TechnicalInfo::html() const
{
    QString res;
    for(const TechInfoTable* item : qAsConst(mItems))
    {
        res += item->html();
        res += QLatin1String("<br><br>");
    }
    return res;
}

QString TechnicalInfo::text() const
{
    int nameWidth = 0;
    for(const TechInfoTable* item : qAsConst(mItems))
        nameWidth = qMax(nameWidth, item->maxNameLength());

    QString res;
    for(const TechInfoTable* item : qAsConst(mItems))
    {
        res += item->text(nameWidth + 2);
        res += QLatin1String("\n\n");
    }
    return res;
}

TechInfoTable *TechnicalInfo::newTable(const QString &title)
{
    TechInfoTable *table = new TechInfoTable(title);
    mItems.append(table);
    return table;
}

TechnicalInfo::~TechnicalInfo()
{
    qDeleteAll(mItems);
}


TechnicalInfo::TechnicalInfo()
{
    TechInfoTable *table;

    // ******************************************
    table = newTable(QStringLiteral("LXQt Desktop Toolbox - Technical Info<p>"));
#ifdef DEBUG
    QString buildType(QStringLiteral("Debug"));
#else
    QString buildType(QStringLiteral("Release"));
#endif

    table->add(QStringLiteral("LXQt About Version"),   QStringLiteral(LXQT_ABOUT_VERSION));
    table->add(QStringLiteral("LXQt Version"),         QStringLiteral(LXQT_VERSION));
    table->add(QStringLiteral("Qt"),                   QLatin1String(qVersion()));
    table->add(QStringLiteral("Build type"),           buildType);
    table->add(QStringLiteral("System Configuration"), QStringLiteral(LXQT_ETC_XDG_DIR));
    table->add(QStringLiteral("Share Directory"),      QStringLiteral(LXQT_SHARE_DIR));
    table->add(QStringLiteral("Translations"),         Translator::translationSearchPaths().join(QStringLiteral("<br>\n")));


    // ******************************************
    table = newTable(QStringLiteral("User Directories"));
    XdgDirs xdgDirs;

    table->add(QStringLiteral("Xdg Data Home"),        xdgDirs.dataHome(false));
    table->add(QStringLiteral("Xdg Config Home"),      xdgDirs.configHome(false));
    table->add(QStringLiteral("Xdg Data Dirs"),        xdgDirs.dataDirs().join(QStringLiteral(":")));
    table->add(QStringLiteral("Xdg Cache Home"),       xdgDirs.cacheHome(false));
    table->add(QStringLiteral("Xdg Runtime Home"),     xdgDirs.runtimeDir());
    table->add(QStringLiteral("Xdg Autostart Dirs"),   xdgDirs.autostartDirs().join(QStringLiteral("<br>\n")));
    table->add(QStringLiteral("Xdg Autostart Home"),   xdgDirs.autostartHome(false));

}

