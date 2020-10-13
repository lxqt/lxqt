/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)GPL2+
 *
 *
 * Copyright: 2014 LXQt team
 *            2014 Sebastian Kügler <sebas@kde.org>
 * Authors:
 *   Julien Lavergne <gilir@ubuntu.com>
 *   Sebastian Kügler <sebas@kde.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef LOCALECONFIG_H
#define LOCALECONFIG_H

#include <QWidget>
#include <LXQt/Settings>

class QTreeWidgetItem;
class QSettings;


namespace Ui {
    class LocaleConfig;
}

class QComboBox;
class QMessageWidget;

class LocaleConfig : public QWidget
{
    Q_OBJECT

public:
    explicit LocaleConfig(LXQt::Settings *settings, LXQt::Settings *session_settings, QWidget *parent = 0);
    ~LocaleConfig();

    void load();
    void save();
    void defaults();

public slots:
    void initControls();
    void saveSettings();

private:
    void addLocaleToCombo(QComboBox *combo, const QLocale &locale);
    void initCombo(QComboBox *combo, const QList<QLocale> &allLocales);
    void connectCombo(QComboBox *combo);
    QList<QComboBox *> m_combos;

    void readConfig();
    void writeConfig();
    void writeExports();

    void updateExample();
    void updateEnabled();

    Ui::LocaleConfig *m_ui;
    bool hasChanged;
    LXQt::Settings *mSettings;
    LXQt::Settings *sSettings;
};

#endif // LOCALECONFIG_H
