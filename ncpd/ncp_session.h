/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2026 Jason Morley <hello@jbmorley.co.uk>
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
#pragma once

#include "config.h"

#include <mutex>
#include <string>
#include <bufferstore.h>
#include <tcpsocket.h>
#include <iowatch.h>

#include "ncp.h"
#include "ncpstatuscallback.h"
#include "socketchannel.h"

/**
* Responsible for orchestrating the high-level life cycle of a daemon-side %NCP server and multiplexing connections
* over a single hardware comms channel (serial port, etc). Creates and manages three threads
* (@ref ncp_session_main_thread, @ref link_thread, and @ref socket_connection_polling_thread) that drive the serial
* ports, accept incoming TCP connections from clients, and poll connected TCP sockets.
*/
class NCPSession {
public:

    NCPSession(int portNumber,
               int baudRate,
               std::string host,
               std::string serialDevice,
               bool autoexit,
               unsigned short nverbose,
               NCPStatusCallback statusCallback = nullptr,
               void *callbackContext = nullptr)
    : portNumber_(portNumber)
    , baudRate_(baudRate)
    , host_(host)
    , serialDevice_(serialDevice)
    , autoexit_(autoexit)
    , nverbose_(nverbose)
    , statusCallback_(statusCallback)
    , callbackContext_(callbackContext) {}

    NCPSession(const NCPSession&) = delete;
    NCPSession& operator=(const NCPSession&) = delete;

    ~NCPSession();

    /**
    * Creates and manages all the threads necessary to run a full session for communicating with a Psion and exposing
    * that to clients via the a TCP socket.
    *
    * This is a non-blocking function. The session should be stopped by calling `stop` or cancelling the session using
    * `cancel` interrupting the session thread with `SIGINT`.
    */
    int start();

    /**
    * Mark the session as cancelled.
    *
    * It is anticipated that this method be called from within an interrupt handler in CLI apps. When using `cancel` to
    * initiate session shutdown, it should be paired with `wait`.
    */
    void cancel();

    /**
    * Wait for the session to terminate.
    *
    * A typical usage pattern might call `stop`, followed by, `wait` to ensure the session is fully terminated and
    * cleaned up before doing further work (e.g., starting a new session with a different configuration).
    */
    void wait();

private:

    // Thread entry points and helpers.

    friend void *ncp_session_main_thread(void *arg);
    friend void *link_thread(void *arg);
    friend void *socket_connection_polling_thread(void *arg);
    friend void check_for_new_socket_connection(NCPSession *session);

    bool isCancelled();

    // Configuration.

    int portNumber_;
    int baudRate_;
    std::string host_;
    std::string serialDevice_;
    bool autoexit_;
    unsigned short nverbose_;
    NCPStatusCallback statusCallback_;
    void *callbackContext_;

    // State.

    pthread_t sessionMainThreadId_ = 0;

    /**
    * @ref NCP instance.
    */
    NCP *ncp_ = nullptr;

    /**
    * Used to watch all active @ref SocketChannel instances (stored in @ref socketChannels_) to see if they're readable.
    */
    IOWatch socketChannelWatch_;

    TCPSocket skt_;
    std::mutex socketChannelLock_;
    std::vector<SocketChannel *>socketChannels_;
    int cancellationPipe_[2] = { -1, -1 };
};
