/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999 Matt J. Gumbley <matt@gumbley.demon.co.uk>
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
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

#include "rfsv16.h"
#include "bufferstore.h"
#include "ppsocket.h"
#include "bufferarray.h"

#include <iostream>
#include <fstream>

#include <stdlib.h>
#include <time.h>

#include "ignore-value.h"

#define RFSV16_MAXDATALEN 852 // 640

using namespace std;

rfsv16::rfsv16(ppsocket *_skt)
{
    serNum = 0;
    status = rfsv::E_PSI_FILE_DISC;
    skt = _skt;
    reset();
}

Enum<rfsv::errs> rfsv16::
fopen(uint32_t attr, const char *name, uint32_t &handle)
{
    bufferStore a;
    string realName = convertSlash(name);

    // Allow random access, rather than forcing the caller to ask for it
    a.addWord((P_FRANDOM | attr) & 0xFFFF);
    a.addStringT(realName.c_str());
    if (!sendCommand(SIBO_FOPEN, a))
        return E_PSI_FILE_DISC;

    Enum<rfsv::errs> res = getResponse(a);
    if (res == 0) {
        handle = (long)a.getWord(0);
        return E_PSI_GEN_NONE;
    }
    return res;
}

Enum<rfsv::errs> rfsv16::
mktemp(uint32_t &handle, string &tmpname)
{
    bufferStore a;

    a.addWord(P_FUNIQUE);
    a.addStringT("TMP");
    if (!sendCommand(SIBO_OPENUNIQUE, a))
        return E_PSI_FILE_DISC;

    Enum<rfsv::errs> res = getResponse(a);
    if (res == E_PSI_GEN_NONE) {
        handle = a.getWord(0);
        tmpname = a.getString(2);
        return res;
    }
    return res;
}

Enum<rfsv::errs> rfsv16::
fcreatefile(uint32_t attr, const char *name, uint32_t &handle)
{
    return fopen(attr | P_FCREATE, name, handle);
}

Enum<rfsv::errs> rfsv16::
freplacefile(uint32_t attr, const char *name, uint32_t &handle)
{
    return fopen(attr | P_FREPLACE, name, handle);
}

Enum<rfsv::errs> rfsv16::
fopendir(const char * const name, uint32_t &handle)
{
    return fopen(P_FDIR, name, handle);
}

Enum<rfsv::errs> rfsv16::
fclose(uint32_t fileHandle)
{
    bufferStore a;
    a.addWord(fileHandle & 0xFFFF);
    if (!sendCommand(SIBO_FCLOSE, a))
        return E_PSI_FILE_DISC;
    return getResponse(a);
}

Enum<rfsv::errs> rfsv16::
opendir(const uint32_t attr, const char *name, rfsvDirhandle &dH) {
    uint32_t handle;
    Enum<rfsv::errs> res = fopendir(name, handle);
    dH.h = handle;
    dH.b.init();
    return res;
}

Enum<rfsv::errs> rfsv16::
closedir(rfsvDirhandle &dH) {
    return fclose(dH.h);
}

Enum<rfsv::errs> rfsv16::
readdir(rfsvDirhandle &dH, PlpDirent &e) {
    Enum<rfsv::errs> res = E_PSI_GEN_NONE;

    if (dH.b.getLen() < 17) {
        dH.b.init();
        dH.b.addWord(dH.h & 0xFFFF);
        if (!sendCommand(SIBO_FDIRREAD, dH.b))
            return E_PSI_FILE_DISC;
        res = getResponse(dH.b);
        if (res == E_PSI_GEN_NONE) {
            uint16_t bufferLen = dH.b.getWord(0);
            dH.b.discardFirstBytes(2);
            if (dH.b.getLen() != bufferLen)
                return E_PSI_GEN_FAIL;
        }
    }
    if ((res == E_PSI_GEN_NONE) && (dH.b.getLen() > 16)) {
        uint16_t version = dH.b.getWord(0);
        if (version != 2)
            return E_PSI_GEN_FAIL;
        e.attr    = attr2std((uint32_t)dH.b.getWord(2));
        e.size    = dH.b.getDWord(4);
        e.time.setSiboTime(dH.b.getDWord(8));
        e.name    = dH.b.getString(16);
        //e.UID     = PlpUID(0,0,0);
        e.attrstr = attr2String(e.attr);

        dH.b.discardFirstBytes(17 + e.name.length());

    }
    return res;
}

