/*
 * Copyright (C) 2014  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef SESSIONAPPLICATION_H
#define SESSIONAPPLICATION_H

#include <LXQt/Application>
#include <LXQt/Settings>

class LXQtModuleManager;
class LockScreenManager;

class SessionApplication : public LXQt::Application
{
    Q_OBJECT
public:
    SessionApplication(int& argc, char** argv);
    ~SessionApplication() override;
    void setWindowManager(const QString & windowManager);
    void setConfigName(const QString & configName);

private Q_SLOTS:
    bool startup();

private:
    void loadEnvironmentSettings(LXQt::Settings& settings);
    void loadKeyboardSettings(LXQt::Settings& settings);
    void loadMouseSettings(LXQt::Settings& settings);
    // void loadFontSettings(LXQt::Settings& settings);

    void setxkbmap(QString layout, QString variant, QString model, QStringList options);

    void mergeXrdb(const char* content, int len);
    void setLeftHandedMouse(bool mouse_left_handed);
private:
    LXQtModuleManager* modman;
    LockScreenManager *lockScreenManager;
    QString configName;
};

#endif // SESSIONAPPLICATION_H
