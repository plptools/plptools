/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999 Matt J. Gumbley <matt@gumbley.demon.co.uk>
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *  Copyright (c) 2026 Jason Morley <hello@jbmorley.co.uk>
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

#include "bufferstore.h"
#include "drive.h"
#include "Enum.h"
#include "ncpclient.h"
#include "plpdirent.h"
#include "rfsv16.h"
#include "rfsv32.h"
#include "tcpsocket.h"

using namespace std;

ENUM_DEFINITION_BEGIN(RFSV::errs, RFSV::E_PSI_GEN_NONE)
    stringRep.add(RFSV::E_PSI_GEN_NONE,        N_("no error"));
    stringRep.add(RFSV::E_PSI_GEN_FAIL,        N_("general"));
    stringRep.add(RFSV::E_PSI_GEN_ARG,         N_("bad argument"));
    stringRep.add(RFSV::E_PSI_GEN_OS,          N_("OS error"));
    stringRep.add(RFSV::E_PSI_GEN_NSUP,        N_("not supported"));
    stringRep.add(RFSV::E_PSI_GEN_UNDER,       N_("numeric underflow"));
    stringRep.add(RFSV::E_PSI_GEN_OVER,        N_("numeric overflow"));
    stringRep.add(RFSV::E_PSI_GEN_RANGE,       N_("numeric exception"));
    stringRep.add(RFSV::E_PSI_GEN_INUSE,       N_("in use"));
    stringRep.add(RFSV::E_PSI_GEN_NOMEMORY,    N_("out of memory"));
    stringRep.add(RFSV::E_PSI_GEN_NOSEGMENTS,  N_("out of segments"));
    stringRep.add(RFSV::E_PSI_GEN_NOSEM,       N_("out of semaphores"));
    stringRep.add(RFSV::E_PSI_GEN_NOPROC,      N_("out of processes"));
    stringRep.add(RFSV::E_PSI_GEN_OPEN,        N_("already open"));
    stringRep.add(RFSV::E_PSI_GEN_NOTOPEN,     N_("not open"));
    stringRep.add(RFSV::E_PSI_GEN_IMAGE,       N_("bad image"));
    stringRep.add(RFSV::E_PSI_GEN_RECEIVER,    N_("receiver error"));
    stringRep.add(RFSV::E_PSI_GEN_DEVICE,      N_("device error"));
    stringRep.add(RFSV::E_PSI_GEN_FSYS,        N_("no filesystem"));
    stringRep.add(RFSV::E_PSI_GEN_START,       N_("not ready"));
    stringRep.add(RFSV::E_PSI_GEN_NOFONT,      N_("no font"));
    stringRep.add(RFSV::E_PSI_GEN_TOOWIDE,     N_("too wide"));
    stringRep.add(RFSV::E_PSI_GEN_TOOMANY,     N_("too many"));
    stringRep.add(RFSV::E_PSI_FILE_EXIST,      N_("file already exists"));
    stringRep.add(RFSV::E_PSI_FILE_NXIST,      N_("no such file"));
    stringRep.add(RFSV::E_PSI_FILE_WRITE,      N_("write error"));
    stringRep.add(RFSV::E_PSI_FILE_READ,       N_("read error"));
    stringRep.add(RFSV::E_PSI_FILE_EOF,        N_("end of file"));
    stringRep.add(RFSV::E_PSI_FILE_FULL,       N_("disk/serial read buffer full"));
    stringRep.add(RFSV::E_PSI_FILE_NAME,       N_("invalid name"));
    stringRep.add(RFSV::E_PSI_FILE_ACCESS,     N_("access denied"));
    stringRep.add(RFSV::E_PSI_FILE_LOCKED,     N_("resource locked"));
    stringRep.add(RFSV::E_PSI_FILE_DEVICE,     N_("no such device"));
    stringRep.add(RFSV::E_PSI_FILE_DIR,        N_("no such directory"));
    stringRep.add(RFSV::E_PSI_FILE_RECORD,     N_("no such record"));
    stringRep.add(RFSV::E_PSI_FILE_RDONLY,     N_("file is read-only"));
    stringRep.add(RFSV::E_PSI_FILE_INV,        N_("invalid I/O operation"));
    stringRep.add(RFSV::E_PSI_FILE_PENDING,    N_("I/O pending (not yet completed)"));
    stringRep.add(RFSV::E_PSI_FILE_VOLUME,     N_("invalid volume name"));
    stringRep.add(RFSV::E_PSI_FILE_CANCEL,     N_("cancelled"));
    stringRep.add(RFSV::E_PSI_FILE_ALLOC,      N_("no memory for control block"));
    stringRep.add(RFSV::E_PSI_FILE_DISC,       N_("unit disconnected"));
    stringRep.add(RFSV::E_PSI_FILE_CONNECT,    N_("already connected"));
    stringRep.add(RFSV::E_PSI_FILE_RETRAN,     N_("retransmission threshold exceeded"));
    stringRep.add(RFSV::E_PSI_FILE_LINE,       N_("physical link failure"));
    stringRep.add(RFSV::E_PSI_FILE_INACT,      N_("inactivity timer expired"));
    stringRep.add(RFSV::E_PSI_FILE_PARITY,     N_("serial parity error"));
    stringRep.add(RFSV::E_PSI_FILE_FRAME,      N_("serial framing error"));
    stringRep.add(RFSV::E_PSI_FILE_OVERRUN,    N_("serial overrun error"));
    stringRep.add(RFSV::E_PSI_MDM_CONFAIL,     N_("modem cannot connect to remote modem"));
    stringRep.add(RFSV::E_PSI_MDM_BUSY,        N_("remote modem busy"));
    stringRep.add(RFSV::E_PSI_MDM_NOANS,       N_("remote modem did not answer"));
    stringRep.add(RFSV::E_PSI_MDM_BLACKLIST,   N_("number blacklisted by the modem"));
    stringRep.add(RFSV::E_PSI_FILE_NOTREADY,   N_("drive not ready"));
    stringRep.add(RFSV::E_PSI_FILE_UNKNOWN,    N_("unknown media"));
    stringRep.add(RFSV::E_PSI_FILE_DIRFULL,    N_("directory full"));
    stringRep.add(RFSV::E_PSI_FILE_PROTECT,    N_("write-protected"));
    stringRep.add(RFSV::E_PSI_FILE_CORRUPT,    N_("media corrupt"));
    stringRep.add(RFSV::E_PSI_FILE_ABORT,      N_("aborted operation"));
    stringRep.add(RFSV::E_PSI_FILE_ERASE,      N_("failed to erase flash media"));
    stringRep.add(RFSV::E_PSI_FILE_INVALID,    N_("invalid file for DBF system"));
    stringRep.add(RFSV::E_PSI_GEN_POWER,       N_("power failure"));
    stringRep.add(RFSV::E_PSI_FILE_TOOBIG,     N_("too big"));
    stringRep.add(RFSV::E_PSI_GEN_DESCR,       N_("bad descriptor"));
    stringRep.add(RFSV::E_PSI_GEN_LIB,         N_("bad entry point"));
    stringRep.add(RFSV::E_PSI_FILE_NDISC,      N_("could not diconnect"));
    stringRep.add(RFSV::E_PSI_FILE_DRIVER,     N_("bad driver"));
    stringRep.add(RFSV::E_PSI_FILE_COMPLETION, N_("operation not completed"));
    stringRep.add(RFSV::E_PSI_GEN_BUSY,        N_("server busy"));
    stringRep.add(RFSV::E_PSI_GEN_TERMINATED,  N_("terminated"));
    stringRep.add(RFSV::E_PSI_GEN_DIED,        N_("died"));
    stringRep.add(RFSV::E_PSI_FILE_HANDLE,     N_("bad handle"));
    stringRep.add(RFSV::E_PSI_NOT_SIBO,        N_("invalid operation for RFSV16"));
    stringRep.add(RFSV::E_PSI_INTERNAL,        N_("libplp internal error"));
