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
#include "config.h"

#include <iostream>

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>

#include "bufferstore.h"
#include "bufferarray.h"

#include "link.h"
#include "packet.h"
#include "ncp.h"
#include "main.h"

extern "C" {
    static void *expire_check(void *arg)
    {
	Link *l = (Link *)arg;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	while (1) {
	    usleep(l->retransTimeout() * 500);
	    l->retransmit();
	}
    }
};

using namespace std;

ENUM_DEFINITION_BEGIN(Link::link_type, Link::LINK_TYPE_UNKNOWN)
    stringRep.add(Link::LINK_TYPE_UNKNOWN, N_("Unknown"));
    stringRep.add(Link::LINK_TYPE_SIBO,    N_("SIBO"));
    stringRep.add(Link::LINK_TYPE_EPOC,    N_("EPOC"));
ENUM_DEFINITION_END(Link::link_type)

Link::Link(const char *fname, int baud, ncp *_ncp, unsigned short _verbose)
    : p(0)
{
    theNCP = _ncp;
    verbose = _verbose;
    txSequence = 1;
    rxSequence = -1;
    failed = false;
    seqMask = 7;
    maxOutstanding = 1;
    linkType = LINK_TYPE_UNKNOWN;
    for (int i = 0; i < 256; i++)
	xoff[i] = false;
    // generate magic number for sendReqCon()
    srandom(time(NULL));
    conMagic = random();

    p = new packet(fname, baud, this, _verbose);

    pthread_mutex_init(&queueMutex, NULL);
    pthread_create(&checkthread, NULL, expire_check, this);

    // submit a link request
    sendReqReq();
}

Link::~Link()
{
    flush();
    pthread_cancel(checkthread);
    pthread_mutex_destroy(&queueMutex);
    delete p;
}

unsigned long Link::
retransTimeout()
{
    return ((unsigned long)getSpeed() * 1000 / 13200) + 200;
}

void Link::
reset() {
    txSequence = 1;
    rxSequence = -1;
    failed = false;
    seqMask = 7;
    maxOutstanding = 1;
    linkType = LINK_TYPE_UNKNOWN;
    purgeAllQueues();
    for (int i = 0; i < 256; i++)
	xoff[i] = false;
    p->reset();
    // submit a link request
    sendReqReq();
}

unsigned short Link::
getVerbose()
{
    return verbose;
}

void Link::
setVerbose(unsigned short _verbose)
{
    verbose = _verbose;
    p->setVerbose(verbose);
}

void Link::
send(const bufferStore & buff)
{
    if (buff.getLen() > 300) {
	failed = true;
    } else
	transmit(buff);
}

void Link::
purgeAllQueues()
{
    pthread_mutex_lock(&queueMutex);
    ackWaitQueue.clear();
    holdQueue.clear();
    pthread_mutex_unlock(&queueMutex);
}

void Link::
purgeQueue(int channel)
{
    pthread_mutex_lock(&queueMutex);
    vector<ackWaitQueueElement>::iterator i;
    for (i = ackWaitQueue.begin(); i != ackWaitQueue.end(); i++)
	if (i->data.getByte(0) == channel) {
	    ackWaitQueue.erase(i);
	    i--;
	}
    vector<bufferStore>::iterator j;
    for (j = holdQueue.begin(); j != holdQueue.end(); j++)
	if (j->getByte(0) == channel) {
	    holdQueue.erase(j);
	    j--;
	}
    pthread_mutex_unlock(&queueMutex);
}

void Link::
sendAck(int seq)
{
    if (hasFailed())
	return;
    bufferStore tmp;
    if (verbose & LNK_DEBUG_LOG)
	lout << "Link: >> ack seq=" << seq << endl;
    if (seq > 7) {
	int hseq = seq >> 3;
	int lseq = (seq & 7) | 8;
	seq = (hseq << 8) + lseq;
	tmp.prependWord(seq);
    } else
	tmp.prependByte(seq);
    p->send(tmp);
}

void Link::
sendReqCon()
{
    if (hasFailed())
	return;
    bufferStore tmp;
    if (verbose & LNK_DEBUG_LOG)
	lout << "Link: >> con seq=4" << endl;
    tmp.addByte(0x24);
    tmp.addDWord(conMagic);
    ackWaitQueueElement e;
    e.seq = 0; // expected ACK is 0, _NOT_ 4!
    gettimeofday(&e.stamp, NULL);
    e.data = tmp;
    e.txcount = 4;
    pthread_mutex_lock(&queueMutex);
    ackWaitQueue.push_back(e);
    pthread_mutex_unlock(&queueMutex);
    p->send(tmp);
}

