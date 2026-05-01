/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Matt J. Gumbley <matt@gumbley.demon.co.uk>
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

#include "rfsv.h"
#include "rfsv16.h"
#include "rfsv32.h"
#include "rfsvfactory.h"
#include "bufferstore.h"
#include "tcpsocket.h"
#include "Enum.h"

#include <stdlib.h>
#include <time.h>

using namespace std;

ENUM_DEFINITION_BEGIN(RFSVFactory::errs, RFSVFactory::FACERR_NONE)
    stringRep.add(RFSVFactory::FACERR_NONE,               N_("no error"));
    stringRep.add(RFSVFactory::FACERR_COULD_NOT_SEND,     N_("could not send version request"));
    stringRep.add(RFSVFactory::FACERR_AGAIN,              N_("try again"));
    stringRep.add(RFSVFactory::FACERR_NOPSION,            N_("no EPOC device connected"));
    stringRep.add(RFSVFactory::FACERR_PROTVERSION,        N_("wrong protocol version"));
    stringRep.add(RFSVFactory::FACERR_NORESPONSE,         N_("no response from ncpd"));
    stringRep.add(RFSVFactory::FACERR_CONNECTION_FAILURE, N_("could not connect to ncpd"));
ENUM_DEFINITION_END(RFSVFactory::errs)

RFSVFactory::RFSVFactory(const std::string &host, int port)
: host_(host)
, port_(port) {}

RFSVFactory::~RFSVFactory() {
}

RFSV* RFSVFactory::create(bool reconnect, Enum<errs> *error) {

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
    // saw, so we can instantiate the correct RFSV protocol handler for the caller. We announce ourselves to the NCP
    // daemon, and the relevant RFSV module will also announce itself.

    BufferStore a;
    a.addStringT("NCP$INFO");
    if (!socket->sendBufferStore(a)) {
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
        return NULL;
    }
    if (socket->getBufferStore(a) == 1) {
        if (a.getLen() > 8 && !strncmp(a.getString(), "Series 3", 8)) {
            return new RFSV16(std::move(socket));
        }
        else if (a.getLen() > 8 && !strncmp(a.getString(), "Series 5", 8)) {
            return new RFSV32(std::move(socket));
        }
        if ((a.getLen() > 8) && !strncmp(a.getString(), "No Psion", 8)) {
            socket->closeSocket();
            socket->reconnect();
            if (error) {
                *error = FACERR_NOPSION;
            }
            return NULL;
        }
        // Invalid protocol version
        if (error) {
            *error = FACERR_PROTVERSION;
        }
    } else
        if (error) {
            *error = FACERR_NORESPONSE;
        }

    return NULL;
}
