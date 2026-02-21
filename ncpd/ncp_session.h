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

#include <signal.h>
#include <string>

#include <bufferstore.h>
#include <ppsocket.h>
#include <iowatch.h>

#include "ncp.h"
#include "socketchan.h"

/**
 * Configuration and state associated with an NCP session.
 *
 * This is shared between the different threads used to run the session and replaces historical global state. Care needs
 * to be taking when writing to this as the datatypes used are not inherently thread-safe.
 */
struct NCPSession {

    // Configuration.
    int sockNum = 0;
    int baudRate = 0;
    std::string host;
    std::string serialDevice;
    bool autoexit = false;
    unsigned short nverbose = 0;

    // State.
    ncp *theNCP = nullptr;
    IOWatch iow;
    IOWatch acceptIOW;
    ppsocket skt;
    int numScp = 0;
    socketChan *scp[257] = {}; // MAX_CHANNELS_PSION + 1
    volatile sig_atomic_t isCancelled = false;
};

void runNCPSession(NCPSession *session);

#endif