Enum<rfsv::errs> rfsv16::
dir(const char *name, PlpDir &files)
{
    rfsvDirhandle h;
    files.clear();
    Enum<rfsv::errs> res = opendir(PSI_A_HIDDEN|PSI_A_SYSTEM|PSI_A_DIR, name, h);
    while (res == E_PSI_GEN_NONE) {
        PlpDirent e;
        res = readdir(h, e);
        if (res == E_PSI_GEN_NONE)
            files.push_back(e);
    }
    closedir(h);
    if (res == E_PSI_FILE_EOF)
        res = E_PSI_GEN_NONE;
    return res;
}

uint32_t rfsv16::
opMode(uint32_t mode)
{
    uint32_t ret = 0;

    ret |= ((mode & 03) == PSI_O_RDONLY) ? 0 : P_FUPDATE;
    ret |= (mode & PSI_O_TRUNC) ? P_FREPLACE : 0;
    ret |= (mode & PSI_O_CREAT) ? P_FCREATE : 0;
    ret |= (mode & PSI_O_APPEND) ? P_FAPPEND : 0;
    if ((mode & 03) == PSI_O_RDONLY)
        ret |= (mode & PSI_O_EXCL) ? 0 : P_FSHARE;
    return ret;
}

Enum<rfsv::errs> rfsv16::
fgetmtime(const char * const name, PsiTime &mtime)
{
    bufferStore a;
    string realName = convertSlash(name);
    a.addStringT(realName.c_str());
    if (!sendCommand(SIBO_FINFO, a))
        return E_PSI_FILE_DISC;

    Enum<rfsv::errs> res = getResponse(a);
    if (res != E_PSI_GEN_NONE)
        return res;
    else if (a.getLen() == 16) {
        // According to Alex's docs, Psion's file times are in
        // seconds since 13:00!!, 1.1.1970
        mtime.setSiboTime(a.getDWord(8));
        return res;
    }
    return E_PSI_GEN_FAIL;
}

Enum<rfsv::errs> rfsv16::
fsetmtime(const char *name, PsiTime mtime)
{
    // According to Alexander's protocol doc, SIBO_SFDATE sets the modification
    // time - and as far as I can see SIBO only keeps a modification
    // time. So call SIBO_SFDATE here.
    bufferStore a;
    string realName = convertSlash(name);
    // According to Alex's docs, Psion's file times are in
    // seconds since 13:00!!, 1.1.1970
    a.addDWord(mtime.getSiboTime());
    a.addStringT(realName.c_str());
    if (!sendCommand(SIBO_SFDATE, a))
        return E_PSI_FILE_DISC;
    return getResponse(a);
}

Enum<rfsv::errs> rfsv16::
fgetattr(const char * const name, uint32_t &attr)
{
    bufferStore a;
    string realName = convertSlash(name);
    a.addStringT(realName.c_str());
    if (!sendCommand(SIBO_FINFO, a))
        return E_PSI_FILE_DISC;

    Enum<rfsv::errs> res = getResponse(a);
    if (res != E_PSI_GEN_NONE)
        return res;
    else if (a.getLen() == 16) {
        attr = attr2std((long)a.getWord(2));
        return res;
    }
    return E_PSI_GEN_FAIL;
}

