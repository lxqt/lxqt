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

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTimer>

namespace Ui {
    class Dialog;
}

namespace LXQt {
    class Settings;
    class PowerManager;
    class ScreenSaver;
}

class CommandListView;
class CommandItemModel;
class ConfigureDialog;

namespace GlobalKeyShortcut
{
class Action;
}


class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    QSize sizeHint() const;

protected:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
    bool eventFilter(QObject *object, QEvent *event);
    bool editKeyPressEvent(QKeyEvent *event);
    bool listKeyPressEvent(QKeyEvent *event);

private:
    Ui::Dialog *ui;
    LXQt::Settings *mSettings;
    GlobalKeyShortcut::Action *mGlobalShortcut;
    CommandItemModel *mCommandItemModel;
    bool mShowOnTop;
    int mMonitor;
    LXQt::PowerManager *mPowerManager;
    LXQt::ScreenSaver *mScreenSaver;

    bool mLockCascadeChanges;
    bool mDesktopChanged; //!< \note flag for changing desktop & activation workaround

    ConfigureDialog *mConfigureDialog;

    QTimer mSearchTimer;

private slots:
    void realign();
    void applySettings();
    void showHide();
    void setFilter(const QString &text, bool onlyHistory=false);
    void dataChanged();
    void runCommand();
    void showConfigDialog();
    void shortcutChanged(const QString &oldShortcut, const QString &newShortcut);
    void onActiveWindowChanged(WId id);
    void onCurrentDesktopChanged(int desktop);
};

#endif // DIALOG_H

