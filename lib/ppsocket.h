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
#ifndef _PPSOCKET_H_
#define _PPSOCKET_H_

#include <string>

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

class bufferStore;
class IOWatch;

/**
 * A class for dealing with sockets.
 */
class ppsocket
{

public:
    /**
    * Constructs a ppsocket
    */
    ppsocket();

    /**
    * Copy constructor
    */
    ppsocket(const ppsocket&);

    /**
    * Destructor
    */
    virtual ~ppsocket();

    /**
    * Connects to a given host.
    *
    * @param Peer     The Host to connect to (name or dotquad-string).
    * @param PeerPort The port to connect to.
    * @param Host     The local address to bind to.
    * @param HostPort The local port to bind to.
    *
    * @returns true on success, false otherwise.
    */
    virtual bool connect(const char * const Peer, int PeerPort, const char * const Host = NULL, int HostPort = 0);

    /**
    * Reopens the connection after closing it.
    *
    * @returns true on success, false otherwise.
    */
    virtual bool reconnect();

    /**
    * Retrieve a string representation of the ppsocket.
    *
    * @returns a string in the form "<host>:<hostport> -> <peer>:<peerport>"
    *          where elements not known, are replaced by "???" and none-existing
    *          elements are represented by the word "none".
    */
    virtual std::string toString();

    /**
    * Starts listening.
    *
    * @param Host The local address to bind to.
    * @param Port The local port to listen on.
    *
    * @returns true on success, false otherwise.
    */
    virtual bool listen(const char * const Host, int Port);

    /**
    * Accept a connection.
    *
    * @param Peer If non-Null, the peer's name is returned here.
    *
    * @returns A pointer to a new instance for the accepted connection or NULL
    *          if an error happened.
    */
    ppsocket *accept(std::string *Peer, IOWatch *);

    /**
    * Check and optionally wait for incoming data.
    *
    * @param sec Timeout in seconds
    * @param usec Timeout in microseconds
    *
    * @returns true if data is available, false otherwise.
    */
    bool dataToGet(int sec, int usec) const;

    /**
    * Receive data into a @ref bufferStore .
    *
    * @param a The bufferStore to fill with received data.
    * @param wait If true, wait until something is received, else return
    *              if no data is available.
    * @returns 1 if a bufferStore received, 0, if no bufferStore received, -1
    *          on error.
    */
    int getBufferStore(bufferStore &a, bool wait = true);

    /**
    * Sends data from a @ref bufferStore .
    *
    * @param a The bufferStore to send.
    * @returns true on success, false otherwise.
    */
    bool sendBufferStore(const bufferStore &a);

    /**
    * Closes the connection.
    *
    * @returns true on success, false otherwise.
    */
    bool closeSocket(void);

    /**
    * Binds to a local address and port.
    *
    * @param Host The local address to bind to.
    * @param Port The local port to listen on.
    *
    * @returns true on success, false otherwise.
    */
    bool bindSocket(const char * const Host, int Port);

    /**
    * Tries repeated binds to a local address and port.
    * If @p Retries is <= @p High - @p Low, then
    * the port to bind is randomly chosen in the given range.
    * Otherwise, all ports starting from @p High up to @p Low
    * are tried in sequence.
    *
    * @param Host The local address to bind to.
    * @param Low  The lowest local port to listen on.
    * @param High The highest local port to listen on.
    * @param Retries The number of retries until giving up.
    *
    * @returns true on success, false otherwise.
    */
    bool bindInRange(const char * const Host, int Low, int High, int Retries);

    /**
    * Sets the linger parameter of the socket.
    *
    * @param LingerOn true, if lingering should be on.
    * @param LingerTime If lingering is on, the linger-time.
    *
    * @returns true on success, false otherwise.
    */
    bool linger(bool LingerOn, int LingerTime = 0);

    /**
    * Retrieves peer information.
    *
    * @param Peer The peers name is returned here.
    * @param Port The peers port is returned here.
    *
    * @returns true on success, false otherwise.
    */
    bool getPeer(std::string *Peer, int *Port);

    /**
    * Retrieves local information.
    *
    * @param Host The local name is returned here.
    * @param Port The local port is returned here.
    *
    * @returns true on success, false otherwise.
    */
    bool getHost(std::string *Host, int *Port);

    /**
    * Registers an @ref IOWatch for this socket.
    * This IOWatch gets the socket added/removed
    * automatically.
    *
    * @param watch The IOWatch to register.
    */
    void setWatch(IOWatch *watch);

private:
    /**
    * Creates the socket.
    */
    virtual bool createSocket(void);

    int getLastError(void) { return(m_LastError); }
    bool setPeer(const char * const Peer, int Port);
    bool setHost(const char * const Host, int Port);
    int recv(void *buf, int len, int flags);
    int send(const void * const buf, int len, int flags);

    struct sockaddr m_HostAddr;
    struct sockaddr m_PeerAddr;
    int m_Socket;
    int m_Port;
    bool m_Bound;
    int m_LastError;
    IOWatch *myWatch;
};

#endif
