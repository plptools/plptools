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

#include <iostream>
#include <string>

#include <time.h>

#include "bufferstore.h"
#include "bufferarray.h"

#include "rfsv.h"
#include "ncp.h"
#include "linkchan.h"
#include "link.h"
#include "main.h"

#define MAX_CHANNELS_PSION 256
#define MAX_CHANNELS_SIBO  8
#define NCP_SENDLEN 250

using namespace std;

ncp::ncp(const char *fname, int baud, unsigned short _verbose)
{
    channelPtr = new channel*[MAX_CHANNELS_PSION + 1];
    assert(channelPtr);
    messageList = new bufferStore[MAX_CHANNELS_PSION + 1];
    assert(messageList);
    remoteChanList = new int[MAX_CHANNELS_PSION + 1];
    assert(remoteChanList);

    failed = false;
    verbose = _verbose;

    // until detected on receipt of INFO we use these.
    maxChannels = MAX_CHANNELS_SIBO;
    protocolVersion = PV_SERIES_5;
    lChan = NULL;

    // init channels
    for (int i = 0; i < MAX_CHANNELS_PSION; i++)
        channelPtr[i] = NULL;

    l = new Link(fname, baud, this, verbose);
    assert(l);
}

ncp::~ncp()
{
    bufferStore b;
    for (int i = 0; i < maxLinks(); i++) {
        if (isValidChannel(i)) {
            bufferStore b2;
            b2.addByte(remoteChanList[i]);
            controlChannel(i, NCON_MSG_CHANNEL_DISCONNECT, b2);
        }
        channelPtr[i] = NULL;
    }
    controlChannel(0, NCON_MSG_NCP_END, b);
    delete l;
    delete [] channelPtr;
    delete [] remoteChanList;
    delete [] messageList;
}

int ncp::
maxLinks() {
    return maxChannels;
}

void ncp::
reset() {
    for (int i = 0; i < maxLinks(); i++) {
        if (isValidChannel(i))
            channelPtr[i]->terminateWhenAsked();
        channelPtr[i] = NULL;
    }
    failed = false;
    if (lChan)
        delete(lChan);
    lChan = NULL;
    protocolVersion = PV_SERIES_5; // until detected on receipt of INFO
    l->reset();
}

unsigned short ncp::
getVerbose()
{
    return verbose;
}

void ncp::
setVerbose(unsigned short _verbose)
{
    verbose = _verbose;
    l->setVerbose(verbose);
}

short int ncp::
getProtocolVersion()
{
    return protocolVersion;
}

void ncp::
receive(bufferStore s) {
    if (s.getLen() > 1) {
        int channel = s.getByte(0);
        s.discardFirstBytes(1);
        if (channel == 0) {
            decodeControlMessage(s);
        } else {
            int allData = s.getByte(1);
            s.discardFirstBytes(2);

            if (protocolVersion == PV_SERIES_3) {
                channel = lastSentChannel;
            }

            if (!isValidChannel(channel)) {
                lerr << "ncp: Got message for unknown channel\n";
            } else {
                messageList[channel].addBuff(s);
                if (allData == LAST_MESS) {
                    channelPtr[channel]->ncpDataCallback(messageList[channel]);
                    messageList[channel].init();
                } else if (allData != NOT_LAST_MESS) {
                    lerr << "ncp: bizarre third byte!\n";
                }
            }
        }
    } else
        lerr << "Got null message\n";
}

void ncp::
controlChannel(int chan, enum interControllerMessageType t, bufferStore & command)
{
    bufferStore open;
    open.addByte(0);  // control

    open.addByte(chan);
    open.addByte(t);
    open.addBuff(command);
    if (verbose & NCP_DEBUG_LOG)
        lout << "ncp: >> " << ctrlMsgName(t) << " " << chan << endl;
    l->send(open);
}

