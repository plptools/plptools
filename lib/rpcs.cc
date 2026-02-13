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
#include "ppsocket.h"
#include "bufferarray.h"
#include "psiprocess.h"
#include "Enum.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

ENUM_DEFINITION_BEGIN(rpcs::machs, rpcs::PSI_MACH_UNKNOWN)
    stringRep.add(rpcs::PSI_MACH_UNKNOWN,   N_("Unknown device"));
    stringRep.add(rpcs::PSI_MACH_PC,        N_("PC"));
    stringRep.add(rpcs::PSI_MACH_MC,        N_("MC"));
    stringRep.add(rpcs::PSI_MACH_HC,        N_("HC"));
    stringRep.add(rpcs::PSI_MACH_S3,        N_("Series 3"));
    stringRep.add(rpcs::PSI_MACH_S3A,       N_("Series 3a, 3c or 3mx"));
    stringRep.add(rpcs::PSI_MACH_WORKABOUT, N_("Workabout"));
    stringRep.add(rpcs::PSI_MACH_SIENA,     N_("Siena"));
    stringRep.add(rpcs::PSI_MACH_S3C,       N_("Series 3c"));
    stringRep.add(rpcs::PSI_MACH_S5,        N_("Series 5"));
    stringRep.add(rpcs::PSI_MACH_WINC,      N_("WinC"));
ENUM_DEFINITION_END(rpcs::machs)

ENUM_DEFINITION_BEGIN(rpcs::batterystates, rpcs::PSI_BATT_DEAD)
    stringRep.add(rpcs::PSI_BATT_DEAD,    N_("Empty"));
    stringRep.add(rpcs::PSI_BATT_VERYLOW, N_("Very Low"));
    stringRep.add(rpcs::PSI_BATT_LOW,     N_("Low"));
    stringRep.add(rpcs::PSI_BATT_GOOD,    N_("Good"));
ENUM_DEFINITION_END(rpcs::batterystates)


ENUM_DEFINITION_BEGIN(rpcs::languages, rpcs::PSI_LANG_TEST)
    stringRep.add(rpcs::PSI_LANG_TEST,  N_("Test"));
    stringRep.add(rpcs::PSI_LANG_en_GB, N_("English"));
    stringRep.add(rpcs::PSI_LANG_de_DE, N_("German"));
    stringRep.add(rpcs::PSI_LANG_fr_FR, N_("French"));
    stringRep.add(rpcs::PSI_LANG_es_ES, N_("Spanish"));
    stringRep.add(rpcs::PSI_LANG_it_IT, N_("Italian"));
    stringRep.add(rpcs::PSI_LANG_sv_SE, N_("Swedish"));
    stringRep.add(rpcs::PSI_LANG_da_DK, N_("Danish"));
    stringRep.add(rpcs::PSI_LANG_no_NO, N_("Norwegian"));
    stringRep.add(rpcs::PSI_LANG_fi_FI, N_("Finnish"));
    stringRep.add(rpcs::PSI_LANG_en_US, N_("American"));
    stringRep.add(rpcs::PSI_LANG_fr_CH, N_("Swiss French"));
    stringRep.add(rpcs::PSI_LANG_de_CH, N_("Swiss German"));
    stringRep.add(rpcs::PSI_LANG_pt_PT, N_("Portugese"));
    stringRep.add(rpcs::PSI_LANG_tr_TR, N_("Turkish"));
    stringRep.add(rpcs::PSI_LANG_is_IS, N_("Icelandic"));
    stringRep.add(rpcs::PSI_LANG_ru_RU, N_("Russian"));
    stringRep.add(rpcs::PSI_LANG_hu_HU, N_("Hungarian"));
    stringRep.add(rpcs::PSI_LANG_nl_NL, N_("Dutch"));
    stringRep.add(rpcs::PSI_LANG_nl_BE, N_("Belgian Flemish"));
    stringRep.add(rpcs::PSI_LANG_en_AU, N_("Australian"));
    stringRep.add(rpcs::PSI_LANG_fr_BE, N_("Belgish French"));
    stringRep.add(rpcs::PSI_LANG_de_AT, N_("Austrian"));
    stringRep.add(rpcs::PSI_LANG_en_NZ, N_("New Zealand English"));
    stringRep.add(rpcs::PSI_LANG_fr_CA, N_("Canadian French"));
    stringRep.add(rpcs::PSI_LANG_cs_CZ, N_("Czech"));
    stringRep.add(rpcs::PSI_LANG_sk_SK, N_("Slovak"));
    stringRep.add(rpcs::PSI_LANG_pl_PL, N_("Polish"));
    stringRep.add(rpcs::PSI_LANG_sl_SI, N_("Slovenian"));
ENUM_DEFINITION_END(rpcs::languages)

rpcs::~rpcs()
{
    skt->closeSocket();
}

