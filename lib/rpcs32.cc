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

#include "rpcs32.h"
#include "bufferstore.h"
#include "bufferarray.h"
#include "ppsocket.h"

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

rpcs32::rpcs32(ppsocket * _skt)
{
    skt = _skt;
    mtCacheS5mx = 0;
    reset();
}

Enum<rfsv::errs> rpcs32::
getCmdLine(const char *process, string &ret)
{
    bufferStore a;
    Enum<rfsv::errs> res;

    a.addStringT(process);
    if (!sendCommand(rpcs::GET_CMDLINE, a))
        return rfsv::E_PSI_FILE_DISC;
    if ((res = getResponse(a, true)) == rfsv::E_PSI_GEN_NONE)
        ret = a.getString(0);
    return res;
}

Enum<rfsv::errs> rpcs32::
getMachineInfo(machineInfo &mi)
{
    bufferStore a;
    Enum<rfsv::errs> res;

    if (!sendCommand(rpcs::GET_MACHINE_INFO, a))
        return rfsv::E_PSI_FILE_DISC;
    if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
        return res;
    if (a.getLen() != 256)
        return rfsv::E_PSI_GEN_FAIL;
    mi.machineType = (enum rpcs::machs)a.getDWord(0);
    strncpy(mi.machineName, a.getString(16), 16);
    mi.machineName[16] = '\0';
    mi.machineUID = a.getDWord(44);
    mi.machineUID <<= 32;
    mi.machineUID |= a.getDWord(40);
    mi.countryCode = a.getDWord(56);
    mi.uiLanguage = (enum rpcs::languages)a.getDWord(164);

    mi.romMajor = a.getByte(4);
    mi.romMinor = a.getByte(5);
    mi.romBuild = a.getWord(6);
    mi.romSize = a.getDWord(140);

    mi.ramSize = a.getDWord(136);
    mi.ramMaxFree = a.getDWord(144);
    mi.ramFree = a.getDWord(148);
    mi.ramDiskSize = a.getDWord(152);

    mi.registrySize = a.getDWord(156);
    mi.romProgrammable = (a.getDWord(160) != 0);

    mi.displayWidth = a.getDWord(32);
    mi.displayHeight = a.getDWord(36);

    mi.time.tv_low = a.getDWord(48);
    mi.time.tv_high = a.getDWord(52);

    mi.tz.utc_offset = a.getSDWord(60);
    mi.tz.dst_zones = a.getDWord(64);
    mi.tz.home_zone = a.getDWord(68);

    PsiZone::getInstance().setZone(mi.tz);

    mi.mainBatteryInsertionTime.tv_low = a.getDWord(72);
    mi.mainBatteryInsertionTime.tv_high = a.getDWord(76);
    mi.mainBatteryStatus = (enum rpcs::batterystates)a.getDWord(80);
    mi.mainBatteryUsedTime.tv_low = a.getDWord(84);
    mi.mainBatteryUsedTime.tv_high = a.getDWord(88);
    mi.mainBatteryCurrent = a.getDWord(92);
    mi.mainBatteryUsedPower = a.getDWord(96);
    mi.mainBatteryVoltage = a.getDWord(100);
    mi.mainBatteryMaxVoltage = a.getDWord(104);

    mi.backupBatteryStatus = (enum rpcs::batterystates)a.getDWord(108);
    mi.backupBatteryVoltage = a.getDWord(112);
    mi.backupBatteryMaxVoltage = a.getDWord(116);
    mi.externalPowerUsedTime.tv_low = a.getDWord(124);
    mi.externalPowerUsedTime.tv_high = a.getDWord(128);
    mi.externalPower = (a.getDWord(120) != 0);

    mtCacheS5mx |= 8;
    if (res == rfsv::E_PSI_GEN_NONE) {
        if (!strcmp(mi.machineName, "SERIES5mx"))
            mtCacheS5mx |= 2;
    }

    return res;
}

Enum<rfsv::errs> rpcs32::
regOpenIter(uint32_t uid, char *match, uint16_t &handle)
{
    bufferStore a;
    Enum<rfsv::errs> res;

    cout << "Oiter" << endl;
    a.addDWord(uid);
    a.addDWord(strlen(match));
    a.addStringT(match);
    if (!sendCommand(rpcs::REG_OPEN_ITER, a))
        return rfsv::E_PSI_FILE_DISC;
    res = getResponse(a, true);
    cout << "ro: r=" << res << " a=" << a << endl;
    if (a.getLen() == 2)
        handle = a.getWord(0);
    return rfsv::E_PSI_GEN_NONE;
}