Enum<rfsv::errs> rfsv16::
fgeteattr(const char * const name, PlpDirent &e)
{
    bufferStore a;
    string realName = convertSlash(name);
    a.addStringT(realName.c_str());
    if (!sendCommand(SIBO_FINFO, a))
        return E_PSI_FILE_DISC;
    Enum<rfsv::errs> res = getResponse(a);
    if (res != E_PSI_GEN_NONE)
        return res;
    else if (a.getLen() == 16) {
        const char *p = strrchr(realName.c_str(), '\\');
        if (p)
            p++;
        else
            p = realName.c_str();
        e.name = p;
        e.attr = attr2std((long)a.getWord(2));
        e.size = a.getDWord(4);
        e.time.setSiboTime(a.getDWord(8));
        e.UID  = PlpUID(0,0,0);
        e.attrstr = string(attr2String(e.attr));
        return res;
    }
    return E_PSI_GEN_FAIL;
}

Enum<rfsv::errs> rfsv16::
fsetattr(const char *name, uint32_t seta, uint32_t unseta)
{
    uint32_t statusword = std2attr(seta) & (~ std2attr(unseta));
    statusword ^= 0x0000001; // r bit is inverted
    uint32_t bitmask = std2attr(seta) | std2attr(unseta);
    bufferStore a;
    a.addWord(statusword & 0xFFFF);
    a.addWord(bitmask & 0xFFFF);
    a.addStringT(name);
    if (!sendCommand(SIBO_SFSTAT, a))
        return E_PSI_FILE_DISC;
    return getResponse(a);
}

Enum<rfsv::errs> rfsv16::
dircount(const char * const name, uint32_t &count)
{
    rfsvDirhandle h;
    Enum<rfsv::errs> res = opendir(PSI_A_HIDDEN|PSI_A_SYSTEM|PSI_A_DIR, name, h);
    while (res == E_PSI_GEN_NONE) {
        PlpDirent e;
        res = readdir(h, e);
        if (res == E_PSI_GEN_NONE)
            count++;
    }
    closedir(h);
    if (res == E_PSI_FILE_EOF)
        res = E_PSI_GEN_NONE;
    return res;
}

Enum<rfsv::errs> rfsv16::
devlist(uint32_t &devbits)
{
    Enum<rfsv::errs> res;
    uint32_t fileHandle;
    devbits = 0;

    // The following is taken from a trace between a Series 3c and PsiWin.
    // Hope it works! We SIBO_PARSE to find the correct node, then FOPEN
    // (P_FDEVICE) this, SIBO_FDEVICEREAD each entry, setting the appropriate
    // drive-letter-bit in devbits, then FCLOSE.

    bufferStore a;
    a.init();
    a.addByte(0x00); // no Name 1
    a.addByte(0x00); // no Name 2
    a.addByte(0x00); // no Name 3
    if (!sendCommand(SIBO_PARSE, a))
        return E_PSI_FILE_DISC;
    res = getResponse(a);
    if (res != E_PSI_GEN_NONE)
        return res;

    // Find the drive to FOPEN
    char name[4] = { 'x', ':', '\\', '\0' } ;
    a.discardFirstBytes(6); // Result, fsys, dev, path, file, file, ending, flag
    /* This leaves R E M : : M : \ */
    name[0] = (char) a.getByte(5); // the M
    res = fopen(P_FDEVICE, name, fileHandle);
    if (res != E_PSI_GEN_NONE)
        return status;

    while (1) {
        a.init();
        a.addWord(fileHandle & 0xFFFF);
        if (!sendCommand(SIBO_FDEVICEREAD, a))
            return E_PSI_FILE_DISC;
        res = getResponse(a);
        if (res)
            break;
        uint16_t version = a.getWord(0);
        if ((version < 1) || (version > 2)) {
            cerr << "devlist: not version 1 or 2" << endl;
            fclose(fileHandle);
            return E_PSI_GEN_FAIL;
        }
        char drive = a.getByte(64);
        if (drive >= 'A' && drive <= 'Z') {
            int shift = (drive - 'A');
            devbits |= (long) ( 1 << shift );
        }
        else {
            cerr << "devlist: non-alphabetic drive letter ("
                 << drive << ")" << endl;
        }
    }
    if (res == E_PSI_FILE_EOF)
        res = E_PSI_GEN_NONE;
    fclose(fileHandle);
    return res;
}

