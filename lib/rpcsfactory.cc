/*
 * This file is part of plptools.
 *
 *  Copyright (C) 2000-2001 Fritz Elfert <felfert@to.com>
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
#include "config.h"

#include "rpcs16.h"
#include "rpcs32.h"
#include "rpcsfactory.h"
#include "bufferstore.h"
#include "tcpsocket.h"
#include "Enum.h"

#include <stdlib.h>
#include <time.h>

RPCSFactory::RPCSFactory(const std::string &host, int port)
: host_(host)
, port_(port) {}

RPCSFactory::~RPCSFactory() {}

RPCS *RPCSFactory::create(bool reconnect, Enum<ConnectionError> *error) {

    if (error) {
        *error = FACERR_NONE;
    }

    auto socket = std::make_unique<TCPSocket>();
    if (!socket->connect(host_.c_str(), port_)) {
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
        if (!reconnect) {
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
            return new RPCS16(std::move(socket));
        }
        else if (bufferStore.getLen() > 8 && !strncmp(bufferStore.getString(), "Series 5", 8)) {
            return new RPCS32(std::move(socket));
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
