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

#include "rpcs.h"
#include "bufferstore.h"
#include "tcpsocket.h"
#include "bufferarray.h"
#include "psiprocess.h"
#include "Enum.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

ENUM_DEFINITION_BEGIN(RPCS::machs, RPCS::PSI_MACH_UNKNOWN)
    stringRep.add(RPCS::PSI_MACH_UNKNOWN,   N_("Unknown device"));
    stringRep.add(RPCS::PSI_MACH_PC,        N_("PC"));
    stringRep.add(RPCS::PSI_MACH_MC,        N_("MC"));
    stringRep.add(RPCS::PSI_MACH_HC,        N_("HC"));
    stringRep.add(RPCS::PSI_MACH_S3,        N_("Series 3"));
    stringRep.add(RPCS::PSI_MACH_S3A,       N_("Series 3a, 3c or 3mx"));
    stringRep.add(RPCS::PSI_MACH_WORKABOUT, N_("Workabout"));
    stringRep.add(RPCS::PSI_MACH_SIENA,     N_("Siena"));
    stringRep.add(RPCS::PSI_MACH_S3C,       N_("Series 3c"));
    stringRep.add(RPCS::PSI_MACH_S5,        N_("Series 5"));
    stringRep.add(RPCS::PSI_MACH_WINC,      N_("WinC"));
ENUM_DEFINITION_END(RPCS::machs)

ENUM_DEFINITION_BEGIN(RPCS::batterystates, RPCS::PSI_BATT_DEAD)
    stringRep.add(RPCS::PSI_BATT_DEAD,    N_("Empty"));
    stringRep.add(RPCS::PSI_BATT_VERYLOW, N_("Very Low"));
    stringRep.add(RPCS::PSI_BATT_LOW,     N_("Low"));
    stringRep.add(RPCS::PSI_BATT_GOOD,    N_("Good"));
ENUM_DEFINITION_END(RPCS::batterystates)


ENUM_DEFINITION_BEGIN(RPCS::languages, RPCS::PSI_LANG_TEST)
    stringRep.add(RPCS::PSI_LANG_TEST,  N_("Test"));
    stringRep.add(RPCS::PSI_LANG_en_GB, N_("English"));
    stringRep.add(RPCS::PSI_LANG_de_DE, N_("German"));
    stringRep.add(RPCS::PSI_LANG_fr_FR, N_("French"));
    stringRep.add(RPCS::PSI_LANG_es_ES, N_("Spanish"));
    stringRep.add(RPCS::PSI_LANG_it_IT, N_("Italian"));
    stringRep.add(RPCS::PSI_LANG_sv_SE, N_("Swedish"));
    stringRep.add(RPCS::PSI_LANG_da_DK, N_("Danish"));
    stringRep.add(RPCS::PSI_LANG_no_NO, N_("Norwegian"));
    stringRep.add(RPCS::PSI_LANG_fi_FI, N_("Finnish"));
    stringRep.add(RPCS::PSI_LANG_en_US, N_("American"));
    stringRep.add(RPCS::PSI_LANG_fr_CH, N_("Swiss French"));
    stringRep.add(RPCS::PSI_LANG_de_CH, N_("Swiss German"));
    stringRep.add(RPCS::PSI_LANG_pt_PT, N_("Portugese"));
    stringRep.add(RPCS::PSI_LANG_tr_TR, N_("Turkish"));
    stringRep.add(RPCS::PSI_LANG_is_IS, N_("Icelandic"));
    stringRep.add(RPCS::PSI_LANG_ru_RU, N_("Russian"));
    stringRep.add(RPCS::PSI_LANG_hu_HU, N_("Hungarian"));
    stringRep.add(RPCS::PSI_LANG_nl_NL, N_("Dutch"));
    stringRep.add(RPCS::PSI_LANG_nl_BE, N_("Belgian Flemish"));
    stringRep.add(RPCS::PSI_LANG_en_AU, N_("Australian"));
    stringRep.add(RPCS::PSI_LANG_fr_BE, N_("Belgish French"));
    stringRep.add(RPCS::PSI_LANG_de_AT, N_("Austrian"));
    stringRep.add(RPCS::PSI_LANG_en_NZ, N_("New Zealand English"));
    stringRep.add(RPCS::PSI_LANG_fr_CA, N_("Canadian French"));
    stringRep.add(RPCS::PSI_LANG_cs_CZ, N_("Czech"));
    stringRep.add(RPCS::PSI_LANG_sk_SK, N_("Slovak"));
    stringRep.add(RPCS::PSI_LANG_pl_PL, N_("Polish"));
    stringRep.add(RPCS::PSI_LANG_sl_SI, N_("Slovenian"));
ENUM_DEFINITION_END(RPCS::languages)

RPCS::~RPCS() {
    socket_->closeSocket();
}

//
// public common API
//
void RPCS::reconnect(void) {
    socket_->reconnect();
    reset();
}

