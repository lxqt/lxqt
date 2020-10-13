/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
 * Authors:
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#ifndef GLOBAL_ACTION_DAEMON__MUTEX__INCLUDED
#define GLOBAL_ACTION_DAEMON__MUTEX__INCLUDED


#include <pthread.h>


class Mutex
{
public:
    Mutex();
    ~Mutex();

    int initResult() const { return mInitResult; }

    int lock() { return pthread_mutex_lock(&mMutex); }
    int tryLock() { return pthread_mutex_trylock(&mMutex); }
    int unlock() { return pthread_mutex_unlock(&mMutex); }

private:
    int mInitResult;
    pthread_mutex_t mMutex;
};

class Lock
{
public:
    Lock(Mutex &mutex)
        : mMutex(mutex)
    {
        mutex.lock();
    }

    ~Lock()
    {
        mMutex.unlock();
    }

private:
    Mutex &mMutex;
};

#define LOCK(lock, mutex) Lock lock(mutex); (void)(lock)

#endif // GLOBAL_ACTION_DAEMON__MUTEX__INCLUDED