ENUM_DEFINITION_END(RFSV::errs)

RFSV *RFSV::connect(const std::string &host, int port, Enum<ConnectionError> *error) {
    return ncp_client::connect<RFSV, RFSV16, RFSV32>(host, port, false, error);
}

const char *RFSV::getConnectName(void) {
    return "SYS$RFSV";
}

RFSV::~RFSV() {
    socket_->closeSocket();
}

void RFSV::reconnect(void) {
    socket_->reconnect();
    operationId_ = 0;
    reset();
}

void RFSV::reset(void) {
    BufferStore a;
    status_ = E_PSI_FILE_DISC;
    a.addStringT(getConnectName());
    if (socket_->sendBufferStore(a)) {
        if (socket_->getBufferStore(a) == 1) {
            if (!strcmp(a.getString(0), "Ok"))
                status_ = E_PSI_GEN_NONE;
        }
    }
}

Enum<RFSV::errs> RFSV::getStatus(void) {
    return status_;
}

string RFSV::convertSlash(const string &name) {
    string tmp = "";
    for (const char *p = name.c_str(); *p; p++)
        tmp += (*p == '/') ? '\\' : *p;
    return tmp;
}

string RFSV::attr2String(const uint32_t attr) {
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

int RFSV::getSpeed() {
    BufferStore a;
    a.addStringT("NCP$GSPD");
    if (!socket_->sendBufferStore(a))
        return -1;
    if (socket_->getBufferStore(a) != 1)
        return -1;
    if (a.getLen() != 5)
        return -1;
    if (a.getByte(0) != E_PSI_GEN_NONE)
        return -1;
    return a.getDWord(1);
}

Enum<RFSV::errs> RFSV::dir(const std::string &path,
                           bool recursive,
                           std::vector<PlpDirent> &_files)  {
    Enum<RFSV::errs> result;

    // List the top level directory.
    PlpDir entries;
    result = dir(path.c_str(), entries);
    if (result != RFSV::E_PSI_GEN_NONE) {
        return result;
    }

    // List the inner directories.
    std::vector<PlpDirent> files;
    for (PlpDirent entry: entries) {
        files.push_back(entry);
        if (recursive && entry.isDirectory()) {
            std::vector<PlpDirent> directoryFiles;
            result = dir(entry.getPath(), recursive, directoryFiles);
            if (result != RFSV::E_PSI_GEN_NONE) {
                return result;
            }
            files.insert(files.end(), directoryFiles.begin(), directoryFiles.end());
        }
    }
    _files = files;
    return RFSV::E_PSI_GEN_NONE;
}

Enum<RFSV::errs> RFSV::drives(std::vector<Drive> &_drives) {
    Enum<RFSV::errs> result;

    // Get the supported drives.
    uint32_t driveBits = 0;
    result = devlist(driveBits);
    if (result != RFSV::E_PSI_GEN_NONE) {
        return result;
    }

    // Convert them to drive letters.
    std::vector<char> driveLetters;
    for (int i = 0; i < 26; i++) {
        if (driveBits & (1 << i)) {
            driveLetters.push_back('A' + i);
        }
    }

    // Iterate over the drive letters and get the info for the available drives.
    std::vector<Drive> drives;
    for (const auto &driveLetter : driveLetters) {
        Drive drive;
        result = devinfo(driveLetter, drive);
        if (result == RFSV::E_PSI_FILE_NOTREADY) {
            // Ignore drives that aren't available.
            continue;
        }
        if (result != RFSV::E_PSI_GEN_NONE) {
            return result;
        }
        drives.push_back(drive);
    }

    _drives = drives;
    return RFSV::E_PSI_GEN_NONE;
}