static int sibo_dattr[] = {
    1, // Unknown
    2, // Floppy
    3, // Disk
    6, // Flash
    5, // RAM
    7, // ROM
    7, // write-protected == ROM ?
};

Enum<rfsv::errs> rfsv16::
devinfo(const char drive, PlpDrive &dinfo)
{
    bufferStore a;
    Enum<rfsv::errs> res;

    // Again, this is taken from an exchange between PsiWin and a 3c.
    // For each drive, we SIBO_PARSE with its drive letter to get a response
    // (which we ignore), then do a SIBO_STATUSDEVICE to get the info.

    a.init();
    a.addByte(toupper(drive)); // Name 1
    a.addByte(':');
    a.addByte(0x00);

    a.addByte(0x00); // No name 2
    a.addByte(0x00); // No name 3
    if (!sendCommand(SIBO_PARSE, a))
        return E_PSI_FILE_DISC;
    if ((res = getResponse(a)) != E_PSI_GEN_NONE)
        return res;

    a.init();
    a.addByte(toupper(drive)); // Name 1
    a.addByte(':');
    a.addByte('\\');
    a.addByte(0x00);
    if (!sendCommand(SIBO_STATUSDEVICE, a))
        return E_PSI_FILE_DISC;
    if ((res = getResponse(a)) != E_PSI_GEN_NONE)
        return res;

    int attr = a.getWord(2);
    attr = sibo_dattr[a.getWord(2) & 0xff];
    dinfo.setMediaType(attr);

    attr = a.getWord(2);
    int changeable = a.getWord(4) ? 32 : 0;
    int internal = (attr & 0x2000) ? 16 : 0;

    dinfo.setDriveAttribute(changeable | internal);

    int variable = (attr & 0x4000) ? 1 : 0;
    int dualdens = (attr & 0x1000) ? 2 : 0;
    int formattable = (attr & 0x0800) ? 4 : 0;
    int protect = ((attr & 0xff) == 6) ? 8 : 0;

    dinfo.setMediaAttribute(variable|dualdens|formattable|protect);

    dinfo.setUID(0);
    dinfo.setSize(a.getDWord(6), 0);
    dinfo.setSpace(a.getDWord(10), 0);

    dinfo.setName(toupper(drive), a.getString(14));


    return res;
}

bool rfsv16::
sendCommand(enum commands cc, bufferStore & data)
{
    if (status == E_PSI_FILE_DISC) {
        reconnect();
        if (status == E_PSI_FILE_DISC)
            return false;
    }

    bool result;
    bufferStore a;
    a.addWord(cc);
    a.addWord(data.getLen());
    a.addBuff(data);
    result = skt->sendBufferStore(a);
    if (!result) {
        reconnect();
        result = skt->sendBufferStore(a);
        if (!result)
            status = E_PSI_FILE_DISC;
    }
    return result;
}


Enum<rfsv::errs> rfsv16::
getResponse(bufferStore & data)
{
    // getWord(2) is the size field
    // which is the body of the response not counting the command (002a) and
    // the size word.
    if (skt->getBufferStore(data) != 1) {
        cerr << "rfsv16::getResponse: duff response. "
            "getBufferStore failed." << endl;
    } else if (data.getWord(0) == 0x2a &&
               data.getWord(2) == data.getLen()-4) {
        Enum<rfsv::errs> ret = (enum errs)(int16_t)data.getWord(4);
        data.discardFirstBytes(6);
        return ret;
    } else {
        cerr << "rfsv16::getResponse: duff response. Size field:" <<
            data.getWord(2) << " Frame size:" <<
            data.getLen()-4 << " Result field:" <<
            data.getWord(4) << endl;
    }
    status = E_PSI_FILE_DISC;
    return status;
}

