/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
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

#include <pthread.h>
#include <string>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iowatch.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>

#include "link.h"
#include "mp_serial.h"
#include "ncp_log.h"
#include "datalink.h"

#define BUFLEN 4096 // Must be a power of 2
#define BUFMASK (BUFLEN-1)
#define hasSpace(dir) (((dir##Write + 1) & BUFMASK) != dir##Read)
#define hasData(dir) (dir##Write != dir##Read)
#define inca(idx,amount) do { \
    idx = (idx + amount) & BUFMASK; \
} while (0)
#define inc1(idx) inca(idx, 1)
#define normalize(idx) do { idx &= BUFMASK; } while (0)

extern "C" {
/**
 * Signal handler does nothing. It just exists
 * for having the select() below return an
 * interrupted system call.
 */
static void usr1handler(int sig) {
    signal(SIGUSR1, usr1handler);
}

void log_data(unsigned short options,
              unsigned short category,
              std::string description,
              unsigned char *buffer, int length) {
    if (!(options & category)) {
        return;
    }
    printf("pump: %s %d bytes: (", description.c_str(), length);
    for (int i = 0; i<length; i++) {
        printf("%02x ", buffer[i]);
    }
    printf(")\n");
}

// TODO: `fd` isn't thread-safe.
static void *data_pump_thread(void *arg) {
    DataLink *dataLink = (DataLink *)arg;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (1) {
        if (dataLink->fd == -1) {
            IOWatch cancellationWatch;
            cancellationWatch.addIO(dataLink->cancellationFd_);
            cancellationWatch.watch(1, 0);
        } else {
            fd_set r_set;
            fd_set w_set;

            // Conditionally watch to to see if we can read and write from the serial port, depending on whether we have
            // space in the input buffer, and data in the output buffer.
            FD_ZERO(&r_set);
            w_set = r_set;
            FD_SET(dataLink->cancellationFd_, &r_set);
            if (hasSpace(dataLink->in))
                FD_SET(dataLink->fd, &r_set);
            if (hasData(dataLink->out))
                FD_SET(dataLink->fd, &w_set);
            struct timeval tv = {1, 0};
            int res = select(MAX(dataLink->fd, dataLink->cancellationFd_) + 1, &r_set, &w_set, NULL, &tv);
            if (res <= 0) {
                // Ignore interrupts and timeouts.
                continue;
            }

            // We can write to the transport; write as much as we can.
            if (FD_ISSET(dataLink->fd, &w_set)) {

                // Work out how much contiguous data there is to write in the out buffer.
                int count = dataLink->outWrite - dataLink->outRead;
                if (count < 0) {
                    count = (BUFLEN - dataLink->outRead);
                }

                // Write as much data as possible.
                res = write(dataLink->fd, &dataLink->outBuffer[dataLink->outRead], count);
                if (res > 0) {
                    log_data(dataLink->verbose_, PKT_DEBUG_DUMP, "wrote", dataLink->outBuffer + dataLink->outRead, res);
                    int hadSpace = hasSpace(dataLink->out);
                    inca(dataLink->outRead, res);
                    if (!hadSpace) {
                        pthread_kill(dataLink->ownerThreadId_, SIGUSR1);
                    }
                }
            }

            // We can read from the transport; read as much as we can.
            if (FD_ISSET(dataLink->fd, &r_set)) {

                // Work out how much contiguous space there is in the buffer.
                int count = dataLink->inRead - dataLink->inWrite;
                if (count <= 0) {
                    count = (BUFLEN - dataLink->inWrite);
                }

                // Read as much data as possible.
                res = read(dataLink->fd, &dataLink->inBuffer[dataLink->inWrite], count);
                if (res > 0) {
                    log_data(dataLink->verbose_, PKT_DEBUG_DUMP, "read", dataLink->inBuffer + dataLink->inWrite, res);
                    inca(dataLink->inWrite, res);
                }
            }

            // Process any available data.
            bool isLinkStable = true;
            if (hasData(dataLink->in)) {
                isLinkStable = dataLink->processInputData();
            }

            // Reset if we were unable to establish a stable link.
            if (!isLinkStable) {
                dataLink->internalReset();
            }
        }
    }
}

};

static const int kBaudRatesTable[] = {
    115200,
    57600,
    38400,
    19200,
    9600,
};
#define BAUD_RATES_TABLE_SIZE (sizeof(kBaudRatesTable) / sizeof(int))

using namespace std;

DataLink::DataLink(const char *fname,
                   int baud,
                   Link *link,
                   unsigned short verbose,
                   const int cancellationFd)
: devname(fname)
, requestedBaudRate_(baud)
, link_(link)
, verbose_(verbose)
, cancellationFd_(cancellationFd) {

    // Initialize CRC table
    crc_table[0] = 0;
    for (int i = 0; i < 128; i++) {
        unsigned int carry = crc_table[i] & 0x8000;
        unsigned int tmp = (crc_table[i] << 1) & 0xffff;
        crc_table[i * 2 + (carry ? 0 : 1)] = tmp ^ 0x1021;
        crc_table[i * 2 + (carry ? 1 : 0)] = tmp;
    }

    inBuffer = new unsigned char[BUFLEN + 1];
    outBuffer = new unsigned char[BUFLEN + 1];

    ownerThreadId_ = pthread_self();
    baudRate_ = requestedBaudRate_;
    if (requestedBaudRate_ < 0) {
        baudRateIndex_ = 1;
        baudRate_ = kBaudRatesTable[0];
    }
    fd = init_serial(devname.c_str(), baudRate_, 0);
    if (fd == -1) {
        lastFatal = true;
    } else {
        signal(SIGUSR1, usr1handler);
        pthread_create(&dataPumpThreadId_, NULL, data_pump_thread, this);
    }
}

