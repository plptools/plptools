/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
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

#include <string>
#include <cstring>
#include <fstream>
#include <iomanip>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>

#include "mp_serial.h"
#include "packet.h"
#include "link.h"
#include "main.h"

#define BUFLEN 4096 // Must be a power of 2
#define BUFMASK (BUFLEN-1)
#define hasSpace(dir) (((dir##Write + 1) & BUFMASK) != dir##Read)
#define hasData(dir) (dir##Write != dir##Read)
#define inca(idx,amount) do { \
    idx = (idx + amount) & BUFMASK; \
} while (0)
#define inc1(idx) inca(idx, 1)
#define normalize(idx) do { idx &= BUFMASK; } while (0)

static unsigned short pumpverbose = 0;

extern "C" {
/**
 * Signal handler does nothing. It just exists
 * for having the select() below return an
 * interrupted system call.
 */
static void usr1handler(int sig)
{
    signal(SIGUSR1, usr1handler);
}


static void *pump_run(void *arg)
{
    packet *p = (packet *)arg;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (1) {
        if (p->fd != -1) {
            fd_set r_set;
            fd_set w_set;
            int res;
            int count;

            FD_ZERO(&r_set);
            w_set = r_set;
            if (hasSpace(p->in))
                FD_SET(p->fd, &r_set);
            if (hasData(p->out))
                FD_SET(p->fd, &w_set);
            res = select(p->fd+1, &r_set, &w_set, NULL, NULL);
            switch (res) {
                case 0:
                    break;
                case -1:
                    break;
                default:
                    if (FD_ISSET(p->fd, &w_set)) {
                        count = p->outWrite - p->outRead;
                        if (count < 0)
                            count = (BUFLEN - p->outRead);
                        res = write(p->fd, &p->outBuffer[p->outRead], count);
                        if (res > 0) {
                            if (pumpverbose & PKT_DEBUG_DUMP) {
                                int i;
                                printf("pump: wrote %d bytes: (", res);
                                for (i = 0; i<res; i++)
                                    printf("%02x ",
                                           p->outBuffer[p->outRead + i]);
                                printf(")\n");
                            }
                            int hadSpace = hasSpace(p->out);
                            inca(p->outRead, res);
                            if (!hadSpace)
                                    pthread_kill(p->thisThread, SIGUSR1);
                        }
                    }
                    if (FD_ISSET(p->fd, &r_set)) {
                        count = p->inRead - p->inWrite;
                        if (count <= 0)
                            count = (BUFLEN - p->inWrite);
                        res = read(p->fd, &p->inBuffer[p->inWrite], count);
                        if (res > 0) {
                            if (pumpverbose & PKT_DEBUG_DUMP) {
                                int i;
                                printf("pump: read %d bytes: (", res);
                                for (i = 0; i<res; i++)
                                    printf("%02x ", p->inBuffer[p->inWrite + i]);
                                printf(")\n");
                            }
                            inca(p->inWrite, res);
                            p->findSync();
                        }
                    } else {
                        if (hasData(p->in))
                            p->findSync();
                    }
                    break;
            }
        }
    }
}

};

static const int baud_table[] = {
    115200,
    57600,
    38400,
    19200,
    // Lower rates don't make sense ?!
};
#define BAUD_TABLE_SIZE (sizeof(baud_table) / sizeof(int))

using namespace std;

packet::
packet(const char *fname, int _baud, Link *_link, unsigned short _verbose)
{
    verbose = pumpverbose = _verbose;
    devname = strdup(fname);
    assert(devname);
    baud = _baud;
    theLINK = _link;
    isEPOC = false;
    justStarted = true;

    // Initialize CRC table
    crc_table[0] = 0;
    for (int i = 0; i < 128; i++) {
        unsigned int carry = crc_table[i] & 0x8000;
        unsigned int tmp = (crc_table[i] << 1) & 0xffff;
        crc_table[i * 2 + (carry ? 0 : 1)] = tmp ^ 0x1021;
        crc_table[i * 2 + (carry ? 1 : 0)] = tmp;
    }

    inRead = inWrite = outRead = outWrite = 0;
    inBuffer = new unsigned char[BUFLEN + 1];
    outBuffer = new unsigned char[BUFLEN + 1];
    assert(inBuffer);
    assert(outBuffer);

    esc = false;
    lastFatal = false;
    serialStatus = -1;
    lastSYN = startPkt = -1;
    crcIn = crcOut = 0;

    thisThread = pthread_self();
    realBaud = baud;
    if (baud < 0) {
        baud_index = 1;
        realBaud = baud_table[0];
    }
    fd = init_serial(devname, realBaud, 0);
    if (fd == -1)
        lastFatal = true;
    else {
        signal(SIGUSR1, usr1handler);
        pthread_create(&datapump, NULL, pump_run, this);
    }
}