void Link::
sendReqReq()
{
    if (hasFailed())
	return;
    bufferStore tmp;
    if (verbose & LNK_DEBUG_LOG)
	lout << "Link: >> con seq=1" << endl;
    tmp.addByte(0x21);
    ackWaitQueueElement e;
    e.seq = 0; // expected response is Ack with seq=0 or ReqCon
    gettimeofday(&e.stamp, NULL);
    e.data = tmp;
    e.txcount = 4;
    pthread_mutex_lock(&queueMutex);
    ackWaitQueue.push_back(e);
    pthread_mutex_unlock(&queueMutex);
    p->send(tmp);
}

void Link::
sendReq()
{
    if (hasFailed())
	return;
    bufferStore tmp;
    if (verbose & LNK_DEBUG_LOG)
	lout << "Link: >> con seq=1" << endl;
    tmp.addByte(0x20);
    // No Ack expected for this, so no new entry in ackWaitQueue
    p->send(tmp);
}

void Link::
receive(bufferStore buff)
{
    if (!p)
	return;

    vector<ackWaitQueueElement>::iterator i;
    bool ackFound;
    bool conFound;
    int type = buff.getByte(0);
    int seq = type & 0x0f;

    type &= 0xf0;
    // Support for incoming extended sequence numbers
    if (seq & 8) {
	int tseq = buff.getByte(1);
	buff.discardFirstBytes(2);
	seq = (tseq << 3) + (seq & 0x07);
    } else
	buff.discardFirstBytes(1);

    switch (type) {
	case 0x30:
	    // Normal data
	    if (verbose & LNK_DEBUG_LOG) {
		lout << "Link: << dat seq=" << seq ;
		if (verbose & LNK_DEBUG_DUMP)
		    lout << " " << buff << endl;
		else
		    lout << " len=" << buff.getLen() << endl;
	    }

	    if (((rxSequence + 1) & seqMask) == seq) {
		rxSequence++;
		rxSequence &= seqMask;

	    	sendAck(rxSequence);
		// Must check for XOFF/XON ncp frames HERE!
		if ((buff.getLen() == 3) && (buff.getByte(0) == 0)) {
		    switch (buff.getByte(2)) {
			case 1:
			    // XOFF
			    xoff[buff.getByte(1)] = true;
			    if (verbose & LNK_DEBUG_LOG)
				lout << "Link: got XOFF for channel "
				     << buff.getByte(1) << endl;
			    break;
			case 2:
			    // XON
			    xoff[buff.getByte(1)] = false;
			    if (verbose & LNK_DEBUG_LOG)
				lout << "Link: got XON for channel "
				     << buff.getByte(1) << endl;
			    // Transmit packets on hold queue
			    transmitHoldQueue(buff.getByte(1));
			    break;
			default:
			    theNCP->receive(buff);
		    }
		} else
		    theNCP->receive(buff);

	    } else {
	    	sendAck(rxSequence);
		if (verbose & LNK_DEBUG_LOG)
		    lout << "Link: DUP\n";
	    }
	    break;

	case 0x00:
	    // Incoming ack
	    // Find corresponding packet in ackWaitQueue
	    ackFound = false;
	    struct timeval refstamp;
	    pthread_mutex_lock(&queueMutex);
	    for (i = ackWaitQueue.begin(); i != ackWaitQueue.end(); i++)
		if (i->seq == seq) {
		    ackFound = true;
		    refstamp = i->stamp;
		    ackWaitQueue.erase(i);
		    if (verbose & LNK_DEBUG_LOG) {
			lout << "Link: << ack seq=" << seq ;
			if (verbose & LNK_DEBUG_DUMP)
			    lout << " " << buff;
			lout << endl;
		    }
		    break;
		}
	    pthread_mutex_unlock(&queueMutex);
	    if (ackFound) {
		if ((linkType == LINK_TYPE_UNKNOWN) && (seq == 0)) {
		    // If the remote device runs SIBO protocol, this ACK
		    // should be 0 (the Ack on our ReqReq request, which is
		    // treated as a normal Req by the SIBO machine.
		    failed = false;
		    linkType = LINK_TYPE_SIBO;
		    seqMask = 7;
		    maxOutstanding = 1;
		    rxSequence = 0;
		    txSequence = 1;
		    purgeAllQueues();
		    p->setEpoc(false);
		    if (verbose & LNK_DEBUG_LOG)
			lout << "Link: 1-linkType set to " << linkType << endl;
		}
		// Older packets implicitely ack'ed
		multiAck(refstamp);
		// Transmit waiting packets
		transmitWaitQueue();
	    } else {
		// If packet with seq+1 is in ackWaitQueue, resend it immediately
		// (Receiving an ack for a packet not on our wait queue is a
		// hint by the Psion about which was the last packet it
		// received successfully.)
		pthread_mutex_lock(&queueMutex);
		struct timeval now;
		gettimeofday(&now, NULL);
		bool nextFound = false;
		for (i = ackWaitQueue.begin(); i != ackWaitQueue.end(); i++)
		    if (i->seq == seq+1) {
			nextFound = true;
			if (i->txcount-- == 0) {
			    // timeout, remove packet
			    if (verbose & LNK_DEBUG_LOG)
				lout << "Link: >> TRANSMIT timeout seq=" <<
				    i->seq << endl;
			    ackWaitQueue.erase(i);
			    i--;
			} else {
			    // retransmit it
			    i->stamp = now;
			    if (verbose & LNK_DEBUG_LOG)
				lout << "Link: >> RETRANSMIT seq=" << i->seq
				     << endl;
			    p->send(i->data);
			}
			break;
		    }
		pthread_mutex_unlock(&queueMutex);
		if ((verbose & LNK_DEBUG_LOG) && (!nextFound)) {
		    lout << "Link: << UNMATCHED ack seq=" << seq;
		    if (verbose & LNK_DEBUG_DUMP)
			lout << " " << buff;
		    lout << endl;
		}
	    }
	    break;

	case 0x20:
	    // New link
	    conFound = false;
	    if (seq > 3) {
		// May be a link confirm packet (EPOC)
		pthread_mutex_lock(&queueMutex);
		for (i = ackWaitQueue.begin(); i != ackWaitQueue.end(); i++)
		    if ((i->seq == 0) && (i->data.getByte(0) == 0x21)) {
			ackWaitQueue.erase(i);
			linkType = LINK_TYPE_EPOC;
			if (verbose & LNK_DEBUG_LOG)
			    lout << "Link: 2-linkType set to " << linkType << endl;
			conFound = true;
			failed = false;
			// EPOC can handle extended sequence numbers
			seqMask = 0x7ff;
			// EPOC can handle up to 8 unacknowledged packets
			maxOutstanding = 8;
			p->setEpoc(true);
			if (verbose & LNK_DEBUG_LOG) {
			    lout << "Link: << con seq=" << seq ;
			    if (verbose & LNK_DEBUG_DUMP)
				lout << " " << buff;
			    lout << endl;
			}
			break;
		    }
		pthread_mutex_unlock(&queueMutex);
	    }
	    if (conFound) {
		rxSequence = 0;
		txSequence = 1;
		sendAck(rxSequence);
	    } else {
		if (verbose & LNK_DEBUG_LOG) {
		    lout << "Link: << req seq=" << seq;
		    if (verbose & LNK_DEBUG_DUMP)
			lout << " " << buff;
		    lout << endl;
		}
		rxSequence = txSequence = 0;
		if (seq > 0) {
		    linkType = LINK_TYPE_EPOC;
		    if (verbose & LNK_DEBUG_LOG)
			lout << "Link: 3-linkType set to " << linkType << endl;
		    // EPOC can handle extended sequence numbers
		    seqMask = 0x7ff;
		    // EPOC can handle up to 8 unacknowledged packets
		    maxOutstanding = 8;
		    p->setEpoc(true);
		    failed = false;
		    sendReqCon();
		} else {
		    // SIBO
		    linkType = LINK_TYPE_SIBO;
		    failed = false;
		    seqMask = 7;
		    maxOutstanding = 1;
		    if (verbose & LNK_DEBUG_LOG)
			lout << "Link: 4-linkType set to " << linkType << endl;
		    rxSequence = 0;
		    txSequence = 1; // Our ReqReq was seq 0
		    purgeAllQueues();
		    p->setEpoc(false);
		    sendAck(rxSequence);
		}
	    }
	    break;

	case 0x10:
	    // Disconnect
	    if (verbose & LNK_DEBUG_LOG)
		lout << "Link: << DISC" << endl;
	    failed = true;
	    break;

	default:
	    lerr << "Link: FATAL: Unknown packet type " << type << endl;
    }
}

