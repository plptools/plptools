/*
 * This file is part of plptools.
 *
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

#include "rclip.h"
#include "bufferstore.h"
#include "ppsocket.h"
#include "bufferarray.h"
#include "Enum.h"

#include <stdlib.h>
#include <time.h>

rclip::rclip(ppsocket * _skt)
{
    skt = _skt;
    reset();
}

rclip::~rclip()
{
    skt->closeSocket();
}

//
// public common API
//
void rclip::
reconnect(void)
{
    //skt->closeSocket();
    skt->reconnect();
    reset();
}

void rclip::
reset(void)
{
    bufferStore a;
    status = rfsv::E_PSI_FILE_DISC;
    a.addStringT(getConnectName());
    if (skt->sendBufferStore(a)) {
        if (skt->getBufferStore(a) == 1) {
            if (!strcmp(a.getString(0), "NAK"))
                status = rfsv::E_PSI_GEN_NSUP;
            if (!strcmp(a.getString(0), "Ok"))
                status = rfsv::E_PSI_GEN_NONE;
        }
    }
}

Enum<rfsv::errs> rclip::
getStatus(void)
{
    return status;
}

const char *rclip::
getConnectName(void)
{
    return "CLIPSVR.RSY";
}

//
// protected internals
//
bool rclip::
sendCommand(enum commands cc)
{
    if (status == rfsv::E_PSI_FILE_DISC) {
        reconnect();
        if (status == rfsv::E_PSI_FILE_DISC)
            return false;
    }
    if (status != rfsv::E_PSI_GEN_NONE)
        return false;

    bool result;
    bufferStore a;
    a.addByte(cc);
    switch (cc) {
        case RCLIP_INIT:
            a.addWord(0x100);
            break;
        case RCLIP_NOTIFY:
            a.addByte(0);
    }
    result = skt->sendBufferStore(a);
    if (!result) {
        reconnect();
        result = skt->sendBufferStore(a);
        if (!result)
            status = rfsv::E_PSI_FILE_DISC;
    }
    return result;
}

Enum<rfsv::errs> rclip::
sendListen() {
    if (sendCommand(RCLIP_LISTEN))
        return rfsv::E_PSI_GEN_NONE;
    else
        return status;
}

Enum<rfsv::errs> rclip::
checkNotify() {
    Enum<rfsv::errs> ret;
    bufferStore a;

    int r = skt->getBufferStore(a, false);
    if (r < 0) {
        ret = status = rfsv::E_PSI_FILE_DISC;
    } else {
        if (r == 0)
            ret = rfsv::E_PSI_FILE_EOF;
        else {
            if ((a.getLen() != 1) || (a.getByte(0) != 0))
                ret = rfsv::E_PSI_GEN_FAIL;
        }
    }
    return ret;
}

Enum<rfsv::errs> rclip::
waitNotify() {
    Enum<rfsv::errs> ret;

    bufferStore a;
    sendCommand(RCLIP_LISTEN);
    if ((ret = getResponse(a)) == rfsv::E_PSI_GEN_NONE) {
        if ((a.getLen() != 1) || (a.getByte(0) != 0))
            ret = rfsv::E_PSI_GEN_FAIL;
    }
    return ret;
}

Enum<rfsv::errs> rclip::
notify() {
    Enum<rfsv::errs> ret;
    bufferStore a;

    sendCommand(RCLIP_NOTIFY);
    if ((ret = getResponse(a)) == rfsv::E_PSI_GEN_NONE) {
        if ((a.getLen() != 1) || (a.getByte(0) != RCLIP_NOTIFY))
            ret = rfsv::E_PSI_GEN_FAIL;
    }
    return ret;
}

Enum<rfsv::errs> rclip::
initClipbd() {
    Enum<rfsv::errs> ret;
    bufferStore a;

    if (status != rfsv::E_PSI_GEN_NONE)
        return status;

    sendCommand(RCLIP_INIT);
    if ((ret = getResponse(a)) == rfsv::E_PSI_GEN_NONE) {
        if ((a.getLen() != 3) || (a.getByte(0) != RCLIP_INIT) ||
            (a.getWord(1) != 0x100))
            ret = rfsv::E_PSI_GEN_FAIL;
    }
    return ret;
}

Enum<rfsv::errs> rclip::
getResponse(bufferStore & data)
{
    Enum<rfsv::errs> ret = rfsv::E_PSI_GEN_NONE;

    if (status == rfsv::E_PSI_GEN_NSUP)
        return status;

    if (skt->getBufferStore(data) == 1)
        return ret;
    else
        status = rfsv::E_PSI_FILE_DISC;
    return status;
}