DataLink::~DataLink() {
    if (fd != -1) {
        pthread_cancel(dataPumpThreadId_);
        pthread_join(dataPumpThreadId_, NULL);
        ser_exit(fd);
    }
    fd = -1;
    delete []inBuffer;
    delete []outBuffer;
}

void DataLink::reset() {
    // This method stops the data pump thread and restarts it, performing a pthread_join. Given this, it's unsafe to be
    // called from the data pump itself. This is a belt and braces check to ensure we don't do that (spoiler: we were).
    assert(pthread_self() != dataPumpThreadId_);
    if (fd != -1) {
        pthread_cancel(dataPumpThreadId_);
        pthread_join(dataPumpThreadId_, NULL);
    }
    internalReset();
    if (fd != -1) {
        pthread_create(&dataPumpThreadId_, NULL, data_pump_thread, this);
        flushOutputBuffer();
    }
}

void DataLink::internalReset() {
    if (verbose_ & PKT_DEBUG_LOG)
        lout << "resetting serial connection" << endl;
    if (fd != -1) {
        ser_exit(fd);
        fd = -1;
    }
    usleep(100000);
    outRead = outWrite = 0;
    inRead = inWrite = 0;
    esc = false;
    lastFatal = false;
    serialStatus = -1;
    lastSYN = startPkt = -1;
    crcIn = 0;
    baudRate_ = requestedBaudRate_;
    justStarted = true;
    if (requestedBaudRate_ < 0) {
        baudRate_ = kBaudRatesTable[baudRateIndex_++];
        if (baudRateIndex_ >= BAUD_RATES_TABLE_SIZE) {
            baudRateIndex_ = 0;
        }
    }
    fd = init_serial(devname.c_str(), baudRate_, 0);
    if (verbose_ & PKT_DEBUG_LOG)
        lout << "serial connection set to " << dec << baudRate_
             << " baud, fd=" << fd << endl;
    if (fd != -1) {
        lastFatal = false;
    }
}

int DataLink:: getSpeed() {
    return baudRate_;
}

void DataLink::send(bufferStore &b, bool isEPOC) {
    opByte(0x16);
    opByte(0x10);
    opByte(0x02);

    unsigned short crcOut = 0;
    long len = b.getLen();

    if (verbose_ & PKT_DEBUG_LOG) {
        lout << "packet: >> ";
        if (verbose_ & PKT_DEBUG_DUMP)
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
                    // Stuff ETX as DLE EOT
                    opByte(0x10);
                    opByte(0x04);
                } else {
                    opByte(c);
                }
                break;
            case 0x10:
                // Stuff DLE as DLE DLE
                opByte(0x10);
                opByte(0x10);
                break;
            default:
                opByte(c);
        }
        addToCrc(c, &crcOut);
    }
    opByte(0x10);
    opByte(0x03);
    opByte(crcOut >> 8);
    opByte(crcOut & 0xff);
    flushOutputBuffer();
}

void DataLink::opByte(unsigned char a) {
    if (!hasSpace(out)) {
        flushOutputBuffer();
    }
    outBuffer[outWrite] = a;
    inc1(outWrite);
}

void DataLink::flushOutputBuffer() {
    pthread_kill(dataPumpThreadId_, SIGUSR1);
    while (!hasSpace(out)) {
        sigset_t sigs;
        int dummy;
        sigemptyset(&sigs);
        sigaddset(&sigs, SIGUSR1);
        sigwait(&sigs, &dummy);
    }
}

bool DataLink::processInputData() {
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
                        if (verbose_ & PKT_DEBUG_LOG)
                            lout << "packet: BAD CRC" << endl;
                    } else {
                        if (verbose_ & PKT_DEBUG_LOG) {
                            lout << "packet: << ";
                            if (verbose_ & PKT_DEBUG_DUMP)
                                lout << rcv;
                            else
                                lout << "len=" << dec << rcv.getLen();
                            lout << endl;
                        }
                        link_->receive(rcv);
                    }
                    rcv.init();
                    if (hasData(out))
                        return true;
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
            if (rx_amount > 15) {
                return false;
            }
        }
    }
    return true;
}

bool DataLink::linkFailed() {
    int arg;
    int res;
    bool failed = false;

    if (fd == -1)
        return false;
    res = ioctl(fd, TIOCMGET, &arg);
    if (res < 0)
        lastFatal = true;
    if ((serialStatus == -1) || (arg != serialStatus)) {
        if (verbose_ & PKT_DEBUG_HANDSHAKE)
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
            if (verbose_ & PKT_DEBUG_HANDSHAKE)
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
    if ((verbose_ & PKT_DEBUG_LOG) && lastFatal)
        lout << "packet: linkFATAL\n";
    if ((verbose_ & PKT_DEBUG_LOG) && failed)
        lout << "packet: linkFAILED\n";
    return (lastFatal || failed);
}