PcServer *ncp::
findPcServer(const char *name)
{
    if (name) {
        vector<PcServer>::iterator i;
        for (i = pcServers.begin(); i != pcServers.end(); i++)
            if (i->getName() == name)
                    return i->self();
    }
    return NULL;
}

void ncp::
registerPcServer(ppsocket *skt, const char *name) {
    pcServers.push_back(PcServer(skt, name));
}

void ncp::
unregisterPcServer(PcServer *server) {
    if (server) {
        vector<PcServer>::iterator i;
        for (i = pcServers.begin(); i != pcServers.end(); i++)
            if (i->self() == server) {
                pcServers.erase(i);
                return;
            }
    }
}

void ncp::
decodeControlMessage(bufferStore & buff)
{
    int remoteChan = buff.getByte(0);

    interControllerMessageType imt = (interControllerMessageType)buff.getByte(1);
    buff.discardFirstBytes(2);
    if (verbose & NCP_DEBUG_LOG)
        lout << "ncp: << " << ctrlMsgName(imt) << " " << remoteChan;

    bufferStore b;
    int localChan;

    switch (imt) {
        case NCON_MSG_CONNECT_TO_SERVER:
            if (verbose & NCP_DEBUG_LOG) {
                if (verbose & NCP_DEBUG_DUMP)
                    lout << " [" << buff << "]";
                lout << endl;
            }

            failed = false;
            if (!strcmp(buff.getString(0), "LINK.*")) {
                if (lChan)
                    localChan = lChan->getNcpChannel();
                else
                    localChan = getFirstUnusedChan();

                // Ack with connect response
                b.addByte(remoteChan);
                b.addByte(0);
                controlChannel(localChan, NCON_MSG_CONNECT_RESPONSE, b);
                if (verbose & NCP_DEBUG_LOG)
                    lout << "ncp: Link UP" << endl;
                linf << _("Connected with a S")
                     << ((protocolVersion == PV_SERIES_5) ? 5 : 3) << _(" at ")
                     << getSpeed() << _("baud") << endl;
                // Create linkchan if it does not yet exist
                if (!lChan) {
                    if (verbose & NCP_DEBUG_LOG)
                        lout << "ncp: new passive linkChan" << endl;
                    channelPtr[localChan] =
                        lChan = new linkChan(this, localChan);
                    lChan->setVerbose(verbose);
                }
                lChan->ncpConnectAck();
            } else {
                PcServer *s = findPcServer(buff.getString(0));
                bool ok = false;

                if (s) {
                    localChan = getFirstUnusedChan();
                    ok = s->clientConnect(localChan, remoteChan);
                    if (!ok)
                        // release channel ptr
                        channelPtr[localChan] = NULL;
                }
                b.addByte(remoteChan);
                if (ok) {
                    b.addByte(rfsv::E_PSI_GEN_NONE);
                    if (verbose & NCP_DEBUG_LOG)
                        lout << "ncp: ACCEPT client connect" << endl;
                } else {
                    localChan = 0;
                    b.addByte(rfsv::E_PSI_FILE_NXIST);
                    if (verbose & NCP_DEBUG_LOG)
                        lout << "ncp: REJECT client connect" << endl;
                }
                controlChannel(localChan, NCON_MSG_CONNECT_RESPONSE, b);

                // Create linkchan if it does not yet exist
                if (!lChan) {
                    if (verbose & NCP_DEBUG_LOG)
                        lout << "ncp: new active linkChan" << endl;
                    channelPtr[localChan] =
                        lChan = new linkChan(this, -1);
                    lChan->setVerbose(verbose);
                }

            }
            break;

        case NCON_MSG_CONNECT_RESPONSE:

            int forChan;

            failed = false;
            forChan = buff.getByte(0);
            if (verbose & NCP_DEBUG_LOG)
                lout << " ch=" << forChan << " stat=";
            if (buff.getByte(1) == 0) {
                if (verbose & NCP_DEBUG_LOG)
                    lout << "OK" << endl;
                if (isValidChannel(forChan)) {
                    remoteChanList[forChan] = remoteChan;
                    channelPtr[forChan]->ncpConnectAck();
                } else {
                    if (verbose & NCP_DEBUG_LOG)
                        lout << "ncp: message for unknown channel" << endl;
                }
            } else {
                if (verbose & NCP_DEBUG_LOG)
                    lout << "Unknown " << (int) buff.getByte(1) << endl;
                if (isValidChannel(forChan))
                    channelPtr[forChan]->ncpConnectNak();
            }
            break;

        case NCON_MSG_NCP_INFO:

            int ver;

            failed = false;
            ver = buff.getByte(0);
            // Series 3c returns '3', as does mclink. PsiWin 1.1
            // returns version 2. We return whatever version we're
            // sent, which is rather crude, but works for Series 3
            // and probably 5. If Symbian have changed EPOC Connect
            // for the Series 5mx/7, this may need to change.
            //
            if (ver == PV_SERIES_5 || ver == PV_SERIES_3) {
                bufferStore b;
                protocolVersion = ver;
                if (verbose & NCP_DEBUG_LOG) {
                    if (verbose & NCP_DEBUG_DUMP)
                        lout << " [" << buff << "]";
                    lout << endl;
                }
                // Fake NCP version 2 for a Series 3 (behave like PsiWin 1.1)
                if (ver == PV_SERIES_3) {
                    ver = 2;
                } else {
                    // Series 5 supports more channels
                    maxChannels = MAX_CHANNELS_PSION;
                }
                b.addByte(ver);
                // Do we send a time of 0 or a real time?
                // The Psion uses this to determine whether to
                // restart. (See protocol docs for details)
                b.addDWord(time(NULL));
                controlChannel(0, NCON_MSG_NCP_INFO, b);
            } else {
                lout << "ALERT!!!! Unexpected Protocol Version!! (Not Series 3/5?)!" << endl;
                failed = true;
            }
            break;

        case NCON_MSG_CHANNEL_DISCONNECT:
            if (verbose & NCP_DEBUG_LOG)
                lout << " ch=" << (int) buff.getByte(0) << endl;
            disconnect(buff.getByte(0));
            l->purgeQueue(remoteChan);
            break;

        case NCON_MSG_DATA_XOFF:
        case NCON_MSG_DATA_XON:
        case NCON_MSG_CHANNEL_CLOSED:
        case NCON_MSG_NCP_END:
        default:
            if (verbose & NCP_DEBUG_LOG) {
                if (verbose & NCP_DEBUG_DUMP)
                    lout << " [" << buff << "]";
                lout << endl;
            }

    }
}

