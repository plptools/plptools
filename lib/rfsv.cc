/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999 Matt J. Gumbley <matt@gumbley.demon.co.uk>
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

#include "rfsv.h"
#include "ppsocket.h"
#include "bufferstore.h"
#include "Enum.h"

using namespace std;

ENUM_DEFINITION_BEGIN(rfsv::errs, rfsv::E_PSI_GEN_NONE)
    stringRep.add(rfsv::E_PSI_GEN_NONE,        N_("no error"));
    stringRep.add(rfsv::E_PSI_GEN_FAIL,        N_("general"));
    stringRep.add(rfsv::E_PSI_GEN_ARG,         N_("bad argument"));
    stringRep.add(rfsv::E_PSI_GEN_OS,          N_("OS error"));
    stringRep.add(rfsv::E_PSI_GEN_NSUP,        N_("not supported"));
    stringRep.add(rfsv::E_PSI_GEN_UNDER,       N_("numeric underflow"));
    stringRep.add(rfsv::E_PSI_GEN_OVER,        N_("numeric overflow"));
    stringRep.add(rfsv::E_PSI_GEN_RANGE,       N_("numeric exception"));
    stringRep.add(rfsv::E_PSI_GEN_INUSE,       N_("in use"));
    stringRep.add(rfsv::E_PSI_GEN_NOMEMORY,    N_("out of memory"));
    stringRep.add(rfsv::E_PSI_GEN_NOSEGMENTS,  N_("out of segments"));
    stringRep.add(rfsv::E_PSI_GEN_NOSEM,       N_("out of semaphores"));
    stringRep.add(rfsv::E_PSI_GEN_NOPROC,      N_("out of processes"));
    stringRep.add(rfsv::E_PSI_GEN_OPEN,        N_("already open"));
    stringRep.add(rfsv::E_PSI_GEN_NOTOPEN,     N_("not open"));
    stringRep.add(rfsv::E_PSI_GEN_IMAGE,       N_("bad image"));
    stringRep.add(rfsv::E_PSI_GEN_RECEIVER,    N_("receiver error"));
    stringRep.add(rfsv::E_PSI_GEN_DEVICE,      N_("device error"));
    stringRep.add(rfsv::E_PSI_GEN_FSYS,        N_("no filesystem"));
    stringRep.add(rfsv::E_PSI_GEN_START,       N_("not ready"));
    stringRep.add(rfsv::E_PSI_GEN_NOFONT,      N_("no font"));
    stringRep.add(rfsv::E_PSI_GEN_TOOWIDE,     N_("too wide"));
    stringRep.add(rfsv::E_PSI_GEN_TOOMANY,     N_("too many"));
    stringRep.add(rfsv::E_PSI_FILE_EXIST,      N_("file already exists"));
    stringRep.add(rfsv::E_PSI_FILE_NXIST,      N_("no such file"));
    stringRep.add(rfsv::E_PSI_FILE_WRITE,      N_("write error"));
    stringRep.add(rfsv::E_PSI_FILE_READ,       N_("read error"));
    stringRep.add(rfsv::E_PSI_FILE_EOF,        N_("end of file"));
    stringRep.add(rfsv::E_PSI_FILE_FULL,       N_("disk/serial read buffer full"));
    stringRep.add(rfsv::E_PSI_FILE_NAME,       N_("invalid name"));
    stringRep.add(rfsv::E_PSI_FILE_ACCESS,     N_("access denied"));
    stringRep.add(rfsv::E_PSI_FILE_LOCKED,     N_("resource locked"));
    stringRep.add(rfsv::E_PSI_FILE_DEVICE,     N_("no such device"));
    stringRep.add(rfsv::E_PSI_FILE_DIR,        N_("no such directory"));
    stringRep.add(rfsv::E_PSI_FILE_RECORD,     N_("no such record"));
    stringRep.add(rfsv::E_PSI_FILE_RDONLY,     N_("file is read-only"));
    stringRep.add(rfsv::E_PSI_FILE_INV,        N_("invalid I/O operation"));
    stringRep.add(rfsv::E_PSI_FILE_PENDING,    N_("I/O pending (not yet completed)"));
    stringRep.add(rfsv::E_PSI_FILE_VOLUME,     N_("invalid volume name"));
    stringRep.add(rfsv::E_PSI_FILE_CANCEL,     N_("cancelled"));
    stringRep.add(rfsv::E_PSI_FILE_ALLOC,      N_("no memory for control block"));
    stringRep.add(rfsv::E_PSI_FILE_DISC,       N_("unit disconnected"));
    stringRep.add(rfsv::E_PSI_FILE_CONNECT,    N_("already connected"));
    stringRep.add(rfsv::E_PSI_FILE_RETRAN,     N_("retransmission threshold exceeded"));
    stringRep.add(rfsv::E_PSI_FILE_LINE,       N_("physical link failure"));
    stringRep.add(rfsv::E_PSI_FILE_INACT,      N_("inactivity timer expired"));
    stringRep.add(rfsv::E_PSI_FILE_PARITY,     N_("serial parity error"));
    stringRep.add(rfsv::E_PSI_FILE_FRAME,      N_("serial framing error"));
    stringRep.add(rfsv::E_PSI_FILE_OVERRUN,    N_("serial overrun error"));
    stringRep.add(rfsv::E_PSI_MDM_CONFAIL,     N_("modem cannot connect to remote modem"));
    stringRep.add(rfsv::E_PSI_MDM_BUSY,        N_("remote modem busy"));
    stringRep.add(rfsv::E_PSI_MDM_NOANS,       N_("remote modem did not answer"));
    stringRep.add(rfsv::E_PSI_MDM_BLACKLIST,   N_("number blacklisted by the modem"));
    stringRep.add(rfsv::E_PSI_FILE_NOTREADY,   N_("drive not ready"));
    stringRep.add(rfsv::E_PSI_FILE_UNKNOWN,    N_("unknown media"));
    stringRep.add(rfsv::E_PSI_FILE_DIRFULL,    N_("directory full"));
    stringRep.add(rfsv::E_PSI_FILE_PROTECT,    N_("write-protected"));
    stringRep.add(rfsv::E_PSI_FILE_CORRUPT,    N_("media corrupt"));
    stringRep.add(rfsv::E_PSI_FILE_ABORT,      N_("aborted operation"));
    stringRep.add(rfsv::E_PSI_FILE_ERASE,      N_("failed to erase flash media"));
    stringRep.add(rfsv::E_PSI_FILE_INVALID,    N_("invalid file for DBF system"));
    stringRep.add(rfsv::E_PSI_GEN_POWER,       N_("power failure"));
    stringRep.add(rfsv::E_PSI_FILE_TOOBIG,     N_("too big"));
    stringRep.add(rfsv::E_PSI_GEN_DESCR,       N_("bad descriptor"));
    stringRep.add(rfsv::E_PSI_GEN_LIB,         N_("bad entry point"));
    stringRep.add(rfsv::E_PSI_FILE_NDISC,      N_("could not diconnect"));
    stringRep.add(rfsv::E_PSI_FILE_DRIVER,     N_("bad driver"));
    stringRep.add(rfsv::E_PSI_FILE_COMPLETION, N_("operation not completed"));
    stringRep.add(rfsv::E_PSI_GEN_BUSY,        N_("server busy"));
    stringRep.add(rfsv::E_PSI_GEN_TERMINATED,  N_("terminated"));
    stringRep.add(rfsv::E_PSI_GEN_DIED,        N_("died"));
    stringRep.add(rfsv::E_PSI_FILE_HANDLE,     N_("bad handle"));
    stringRep.add(rfsv::E_PSI_NOT_SIBO,        N_("invalid operation for RFSV16"));
    stringRep.add(rfsv::E_PSI_INTERNAL,        N_("libplp internal error"));