packet::
~packet()
{
    if (fd != -1) {
        pthread_cancel(datapump);
        pthread_join(datapump, NULL);
        ser_exit(fd);
    }
    fd = -1;
    delete []inBuffer;
    delete []outBuffer;
    free(devname);
}

void packet::
reset()
{
    if (fd != -1) {
        pthread_cancel(datapump);
        pthread_join(datapump, NULL);
    }
    outRead = outWrite = 0;
    internalReset();
    if (fd != -1) {
        pthread_create(&datapump, NULL, pump_run, this);
        realWrite();
    }
}

void packet::
internalReset()
{
    if (verbose & PKT_DEBUG_LOG)
        lout << "resetting serial connection" << endl;
    if (fd != -1) {
        ser_exit(fd);
        fd = -1;
    }
    usleep(100000);
    inRead = inWrite = 0;
    esc = false;
    lastFatal = false;
    serialStatus = -1;
    lastSYN = startPkt = -1;
    crcIn = crcOut = 0;
    realBaud = baud;
    justStarted = true;
    if (baud < 0) {
        realBaud = baud_table[baud_index++];
        if (baud_index >= BAUD_TABLE_SIZE)
            baud_index = 0;
    }

    fd = init_serial(devname, realBaud, 0);
    if (verbose & PKT_DEBUG_LOG)
        lout << "serial connection set to " << dec << realBaud
             << " baud, fd=" << fd << endl;
    if (fd != -1)
        lastFatal = false;
}

short int packet::
getVerbose()
{
    return verbose;
}

void packet::
setVerbose(short int _verbose)
{
    verbose = pumpverbose = _verbose;
}

void packet::
setEpoc(bool _epoc)
{
    isEPOC = _epoc;
}

int packet::
getSpeed()
{
    return realBaud;
}

void packet::
send(bufferStore &b)
{
    opByte(0x16);
    opByte(0x10);
    opByte(0x02);

    crcOut = 0;
    long len = b.getLen();

    if (verbose & PKT_DEBUG_LOG) {
        lout << "packet: >> ";
        if (verbose & PKT_DEBUG_DUMP)
            lout << b;
        else
            lout << " len=" << dec << len;
        lout << endl;
    }

    for (int i = 0; i < len; i++) {
        unsigned char c = b.getByte(i);
        switch (c) {
            case 0x03:
                if (isEPOC) {
                    opByte(0x10);
                    opByte(0x04);
                    addToCrc(0x03, &crcOut);
                } else
                    opCByte(c, &crcOut);
                break;
            case 0x10:
                opByte(0x10);
                // fall thru
            default:
                opCByte(c, &crcOut);
        }
    }
    opByte(0x10);
    opByte(0x03);
    opByte(crcOut >> 8);
    opByte(crcOut & 0xff);
    realWrite();
}

void packet::
opByte(unsigned char a)
{
    if (!hasSpace(out))
        realWrite();
    outBuffer[outWrite] = a;
    inc1(outWrite);
}

void packet::
opCByte(unsigned char a, unsigned short *crc)
{
    addToCrc(a, crc);
    if (!hasSpace(out))
        realWrite();
    outBuffer[outWrite] = a;
    inc1(outWrite);
}

void packet::
realWrite()
{
    pthread_kill(datapump, SIGUSR1);
    while (!hasSpace(out)) {
        sigset_t sigs;
        int dummy;
        sigemptyset(&sigs);
        sigaddset(&sigs, SIGUSR1);
        sigwait(&sigs, &dummy);
    }
}

