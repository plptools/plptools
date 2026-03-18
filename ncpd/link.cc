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
#include "ncp_log.h"
#include "ncp.h"
#include "datalink.h"

extern "C" {

    static void *expire_check(void *arg) {
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

Link::Link(const char *fname, int baud, NCP *ncp, unsigned short verbose, const int cancellationFd)
: ncp_(ncp)
, verbose_(verbose) {
    failed_ = false;
    linkType_ = LINK_TYPE_UNKNOWN;
    for (int i = 0; i < 256; i++)
        xoff[i] = false;
    // generate magic number for sendReqCon()
    srandom(time(NULL));
    conMagic_ = random();

    dataLink_ = new DataLink(fname, baud, this, verbose, cancellationFd);

    pthread_mutex_init(&queueMutex_, NULL);
    pthread_create(&checkThreadId_, NULL, expire_check, this);

    // submit a link request
    sendReqReq();
}

Link::~Link() {
    pthread_cancel(checkThreadId_);
    pthread_join(checkThreadId_, NULL);
    pthread_mutex_destroy(&queueMutex_);
    delete dataLink_;
}

unsigned long Link::retransTimeout() {
    return ((unsigned long)getSpeed() * 1000 / 13200) + 200;
}

void Link::reset() {
    txSequence_ = 1;
    rxSequence_ = -1;
    failed_ = false;
    seqMask_ = 7;
    maxOutstanding_ = 1;
    linkType_ = LINK_TYPE_UNKNOWN;
    purgeAllQueues();
    for (int i = 0; i < 256; i++)
        xoff[i] = false;
    dataLink_->reset();
    // submit a link request
    sendReqReq();
}

void Link::send(const BufferStore & buff) {
    if (buff.getLen() > 300) {
        failed_ = true;
    } else {
        transmit(buff);
    }
}

void Link::purgeAllQueues() {
    pthread_mutex_lock(&queueMutex_);
    ackWaitQueue.clear();
    holdQueue_.clear();
    pthread_mutex_unlock(&queueMutex_);
}

void Link::purgeQueue(int channel) {
    pthread_mutex_lock(&queueMutex_);
    vector<AckWaitQueueElement>::iterator i;
    for (i = ackWaitQueue.begin(); i != ackWaitQueue.end(); i++)
        if (i->data.getByte(0) == channel) {
            ackWaitQueue.erase(i);
            i--;
        }
    vector<BufferStore>::iterator j;
    for (j = holdQueue_.begin(); j != holdQueue_.end(); j++)
        if (j->getByte(0) == channel) {
            holdQueue_.erase(j);
            j--;
        }
    pthread_mutex_unlock(&queueMutex_);
}

void Link::sendAck(int seq) {
    if (hasFailed())
        return;
    BufferStore tmp;
    if (verbose_ & LNK_DEBUG_LOG)
        lout << "Link: >> ack seq=" << seq << endl;
    if (seq > 7) {
        int hseq = seq >> 3;
        int lseq = (seq & 7) | 8;
        seq = (hseq << 8) + lseq;
        tmp.prependWord(seq);
    } else {
        tmp.prependByte(seq);
    }
    dataLink_->send(tmp, isEPOC_);
}

void Link::sendReqCon() {
    if (hasFailed())
        return;
    BufferStore tmp;
    if (verbose_ & LNK_DEBUG_LOG)
        lout << "Link: >> con seq=4" << endl;
    tmp.addByte(0x24);
    tmp.addDWord(conMagic_);
    AckWaitQueueElement e;
    e.seq = 0; // expected ACK is 0, _NOT_ 4!
    gettimeofday(&e.stamp, NULL);
    e.data = tmp;
    e.txcount = 4;
    pthread_mutex_lock(&queueMutex_);
    ackWaitQueue.push_back(e);
    pthread_mutex_unlock(&queueMutex_);
    dataLink_->send(tmp, isEPOC_);
}

void Link::sendReqReq() {
    if (hasFailed())
        return;
    BufferStore tmp;
    if (verbose_ & LNK_DEBUG_LOG)
        lout << "Link: >> con seq=1" << endl;
    tmp.addByte(0x21);
    AckWaitQueueElement e;
    e.seq = 0; // expected response is Ack with seq=0 or ReqCon
    gettimeofday(&e.stamp, NULL);
    e.data = tmp;
    e.txcount = 4;
    pthread_mutex_lock(&queueMutex_);
    ackWaitQueue.push_back(e);
    pthread_mutex_unlock(&queueMutex_);
    dataLink_->send(tmp, isEPOC_);
}

void Link::sendReq() {
    if (hasFailed())
        return;
    BufferStore tmp;
    if (verbose_ & LNK_DEBUG_LOG)
        lout << "Link: >> con seq=1" << endl;
    tmp.addByte(0x20);
    // No Ack expected for this, so no new entry in ackWaitQueue
    dataLink_->send(tmp, isEPOC_);
}

// This is called on the write queue's thread.
void Link::receive(BufferStore buff) {
    if (!dataLink_) {
        return;
    }

    vector<AckWaitQueueElement>::iterator i;
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
            if (verbose_ & LNK_DEBUG_LOG) {
                lout << "Link: << dat seq=" << seq ;
                if (verbose_ & LNK_DEBUG_DUMP)
                    lout << " " << buff << endl;
                else
                    lout << " len=" << buff.getLen() << endl;
            }

            if (((rxSequence_ + 1) & seqMask_) == seq) {
                rxSequence_++;
                rxSequence_ &= seqMask_;

                    sendAck(rxSequence_);
                // Must check for XOFF/XON ncp frames HERE!
                if ((buff.getLen() == 3) && (buff.getByte(0) == 0)) {
                    switch (buff.getByte(2)) {
                        case 1:
                            // XOFF
                            xoff[buff.getByte(1)] = true;
                            if (verbose_ & LNK_DEBUG_LOG)
                                lout << "Link: got XOFF for channel "
                                     << buff.getByte(1) << endl;
                            break;
                        case 2:
                            // XON
                            xoff[buff.getByte(1)] = false;
                            if (verbose_ & LNK_DEBUG_LOG)
                                lout << "Link: got XON for channel "
                                     << buff.getByte(1) << endl;
                            // Transmit packets on hold queue
                            transmitHoldQueue(buff.getByte(1));
                            break;
                        default:
                            ncp_->receive(buff);
                    }
                } else {
                    ncp_->receive(buff);
                }
            } else {
                sendAck(rxSequence_);
                if (verbose_ & LNK_DEBUG_LOG)
                    lout << "Link: DUP\n";
            }
            break;

        case 0x00:
            // Incoming ack
            // Find corresponding packet in ackWaitQueue
            ackFound = false;
            struct timeval refstamp;
            pthread_mutex_lock(&queueMutex_);
            for (i = ackWaitQueue.begin(); i != ackWaitQueue.end(); i++)
                if (i->seq == seq) {
                    ackFound = true;
                    refstamp = i->stamp;
                    ackWaitQueue.erase(i);
                    if (verbose_ & LNK_DEBUG_LOG) {
                        lout << "Link: << ack seq=" << seq ;
                        if (verbose_ & LNK_DEBUG_DUMP)
                            lout << " " << buff;
                        lout << endl;
                    }
                    break;
                }
            pthread_mutex_unlock(&queueMutex_);
            if (ackFound) {
                if ((linkType_ == LINK_TYPE_UNKNOWN) && (seq == 0)) {
                    // If the remote device runs SIBO protocol, this ACK
                    // should be 0 (the Ack on our ReqReq request, which is
                    // treated as a normal Req by the SIBO machine.
                    failed_ = false;
                    linkType_ = LINK_TYPE_SIBO;
                    seqMask_ = 7;
                    maxOutstanding_ = 1;
                    rxSequence_ = 0;
                    txSequence_ = 1;
                    purgeAllQueues();
                    isEPOC_ = false;
                    if (verbose_ & LNK_DEBUG_LOG)
                        lout << "Link: 1-linkType set to " << linkType_ << endl;
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
                pthread_mutex_lock(&queueMutex_);
                struct timeval now;
                gettimeofday(&now, NULL);
                bool nextFound = false;
                for (i = ackWaitQueue.begin(); i != ackWaitQueue.end(); i++)
                    if (i->seq == seq+1) {
                        nextFound = true;
                        if (i->txcount-- == 0) {
                            // timeout, remove packet
                            if (verbose_ & LNK_DEBUG_LOG)
                                lout << "Link: >> TRANSMIT timeout seq=" <<
                                    i->seq << endl;
                            ackWaitQueue.erase(i);
                            i--;
                        } else {
                            // retransmit it
                            i->stamp = now;
                            if (verbose_ & LNK_DEBUG_LOG)
                                lout << "Link: >> RETRANSMIT seq=" << i->seq
                                     << endl;
                            dataLink_->send(i->data, isEPOC_);
                        }
                        break;
                    }
                pthread_mutex_unlock(&queueMutex_);
                if ((verbose_ & LNK_DEBUG_LOG) && (!nextFound)) {
                    lout << "Link: << UNMATCHED ack seq=" << seq;
                    if (verbose_ & LNK_DEBUG_DUMP)
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
                pthread_mutex_lock(&queueMutex_);
                for (i = ackWaitQueue.begin(); i != ackWaitQueue.end(); i++)
                    if ((i->seq == 0) && (i->data.getByte(0) == 0x21)) {
                        ackWaitQueue.erase(i);
                        linkType_ = LINK_TYPE_EPOC;
                        if (verbose_ & LNK_DEBUG_LOG)
                            lout << "Link: 2-linkType set to " << linkType_ << endl;
                        conFound = true;
                        failed_ = false;
                        // EPOC can handle extended sequence numbers
                        seqMask_ = 0x7ff;
                        // EPOC can handle up to 8 unacknowledged packets
                        maxOutstanding_ = 8;
                        isEPOC_ = true;
                        if (verbose_ & LNK_DEBUG_LOG) {
                            lout << "Link: << con seq=" << seq ;
                            if (verbose_ & LNK_DEBUG_DUMP)
                                lout << " " << buff;
                            lout << endl;
                        }
                        break;
                    }
                pthread_mutex_unlock(&queueMutex_);
            }
            if (conFound) {
                rxSequence_ = 0;
                txSequence_ = 1;
                sendAck(rxSequence_);
            } else {
                if (verbose_ & LNK_DEBUG_LOG) {
                    lout << "Link: << req seq=" << seq;
                    if (verbose_ & LNK_DEBUG_DUMP)
                        lout << " " << buff;
                    lout << endl;
                }
                rxSequence_ = txSequence_ = 0;
                if (seq > 0) {
                    linkType_ = LINK_TYPE_EPOC;
                    if (verbose_ & LNK_DEBUG_LOG)
                        lout << "Link: 3-linkType set to " << linkType_ << endl;
                    // EPOC can handle extended sequence numbers
                    seqMask_ = 0x7ff;
                    // EPOC can handle up to 8 unacknowledged packets
                    maxOutstanding_ = 8;
                    isEPOC_ = true;
                    failed_ = false;
                    sendReqCon();
                } else {
                    // SIBO
                    linkType_ = LINK_TYPE_SIBO;
                    failed_ = false;
                    seqMask_ = 7;
                    maxOutstanding_ = 1;
                    if (verbose_ & LNK_DEBUG_LOG)
                        lout << "Link: 4-linkType set to " << linkType_ << endl;
                    rxSequence_ = 0;
                    txSequence_ = 1; // Our ReqReq was seq 0
                    purgeAllQueues();
                    isEPOC_ = false;
                    sendAck(rxSequence_);
                }
            }
            break;

        case 0x10:
            // Disconnect
            if (verbose_ & LNK_DEBUG_LOG)
                lout << "Link: << DISC" << endl;
            failed_ = true;
            break;

        default:
            lerr << "Link: FATAL: Unknown packet type " << type << endl;
    }
}

void Link::transmitHoldQueue(int channel) {
    vector<BufferStore> tmpQueue;
    vector<BufferStore>::iterator i;

    // First, move desired packets to a temporary queue
    pthread_mutex_lock(&queueMutex_);
    for (i = holdQueue_.begin(); i != holdQueue_.end(); i++)
        if (i->getByte(0) == channel) {
            tmpQueue.push_back(*i);
            holdQueue_.erase(i);
            i--;
        }
    pthread_mutex_unlock(&queueMutex_);

    // ... then transmit the moved packets
    for (i = tmpQueue.begin(); i != tmpQueue.end(); i++)
        transmit(*i);
}

void Link::transmitWaitQueue() {
    vector<BufferStore> tmpQueue;
    vector<BufferStore>::iterator i;

    // First, move desired packets to a temporary queue
    for (i = waitQueue_.begin(); i != waitQueue_.end(); i++)
        tmpQueue.push_back(*i);
    waitQueue_.clear();
    // transmit the moved packets. If the backlock gets
    // full, they are put into waitQueue again.
    for (i = tmpQueue.begin(); i != tmpQueue.end(); i++)
        transmit(*i);
}

void Link::transmit(BufferStore buf) {
    if (hasFailed())
        return;

    int remoteChan = buf.getByte(0);
    if (xoff[remoteChan]) {
        pthread_mutex_lock(&queueMutex_);
        holdQueue_.push_back(buf);
        pthread_mutex_unlock(&queueMutex_);
    } else {

        // If backlock is full, put on waitQueue
        int ql;
        pthread_mutex_lock(&queueMutex_);
        ql = ackWaitQueue.size();
        pthread_mutex_unlock(&queueMutex_);
        if (ql >= maxOutstanding_) {
            waitQueue_.push_back(buf);
            return;
        }

        AckWaitQueueElement e;
        e.seq = txSequence_++;
        txSequence_ &= seqMask_;
        gettimeofday(&e.stamp, NULL);
        // An empty buffer is considered a new link request
        if (buf.empty()) {
            // Request for new link
            e.txcount = 4;
            if (verbose_ & LNK_DEBUG_LOG)
                lout << "Link: >> req seq=" << e.seq << endl;
            buf.prependByte(0x20 + e.seq);
        } else {
            e.txcount = 8;
            if (verbose_ & LNK_DEBUG_LOG) {
                lout << "Link: >> dat seq=" << e.seq;
                if (verbose_ & LNK_DEBUG_DUMP)
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
        pthread_mutex_lock(&queueMutex_);
        ackWaitQueue.push_back(e);
        pthread_mutex_unlock(&queueMutex_);
        dataLink_->send(buf, isEPOC_);
    }
}

static void timesub(struct timeval *tv, unsigned long millisecs) {
    uint64_t micros = tv->tv_sec;
    uint64_t sub = millisecs;

    micros <<= 32;
    micros += tv->tv_usec;
    micros -= (sub * 1000);
    tv->tv_usec = micros & 0xffffffff;
    tv->tv_sec  = (micros >>= 32) & 0xffffffff;
}

static bool olderthan(struct timeval t1, struct timeval t2) {
    uint64_t m1 = t1.tv_sec;
    uint64_t m2 = t2.tv_sec;
    m1 <<= 32;
    m2 <<= 32;
    m1 += t1.tv_usec;
    m2 += t2.tv_usec;
    return (m1 < m2);
}

void Link::multiAck(struct timeval refstamp) {
    vector<AckWaitQueueElement>::iterator i;
    pthread_mutex_lock(&queueMutex_);
    for (i = ackWaitQueue.begin(); i != ackWaitQueue.end(); i++)
        if (olderthan(i->stamp, refstamp)) {
            ackWaitQueue.erase(i);
            i--;
        }
    pthread_mutex_unlock(&queueMutex_);
}

// Called from the retransmit thread.
void Link::retransmit() {

    if (hasFailed()) {
        purgeAllQueues();
        return;
    }

    pthread_mutex_lock(&queueMutex_);
    vector<AckWaitQueueElement>::iterator i;
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval expired = now;
    timesub(&expired, retransTimeout());
    for (i = ackWaitQueue.begin(); i != ackWaitQueue.end(); i++)
        if (olderthan(i->stamp, expired)) {
            if (i->txcount-- == 0) {
                // timeout, remove packet
                if (verbose_ & LNK_DEBUG_LOG)
                    lout << "Link: >> TRANSMIT timeout seq=" << i->seq << endl;
                ackWaitQueue.erase(i);
                failed_ = true;
                i--;
            } else {
                // retransmit it
                i->stamp = now;
                if (verbose_ & LNK_DEBUG_LOG)
                    lout << "Link: >> RETRANSMIT seq=" << i->seq << endl;
                dataLink_->send(i->data, isEPOC_);
            }
        }
    pthread_mutex_unlock(&queueMutex_);
}

bool Link::stuffToSend() {
    bool result;
    pthread_mutex_lock(&queueMutex_);
    result = ((!failed_) && (!ackWaitQueue.empty()));
    pthread_mutex_unlock(&queueMutex_);
    return result;
}

bool Link::hasFailed() {
    bool lfailed = dataLink_->linkFailed();
    if (failed_ || lfailed) {
        if (verbose_ & LNK_DEBUG_LOG)
            lout << "Link: hasFailed: " << failed_ << ", " << lfailed << endl;
    }
    failed_ |= lfailed;
    return failed_;
}

Enum<Link::link_type> Link::getLinkType() {
    return linkType_;
}

int Link::getSpeed() {
    return dataLink_->getSpeed();
}
