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

#include "wprt.h"
#include "bufferstore.h"
#include "tcpsocket.h"
#include "bufferarray.h"
#include "Enum.h"

#include <iostream>

#include <stdlib.h>
#include <time.h>

using namespace std;

wprt::wprt(TCPSocket * _skt)
{
    skt = _skt;
    reset();
}

wprt::~wprt()
{
    skt->closeSocket();
}

//
// public common API
//
void wprt::
reconnect(void)
{
    //skt->closeSocket();
    skt->reconnect();
    reset();
}

void wprt::
reset(void)
{
    BufferStore a;
    status = RFSV::E_PSI_FILE_DISC;
    a.addStringT(getConnectName());
    if (skt->sendBufferStore(a)) {
        if (skt->getBufferStore(a) == 1) {
            if (!strcmp(a.getString(0), "Ok"))
                status = RFSV::E_PSI_GEN_NONE;
        }
    }
}

Enum<RFSV::errs> wprt::
getStatus(void)
{
    return status;
}

const char *wprt::
getConnectName(void)
{
    return "SYS$WPRT";
}

//
// protected internals
//
bool wprt::
sendCommand(enum commands cc, BufferStore & data)
{
    if (status == RFSV::E_PSI_FILE_DISC) {
        reconnect();
        if (status == RFSV::E_PSI_FILE_DISC)
            return false;
    }
    bool result;
    BufferStore a;
    a.addByte(cc);
    a.addBuff(data);
    result = skt->sendBufferStore(a);
    if (!result) {
        reconnect();
        result = skt->sendBufferStore(a);
        if (!result)
            status = RFSV::E_PSI_FILE_DISC;
    }
    return result;
}

Enum<RFSV::errs> wprt::
initPrinter() {
    Enum<RFSV::errs> ret;

    BufferStore a;
    a.addByte(2); // Major printer version
    a.addByte(0); // Minor printer version
    sendCommand(WPRT_INIT, a);
    if ((ret = getResponse(a)) != RFSV::E_PSI_GEN_NONE)
        cerr << "WPRT ERR:" << a << endl;
    else {
        if (a.getLen() != 3)
            ret = RFSV::E_PSI_GEN_FAIL;
        if ((a.getByte(0) != 0) || (a.getWord(1) != 2))
            ret = RFSV::E_PSI_GEN_FAIL;
    }
    return ret;
}

Enum<RFSV::errs> wprt::
getData(BufferStore &buf) {
    Enum<RFSV::errs> ret;

    sendCommand(WPRT_GET, buf);
    if ((ret = getResponse(buf)) != RFSV::E_PSI_GEN_NONE)
        cerr << "WPRT ERR:" << buf << endl;
    return ret;
}

Enum<RFSV::errs> wprt::
cancelJob() {
    Enum<RFSV::errs> ret;
    BufferStore a;

    sendCommand(WPRT_CANCEL, a);
    if ((ret = getResponse(a)) != RFSV::E_PSI_GEN_NONE)
        cerr << "WPRT ERR:" << a << endl;
    return ret;
}

bool wprt::
stop() {
    BufferStore a;
    return sendCommand(WPRT_STOP, a);
}

Enum<RFSV::errs> wprt::
getResponse(BufferStore & data)
{
    Enum<RFSV::errs> ret = RFSV::E_PSI_GEN_NONE;
    if (skt->getBufferStore(data) == 1)
        return ret;
    else
        status = RFSV::E_PSI_FILE_DISC;
    return status;
}