void packet::
findSync()
{
    int inw = inWrite;
    int p;

 outerLoop:
    p = (lastSYN >= 0) ? lastSYN : inRead;
    if (startPkt < 0) {
        while (p != inw) {
            normalize(p);
            if (inBuffer[p++] != 0x16)
                continue;
            lastSYN = p - 1;
            normalize(p);
            if (p == inw)
                break;
            if (inBuffer[p++] != 0x10)
                continue;
            normalize(p);
            if (p == inw)
                break;
            if (inBuffer[p++] != 0x02)
                continue;
            normalize(p);
            lastSYN = startPkt = p;
            crcIn = inCRCstate = 0;
            rcv.init();
            esc = false;
            break;
        }
    }
    if (startPkt >= 0) {
        justStarted = false;
        while (p != inw) {
            unsigned char c = inBuffer[p];
            switch (inCRCstate) {
                case 0:
                    if (esc) {
                        esc = false;
                        switch (c) {
                            case 0x03:
                                inCRCstate = 1;
                                break;
                            case 0x04:
                                addToCrc(0x03, &crcIn);
                                rcv.addByte(0x03);
                                break;
                            default:
                                addToCrc(c, &crcIn);
                                rcv.addByte(c);
                                break;
                        }
                    } else {
                        if (c == 0x10)
                            esc = true;
                        else {
                            addToCrc(c, &crcIn);
                            rcv.addByte(c);
                        }
                    }
                    break;
                case 1:
                    receivedCRC = c;
                    receivedCRC <<= 8;
                    inCRCstate = 2;
                    break;
                case 2:
                    receivedCRC |= c;
                    inc1(p);
                    inRead = p;
                    startPkt = lastSYN = -1;
                    inCRCstate = 0;
                    if (receivedCRC != crcIn) {
                        if (verbose & PKT_DEBUG_LOG)
                            lout << "packet: BAD CRC" << endl;
                    } else {
                        if (verbose & PKT_DEBUG_LOG) {
                            lout << "packet: << ";
                            if (verbose & PKT_DEBUG_DUMP)
                                lout << rcv;
                            else
                                lout << "len=" << dec << rcv.getLen();
                            lout << endl;
                        }
                        theLINK->receive(rcv);
                    }
                    rcv.init();
                    if (hasData(out))
                        return;
                    goto outerLoop;
            }
            inc1(p);
        }
        lastSYN = p;
    } else {
        // If we get here, no sync was found.
        // If we are just started and the amount of received data exceeds
        // 15 bytes, the baudrate is obviously wrong.
        // (or the connected device is not an EPOC device). Reset the
        // serial connection and try next baudrate, if auto-baud is set.
        if (justStarted) {
            int rx_amount = (inw > inRead) ?
                inw - inRead : BUFLEN - inRead + inw;
            if (rx_amount > 15)
                reset();
        }
    }
}

bool packet::
linkFailed()
{
    int arg;
    int res;
    bool failed = false;

    if (fd == -1)
        return false;
    res = ioctl(fd, TIOCMGET, &arg);
    if (res < 0)
        lastFatal = true;
    if ((serialStatus == -1) || (arg != serialStatus)) {
        if (verbose & PKT_DEBUG_HANDSHAKE)
            lout << "packet: < DTR:" << ((arg & TIOCM_DTR)?1:0)
                 << " RTS:" << ((arg & TIOCM_RTS)?1:0)
                 << " DCD:" << ((arg & TIOCM_CAR)?1:0)
                 << " DSR:" << ((arg & TIOCM_DSR)?1:0)
                 << " CTS:" << ((arg & TIOCM_CTS)?1:0) << endl;
        if (!((arg & TIOCM_RTS) && (arg & TIOCM_DTR))) {
            arg |= (TIOCM_DTR | TIOCM_RTS);
            res = ioctl(fd, TIOCMSET, &arg);
            if (res < 0)
                lastFatal = true;
            if (verbose & PKT_DEBUG_HANDSHAKE)
                lout << "packet: > DTR:" << ((arg & TIOCM_DTR)?1:0)
                     << " RTS:" << ((arg & TIOCM_RTS)?1:0)
                     << " DCD:" << ((arg & TIOCM_CAR)?1:0)
                     << " DSR:" << ((arg & TIOCM_DSR)?1:0)
                     << " CTS:" << ((arg & TIOCM_CTS)?1:0) << endl;
        }
        serialStatus = arg;
    }
    // TODO: Check for a solution on Solaris.
    if ((arg & TIOCM_DSR) == 0) {
        failed = true;
    }
    if ((verbose & PKT_DEBUG_LOG) && lastFatal)
        lout << "packet: linkFATAL\n";
    if ((verbose & PKT_DEBUG_LOG) && failed)
        lout << "packet: linkFAILED\n";
    return (lastFatal || failed);
}
