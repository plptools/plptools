/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
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
 *  You should have received a copy of the GNU General Public License along
 *  along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */
#include "config.h"

#include "bufferstore.h"
#include "ppsocket.h"
#include "iowatch.h"

#include <cstring>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define  INVALID_SOCKET -1
#define  SOCKET_ERROR   -1

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

using namespace std;

ppsocket::ppsocket(const ppsocket & another)
{
    m_Socket = another.m_Socket;
    m_HostAddr = another.m_HostAddr;
    m_PeerAddr = another.m_PeerAddr;
    m_Bound = another.m_Bound;
    m_LastError = another.m_LastError;
    myWatch = another.myWatch;
}


ppsocket::ppsocket()
{
    m_Socket = INVALID_SOCKET;

    memset(&m_HostAddr, 0, sizeof(m_HostAddr));
    memset(&m_PeerAddr, 0, sizeof(m_PeerAddr));

    ((struct sockaddr_in *) &m_HostAddr)->sin_family = AF_INET;
    ((struct sockaddr_in *) &m_PeerAddr)->sin_family = AF_INET;

    m_Bound = false;
    m_LastError = 0;
    myWatch = 0L;
}

ppsocket::~ppsocket()
{
    if (m_Socket != INVALID_SOCKET) {
        if (myWatch)
            myWatch->remIO(m_Socket);
        shutdown(m_Socket, SHUT_RDWR);
        ::close(m_Socket);
    }
}

void ppsocket::
setWatch(IOWatch *watch) {
    if (watch) {
        if (myWatch && (m_Socket != INVALID_SOCKET))
           myWatch->remIO(m_Socket);
        myWatch = watch;
    }
}

bool ppsocket::
reconnect()
{
    if (m_Socket != INVALID_SOCKET) {
        if (myWatch)
            myWatch->remIO(m_Socket);
        shutdown(m_Socket, SHUT_RDWR);
        ::close(m_Socket);
    }
    m_Socket = INVALID_SOCKET;
    if (!createSocket())
        return (false);
    m_LastError = 0;
    m_Bound = false;
    if (::bind(m_Socket, &m_HostAddr, sizeof(m_HostAddr)) != 0) {
        m_LastError = errno;
        return (false);
    }
    if (::connect(m_Socket, &m_PeerAddr, sizeof(m_PeerAddr)) != 0) {
        m_LastError = errno;
        return (false);
    }
    if (myWatch)
        myWatch->addIO(m_Socket);
    return (true);
}

string ppsocket::
toString()
{
    string ret = "";
    char nbuf[10];
    char *tmp = 0L;

    tmp = inet_ntoa(((struct sockaddr_in *) &m_HostAddr)->sin_addr);
    ret += tmp ? tmp : "none:none";
    if (tmp) {
        ret += ':';
        sprintf(nbuf, "%d", ntohs(((struct sockaddr_in *) &m_HostAddr)->sin_port));
        ret += nbuf;
    }
    ret += " -> ";
    tmp = inet_ntoa(((struct sockaddr_in *) &m_PeerAddr)->sin_addr);
    ret += tmp ? tmp : "none:none";
    if (tmp) {
        ret += ':';
        sprintf(nbuf, "%d", ntohs(((struct sockaddr_in *) &m_PeerAddr)->sin_port));
        ret += nbuf;
    }
    return ret;
}

bool ppsocket::
connect(const char * const Peer, int PeerPort, const char * const Host, int HostPort)
{
    //****************************************************
    //* If we aren't already bound set the host and bind *
    //****************************************************

    if (!bindSocket(Host, HostPort)) {
        if (m_LastError != 0) {
            return (false);
        }
    }
    //****************
    //* Set the peer *
    //****************
    if (!setPeer(Peer, PeerPort)) {
        return (false);
    }
    //***********
    //* Connect *
    //***********
    if (::connect(m_Socket, &m_PeerAddr, sizeof(m_PeerAddr)) != 0) {
        m_LastError = errno;
        return (false);
    }
    if (myWatch)
        myWatch->addIO(m_Socket);
    return (true);
}

bool ppsocket::
listen(const char * const Host, int Port)
{
    //****************************************************
    //* If we aren't already bound set the host and bind *
    //****************************************************

    if (!bindSocket(Host, Port)) {
        if (m_LastError != 0) {
            return (false);
        }
    }
    //**********************
    //* Listen on the port *
    //**********************

    if (myWatch)
        myWatch->addIO(m_Socket);
    if (::listen(m_Socket, 5) != 0) {
        m_LastError = errno;
        return (false);
    }
    return (true);
}

