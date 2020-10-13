/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2014 LXQt team
 * Authors:
 *   Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef LXQTPLATFORMTHEME_H
#define LXQTPLATFORMTHEME_H

#include "lxqtsystemtrayicon.h"

#include <qpa/qplatformtheme.h> // this private header is subject to changes
#include <QtGlobal>
#include <QVariant>
#include <QString>
#include <QFileSystemWatcher>
#include <QFont>

class Q_GUI_EXPORT LXQtPlatformTheme : public QObject, public QPlatformTheme {
    Q_OBJECT
public:
    LXQtPlatformTheme();
    ~LXQtPlatformTheme() override;

    // virtual QPlatformMenuItem* createPlatformMenuItem() const;
    // virtual QPlatformMenu* createPlatformMenu() const;
    // virtual QPlatformMenuBar* createPlatformMenuBar() const;

    bool usePlatformNativeDialog(DialogType type) const override;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const override;

    const QPalette *palette(Palette type = SystemPalette) const override;

    const QFont *font(Font type = SystemFont) const override;

    QVariant themeHint(ThemeHint hint) const override;

    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override
    {
        auto trayIcon = new LXQtSystemTrayIcon;
        if (trayIcon->isSystemTrayAvailable())
            return trayIcon;
        else
        {
            delete trayIcon;
            return nullptr;
        }
    }

    // virtual QPixmap standardPixmap(StandardPixmap sp, const QSizeF &size) const;
    // virtual QPixmap fileIconPixmap(const QFileInfo &fileInfo, const QSizeF &size,
    //                               QPlatformTheme::IconOptions iconOptions = 0) const;

    QIconEngine *createIconEngine(const QString &iconName) const override;

    // virtual QList<QKeySequence> keyBindings(QKeySequence::StandardKey key) const;

    // virtual QString standardButtonText(int button) const;

public Q_SLOTS:
    void lazyInit();

private:
    void loadSettings();

private Q_SLOTS:
    void onSettingsChanged();

private:
    // LXQt settings
    QString iconTheme_;
    Qt::ToolButtonStyle toolButtonStyle_;
    bool singleClickActivate_;
    bool iconFollowColorScheme_;

    // other Qt settings
    // widget
    QString style_;
    QColor winColor_;
    QString fontStr_;
    QFont font_;
    QString fixedFontStr_;
    QFont fixedFont_;
    // mouse
    QVariant doubleClickInterval_;
    QVariant wheelScrollLines_;
    // keyboard
    QVariant cursorFlashTime_;
    QFileSystemWatcher *settingsWatcher_;
    QString settingsFile_;

    QPalette *LXQtPalette_;

    QStringList xdgIconThemePaths() const;
};

#endif // LXQTPLATFORMTHEME_H
