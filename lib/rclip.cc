/*
 * This file is part of plptools.
 *
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
#include "config.h"

#include "rclip.h"

#include "bufferarray.h"
#include "bufferstore.h"
#include "Enum.h"
#include "tcpsocket.h"

#include <stdlib.h>
#include <time.h>

rclip* rclip::connect(const std::string &host, int port, Enum<ConnectionError> *error) {

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

    return new rclip(std::move(socket));
}

rclip::rclip(std::unique_ptr<TCPSocket> socket) {
    socket_ = std::move(socket);
    reset();
}

rclip::~rclip() {
    socket_->closeSocket();
}

//
// public common API
//
void rclip::reconnect(void) {
    socket_->reconnect();
    reset();
}

void rclip::reset(void) {
    BufferStore a;
    status = RFSV::E_PSI_FILE_DISC;
    a.addStringT(getConnectName());
    if (socket_->sendBufferStore(a)) {
        if (socket_->getBufferStore(a) == 1) {
            if (!strcmp(a.getString(0), "NAK")) {
                status = RFSV::E_PSI_GEN_NSUP;
            }
            if (!strcmp(a.getString(0), "Ok")) {
                status = RFSV::E_PSI_GEN_NONE;
            }
        }
    }
}

Enum<RFSV::errs> rclip::getStatus(void) {
    return status;
}

const char *rclip::getConnectName(void) {
    return "CLIPSVR.RSY";
}

//
// protected internals
//
bool rclip::sendCommand(enum commands cc) {
    if (status == RFSV::E_PSI_FILE_DISC) {
        reconnect();
        if (status == RFSV::E_PSI_FILE_DISC)
            return false;
    }
    if (status != RFSV::E_PSI_GEN_NONE)
        return false;

    bool result;
    BufferStore a;
    a.addByte(cc);
    switch (cc) {
        case RCLIP_INIT:
            a.addWord(0x100);
            break;
        case RCLIP_NOTIFY:
            a.addByte(0);
    }
    result = socket_->sendBufferStore(a);
    if (!result) {
        reconnect();
        result = socket_->sendBufferStore(a);
        if (!result)
            status = RFSV::E_PSI_FILE_DISC;
    }
    return result;
}

Enum<RFSV::errs> rclip::sendListen() {
    if (sendCommand(RCLIP_LISTEN)) {
        return RFSV::E_PSI_GEN_NONE;
    } else {
        return status;
    }
}

Enum<RFSV::errs> rclip::checkNotify() {
    Enum<RFSV::errs> ret;
    BufferStore a;

    int r = socket_->getBufferStore(a, false);
    if (r < 0) {
        ret = status = RFSV::E_PSI_FILE_DISC;
    } else {
        if (r == 0) {
            ret = RFSV::E_PSI_FILE_EOF;
        } else {
            if ((a.getLen() != 1) || (a.getByte(0) != 0)) {
                ret = RFSV::E_PSI_GEN_FAIL;
            }
        }
    }
    return ret;
}

Enum<RFSV::errs> rclip::waitNotify() {
    Enum<RFSV::errs> ret;

    BufferStore a;
    sendCommand(RCLIP_LISTEN);
    if ((ret = getResponse(a)) == RFSV::E_PSI_GEN_NONE) {
        if ((a.getLen() != 1) || (a.getByte(0) != 0)) {
            ret = RFSV::E_PSI_GEN_FAIL;
        }
    }
    return ret;
}

Enum<RFSV::errs> rclip::notify() {
    Enum<RFSV::errs> ret;
    BufferStore a;

    sendCommand(RCLIP_NOTIFY);
    if ((ret = getResponse(a)) == RFSV::E_PSI_GEN_NONE) {
        if ((a.getLen() != 1) || (a.getByte(0) != RCLIP_NOTIFY)) {
            ret = RFSV::E_PSI_GEN_FAIL;
        }
    }
    return ret;
}

Enum<RFSV::errs> rclip::initClipbd() {
    Enum<RFSV::errs> ret;
    BufferStore a;

    if (status != RFSV::E_PSI_GEN_NONE) {
        return status;
    }

    sendCommand(RCLIP_INIT);
    if ((ret = getResponse(a)) == RFSV::E_PSI_GEN_NONE) {
        if ((a.getLen() != 3) || (a.getByte(0) != RCLIP_INIT) || (a.getWord(1) != 0x100)) {
            ret = RFSV::E_PSI_GEN_FAIL;
        }
    }
    return ret;
}

Enum<RFSV::errs> rclip::getResponse(BufferStore & data) {
    Enum<RFSV::errs> ret = RFSV::E_PSI_GEN_NONE;

    if (status == RFSV::E_PSI_GEN_NSUP) {
        return status;
    }

    if (socket_->getBufferStore(data) == 1) {
        return ret;
    } else {
        status = RFSV::E_PSI_FILE_DISC;
    }
    return status;
}
