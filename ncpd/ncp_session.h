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
#ifndef _ncp_session_h_
 #define _ncp_session_h_

#include "config.h"

#include <string>
#include <bufferstore.h>
#include <ppsocket.h>
#include <iowatch.h>

#include "ncp.h"
#include "socketchan.h"

class NCPSession {
public:

    NCPSession(int _sockNum,
               int _baudRate,
               std::string _host,
               std::string _serialDevice,
               bool _autoexit,
               unsigned short _nverbose)
    : sockNum(_sockNum)
    , baudRate(_baudRate)
    , host(_host)
    , serialDevice(_serialDevice)
    , autoexit(_autoexit)
    , nverbose(_nverbose) {}

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

// private:

    bool isCancelled();

    // Configuration.
    int sockNum;
    int baudRate;
    std::string host;
    std::string serialDevice;
    bool autoexit;
    unsigned short nverbose;

    // State.
    pthread_t threadId_ = 0;
    NCP *ncp = nullptr;
    IOWatch iow;
    IOWatch acceptIOW;
    ppsocket skt;
    int numScp = 0;
    socketChan *scp[257] = {}; // MAX_CHANNELS_PSION + 1
    int cancellationPipe[2] = { -1, -1 };
};

#endif