Enum<rfsv::errs> rpcs32::
regReadIter(uint16_t handle)
{
    bufferStore a;
    Enum<rfsv::errs> res;

    cout << "Riter" << endl;
    a.addWord(handle);
    if (!sendCommand(rpcs::REG_READ_ITER, a))
        return rfsv::E_PSI_FILE_DISC;
    res = getResponse(a, true);
    cout << "ro: r=" << res << " a=" << a << endl;
    if ((a.getLen() == 3) && (a.getByte(2) == 0xff))
        return rfsv::E_PSI_FILE_EOF;
    return rfsv::E_PSI_GEN_NONE;
}

Enum<rfsv::errs> rpcs32::
setTime(time_t time)
{
    bufferStore a;
    Enum<rfsv::errs> res;
    PsiTime pt;
    psi_timezone ptz;
    rpcs::machineInfo mi;

    // cout << "settime" << endl;
    if ((res = getMachineInfo(mi)) == rfsv::E_PSI_GEN_NONE) {
        if (PsiZone::getInstance().getZone(ptz)) {
            pt = PsiTime(time + ptz.utc_offset);
            a.addDWord(pt.getPsiTimeLo());
            a.addDWord(pt.getPsiTimeHi());
            a.addDWord(mi.countryCode);
            a.addDWord(ptz.utc_offset);
            a.addDWord(ptz.dst_zones);
            a.addDWord(ptz.home_zone);
            // cout << "a=" << a << endl;
            if (!sendCommand(rpcs::SET_TIME, a))
                return rfsv::E_PSI_FILE_DISC;
            return rfsv::E_PSI_GEN_NONE;
        } else
            return rfsv::E_PSI_GEN_FAIL;
    } else
        return res;
}

Enum<rfsv::errs> rpcs32::
configOpen(uint16_t &handle, uint32_t size)
{
    bufferStore a;
    Enum<rfsv::errs> res;

    a.addDWord(size);
    if (!sendCommand(rpcs::CONFIG_OPEN, a))
        return rfsv::E_PSI_FILE_DISC;
    res = getResponse(a, true);
    if (res == rfsv::E_PSI_GEN_NONE && (a.getLen() >= 2))
        handle = a.getWord(0);
    return res;
}

Enum<rfsv::errs> rpcs32::
configRead(uint32_t size, bufferStore &ret)
{
    bufferStore a;
    uint16_t handle;
    Enum<rfsv::errs> res;

    ret.init();
    if ((res = configOpen(handle, size)) != rfsv::E_PSI_GEN_NONE)
        return res;
    do {
        a.init();
        a.addWord(handle);
        a.addDWord(2047);
        if (!sendCommand(rpcs::CONFIG_READ, a))
            return rfsv::E_PSI_FILE_DISC;
        if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE) {
            closeHandle(handle);
            return res;
        }
        if (a.getLen() > 0)
            ret.addBuff(a);
    } while (a.getLen() > 0);
    return rfsv::E_PSI_GEN_NONE;
}

Enum<rfsv::errs> rpcs32::
configWrite(bufferStore data)
{
    bufferStore a;
    uint16_t handle;
    Enum<rfsv::errs> res;

    return rfsv::E_PSI_GEN_NONE;
    if ((res = configOpen(handle, data.getLen())) != rfsv::E_PSI_GEN_NONE)
        return res;
    do {
        a.init();
        long l = (data.getLen() > 2047) ? 2047 : data.getLen();
        a.addWord(handle);
        a.addBuff(data, l);
        data.discardFirstBytes(l);
        if (!sendCommand(rpcs::CONFIG_WRITE, a))
            return rfsv::E_PSI_FILE_DISC;
        if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE) {
            closeHandle(handle);
            return res;
        }
    } while (data.getLen() > 0);
    return rfsv::E_PSI_GEN_NONE;
}

Enum<rfsv::errs> rpcs32::
closeHandle(uint16_t handle)
{
    bufferStore a;

    a.addWord(handle);
    if (!sendCommand(rpcs::CLOSE_HANDLE, a))
        return rfsv::E_PSI_FILE_DISC;
    return getResponse(a, true);
}
