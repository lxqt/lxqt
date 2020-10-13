/***************************************************************************
 *   Copyright (C) 2010 - 2013 by Artem 'DOOMer' Galichkin                 *
 *   doomer3d@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <QTimer>
#include <QByteArray>

#include <QLocalSocket>
#include "singleapp.h"

/*!
    Creates of sungle application object
    \param argc Nuber of command line arguments
    \param argv Array of command line argwuments
    \param uniqueKey String key fo unicue shared data indefier
 */
SingleApp::SingleApp(int& argc, char* argv[], const QString &keyString) : QApplication(argc, argv), uniqueKey(keyString)
{
        sharedMemory.setKey(uniqueKey);
        if (sharedMemory.attach())
                runned = true;
        else
        {
                runned = false;
                // create shared memory.
                if (!sharedMemory.create(1))
                {
                        qDebug("Unable to create single instance.");
                        return;
                }
                // create local server and listen to incomming messages from other instances.
                localServer = new QLocalServer(this);
                connect(localServer, SIGNAL(newConnection()), this, SLOT(receiveMessage()));
                localServer->listen(uniqueKey);
        }
}

// public slots.

void SingleApp::receiveMessage()
{
        QLocalSocket *localSocket = localServer->nextPendingConnection();
        if (!localSocket->waitForReadyRead(timeout))
        {
                qDebug("%s", qPrintable(localSocket->errorString()));
                return;
        }
        QByteArray byteArray = localSocket->readAll();
        QString message = QString::fromUtf8(byteArray.constData());
        emit messageReceived(message);
        localSocket->disconnectFromServer();
}

// public functions.

/*!
    Checking instance application
    \return bool Return tue is another instance running
 */
bool SingleApp::isRunning()
{
        return runned;
}

/*1 
    Sending message to another running instance of application
    \param message String dended message
    \tryitn bool Return status ofsending message process
 */
bool SingleApp::sendMessage(const QString &message)
{
        if (!runned)
                return false;
        QLocalSocket localSocket(this);
        localSocket.connectToServer(uniqueKey, QIODevice::WriteOnly);
        if (!localSocket.waitForConnected(timeout))
        {
                qDebug("%s",qPrintable(localSocket.errorString()));
                return false;
        }
        localSocket.write(message.toUtf8());
        if (!localSocket.waitForBytesWritten(timeout))
        {
                qDebug("%s",qPrintable(localSocket.errorString()));
                return false;
        }
        localSocket.disconnectFromServer();
        return true;
}