void Link::
transmitHoldQueue(int channel)
{
    vector<bufferStore> tmpQueue;
    vector<bufferStore>::iterator i;

    // First, move desired packets to a temporary queue
    pthread_mutex_lock(&queueMutex);
    for (i = holdQueue.begin(); i != holdQueue.end(); i++)
	if (i->getByte(0) == channel) {
	    tmpQueue.push_back(*i);
	    holdQueue.erase(i);
	    i--;
	}
    pthread_mutex_unlock(&queueMutex);

    // ... then transmit the moved packets
    for (i = tmpQueue.begin(); i != tmpQueue.end(); i++)
	transmit(*i);
}

void Link::
transmitWaitQueue()
{
    vector<bufferStore> tmpQueue;
    vector<bufferStore>::iterator i;

    // First, move desired packets to a temporary queue
    for (i = waitQueue.begin(); i != waitQueue.end(); i++)
	tmpQueue.push_back(*i);
    waitQueue.clear();
    // transmit the moved packets. If the backlock gets
    // full, they are put into waitQueue again.
    for (i = tmpQueue.begin(); i != tmpQueue.end(); i++)
	transmit(*i);
}

void Link::
transmit(bufferStore buf)
{
    if (hasFailed())
	return;

    int remoteChan = buf.getByte(0);
    if (xoff[remoteChan]) {
	pthread_mutex_lock(&queueMutex);
	holdQueue.push_back(buf);
	pthread_mutex_unlock(&queueMutex);
    } else {

	// If backlock is full, put on waitQueue
	int ql;
	pthread_mutex_lock(&queueMutex);
	ql = ackWaitQueue.size();
	pthread_mutex_unlock(&queueMutex);
	if (ql >= maxOutstanding) {
	    waitQueue.push_back(buf);
	    return;
	}

	ackWaitQueueElement e;
	e.seq = txSequence++;
	txSequence &= seqMask;
	gettimeofday(&e.stamp, NULL);
	// An empty buffer is considered a new link request
	if (buf.empty()) {
	    // Request for new link
	    e.txcount = 4;
	    if (verbose & LNK_DEBUG_LOG)
		lout << "Link: >> req seq=" << e.seq << endl;
	    buf.prependByte(0x20 + e.seq);
	} else {
	    e.txcount = 8;
	    if (verbose & LNK_DEBUG_LOG) {
		lout << "Link: >> dat seq=" << e.seq;
		if (verbose & LNK_DEBUG_DUMP)
		    lout << " " << buf;
		lout << endl;
	    }
	    if (e.seq > 7) {
		int hseq = e.seq >> 3;
		int lseq = 0x30 + ((e.seq & 7) | 8);
		int seq = (hseq << 8) + lseq;
		buf.prependWord(seq);
	    } else
		buf.prependByte(0x30 + e.seq);
	}
	e.data = buf;
	pthread_mutex_lock(&queueMutex);
	ackWaitQueue.push_back(e);
	pthread_mutex_unlock(&queueMutex);
	p->send(buf);
    }
}

