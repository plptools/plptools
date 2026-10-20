/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2006-2025 Reuben Thomas <rrt@sc3d.org>
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

#include <memory>
#include <string>
#include <vector>

#include "connectionerror.h"
#include "bufferstore.h"
#include "Enum.h"
#include "tcpsocket.h"

class TCPClient;

namespace ncp_client {

/**
* Create a new NCP client instance, selecting the appropriate subclass based on the device type reported by an
* `NCP$INFO` query. Fails instantly if ncpd isn't running or accepting connections.
*
* @param host The host be used for connecting to the ncpd daemon.
* @param port The port be used for connecting to the ncpd daemon.
* @param waitForDevice Continue reconnecting to the ncpd daemon until a Psion is available for connection; blocking.
* @param error Out-parameter returning any error encountered; set to @ref FACERR_NONE on success.
*
* @return A newly instantiated ncpd client corresponding with the device type (EPOC16 or EPOC32); nullptr on failure.
*/
template<typename Client, typename Client16, typename Client32>
Client *connect(const std::string &host, int port, bool waitForDevice, Enum<ConnectionError> *error) {

    if (error) {
        *error = FACERR_NONE;
    }

    auto socket = std::make_unique<TCPSocket>();
    if (!socket->connect(host.c_str(), port)) {
        if (error) {
            *error = FACERR_CONNECTION_FAILURE;
        }
        return nullptr;
    }

    // At this point the socket is connected to the ncp daemon, which will have (hopefully) seen an INFO exchange, where
    // the protocol version of the remote Psion was sent, and noted. We have to ask the ncp daemon which protocol it
    // saw, so we can instantiate the correct RPCS protocol handler for the caller. We announce ourselves to the NCP
    // daemon, and the relevant RPCS module will also announce itself.

    BufferStore bufferStore;

    bufferStore.addStringT("NCP$INFO");
    if (!socket->sendBufferStore(bufferStore)) {
        if (!waitForDevice) {
            if (error) {
                *error = FACERR_COULD_NOT_SEND;
            }
        } else {
            socket->closeSocket();
            socket->reconnect();
            if (error) {
                *error = FACERR_AGAIN;
            }
        }
        return nullptr;
    }
    if (socket->getBufferStore(bufferStore) == 1) {
        if (bufferStore.getLen() > 8 && !strncmp(bufferStore.getString(), "Series 3", 8)) {
            return new Client16(std::move(socket));
        }
        else if (bufferStore.getLen() > 8 && !strncmp(bufferStore.getString(), "Series 5", 8)) {
            return new Client32(std::move(socket));
        }
        if ((bufferStore.getLen() > 8) && !strncmp(bufferStore.getString(), "No Psion", 8)) {
            socket->closeSocket();
            socket->reconnect();
            if (error) {
                *error = FACERR_NOPSION;
            }
            return nullptr;
        }
        // Invalid protocol version
        if (error) {
            *error = FACERR_PROTVERSION;
        }
    } else {
        if (error) {
            *error = FACERR_NORESPONSE;
        }
    }

    // No message returned.
    return nullptr;
}

};
