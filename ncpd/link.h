/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
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
#pragma once

#include "config.h"
#include <pthread.h>
#include <sys/time.h>

#include "bufferstore.h"
#include "bufferarray.h"
#include "Enum.h"
#include <vector>

class DataLink;
class NCP;

/**
 * Describes a transmitted packet which has not yet
 * been acknowledged by the peer.
 */
typedef struct {
    /**
     * Original sequence number.
     */
    int seq;
    /**
     * Number of remaining transmit retries.
     */
    int txcount;
    /**
     * Time of last transmit.
     */
    struct timeval stamp;
    /**
     * Packet content.
     */
    BufferStore data;
} AckWaitQueueElement;

extern "C" {
    static void *expire_check(void *);
}

class Link {
public:

    enum link_type {
        LINK_TYPE_UNKNOWN = 0,
        LINK_TYPE_SIBO    = 1,
        LINK_TYPE_EPOC    = 2,
    };

    /**
     * Construct a new link instance.
     *
     * @param fname Name of serial device.
     * @param baud Speed of serial device.
     * @param ncp The calling NCP instance.
     * @param noDSRCheck Disable checking DSR (for buggy serial drivers)
     * @param verbose Verbosity (for debugging/troubleshooting)
     * @param cancellationFd File descriptor that can be used to signal that the `Link` should shutdown.
     */
    Link(const char *fname, int baud, NCP *ncp, bool noDSRCheck, unsigned short verbose, const int cancellationFd);

    /**
     * Disconnects from device and destroys instance.
     */
    ~Link();

    /**
     * Send a PLP packet to the Peer.
     *
     * @param buff The contents of the PLP packet.
     */
    void send(const BufferStore &buff);

    /**
     * Query outstanding packets.
     *
     * @returns true, if packets are outstanding (not yet acknowledged), false
     *  otherwise.
     */
    bool stuffToSend();

    /**
     * Query connection failure.
     *
     * @returns true, if the peer could not be contacted or did not response,
     *  false if everything is ok.
     */
    bool hasFailed();

    /**
     * Reset connection and attempt to reconnect to the peer.
     */
    void reset();

    /**
     * Purge all outstanding packets for a specified remote channel.
     *
     * @param channel The of the channel for which to remove outstanding
     *  packets.
     */
    void purgeQueue(int channel);

    /**
     * Get the current link type.
     *
     * @returns One of LINK_TYPE_... values.
     */
    Enum<link_type> getLinkType();

    /**
     * Get current speed of the serial device
     *
     * @returns The current speed in baud.
     */
    int getSpeed();

private:
    friend class DataLink;
    friend void * expire_check(void *);

    /**
    * Effectively a delegate method that accepts data from our @ref DataLink instance.
    *
    * Called on the @ref DataLink's internal thread.
    */
    void receive(BufferStore buf);
    void transmit(BufferStore buf);
    void sendAck(int seq, Enum<link_type> linkType);
    void sendReqReq();
    void sendReqCon();
    void sendReq(Enum<link_type> linkType);
    void multiAck(struct timeval);
    void retransmit();
    void transmitHoldQueue(int channel);
    void transmitWaitQueue();
    void purgeAllQueues();
    unsigned long retransTimeout();

    pthread_t checkThreadId_;
    pthread_mutex_t queueMutex_;

    NCP * const ncp_;
    DataLink *dataLink_ = nullptr;
    int txSequence_ = 1;
    int rxSequence_ = -1;
    int seqMask_ = 7;
    int maxOutstanding_ = 1;
    unsigned long conMagic_;
    const unsigned short verbose_;
    bool failed_;
    Enum<link_type> linkType_;

    std::vector<AckWaitQueueElement> ackWaitQueue;
    std::vector<BufferStore> holdQueue_;
    std::vector<BufferStore> waitQueue_;
    bool xoff_[256];

    /**
    * Used to signal cancellation.
    *
    * Should never be read.
    */
    const int cancellationFd_;
};
