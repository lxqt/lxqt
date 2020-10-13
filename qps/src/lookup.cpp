/*
 * lookup.cpp
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 1997-1999 Mattias Engdegård
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

// This module implements asynchronous address->hostname lookup.

#include "lookup.h"

#include "svec.h"

#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <csignal>
#include <netinet/in.h>
#include <netdb.h>

#define fatal printf

// declarations of static members
char *Lookup::argv0;
int Lookup::maxtitlelen;

UintQueue::UintQueue() { first = last = 0; };

void UintQueue::enqueue(unsigned x)
{
    if (last < 0)
    {
        // make room for inserting more elements
        int step = qMax(first + 1, 8);
        queue.setSize(first + step + 1);
        for (int i = first; i > last; i--)
            queue[i + step] = queue[i];
        first += step;
        last += step;
    }
    queue.set(last--, x);
}

unsigned UintQueue::dequeue()
{
    if (isEmpty())
        fatal("UintQueue: queue empty");
    return queue[first--];
}

// constructor for node head
Hostnode::Hostnode() : next(this), prev(this) {}

// create a new cache node, initialized with null string
Hostnode::Hostnode(unsigned addr) : ipaddr(addr), next(nullptr), prev(nullptr) {}

// must be called on the head of the list
void Hostnode::moveToFront(Hostnode *node)
{
    if (next != node)
    {
        Hostnode *p = node->prev, *n = node->next;
        p->next = n;
        n->prev = p;
        node->next = next;
        node->prev = this;
        next->prev = node;
        next = node;
    }
}

// must be called on the head of the list
void Hostnode::deleteLast()
{
    Hostnode *nuke = prev;
    prev = nuke->prev;
    prev->next = this;
    delete nuke;
}

// must be called on the head of the list
void Hostnode::insertFirst(Hostnode *node)
{
    node->prev = this;
    node->next = next;
    next->prev = node;
    next = node;
}

Lookup::Lookup() // : hostdict(17)
{
    sockfd = -1; // no lookup helper is running
    readsn = writesn = nullptr;
    outstanding = 0;
}

// empty destructor, workaround for gcc bug
Lookup::~Lookup() {}

// look up host name (addr is in host byte order)
// a null name means it is been looked up (signal will be sent when done)
QString Lookup::hostname(unsigned addr)
{
    // first look in our cache
    Hostnode *hn = hostdict.value(addr, NULL);
    if (hn)
    {
        hostlru.moveToFront(hn);
    }
    else
    {
        hn = new Hostnode(addr);
        if (hostdict.count() >= hostname_cache_size)
        {
            // remove least recently used item
            hostdict.remove(hostlru.last()->ipaddr);
            hostlru.deleteLast();
        }
        hostlru.insertFirst(hn);
        hostdict.insert(addr, hn);
        // if(hostdict.count() > hostdict.size() * 3)
        // hostdict.resize(hostdict.count());
        if (addr == 0)
            hn->name = "*";
        else
            request(addr);
    }
    return hn->name;
}

void Lookup::request(unsigned addr)
{
    addrqueue.enqueue(addr);
    if (sockfd < 0)
    {
        int socks[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, socks);
        // launch a new helper
        signal(SIGCHLD, SIG_IGN); // Linux does automatic child reaping, nice
        switch (fork())
        {
        case -1:    // error
            return; // don't bother, we'll try again next time
        case 0:     // child
            close(socks[0]);
            do_child(socks[1]);
            break;
        default: // parent
            close(socks[1]);
            sockfd = socks[0];
            readsn = new QSocketNotifier(sockfd, QSocketNotifier::Read, this);
            connect(readsn, SIGNAL(activated(int)), SLOT(receive_result(int)));
            writesn = new QSocketNotifier(sockfd, QSocketNotifier::Write, this);
            connect(writesn, SIGNAL(activated(int)), SLOT(send_request(int)));
            break;
        }
    }
    writesn->setEnabled(true);
}

// the child helper process loop
void Lookup::do_child(int fd)
{
    setproctitle("qps-dns-helper");
    // close unused fds
    for (int i = 0; i < fd; i++)
        close(i);
    for (;;)
    {
        unsigned addr;
        int ret = read(fd, &addr, sizeof(addr));
        if (ret <= 0)
        {
            _exit(0); // connection closed
        }
        struct hostent *h = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
        char buf[256];
        if (!h)
        {
            unsigned a = htonl(addr);
            sprintf(buf, "%d.%d.%d.%d", (a >> 24) & 0xff, (a >> 16) & 0xff,
                    (a >> 8) & 0xff, a & 0xff);
        }
        else
        {
            strncpy(buf, h->h_name, sizeof(buf));
        }
        // if parent died, we'll get SIGPIPE here and terminate
        // automatically
        write(fd, &addr, sizeof(addr));
        int len = strlen(buf);
        write(fd, &len, sizeof(len));
        write(fd, buf, len);
    }
}

// slot: receive result from helper
void Lookup::receive_result(int)
{
    unsigned addr;
    int len;
    char buf[256];

    if (read(sockfd, &addr, sizeof(addr)) <= 0 ||
        read(sockfd, &len, sizeof(len)) <= 0 || read(sockfd, buf, len) <= 0)
    {
        // helper has died
        delete readsn;
        delete writesn;
        close(sockfd);
        sockfd = -1;
        return;
    }
    buf[len] = '\0';
    Hostnode *hn = hostdict.value(addr, NULL);
    if (!hn)
        return; // gone from cache
    hn->name = buf;
    emit resolved(addr);

    outstanding--;
    // if there is nothing more in the queue, kill the helper
    if (addrqueue.isEmpty() && outstanding == 0)
    {
        close(sockfd);
        sockfd = -1;
        delete readsn;
        delete writesn;
    }
}

// slot: send request to the helper
void Lookup::send_request(int)
{
    if (addrqueue.isEmpty())
    {
        writesn->setEnabled(false);
        return;
    }

    unsigned addr = addrqueue.dequeue();
    if (write(sockfd, &addr, sizeof(addr)) < 0)
    {
        // This shouldn't happen, try to repair it anyway
        addrqueue.enqueue(addr);
        delete readsn;
        delete writesn;
        close(sockfd);
        sockfd = -1;
        return;
    }
    outstanding++;
}

// register and measure the space for modifying the visible command line
void Lookup::initproctitle(char **argv, char **envp)
{
    argv0 = argv[0];
    while (*envp)
        envp++;
    maxtitlelen = envp[-1] + strlen(envp[-1]) - argv0 - 2;
}

// set the process title (idea snarfed from sysvinit (thanks Miquel) and
// refined by peeking into wu-ftpd)
void Lookup::setproctitle(const char *txt)
{
    memset(argv0, 0, maxtitlelen);
    strncpy(argv0, txt, maxtitlelen - 1);
}
