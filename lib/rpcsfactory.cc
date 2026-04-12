/*
 * This file is part of plptools.
 *
 *  Copyright (C) 2000-2001 Fritz Elfert <felfert@to.com>
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

ENUM_DEFINITION_BEGIN(RPCSFactory::errs, RPCSFactory::FACERR_NONE)
    stringRep.add(RPCSFactory::FACERR_NONE,           N_("no error"));
    stringRep.add(RPCSFactory::FACERR_COULD_NOT_SEND, N_("could not send version request"));
    stringRep.add(RPCSFactory::FACERR_AGAIN,          N_("try again"));
    stringRep.add(RPCSFactory::FACERR_NOPSION,        N_("no EPOC device connected"));
    stringRep.add(RPCSFactory::FACERR_PROTVERSION,    N_("wrong protocol version"));
    stringRep.add(RPCSFactory::FACERR_NORESPONSE,     N_("no response from ncpd"));
ENUM_DEFINITION_END(RPCSFactory::errs)

RPCSFactory::RPCSFactory(TCPSocket *_skt) {
    err = FACERR_NONE;
    skt = _skt;
}

RPCSFactory::~RPCSFactory() {

}

RPCS *RPCSFactory::create(bool reconnect) {
    // skt is connected to the ncp daemon, which will have (hopefully) seen
    // an INFO exchange, where the protocol version of the remote Psion was
    // sent, and noted. We have to ask the ncp daemon which protocol it saw,
    // so we can instantiate the correct RPCS protocol handler for the
    // caller. We announce ourselves to the NCP daemon, and the relevant
    // RPCS module will also announce itself.

    BufferStore a;

    err = FACERR_NONE;
    a.addStringT("NCP$INFO");
    if (!skt->sendBufferStore(a)) {
        if (!reconnect)
            err = FACERR_COULD_NOT_SEND;
        else {
            skt->closeSocket();
            skt->reconnect();
            err = FACERR_AGAIN;
        }
        return NULL;
    }
    if (skt->getBufferStore(a) == 1) {
        if (a.getLen() > 8 && !strncmp(a.getString(), "Series 3", 8)) {
            return new RPCS16(skt);
        }
        else if (a.getLen() > 8 && !strncmp(a.getString(), "Series 5", 8)) {
            return new RPCS32(skt);
        }
        if ((a.getLen() > 8) && !strncmp(a.getString(), "No Psion", 8)) {
            skt->closeSocket();
            skt->reconnect();
            err = FACERR_NOPSION;
            return NULL;
        }
        // Invalid protocol version
        err = FACERR_PROTVERSION;
    } else
        err = FACERR_NORESPONSE;

    // No message returned.
    return NULL;
}
