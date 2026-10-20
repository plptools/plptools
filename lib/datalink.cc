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
#include "bufferarray.h"
#include "config.h"

#include <cassert>
#include <mutex>
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
#include <fcntl.h>

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

static void *data_pump_thread(void *arg) {
    DataLink *dataLink = (DataLink *)arg;
    while (1) {

        // Get the serial port file descriptor.
        int serialFd = -1;
        {
            std::lock_guard<std::mutex> lock(dataLink->serialMutex_);
            serialFd = dataLink->fd;
        }

        if (serialFd == -1) {
            IOWatch cancellationWatch;
            cancellationWatch.addIO(dataLink->cancellationFd_);
            if (cancellationWatch.watch(1, 0)) {
                // If the watch returned true, cancellationFd_ is readable and we need to shut down.
                dataLink->shutdown();
                return nullptr;
            }
        } else {
            fd_set r_set;
            fd_set w_set;

            // Conditionally watch to to see if we can read and write from the serial port, depending on whether we have
            // space in the input buffer, and data in the output buffer.
            FD_ZERO(&r_set);
            w_set = r_set;
            FD_SET(dataLink->cancellationFd_, &r_set);
            {
                std::lock_guard<std::mutex> inputLock(dataLink->inputMutex_);
                if (hasSpace(dataLink->in)) {
                    FD_SET(serialFd, &r_set);
                }
            }
            {
                std::lock_guard<std::mutex> outputLock(dataLink->outputMutex_);
                if (hasData(dataLink->out)) {
                    FD_SET(serialFd, &w_set);
                }
            }

            FD_SET(dataLink->outputDataReadyPipe_[0], &r_set);

            int nfds = MAX(MAX(serialFd, dataLink->cancellationFd_), dataLink->outputDataReadyPipe_[0]) + 1;
            struct timeval tv = {1, 0};
            int res = select(nfds, &r_set, &w_set, NULL, &tv);
            if (res <= 0) {
                // Ignore interrupts and timeouts.
                continue;
            }

            // Check to see if we were cancelled and, if we were, unblock the writers and exit.
            if (FD_ISSET(dataLink->cancellationFd_, &r_set)) {
                dataLink->shutdown();
                return nullptr;
            }

            if (FD_ISSET(dataLink->outputDataReadyPipe_[0], &r_set)) {
                uint8_t byte;
                ssize_t n = read(dataLink->outputDataReadyPipe_[0], &byte, 1);
            }

            // We can write to the transport; write as much as we can.
            if (FD_ISSET(serialFd, &w_set)) {
                std::lock_guard<std::mutex> serialLock(dataLink->serialMutex_);
                std::lock_guard<std::mutex> outputLock(dataLink->outputMutex_);

                // Work out how much contiguous data there is to write in the out buffer.
                int count = dataLink->outWrite - dataLink->outRead;
                if (count < 0) {
                    count = (BUFLEN - dataLink->outRead);
                }

                // Write as much data as possible.
                res = write(serialFd, &dataLink->outBuffer[dataLink->outRead], count);
                if (res > 0) {
                    log_data(dataLink->verbose_, PKT_DEBUG_DUMP, "wrote", dataLink->outBuffer + dataLink->outRead, res);
                    inca(dataLink->outRead, res);
                    dataLink->outputCondition_.notify_all();
                }
            }

            // We can read from the transport; read as much as we can.
            if (FD_ISSET(serialFd, &r_set)) {
                std::lock_guard<std::mutex> serialLock(dataLink->serialMutex_);
                std::lock_guard<std::mutex> inputLock(dataLink->inputMutex_);

                // Work out how much contiguous space there is in the buffer.
                int count = dataLink->inRead - dataLink->inWrite;
                if (count <= 0) {
                    count = (BUFLEN - dataLink->inWrite);
                }

                // Read as much data as possible.
                res = read(serialFd, &dataLink->inBuffer[dataLink->inWrite], count);
                if (res > 0) {
                    log_data(dataLink->verbose_, PKT_DEBUG_DUMP, "read", dataLink->inBuffer + dataLink->inWrite, res);
                    inca(dataLink->inWrite, res);
                }
            }

            // Process any available data.
            std::vector<BufferStore> receivedData;
            bool isLinkStable = true;
            {
                bool hasInputData = false;
                {
                    std::lock_guard<std::mutex> inputLock(dataLink->inputMutex_);
                    hasInputData = hasData(dataLink->in);
                }
                if (hasInputData) {
                    isLinkStable = dataLink->processInputData(receivedData);
                }
            }

            // Reset if we were unable to establish a stable link.
            if (!isLinkStable) {
                dataLink->internalReset(false);
            }

            // Dispatch received data to @ref link_.
            // Since receivedData is only ever accessed on this thread, we can safely perform this operation without
            // holding any locks meaning our target can't deadlock against us by calling any of our public APIs that
            // require locks.
            dataLink->sendReceivedData(receivedData);
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
                   bool noDSRCheck,
                   unsigned short verbose,
                   const int cancellationFd)
: devname(fname)
, requestedBaudRate_(baud)
, link_(link)
, noDSRCheck_(noDSRCheck)
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

    int result = pipe(outputDataReadyPipe_);
    assert(result == 0);

    baudRate_ = requestedBaudRate_;
    if (requestedBaudRate_ < 0) {
        baudRate_ = kBaudRatesTable[baudRateIndex_];
        baudRateIndex_ = baudRateIndex_ + 1;
    }
    fd = init_serial(devname.c_str(), baudRate_, 0);
    if (verbose_ & PKT_DEBUG_LOG) {
        lout << "serial connection set to " << dec << baudRate_
            << " baud, fd=" << fd << endl;
    }
    if (fd == -1) {
        fcntl(fd, F_SETFL, O_NONBLOCK);
        lastFatal = true;
    } else {
        pthread_create(&dataPumpThreadId_, NULL, data_pump_thread, this);
    }
}

DataLink::~DataLink() {

    // Ensure there are no lingering readers.
    {
        std::lock_guard<std::mutex> outputLock(outputMutex_);
        isCancelled_ = true;
    }
    outputCondition_.notify_all();

    // Stop the data pump thread and close the serial port.
    if (fd != -1) {
        pthread_join(dataPumpThreadId_, NULL);
        ser_exit(fd);
    }
    fd = -1;

    delete []inBuffer;
    delete []outBuffer;

    close(outputDataReadyPipe_[0]);
    close(outputDataReadyPipe_[1]);
    outputDataReadyPipe_[0] = -1;
    outputDataReadyPipe_[1] = -1;
}

void DataLink::reset() {
    internalReset(true);
}

void DataLink::shutdown() {
    std::lock_guard<std::mutex> outputLock(outputMutex_);
    isCancelled_ = true;
    outputCondition_.notify_all();

}

void DataLink::internalReset(bool resetBaudRateIndex) {
    std::lock_guard<std::mutex> serialLock(serialMutex_);
    std::lock_guard<std::mutex> inputLock(inputMutex_);
    std::lock_guard<std::mutex> outputLock(outputMutex_);

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
    if (resetBaudRateIndex) {
        baudRateIndex_ = 0;
    }
    justStarted = true;
    if (requestedBaudRate_ < 0) {
        baudRate_ = kBaudRatesTable[baudRateIndex_];
        baudRateIndex_ = baudRateIndex_ + 1;
        if (baudRateIndex_ >= BAUD_RATES_TABLE_SIZE) {
            baudRateIndex_ = 0;
        }
    }
    fd = init_serial(devname.c_str(), baudRate_, 0);
    if (verbose_ & PKT_DEBUG_LOG) {
        lout << "serial connection set to " << dec << baudRate_
             << " baud, fd=" << fd << endl;
    }
    if (fd != -1) {
        fcntl(fd, F_SETFL, O_NONBLOCK);
        lastFatal = false;
    }
}

void DataLink::signalPumpThread() {
    char b = 0;
    write(outputDataReadyPipe_[1], &b, 1);
}

int DataLink:: getSpeed() {
    std::lock_guard<std::mutex> serialLock(serialMutex_);
    return baudRate_;
}

void DataLink::send(BufferStore &b, bool isEPOC) {

    // Assemble the message.
    BufferStore message;
    message.addByte(0x16);
    message.addByte(0x10);
    message.addByte(0x02);

    long len = b.getLen();

    if (verbose_ & PKT_DEBUG_LOG) {
        lout << "packet: >> ";
        if (verbose_ & PKT_DEBUG_DUMP)
            lout << b;
        else
            lout << " len=" << dec << len;
        lout << endl;
    }

    unsigned short crcOut = 0;
    for (int i = 0; i < len; i++) {
        unsigned char c = b.getByte(i);
        switch (c) {
            case 0x03:
                if (isEPOC) {
                    // Stuff ETX as DLE EOT
                    message.addByte(0x10);
                    message.addByte(0x04);
                } else {
                    message.addByte(c);
                }
                break;
            case 0x10:
                // Stuff DLE as DLE DLE
                message.addByte(0x10);
                message.addByte(0x10);
                break;
            default:
                message.addByte(c);
        }
        addToCrc(c, &crcOut);
    }
    message.addByte(0x10);
    message.addByte(0x03);
    message.addByte(crcOut >> 8);
    message.addByte(crcOut & 0xff);

    // Signal the data pump thread to write some data and wait on a condition variable for enough space.
    signalPumpThread();
    std::unique_lock<std::mutex> outputLock(outputMutex_);
    outputCondition_.wait(outputLock, [&] {
        unsigned long free = (outRead - outWrite - 1 + BUFLEN) & BUFMASK;
        return free >= message.getLen() || isCancelled_;
    });

    // Exit early if we've been cancelled, dropping the data on the floor.
    if (isCancelled_) {
        return;
    }

    for (unsigned long i = 0; i < message.getLen(); i++) {
        outBuffer[outWrite] = message.getByte(i);
        inc1(outWrite);
    }

    // Signal the data pump thread to tell it there's new data.
    signalPumpThread();
}

bool DataLink::processInputData(std::vector<BufferStore> &receivedData) {
    std::lock_guard<std::mutex> inputLock(inputMutex_);

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
                        receivedData.push_back(rcv);
                    }
                    rcv.init();

                    // Check to see if there's pending data to be sent to the Psion in an effort to avoid starvation.
                    // We should revisit whether this is an unnecessary optimization in the future.
                    bool hasOutputData = false;
                    {
                        std::lock_guard<std::mutex> outputLock(outputMutex_);
                        hasOutputData = hasData(out);
                    }
                    if (hasOutputData) {
                        return true;
                    }
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

void DataLink::sendReceivedData(std::vector<BufferStore> &receivedData) {
    for (vector<BufferStore>::iterator i = receivedData.begin(); i != receivedData.end(); i++) {
        link_->receive(*i);
    }
}

bool DataLink::linkFailed() {
    int arg;
    int res;
    bool failed = false;

    std::lock_guard<std::mutex> serialLock(serialMutex_);

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
    if (!noDSRCheck_ && (arg & TIOCM_DSR) == 0) {
        failed = true;
    }
    if ((verbose_ & PKT_DEBUG_LOG) && lastFatal)
        lout << "packet: linkFATAL\n";
    if ((verbose_ & PKT_DEBUG_LOG) && failed)
        lout << "packet: linkFAILED\n";
    return (lastFatal || failed);
}