ppsocket *ppsocket::
accept(string *Peer, IOWatch *iow)
{
    socklen_t len;
    ppsocket *accepted;
    char *peer;

    //*****************************************************
    //* Allocate a new object to hold the accepted socket *
    //*****************************************************
    accepted = new ppsocket;

    if (!iow)
        iow = myWatch;
    if (!accepted) {
        m_LastError = errno;
        return NULL;
    }
    //***********************
    //* Accept a connection *
    //***********************

    len = sizeof(struct sockaddr);
    accepted->m_Socket = ::accept(m_Socket, &accepted->m_PeerAddr, &len);

    if (accepted->m_Socket == INVALID_SOCKET) {
        m_LastError = errno;
        delete accepted;
        return NULL;
    }
    //****************************************************
    //* Got a connection so fill in the other attributes *
    //****************************************************

    // Make sure the new socket hasn't inherited O_NONBLOCK
    // from the accept socket
    int flags = fcntl(accepted->m_Socket, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(accepted->m_Socket, F_SETFL, flags);

    accepted->m_HostAddr = m_HostAddr;
    accepted->m_Bound = true;

    //****************************************************
    //* If required get the name of the connected client *
    //****************************************************
    if (Peer) {
        peer = inet_ntoa(((struct sockaddr_in *) &accepted->m_PeerAddr)->sin_addr);
        if (peer)
            *Peer = peer;
    }
    if (accepted && iow) {
        accepted->setWatch(iow);
        iow->addIO(accepted->m_Socket);
    }
    return accepted;
}

bool ppsocket::
dataToGet(int sec, int usec) const
{
    fd_set io;
    FD_ZERO(&io);
    FD_SET(m_Socket, &io);
    struct timeval t;
    t.tv_usec = usec;
    t.tv_sec = sec;
    return (select(m_Socket + 1, &io, NULL, NULL, &t) != 0) ? true : false;
}

int ppsocket::
getBufferStore(bufferStore & a, bool wait)
{
    /* Returns a 0 for for no message,
    * 1 for message OK, and -1 for socket problem
    */

    uint32_t l;
    long count = 0;
    unsigned char *buff;
    unsigned char *bp;
    if (!wait && !dataToGet(0, 0))
        return 0;
    a.init();
    if (recv(&l, sizeof(l), MSG_NOSIGNAL) != sizeof(l)) {
        return -1;
    }
    l = ntohl(l);
    if (l > 16384)
            return -1;
    bp = buff = new unsigned char[l];
    while (l > 0) {
        int j = recv(bp, l, MSG_NOSIGNAL);
        if (j == SOCKET_ERROR || j == 0) {
            delete[]buff;
            return -1;
        }
        count += j;
        l -= j;
        bp += j;
    };
    a.init(buff, count);
    delete[]buff;
    return (a.getLen() == 0) ? 0 : 1;
}

bool ppsocket::
sendBufferStore(const bufferStore & a)
{
    long l = a.getLen();
    uint32_t hl = htonl(l);
    long sent = 0;
    int retries = 0;
    int i;

    bufferStore b;
    b.addBytes(reinterpret_cast<const unsigned char *>(&hl), sizeof(hl));
    b.addBuff(a);
    l += 4;
    while (l > 0) {
        i = send((const char *)b.getString(sent), l, MSG_NOSIGNAL);
        if (i == SOCKET_ERROR || i == 0)
            return (false);
        sent += i;
        l -= i;
        if (++retries > 5) {
            m_LastError = 0;
            return (false);
        }
    }
    return true;
}

int ppsocket::
recv(void *buf, int len, int flags)
{
    int i = ::recv(m_Socket, buf, len, flags);

    if (i < 0)
        m_LastError = errno;

    return (i);
}

int ppsocket::
send(const void * const buf, int len, int flags)
{
    int i = ::send(m_Socket, buf, len, flags);

    if (i < 0)
        m_LastError = errno;

    return (i);
}

bool ppsocket::
closeSocket(void)
{
    if (myWatch)
        myWatch->remIO(m_Socket);
    shutdown(m_Socket, SHUT_RDWR);
    if (::close(m_Socket) != 0) {
        m_LastError = errno;
        return false;
    }
    m_Socket = INVALID_SOCKET;
    return true;
}

bool ppsocket::
bindSocket(const char * const Host, int Port)
{

    // If we are already bound return false but with no last error
    if (m_Bound) {
        m_LastError = 0;
        return false;
    }

    // If the socket hasn't been created create it now

    if (m_Socket == INVALID_SOCKET) {
        if (!createSocket())
            return false;
    }

    // Set SO_REUSEADDR
    int one = 1;
    if (setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR,
                   (const char *)&one, sizeof(int)) < 0)
        cerr << "Warning: Unable to set SO_REUSEADDR option\n";

    // If a host name was supplied then use it
    if (!setHost(Host, Port))
        return false;

    // Now bind the socket
    if (::bind(m_Socket, &m_HostAddr, sizeof(m_HostAddr)) != 0) {
        m_LastError = errno;
        return false;
    }

    m_Bound = true;
    return true;
}