Enum<rfsv::errs> rfsv16::
fread(const uint32_t handle, unsigned char * const buf, const uint32_t len, uint32_t &count)
{
    Enum<rfsv::errs> res;
    unsigned char *p = buf;
    long l;

    count = 0;
    do {
        bufferStore a;

        // Read in blocks of 291 bytes; the maximum payload for
        // an RFSV frame. ( As seen in traces ) - this isn't optimal:
        // RFSV can handle fragmentation of frames, where only the
        // first SIBO_FREAD RESPONSE frame has a RESPONSE (00 2A), SIZE
        // and RESULT field. Every subsequent frame
        // just has data, 297 bytes (or less) of it.
        //
        a.addWord(handle);
        a.addWord((len - count) > RFSV16_MAXDATALEN
                  ? RFSV16_MAXDATALEN
                  : (len - count));
        if (!sendCommand(SIBO_FREAD, a))
            return E_PSI_FILE_DISC;
        if ((res = getResponse(a)) != E_PSI_GEN_NONE) {
            if (res == E_PSI_FILE_EOF)
                return E_PSI_GEN_NONE;
            return res;
        }
        if ((l = a.getLen()) > 0) {
            memcpy(p, a.getString(), l);
            count += l;
            p += l;
        }
    } while ((count < len) && (l > 0));
    return res;
}

Enum<rfsv::errs> rfsv16::
fwrite(const uint32_t handle, const unsigned char * const buf, const uint32_t len, uint32_t &count)
{
    Enum<rfsv::errs> res;
    const unsigned char *p = buf;

    count = 0;
    while (count < len) {
        bufferStore a;
        int nbytes;

        // Write in blocks of 291 bytes; the maximum payload for
        // an RFSV frame. ( As seen in traces ) - this isn't optimal:
        // RFSV can handle fragmentation of frames, where only the
        // first SIBO_FREAD RESPONSE frame has a RESPONSE (00 2A), SIZE
        // and RESULT field. Every subsequent frame
        // just has data, 297 bytes (or less) of it.
        nbytes = (len - count) > RFSV16_MAXDATALEN
            ? RFSV16_MAXDATALEN
            : (len - count);
        a.addWord(handle);
        a.addBytes(p, nbytes);
        if (!sendCommand(SIBO_FWRITE, a))
            return E_PSI_FILE_DISC;
        if ((res = getResponse(a)) != E_PSI_GEN_NONE)
            return res;

        count += nbytes;
        p += nbytes;
    }
    return res;
}

Enum<rfsv::errs> rfsv16::
copyFromPsion(const char *from, const char *to, void *ptr, cpCallback_t cb)
{
    Enum<rfsv::errs> res;
    uint32_t handle;
    uint32_t len;
    uint32_t total = 0;

    if ((res = fopen(P_FSHARE | P_FSTREAM, from, handle)) != E_PSI_GEN_NONE)
        return res;
    ofstream op(to);
    if (!op) {
        fclose(handle);
        return E_PSI_GEN_FAIL;
    }
    do {
        unsigned char buf[RFSV_SENDLEN];
        if ((res = fread(handle, buf, sizeof(buf), len)) == E_PSI_GEN_NONE) {
            if (len > 0)
                op.write((char *)buf, len);
            total += len;
            if (cb && !cb(ptr, total))
                res = E_PSI_FILE_CANCEL;
        }
    } while (len > 0 && (res == E_PSI_GEN_NONE));

    fclose(handle);
    op.close();
    if (res == E_PSI_FILE_EOF)
        res = E_PSI_GEN_NONE;
    return res;
}

