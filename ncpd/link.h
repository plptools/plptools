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
#ifndef _link_h_
#define _link_h_

#include "config.h"
#include <pthread.h>
#include <sys/time.h>

#include "bufferstore.h"
#include "bufferarray.h"
#include "Enum.h"
#include <vector>

#define LNK_DEBUG_LOG  4
#define LNK_DEBUG_DUMP 8

class ncp;
class packet;

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
    bufferStore data;
} ackWaitQueueElement;

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
     * @param baud  Speed of serial device.
     * @param ncp   The calling ncp instance.
     * @_verbose    Verbosity (for debugging/troubleshooting)
     */
    Link(const char *fname, int baud, ncp *ncp, unsigned short _verbose = 0);

    /**
     * Disconnects from device and destroys instance.
     */
    ~Link();

    /**
     * Send a PLP packet to the Peer.
     *
     * @param buff The contents of the PLP packet.
     */
    void send(const bufferStore &buff);

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
     * Wait, until all outstanding packets are acknowledged or timed out.
     */
    void flush();

    /**
     * Purge all outstanding packets for a specified remote channel.
     *
     * @param channel The of the channel for which to remove outstanding
     *  packets.
     */
    void purgeQueue(int channel);

    /**
     * Set verbosity of Link and underlying packet instance.
     *
     * @param _verbose Verbosity (a bitmapped value, see LINK_DEBUG_.. constants)
     */
    void setVerbose(unsigned short _verbose);

    /**
     * Get current verbosity of Link.
     *
     * @returns The verbosity, specified at construction or last call to
     *  setVerbosity();
     */
    unsigned short getVerbose();

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
    friend class packet;
    friend void * expire_check(void *);

    void receive(bufferStore buf);
    void transmit(bufferStore buf);
    void sendAck(int seq);
    void sendReqReq();
    void sendReqCon();
    void sendReq();
    void multiAck(struct timeval);
    void retransmit();
    void transmitHoldQueue(int channel);
    void transmitWaitQueue();
    void purgeAllQueues();
    unsigned long retransTimeout();

    pthread_t checkthread;
    pthread_mutex_t queueMutex;

    ncp *theNCP;
    packet *p;
    int txSequence;
    int rxSequence;
    int seqMask;
    int maxOutstanding;
    unsigned long conMagic;
    unsigned short verbose;
    bool failed;
    Enum<link_type> linkType;

    std::vector<ackWaitQueueElement> ackWaitQueue;
    std::vector<bufferStore> holdQueue;
    std::vector<bufferStore> waitQueue;
    bool xoff[256];
};

#endif