static void
timesub(struct timeval *tv, unsigned long millisecs)
{
    uint64_t micros = tv->tv_sec;
    uint64_t sub = millisecs;

    micros <<= 32;
    micros += tv->tv_usec;
    micros -= (sub * 1000);
    tv->tv_usec = micros & 0xffffffff;
    tv->tv_sec  = (micros >>= 32) & 0xffffffff;
}

static bool
olderthan(struct timeval t1, struct timeval t2)
{
    uint64_t m1 = t1.tv_sec;
    uint64_t m2 = t2.tv_sec;
    m1 <<= 32;
    m2 <<= 32;
    m1 += t1.tv_usec;
    m2 += t2.tv_usec;
    return (m1 < m2);
}

void Link::
multiAck(struct timeval refstamp)
{
    vector<ackWaitQueueElement>::iterator i;
    pthread_mutex_lock(&queueMutex);
    for (i = ackWaitQueue.begin(); i != ackWaitQueue.end(); i++)
	if (olderthan(i->stamp, refstamp)) {
	    ackWaitQueue.erase(i);
	    i--;
	}
    pthread_mutex_unlock(&queueMutex);
}

void Link::
retransmit()
{

    if (hasFailed()) {
	purgeAllQueues();
	return;
    }

    pthread_mutex_lock(&queueMutex);
    vector<ackWaitQueueElement>::iterator i;
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval expired = now;
    timesub(&expired, retransTimeout());
    for (i = ackWaitQueue.begin(); i != ackWaitQueue.end(); i++)
	if (olderthan(i->stamp, expired)) {
	    if (i->txcount-- == 0) {
		// timeout, remove packet
		if (verbose & LNK_DEBUG_LOG)
		    lout << "Link: >> TRANSMIT timeout seq=" << i->seq << endl;
		ackWaitQueue.erase(i);
		failed = true;
		i--;
	    } else {
		// retransmit it
		i->stamp = now;
		if (verbose & LNK_DEBUG_LOG)
		    lout << "Link: >> RETRANSMIT seq=" << i->seq << endl;
		p->send(i->data);
	    }
	}
    pthread_mutex_unlock(&queueMutex);
}

void Link::
flush() {
    while (stuffToSend())
	sleep(1);
}

bool Link::
stuffToSend()
{
    return ((!failed) && (!ackWaitQueue.empty()));
}

bool Link::
hasFailed()
{
    bool lfailed = p->linkFailed();
    if (failed || lfailed) {
	if (verbose & LNK_DEBUG_LOG)
	    lout << "Link: hasFailed: " << failed << ", " << lfailed << endl;
    }
    failed |= lfailed;
    return failed;
}

Enum<Link::link_type> Link::
getLinkType()
{
    return linkType;
}

int Link::
getSpeed()
{
    return p->getSpeed();
}