Enum<rfsv::errs> rfsv16::
copyFromPsion(const char *from, int fd, cpCallback_t cb)
{
    Enum<rfsv::errs> res;
    uint32_t handle;
    uint32_t len;
    uint32_t total = 0;

    if ((res = fopen(P_FSHARE | P_FSTREAM, from, handle)) != E_PSI_GEN_NONE)
        return res;
    do {
        unsigned char buf[RFSV_SENDLEN];
        if ((res = fread(handle, buf, sizeof(buf), len)) == E_PSI_GEN_NONE) {
            if (len > 0)
                // FIXME: return UNIX errors from this method.
                ignore_value(write(fd, buf, len));
            total += len;
            if (cb && !cb(NULL, total))
                res = E_PSI_FILE_CANCEL;
        }
    } while (len > 0 && (res == E_PSI_GEN_NONE));

    fclose(handle);
    if (res == E_PSI_FILE_EOF)
        res = E_PSI_GEN_NONE;
    return res;
}

Enum<rfsv::errs> rfsv16::
copyToPsion(const char *from, const char *to, void *ptr, cpCallback_t cb)
{
    uint32_t handle;
    uint32_t len = 0;
    uint32_t total = 0;
    Enum<rfsv::errs> res;

    ifstream ip(from);
    if (!ip)
        return E_PSI_FILE_NXIST;
    res = fcreatefile(P_FSTREAM | P_FUPDATE, to, handle);
    if (res != E_PSI_GEN_NONE) {
        res = freplacefile(P_FSTREAM | P_FUPDATE, to, handle);
        if (res != E_PSI_GEN_NONE)
            return res;
    }
    unsigned char *buff = new unsigned char[RFSV_SENDLEN];
    while (res == E_PSI_GEN_NONE && ip && !ip.eof()) {
        ip.read((char *)buff, RFSV_SENDLEN);
        if ((res = fwrite(handle, buff, ip.gcount(), len)) == E_PSI_GEN_NONE) {
            total += len;
            if (cb && !cb(ptr, total))
                res = E_PSI_FILE_CANCEL;
        }
    }
    delete[]buff;
    fclose(handle);
    ip.close();
    return res;
}

Enum<rfsv::errs> rfsv16::
copyOnPsion(const char *from, const char *to, void *ptr, cpCallback_t cb)
{
    uint32_t handle_from;
    uint32_t handle_to;
    uint32_t len;
    uint32_t wlen;
    uint32_t total = 0;
    Enum<rfsv::errs> res;

    if ((res = fopen(P_FSHARE | P_FSTREAM, from, handle_from)) != E_PSI_GEN_NONE)
        return res;
    res = fcreatefile(P_FSTREAM | P_FUPDATE, to, handle_to);
    if (res != E_PSI_GEN_NONE) {
        res = freplacefile(P_FSTREAM | P_FUPDATE, to, handle_to);
        if (res != E_PSI_GEN_NONE)
            return res;
    }
    do {
        unsigned char buf[RFSV_SENDLEN];
        if ((res = fread(handle_from, buf, sizeof(buf), len)) == E_PSI_GEN_NONE) {
            if (len > 0) {
                if ((res = fwrite(handle_to, buf, len, wlen)) == E_PSI_GEN_NONE) {
                    total += wlen;
                    if (cb && !cb(ptr, total))
                        res = E_PSI_FILE_CANCEL;
                }
            }
        }
    } while (len > 0 && wlen > 0 && (res == E_PSI_GEN_NONE));
    fclose(handle_from);
    fclose(handle_to);
    if (res == E_PSI_FILE_EOF)
        res = E_PSI_GEN_NONE;
    return res;
}

Enum<rfsv::errs> rfsv16::
pathtest(const char * const path)
{
    string realName = convertSlash(path);
    bufferStore a;
    a.addStringT(realName.c_str());
    sendCommand(SIBO_PATHTEST, a);
    return getResponse(a);
}

