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
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * END_COMMON_COPYRIGHT_HEADER */


#ifndef TRANSLATORSINFO_H
#define TRANSLATORSINFO_H
#include <map>
#include <set>
#include <tuple>

#include <QString>

#if 0
namespace LXQt
{
#endif

class TranslatorPerson
{
public:

    TranslatorPerson(const QString &englishName, const QString &nativeName, const QString &contact)
        :mEnglishName (englishName), mNativeName (nativeName != englishName ? nativeName : QLatin1String("")), mContact (contact)
    { }
    TranslatorPerson(TranslatorPerson &&other) = default;

    QString englishName() const { return mEnglishName; }
    QString nativeName() const { return mNativeName; }
    QString contact() const { return mContact; }

    void addLanguage(QString langId);
    QString asHtml() const;

    // A comparison operator for std::set
    bool operator<(const TranslatorPerson & other) const {
        return std::tie(this->mEnglishName, this->mNativeName, this->mContact)
             < std::tie(other.mEnglishName, other.mNativeName, other.mContact);
    }

private:
    QString mEnglishName;
    QString mNativeName;
    QString mContact;
};

class TranslatorsInfo
{
public:
    TranslatorsInfo();
    QString asHtml() const;

private:
    std::map<QString, std::set<TranslatorPerson> > mLangTranslators; //< maps a language to set of it's translators
    void process(const QString &lang, const QString &englishName, const QString &nativeName, const QString &contact);
};

//} //namecpace LXQt
#endif // TRANSLATORSINFO_H