int ncp::
getFirstUnusedChan()
{
    for (int cNum = 1; cNum < maxLinks(); cNum++) {
        if (channelPtr[cNum] == NULL) {
            if (verbose & NCP_DEBUG_LOG)
                lout << "ncp: getFirstUnusedChan=" << cNum << endl;
            channelPtr[cNum] = (channel *)0xdeadbeef;
            return cNum;
        }
    }
    return 0;
}

bool ncp::
isValidChannel(int channel)
{
    return (channelPtr[channel] && ((long)channelPtr[channel] != 0xdeadbeef));
}

void ncp::
RegisterAck(int chan, const char *name)
{
    if (verbose & NCP_DEBUG_LOG)
        lout << "ncp: RegisterAck: chan=" << chan << endl;
    for (int cNum = 1; cNum < maxLinks(); cNum++) {
        channel *ch = channelPtr[cNum];
        if (isValidChannel(cNum) && ch->getNcpChannel() == chan) {
            ch->setNcpConnectName(name);
            ch->ncpRegisterAck();
            return;
        }
    }
    lerr << "ncp: RegisterAck: no channel to deliver" << endl;
}

void ncp::
Register(channel * ch)
{
    if (lChan) {
        int cNum = ch->getNcpChannel();
        if (cNum == 0)
            cNum = getFirstUnusedChan();
        if (cNum > 0) {
            channelPtr[cNum] = ch;
            ch->setNcpChannel(cNum);
            lChan->Register(ch);
        } else
            lerr << "ncp: Out of channels in register" << endl;
    } else
        lerr << "ncp: Register without established lChan" << endl;
}