Enum<rfsv::errs> rfsv16::
fsetsize(uint32_t handle, uint32_t size)
{
    bufferStore a;
    a.addWord(handle & 0xffff);
    a.addDWord(size);
    if (!sendCommand(SIBO_FSETEOF, a))
        return E_PSI_FILE_DISC;
    return getResponse(a);
}

/*
 * Unix-like implementation off fseek with one
 * exception: If seeking beyond eof, the gap
 * contains garbage instead of zeroes.
 */
Enum<rfsv::errs> rfsv16::
fseek(const uint32_t handle, const int32_t pos, const uint32_t mode, uint32_t &resultpos)
{
    bufferStore a;
    Enum<rfsv::errs> res;
    uint32_t savpos = 0;
    uint32_t realpos;
    uint32_t calcpos = 0;

/*
  seek-parameter for psion:
   dword position
   dword handle
   dword mode
   1 = from start
   2 = from current pos
   3 = from end
   ??no more?? 4 = sense recpos
   ??no more?? 5 = set recpos
   ??no more?? 6 = text-rewind
*/

    if ((mode < PSI_SEEK_SET) || (mode > PSI_SEEK_END))
        return E_PSI_GEN_ARG;

    if ((mode == PSI_SEEK_CUR) && (pos >= 0)) {
        /* get and save current position */
        a.init();
        a.addWord(handle);
        a.addDWord(0);
        a.addWord(PSI_SEEK_CUR);
        if (!sendCommand(SIBO_FSEEK, a))
            return E_PSI_FILE_DISC;
        if ((res = getResponse(a)) != E_PSI_GEN_NONE)
            return res;
        savpos = a.getDWord(0);
        if (pos == 0) {
            resultpos = savpos;
            return res;
        }
    }
    if ((mode == PSI_SEEK_END) && (pos >= 0)) {
        /* get and save end position */
        a.init();
        a.addWord(handle);
        a.addDWord(0);
        a.addWord(PSI_SEEK_END);
        if (!sendCommand(SIBO_FSEEK, a))
            return E_PSI_FILE_DISC;
        if ((res = getResponse(a)) != E_PSI_GEN_NONE)
            return res;
        savpos = a.getDWord(0);
        if (pos == 0) {
            resultpos = savpos;
            return res;
        }
    }
    /* Now the real seek */
    a.addWord(handle);
    a.addDWord(pos);
    a.addWord(mode);
    if (!sendCommand(SIBO_FSEEK, a))
        return E_PSI_FILE_DISC;
    if ((res = getResponse(a)) != 0)
        return res;
    realpos = a.getDWord(0);
    switch (mode) {
        case PSI_SEEK_SET:
            calcpos = pos;
            break;
        case PSI_SEEK_CUR:
            calcpos = savpos + pos;
            break;
        case PSI_SEEK_END:
            resultpos = realpos;
            return res;
            break;
    }
    if (calcpos > realpos) {
        /* Beyond end of file */
        res = fsetsize(handle, calcpos);
        if (res != E_PSI_GEN_NONE)
            return res;
        a.init();
        a.addWord(handle);
        a.addDWord(calcpos);
        a.addWord(PSI_SEEK_SET);
        if (!sendCommand(SIBO_FSEEK, a))
            return E_PSI_FILE_DISC;
        if ((res = getResponse(a)) != 0)
            return res;
        realpos = a.getDWord(0);
    }
    resultpos = realpos;
    return res;
}

Enum<rfsv::errs> rfsv16::
mkdir(const char* dirName)
{
    string realName = convertSlash(dirName);
    bufferStore a;
    a.addStringT(realName.c_str());
    sendCommand(SIBO_MKDIR, a);
    return getResponse(a);
}

