/*
 * Copyright (C) 2016  P.L. Lucas
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __LinuxBackend_H__
#define __LinuxBackend_H__

#include "../virtual_backend.h"
#include <QFileSystemWatcher>
#include <QTextStream>

namespace LXQt
{

class LinuxBackend:public VirtualBackEnd
{
Q_OBJECT

public:    
    LinuxBackend(QObject *parent = nullptr);
    ~LinuxBackend() override;
    
    bool isBacklightAvailable() override;
    bool isBacklightOff() override;
    void setBacklight(int value) override;
    int getBacklight() override;
    int getMaxBacklight() override;
    
private Q_SLOTS:
    void closeBacklightStream();
    void fileSystemChanged(const QString & path);

private:
    int maxBacklight;
    int actualBacklight;
    QFileSystemWatcher *fileSystemWatcher;
    FILE *backlightStream;
};

} // namespace LXQt

#endif  // __LinuxBackend_H__