void RPCS::reset(void) {
    BufferStore a;
    mtCacheS5mx = 0;
    status = RFSV::E_PSI_FILE_DISC;
    a.addStringT(getConnectName());
    if (socket_->sendBufferStore(a)) {
        if (socket_->getBufferStore(a) == 1) {
            if (!strcmp(a.getString(0), "Ok")) {
                status = RFSV::E_PSI_GEN_NONE;
            }
        }
    }
}

Enum<RFSV::errs> RPCS::getStatus(void) {
    return status;
}

const char *RPCS::getConnectName(void) {
    return "SYS$RPCS";
}

//
// protected internals
//
bool RPCS::sendCommand(enum commands cc, BufferStore & data) {
    if (status == RFSV::E_PSI_FILE_DISC) {
        reconnect();
        if (status == RFSV::E_PSI_FILE_DISC) {
            return false;
        }
    }
    bool result;
    BufferStore a;
    a.addByte(cc);
    a.addBuff(data);
    result = socket_->sendBufferStore(a);
    if (!result) {
        reconnect();
        result = socket_->sendBufferStore(a);
        if (!result) {
            status = RFSV::E_PSI_FILE_DISC;
        }
    }
    return result;
}

Enum<RFSV::errs> RPCS::getResponse(BufferStore & data, bool statusIsFirstByte) {
    Enum<RFSV::errs> ret;
    if (socket_->getBufferStore(data) == 1) {
        if (statusIsFirstByte) {
            ret = (enum RFSV::errs)((char)data.getByte(0));
            data.discardFirstBytes(1);
        } else {
            int l = data.getLen();
            if (l > 0) {
                ret = (enum RFSV::errs)((char)data.getByte(data.getLen() - 1));
                data.init((const unsigned char *)data.getString(), l - 1);
            } else {
                ret = RFSV::E_PSI_GEN_FAIL;
            }
        }
        return ret;
    } else {
        status = RFSV::E_PSI_FILE_DISC;
    }
    return status;
}

//
// APIs, identical on SIBO and EPOC
//
Enum<RFSV::errs> RPCS::getNCPversion(int &major, int &minor) {
    Enum<RFSV::errs> res;
    BufferStore a;

    if (!sendCommand(QUERY_NCP, a)) {
        return RFSV::E_PSI_FILE_DISC;
    }
    if ((res = getResponse(a, true)) != RFSV::E_PSI_GEN_NONE) {
        return res;
    }
    if (a.getLen() != 2) {
        return RFSV::E_PSI_GEN_FAIL;
    }
    major = a.getByte(0);
    minor = a.getByte(1);
    return res;
}

Enum<RFSV::errs> RPCS::execProgram(const char *program, const char *args) {
    BufferStore a;

    a.addStringT(program);
    int l = strlen(program);
    for (int i = 127; i > l; i--) {
        a.addByte(0);
    }

    /**
    * This is a hack for the jotter app on mx5 pro. (and probably others)
    * Jotter seems to read its arguments one char past normal apps.
    * Without this hack, The Drive-Character gets lost. Other apps don't
    * seem to be hurt by the additional blank.
    */
    a.addByte(strlen(args)+1);
    a.addByte(' ');

    a.addStringT(args);

    if (!sendCommand(EXEC_PROG, a)) {
        return RFSV::E_PSI_FILE_DISC;
    }
    return getResponse(a, true);
}

Enum<RFSV::errs> RPCS::stopProgram(const char *program) {
    BufferStore a;

    a.addStringT(program);
    if (!sendCommand(STOP_PROG, a)) {
        return RFSV::E_PSI_FILE_DISC;
    }
    return getResponse(a, true);
}

Enum<RFSV::errs> RPCS::queryProgram(const char *program) {
    BufferStore a;

    a.addStringT(program);
    if (!sendCommand(QUERY_PROG, a)) {
        return RFSV::E_PSI_FILE_DISC;
    }
    return getResponse(a, true);
}