ENUM_DEFINITION_END(rfsv::errs)


const char *rfsv::getConnectName(void) {
    return "SYS$RFSV";
}

rfsv::~rfsv() {
    skt->closeSocket();
}

void rfsv::reconnect(void)
{
    skt->reconnect();
    serNum = 0;
    reset();
}

void rfsv::reset(void) {
    bufferStore a;
    status = E_PSI_FILE_DISC;
    a.addStringT(getConnectName());
    if (skt->sendBufferStore(a)) {
        if (skt->getBufferStore(a) == 1) {
            if (!strcmp(a.getString(0), "Ok"))
                status = E_PSI_GEN_NONE;
        }
    }
}

Enum<rfsv::errs> rfsv::getStatus(void) {
    return status;
}

string rfsv::
convertSlash(const string &name)
{
    string tmp = "";
    for (const char *p = name.c_str(); *p; p++)
        tmp += (*p == '/') ? '\\' : *p;
    return tmp;
}

string rfsv::
attr2String(const uint32_t attr)
{
    string tmp = "";
    tmp += ((attr & PSI_A_DIR) ? 'd' : '-');
    tmp += ((attr & PSI_A_READ) ? 'r' : '-');
    tmp += ((attr & PSI_A_RDONLY) ? '-' : 'w');
    tmp += ((attr & PSI_A_HIDDEN) ? 'h' : '-');
    tmp += ((attr & PSI_A_SYSTEM) ? 's' : '-');
    tmp += ((attr & PSI_A_ARCHIVE) ? 'a' : '-');
    tmp += ((attr & PSI_A_VOLUME) ? 'v' : '-');

    // EPOC
    tmp += ((attr & PSI_A_NORMAL) ? 'n' : '-');
    tmp += ((attr & PSI_A_TEMP) ? 't' : '-');
    tmp += ((attr & PSI_A_COMPRESSED) ? 'c' : '-');
    // SIBO
    tmp[7] = ((attr & PSI_A_EXEC) ? 'x' : tmp[7]);
    tmp[8] = ((attr & PSI_A_STREAM) ? 'b' : tmp[8]);
    tmp[9] = ((attr & PSI_A_TEXT) ? 't' : tmp[9]);
    return tmp;
}

int rfsv::
getSpeed()
{
    bufferStore a;
    a.addStringT("NCP$GSPD");
    if (!skt->sendBufferStore(a))
        return -1;
    if (skt->getBufferStore(a) != 1)
        return -1;
    if (a.getLen() != 5)
        return -1;
    if (a.getByte(0) != E_PSI_GEN_NONE)
        return -1;
    return a.getDWord(1);
}