//
// public common API
//
void rpcs::
reconnect(void)
{
    skt->reconnect();
    reset();
}

void rpcs::
reset(void)
{
    bufferStore a;
    mtCacheS5mx = 0;
    status = rfsv::E_PSI_FILE_DISC;
    a.addStringT(getConnectName());
    if (skt->sendBufferStore(a)) {
        if (skt->getBufferStore(a) == 1) {
            if (!strcmp(a.getString(0), "Ok"))
                status = rfsv::E_PSI_GEN_NONE;
        }
    }
}

Enum<rfsv::errs> rpcs::
getStatus(void)
{
    return status;
}

const char *rpcs::
getConnectName(void)
{
    return "SYS$RPCS";
}

//
// protected internals
//
bool rpcs::
sendCommand(enum commands cc, bufferStore & data)
{
    if (status == rfsv::E_PSI_FILE_DISC) {
        reconnect();
        if (status == rfsv::E_PSI_FILE_DISC)
            return false;
    }
    bool result;
    bufferStore a;
    a.addByte(cc);
    a.addBuff(data);
    result = skt->sendBufferStore(a);
    if (!result) {
        reconnect();
        result = skt->sendBufferStore(a);
        if (!result)
            status = rfsv::E_PSI_FILE_DISC;
    }
    return result;
}

Enum<rfsv::errs> rpcs::
getResponse(bufferStore & data, bool statusIsFirstByte)
{
    Enum<rfsv::errs> ret;
    if (skt->getBufferStore(data) == 1) {
        if (statusIsFirstByte) {
            ret = (enum rfsv::errs)((char)data.getByte(0));
            data.discardFirstBytes(1);
        } else {
            int l = data.getLen();
            if (l > 0) {
                ret = (enum rfsv::errs)((char)data.getByte(data.getLen() - 1));
                data.init((const unsigned char *)data.getString(), l - 1);
            } else
                ret = rfsv::E_PSI_GEN_FAIL;
        }
        return ret;
    } else
        status = rfsv::E_PSI_FILE_DISC;
    return status;
}

//
// APIs, identical on SIBO and EPOC
//
Enum<rfsv::errs> rpcs::
getNCPversion(int &major, int &minor)
{
    Enum<rfsv::errs> res;
    bufferStore a;

    if (!sendCommand(QUERY_NCP, a))
        return rfsv::E_PSI_FILE_DISC;
    if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
        return res;
    if (a.getLen() != 2)
        return rfsv::E_PSI_GEN_FAIL;
    major = a.getByte(0);
    minor = a.getByte(1);
    return res;
}

Enum<rfsv::errs> rpcs::
execProgram(const char *program, const char *args)
{
    bufferStore a;

    a.addStringT(program);
    int l = strlen(program);
    for (int i = 127; i > l; i--)
        a.addByte(0);

    /**
    * This is a hack for the jotter app on mx5 pro. (and probably others)
    * Jotter seems to read its arguments one char past normal apps.
    * Without this hack, The Drive-Character gets lost. Other apps don't
    * seem to be hurt by the additional blank.
    */
    a.addByte(strlen(args)+1);
    a.addByte(' ');

    a.addStringT(args);

    if (!sendCommand(EXEC_PROG, a))
        return rfsv::E_PSI_FILE_DISC;
    return getResponse(a, true);
}

Enum<rfsv::errs> rpcs::
stopProgram(const char *program)
{
    bufferStore a;

    a.addStringT(program);
    if (!sendCommand(STOP_PROG, a))
        return rfsv::E_PSI_FILE_DISC;
    return getResponse(a, true);
}

Enum<rfsv::errs> rpcs::
queryProgram(const char *program)
{
    bufferStore a;

    a.addStringT(program);
    if (!sendCommand(QUERY_PROG, a))
        return rfsv::E_PSI_FILE_DISC;
    return getResponse(a, true);
}

