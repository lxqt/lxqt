/*
* Copyright (c) 2014,2015 Christian Surlykke, Paulo Lieuthier
*
* This file is part of the LXQt project. <https://lxqt.org>
* It is distributed under the LGPL 2.1 or later license.
* Please refer to the LICENSE file for a copy of the license.
*/

#ifndef WATCHER_H
#define WATCHER_H

#include <QObject>
#include <QEventLoop>
#include <LXQt/Power>
#include <LXQt/ScreenSaver>

class Watcher : public QObject
{
    Q_OBJECT

public:
    Watcher(QObject *parent = nullptr);
    ~Watcher() override;

protected:
    void doAction(int action);

signals:
    void done();

private:
    LXQt::Power mPower;
    LXQt::ScreenSaver mScreenSaver;
};

#endif // WATCHER_H