int ncp::
connect(channel * ch)
{
    // look for first unused chan

    int cNum = ch->getNcpChannel();
    if (cNum == 0)
        cNum = getFirstUnusedChan();
    if (cNum > 0) {
        channelPtr[cNum] = ch;
        ch->setNcpChannel(cNum);
        bufferStore b;
        if (ch->getNcpConnectName())
            b.addString(ch->getNcpConnectName());
        else {
            b.addString(ch->getNcpRegisterName());
            b.addString(".*");
        }
        b.addByte(0);
        controlChannel(cNum, NCON_MSG_CONNECT_TO_SERVER, b);
        return cNum;
    }
    return -1;
}

void ncp::
send(int channel, bufferStore & a)
{
    bool last;
    do {
        last = true;

        if (a.getLen() > NCP_SENDLEN)
            last = false;

        bufferStore out;
        out.addByte(remoteChanList[channel]);
        out.addByte(channel);

        if (last) {
            out.addByte(LAST_MESS);
        } else {
            out.addByte(NOT_LAST_MESS);
        }

        out.addBuff(a, NCP_SENDLEN);
        a.discardFirstBytes(NCP_SENDLEN);
        l->send(out);
    } while (!last);
    lastSentChannel = channel;
}

void ncp::
disconnect(int channel)
{
    if (!isValidChannel(channel)) {
        lerr << "ncp: Ignored disconnect for unknown channel #" << channel << endl;
        return;
    }
    channelPtr[channel]->terminateWhenAsked();
    if (verbose & NCP_DEBUG_LOG)
        lout << "ncp: disconnect: channel=" << channel << endl;
    channelPtr[channel] = NULL;
    bufferStore b;
    b.addByte(remoteChanList[channel]);
    controlChannel(channel, NCON_MSG_CHANNEL_DISCONNECT, b);
}

bool ncp::
stuffToSend()
{
    return l->stuffToSend();
}

bool ncp::
hasFailed()
{
    bool lfailed = l->hasFailed();
    if (failed || lfailed) {
        if (verbose & NCP_DEBUG_LOG)
            lout << "ncp: hasFailed: " << failed << ", " << lfailed << endl;
    }
    failed |= lfailed;
    if (failed) {
        if (lChan) {
            channelPtr[lChan->getNcpChannel()] = NULL;
            delete lChan;
        }
        lChan = NULL;
    }
    return failed;
}

bool ncp::
gotLinkChannel()
{
    return (lChan != NULL);
}

int ncp::
getSpeed()
{
    return l->getSpeed();
}

const char *ncp::
ctrlMsgName(unsigned char msgType)
{
    switch (msgType) {
        case NCON_MSG_DATA_XOFF:
            return "NCON_MSG_DATA_XOFF";
        case NCON_MSG_DATA_XON:
            return "NCON_MSG_DATA_XON";
        case NCON_MSG_CONNECT_TO_SERVER:
            return "NCON_MSG_CONNECT_TO_SERVER";
        case NCON_MSG_CONNECT_RESPONSE:
            return "NCON_MSG_CONNECT_RESPONSE";
        case NCON_MSG_CHANNEL_CLOSED:
            return "NCON_MSG_CHANNEL_CLOSED";
        case NCON_MSG_NCP_INFO:
            return "NCON_MSG_NCP_INFO";
        case NCON_MSG_CHANNEL_DISCONNECT:
            return "NCON_MSG_CHANNEL_DISCONNECT";
        case NCON_MSG_NCP_END:
            return "NCON_MSG_NCP_END";
    }
    return "NCON_MSG_UNKNOWN";
}