Enum<rfsv::errs> rpcs::
queryPrograms(processList &ret)
{
    bufferStore a;
    const char *drives;
    const char *dptr;
    bool anySuccess = false;
    Enum<rfsv::errs> res;

    // First, check how many drives we need to query
    a.addStringT("M:"); // Drive M only exists on a SIBO
    if (!sendCommand(rpcs::GET_UNIQUEID, a))
        return rfsv::E_PSI_FILE_DISC;
    if (getResponse(a, false) == rfsv::E_PSI_GEN_NONE)
        // A SIBO; Must query all possible drives
        drives = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    else
        // A Series 5; Query of C is sufficient
        drives = "C";

    dptr = drives;
    ret.clear();

    if ((mtCacheS5mx & 4) == 0) {
        Enum<machs> tmp;
        if (getMachineType(tmp) != rfsv::E_PSI_GEN_NONE)
            return rfsv::E_PSI_GEN_FAIL;

    }
    if ((mtCacheS5mx & 9) == 1) {
        machineInfo tmp;
        if (getMachineInfo(tmp) == rfsv::E_PSI_FILE_DISC)
            return rfsv::E_PSI_FILE_DISC;
    }
    bool s5mx = (mtCacheS5mx == 15);
    while (*dptr) {
        a.init();
        a.addByte(*dptr);
        if (!sendCommand(rpcs::QUERY_DRIVE, a))
            return rfsv::E_PSI_FILE_DISC;
        if (getResponse(a, false) == rfsv::E_PSI_GEN_NONE) {
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
                } else
                    pid = 0;
                PsiProcess proc(pid, s, a.getString(0), s5mx);
                ret.push_back(proc);
                sl = strlen(a.getString(0)) + 1;
                l -= sl;
                a.discardFirstBytes(sl);
            }
        }
        dptr++;
    }
    if (anySuccess && !ret.empty())
        for (processList::iterator i = ret.begin(); i != ret.end(); i++) {
            string cmdline;
            if (getCmdLine(i->getProcId(), cmdline) == rfsv::E_PSI_GEN_NONE)
                i->setArgs(cmdline + " " + i->getArgs());
        }
    return anySuccess ? rfsv::E_PSI_GEN_NONE : rfsv::E_PSI_GEN_FAIL;
}

Enum<rfsv::errs> rpcs::
formatOpen(const char drive, int &handle, int &count)
{
    Enum<rfsv::errs> res;
    bufferStore a;

    a.addByte(toupper(drive));
    a.addByte(':');
    a.addByte(0);
    if (!sendCommand(FORMAT_OPEN, a))
        return rfsv::E_PSI_FILE_DISC;
    if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
        return res;
    if (a.getLen() != 4)
        return rfsv::E_PSI_GEN_FAIL;
    handle = a.getWord(0);
    count = a.getWord(2);
    return res;
}

Enum<rfsv::errs> rpcs::
formatRead(int handle)
{
    bufferStore a;

    a.addWord(handle);
    if (!sendCommand(FORMAT_READ, a))
        return rfsv::E_PSI_FILE_DISC;
    return getResponse(a, true);
}

Enum<rfsv::errs> rpcs::
getUniqueID(const char *device, long &id)
{
    Enum<rfsv::errs> res;
    bufferStore a;

    a.addStringT(device);
    if (!sendCommand(GET_UNIQUEID, a))
        return rfsv::E_PSI_FILE_DISC;
    if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
        return res;
    if (a.getLen() != 4)
        return rfsv::E_PSI_GEN_FAIL;
    id = a.getDWord(0);
    return res;
}

Enum<rfsv::errs> rpcs::
getOwnerInfo(bufferArray &owner)
{
    Enum<rfsv::errs> res;
    bufferStore a;

    if (!sendCommand(GET_OWNERINFO, a))
        return rfsv::E_PSI_FILE_DISC;
    if ((res = (enum rfsv::errs)getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
        return res;
    a.addByte(0);
    string s = a.getString(0);
    owner.clear();
    int p = 0;
    int l;
    while ((l = s.find('\006', p)) != s.npos) {
        bufferStore b;
        b.addStringT(s.substr(p, l - p).c_str());
        owner += b;
        p = l + 1;
    }
    if (s.substr(p).length()) {
        bufferStore b;
        b.addStringT(s.substr(p).c_str());
        owner += b;
    }
    return res;
}

Enum<rfsv::errs> rpcs::
getMachineType(Enum<machs> &type)
{
    Enum<rfsv::errs> res;
    bufferStore a;

    if (!sendCommand(GET_MACHINETYPE, a))
        return rfsv::E_PSI_FILE_DISC;
    if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
        return res;
    if (a.getLen() != 2)
        return rfsv::E_PSI_GEN_FAIL;
    type = (enum machs)a.getWord(0);
    mtCacheS5mx |= 4;
    if (res == rfsv::E_PSI_GEN_NONE) {
        if (type == rpcs::PSI_MACH_S5)
            mtCacheS5mx |= 1;
    }
    return res;
}

Enum<rfsv::errs> rpcs::
fuser(const char *name, char *buf, int maxlen)
{
    Enum<rfsv::errs> res;
    bufferStore a;
    char *p;

    a.addStringT(name);
    if (!sendCommand(FUSER, a))
        return rfsv::E_PSI_FILE_DISC;
    if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
        return res;
    strncpy(buf, a.getString(0), maxlen - 1);
    while ((p = strchr(buf, 6)))
        *p = '\0';
    return res;
}

Enum<rfsv::errs> rpcs::
quitServer(void)
{
    bufferStore a;
    if (!sendCommand(QUIT_SERVER, a))
        return rfsv::E_PSI_FILE_DISC;
    return getResponse(a, true);
}