Enum<rfsv::errs> rfsv16::
rmdir(const char *dirName)
{
    // EPOC16 doesn't provide an RMDIR command, so we fake it using DELETE (and a few little tweaks).

    // Rudimentary argument checking.
    string path(dirName);
    if (path.empty()) {
        return E_PSI_FILE_NAME;
    }

    // The EPOC32 implementation of RMDIR allows for deletion of paths with or without a trailing forward slash.
    // However, EPOC16's DELETE (which we use here) doesn't like these, so we strip them to ensure parity between the
    // two implementations.
    if (path.back() == '\\') {
        path.pop_back();
    }

    // Get the path's attributes and ensure it's a directory.
    uint32_t attr;
    Enum<rfsv::errs> res = fgetattr(path.c_str(), attr);
    if (res != E_PSI_GEN_NONE) {
        return res;
    }
    if ((attr & PSI_A_DIR) == 0) {
        return E_PSI_FILE_DIR;
    }

    return remove(path.c_str());
}

Enum<rfsv::errs> rfsv16::
rename(const char *oldName, const char *newName)
{
    string realOldName = convertSlash(oldName);
    string realNewName = convertSlash(newName);
    bufferStore a;
    a.addStringT(realOldName.c_str());
    a.addStringT(realNewName.c_str());
    sendCommand(SIBO_RENAME, a);
    return getResponse(a);
}

Enum<rfsv::errs> rfsv16::
remove(const char* psionName)
{
    string realName = convertSlash(psionName);
    bufferStore a;
    a.addStringT(realName.c_str());
    // and this needs sending in the length word.
    sendCommand(SIBO_DELETE, a);
    return getResponse(a);
}

Enum<rfsv::errs> rfsv16::
setVolumeName(const char drive , const char * const name)
{
// Not yet ...
    return E_PSI_GEN_FAIL;
}

/*
 * Translate SIBO attributes to standard attributes.
 */
uint32_t rfsv16::
attr2std(uint32_t attr)
{
    uint32_t res = 0;

    // Common attributes
    if (!(attr & P_FAWRITE))
        res |= PSI_A_RDONLY;
    if (attr & P_FAHIDDEN)
        res |= PSI_A_HIDDEN;
    if (attr & P_FASYSTEM)
        res |= PSI_A_SYSTEM;
    if (attr & P_FADIR)
        res |= PSI_A_DIR;
    if (attr & P_FAMOD)
        res |= PSI_A_ARCHIVE;
    if (attr & P_FAVOLUME)
        res |= PSI_A_VOLUME;

    // SIBO-specific
    if (attr & P_FAREAD)
        res |= PSI_A_READ;
    if (attr & P_FAEXEC)
        res |= PSI_A_EXEC;
    if (attr & P_FASTREAM)
        res |= PSI_A_STREAM;
    if (attr & P_FATEXT)
        res |= PSI_A_TEXT;

    // Do what we can for EPOC
    res |= PSI_A_NORMAL;

    return res;
}

/*
 * Translate standard attributes to SIBO attributes.
 */
uint32_t rfsv16::
std2attr(const uint32_t attr)
{
    uint32_t res = 0;

    // Common attributes
    if (!(attr & PSI_A_RDONLY))
        res |= P_FAWRITE;
    if (attr & PSI_A_HIDDEN)
        res |= P_FAHIDDEN;
    if (attr & PSI_A_SYSTEM)
        res |= P_FASYSTEM;
    if (attr & PSI_A_DIR)
        res |= P_FADIR;
    if (attr & PSI_A_ARCHIVE)
        res |= P_FAMOD;
    if (attr & PSI_A_VOLUME)
        res |= P_FAVOLUME;

    // SIBO-specific
    if (attr & PSI_A_READ)
        res |= P_FAREAD;
    if (attr & PSI_A_EXEC)
        res |= P_FAEXEC;
    if (attr & PSI_A_STREAM)
        res |= P_FASTREAM;
    if (attr & PSI_A_TEXT)
        res |= P_FATEXT;

    return res;
}