bool ppsocket::
bindInRange(const char * const Host, int Low, int High, int Retries)
{
    int port;
    int i;

    // If we are already bound return false but with no last error
    if (m_Bound) {
        m_LastError = 0;
        return (false);
    }

    // If the socket hasn't been created create it now
    if (m_Socket == INVALID_SOCKET) {
        if (!createSocket())
            return false;
    }

    // If the number of retries is greater than the range then work
    // through the range sequentially.
    if (Retries > High - Low) {
        for (port = Low; port <= High; port++) {
            if (!setHost(Host, port))
                return false;
            if (::bind(m_Socket, &m_HostAddr, sizeof(m_HostAddr)) == 0)
                break;
        }
        if (port > High) {
            m_LastError = errno;
            return false;
        }
    } else {
        for (i = 0; i < Retries; i++) {
            port = Low + (rand() % (High - Low));
            if (!setHost(Host, port))
                return false;
            if (::bind(m_Socket, &m_HostAddr, sizeof(m_HostAddr)) == 0)
                break;
        }
        if (i >= Retries) {
            m_LastError = errno;
            return false;
        }
    }
    m_Bound = true;
    return true;
}

bool ppsocket::
linger(bool LingerOn, int LingerTime)
{
    int i;
    struct linger l;

    // If the socket hasn't been created create it now
    if (m_Socket == INVALID_SOCKET) {
        if (!createSocket())
            return false;
    }

    // Set the lingering
    if (LingerOn) {
        l.l_onoff = 1;
        l.l_linger = LingerTime;
    } else {
        l.l_onoff = 0;
        l.l_linger = 0;
    }
    i = setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (const char *) &l, sizeof(l));
    // Check for errors
    if (i != 0) {
        m_LastError = errno;
        return false;
    }
    return true;
}

bool ppsocket::
createSocket(void)
{
    // If the socket has already been created just return true
    if (m_Socket != INVALID_SOCKET)
        return true;

    // Create the socket
    m_Socket = ::socket(PF_INET, SOCK_STREAM, 0);
    if (m_Socket == INVALID_SOCKET) {
        m_LastError = errno;
        return false;
    }

    // By default set no lingering
    linger(false);

    // Return indicating success
    return true;
}

bool ppsocket::
setPeer(const char * const Peer, int Port)
{
    struct hostent *he = NULL;

    // If a peer name was supplied then use it
    if (Peer) {
        if (!isdigit(Peer[0]))
            // RFC1035 specifies that hostnames must not start
            // with a digit. So we can speed up things here.
            he = gethostbyname(Peer);
        if (!he) {
            struct in_addr ipaddr;

            if (!inet_aton(Peer, &ipaddr)) {
                m_LastError = errno;
                return false;
            }
            he = gethostbyaddr((const char *)&ipaddr.s_addr, sizeof(ipaddr.s_addr), PF_INET);
            if (!he) {
                m_LastError = errno;
                return (false);
            }
        }
        memcpy(&((struct sockaddr_in *)&m_PeerAddr)->sin_addr, he->h_addr_list[0],
               sizeof(((struct sockaddr_in *)&m_PeerAddr)->sin_addr));
    }
    // If a port name was supplied use it
    if (Port > 0)
        ((struct sockaddr_in *)&m_PeerAddr)->sin_port = htons(Port);
    return true;
}

bool ppsocket::
getPeer(string *Peer, int *Port)
{
    char *peer;

    if (Peer) {
        peer = inet_ntoa(((struct sockaddr_in *) &m_PeerAddr)->sin_addr);
        if (!peer) {
            m_LastError = errno;
            return (false);
        }
        *Peer = peer;
    }
    if (Port)
        *Port = ntohs(((struct sockaddr_in *) &m_PeerAddr)->sin_port);
    return false;
}

bool ppsocket::
setHost(const char * const Host, int Port)
{
    struct hostent *he;

    // If a host name was supplied then use it
    if (Host) {
        if (!isdigit(Host[0]))
            // RFC1035 specifies that hostnames must not start
            // with a digit. So we can speed up things here.
            he = gethostbyname(Host);
        he = gethostbyname(Host);
        if (!he) {
            struct in_addr ipaddr;

            if (!inet_aton(Host, &ipaddr)) {
                m_LastError = errno;
                return false;
            }
            he = gethostbyaddr((const char *)&ipaddr.s_addr, sizeof(ipaddr.s_addr), PF_INET);
            if (!he) {
                m_LastError = errno;
                return false;
            }
        }
        memcpy(&((struct sockaddr_in *)&m_HostAddr)->sin_addr, he->h_addr_list[0],
               sizeof(((struct sockaddr_in *)&m_HostAddr)->sin_addr));
    }

    // If a port name was supplied use it
    if (Port > 0)
        ((struct sockaddr_in *)&m_HostAddr)->sin_port = htons(Port);
    return true;
}

bool ppsocket::
getHost(string *Host, int *Port)
{
    char *host;

    if (Host) {
        host = inet_ntoa(((struct sockaddr_in *)&m_HostAddr)->sin_addr);
        if (!host) {
            m_LastError = errno;
            return false;
        }
        *Host = host;
    }
    if (Port)
        *Port = ntohs(((struct sockaddr_in *)&m_HostAddr)->sin_port);
    return true;
}