Enum<RFSV::errs> RPCS::queryPrograms(processList &ret) {
    BufferStore a;
    const char *drives;
    const char *dptr;
    bool anySuccess = false;
    Enum<RFSV::errs> res;

    // First, check how many drives we need to query
    a.addStringT("M:"); // Drive M only exists on a SIBO
    if (!sendCommand(RPCS::GET_UNIQUEID, a))
        return RFSV::E_PSI_FILE_DISC;
    if (getResponse(a, false) == RFSV::E_PSI_GEN_NONE) {
        // A SIBO; Must query all possible drives
        drives = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    } else {
        // A Series 5; Query of C is sufficient
        drives = "C";
    }

    dptr = drives;
    ret.clear();

    if ((mtCacheS5mx & 4) == 0) {
        Enum<machs> tmp;
        if (getMachineType(tmp) != RFSV::E_PSI_GEN_NONE) {
            return RFSV::E_PSI_GEN_FAIL;
        }

    }
    if ((mtCacheS5mx & 9) == 1) {
        machineInfo tmp;
        if (getMachineInfo(tmp) == RFSV::E_PSI_FILE_DISC) {
            return RFSV::E_PSI_FILE_DISC;
        }
    }
    bool s5mx = (mtCacheS5mx == 15);
    while (*dptr) {
        a.init();
        a.addByte(*dptr);
        if (!sendCommand(RPCS::QUERY_DRIVE, a)) {
            return RFSV::E_PSI_FILE_DISC;
        }
        if (getResponse(a, false) == RFSV::E_PSI_GEN_NONE) {
            anySuccess = true;
            int l = a.getLen();
            while (l > 0) {
                const char *s;
                char *p;
                int pid;
                int sl;

                s = a.getString(0);
                sl = strlen(s) + 1;
                l -= sl;
                a.discardFirstBytes(sl);
                if ((p = strstr((char *)s, ".$"))) {
                    *p = '\0'; p += 2;
                    sscanf(p, "%d", &pid);
                } else {
                    pid = 0;
                }
                PsiProcess proc(pid, s, a.getString(0), s5mx);
                ret.push_back(proc);
                sl = strlen(a.getString(0)) + 1;
                l -= sl;
                a.discardFirstBytes(sl);
            }
        }
        dptr++;
    }
    if (anySuccess && !ret.empty()) {
        for (processList::iterator i = ret.begin(); i != ret.end(); i++) {
            string cmdline;
            if (getCmdLine(i->getProcId(), cmdline) == RFSV::E_PSI_GEN_NONE) {
                i->setArgs(cmdline + " " + i->getArgs());
            }
        }
    }
    return anySuccess ? RFSV::E_PSI_GEN_NONE : RFSV::E_PSI_GEN_FAIL;
}

Enum<RFSV::errs> RPCS::formatOpen(const char drive, int &handle, int &count) {
    Enum<RFSV::errs> res;
    BufferStore a;

    a.addByte(toupper(drive));
    a.addByte(':');
    a.addByte(0);
    if (!sendCommand(FORMAT_OPEN, a)) {
        return RFSV::E_PSI_FILE_DISC;
    }
    if ((res = getResponse(a, true)) != RFSV::E_PSI_GEN_NONE) {
        return res;
    }
    if (a.getLen() != 4) {
        return RFSV::E_PSI_GEN_FAIL;
    }
    handle = a.getWord(0);
    count = a.getWord(2);
    return res;
}

Enum<RFSV::errs> RPCS::formatRead(int handle) {
    BufferStore a;

    a.addWord(handle);
    if (!sendCommand(FORMAT_READ, a)) {
        return RFSV::E_PSI_FILE_DISC;
    }
    return getResponse(a, true);
}

Enum<RFSV::errs> RPCS::getUniqueID(const char *device, long &id) {
    Enum<RFSV::errs> res;
    BufferStore a;

    a.addStringT(device);
    if (!sendCommand(GET_UNIQUEID, a)) {
        return RFSV::E_PSI_FILE_DISC;
    }
    if ((res = getResponse(a, true)) != RFSV::E_PSI_GEN_NONE) {
        return res;
    }
    if (a.getLen() != 4) {
        return RFSV::E_PSI_GEN_FAIL;
    }
    id = a.getDWord(0);
    return res;
}

Enum<RFSV::errs> RPCS::getMachineType(Enum<machs> &type) {
    Enum<RFSV::errs> res;
    BufferStore a;

    if (!sendCommand(GET_MACHINETYPE, a)) {
        return RFSV::E_PSI_FILE_DISC;
    }
    if ((res = getResponse(a, true)) != RFSV::E_PSI_GEN_NONE) {
        return res;
    }
    if (a.getLen() != 2) {
        return RFSV::E_PSI_GEN_FAIL;
    }
    type = (enum machs)a.getWord(0);
    mtCacheS5mx |= 4;
    if (res == RFSV::E_PSI_GEN_NONE) {
        if (type == RPCS::PSI_MACH_S5) {
            mtCacheS5mx |= 1;
        }
    }
    return res;
}

Enum<RFSV::errs> RPCS::fuser(const char *name, char *buf, int maxlen) {
    Enum<RFSV::errs> res;
    BufferStore a;
    char *p;

    a.addStringT(name);
    if (!sendCommand(FUSER, a)) {
        return RFSV::E_PSI_FILE_DISC;
    }
    if ((res = getResponse(a, true)) != RFSV::E_PSI_GEN_NONE) {
        return res;
    }
    strncpy(buf, a.getString(0), maxlen - 1);
    while ((p = strchr(buf, 6))) {
        *p = '\0';
    }
    return res;
}

Enum<RFSV::errs> RPCS::quitServer(void) {
    BufferStore a;
    if (!sendCommand(QUIT_SERVER, a)) {
        return RFSV::E_PSI_FILE_DISC;
    }
    return getResponse(a, true);
}
