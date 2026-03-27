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

#include <stdio.h>
#include <stdlib.h>

#include "ncp_log.h"
#include "ncp.h"
#include "tcpsocket.h"
#include "rfsv.h"
#include "socketchannel.h"

using namespace std;

SocketChannel::SocketChannel(TCPSocket* socket, NCP* ncp)
: Channel(ncp)
, socket_(socket) {
    registerName_ = nullptr;
    connectTry_ = 0;
    isConnected_ = false;
}

SocketChannel::~SocketChannel() {
    socket_->closeSocket();
    delete socket_;
    socket_ = 0;
    if (registerName_)
        free(registerName_);
}

void SocketChannel::ncpDataCallback(BufferStore & a) {
    if (registerName_ != 0) {
        socket_->sendBufferStore(a);
    } else
        lerr << "socketchan: Connect without name!!!\n";
}

const char *SocketChannel::getNcpRegisterName() {
    return registerName_;
}

// NCP Command processing
bool SocketChannel::ncpCommand(BufferStore & a) {
    // str is guaranteed to begin with NCP$, and all NCP commands are
    // greater than or equal to 8 characters in length.
    const char *str = a.getString(4);
    bool ok = false;

    if (!strncmp(str, "INFO", 4)) {
        // Send information discovered during the receipt of the
        // NCON_MSG_NCP_INFO message.
        a.init();
        switch (ncpProtocolVersion()) {
            case PV_SERIES_3:
                a.addStringT("Series 3");
                break;
            case PV_SERIES_5:
                a.addStringT("Series 5");
                break;
            default:
                lerr << "ncpd: protocol version not known" << endl;
                a.addStringT("Unknown!");
                break;
        }
        socket_->sendBufferStore(a);
        ok = true;
    } else if (!strncmp(str, "CONN", 4)) {
        // Connect to a channel that was placed in 'pending' mode, by
        // checking the channel table against the ID...
        // DO ME LATER
        ok = true;
    } else if (!strncmp(str, "CHAL", 4)) {
        // Challenge
        // The idea is that the channel stays in 'secure' mode until a
        // valid RESP is received
        // DO ME LATER
        ok = true;
    } else if (!strncmp(str, "RESP", 4)) {
        // Reponse - here is the plaintext as sent in the CHAL - the
        // channel will only open up if this matches what ncpd has for
        // the encrypted plaintext.
        // DO ME LATER
        ok = true;
    } else if (!strncmp(str, "GSPD", 4)) {
        // Get Speed of serial device
        a.init();
        a.addByte(RFSV::E_PSI_GEN_NONE);
        a.addDWord(ncpGetSpeed());
        socket_->sendBufferStore(a);
        ok = true;
    } else if (strncmp(str, "REGS", 4) == 0) {

        // Register a server-process on the PC side.
        a.init();
        const char *name = a.getString(8);
        if (ncpFindPcServer(name))
            a.addByte(RFSV::E_PSI_FILE_EXIST);
        else {
            ncpRegisterPcServer(socket_, name);
            a.addByte(RFSV::E_PSI_GEN_NONE);
        }
        socket_->sendBufferStore(a);
        ok = true;
    }
    if (!ok) {
        lerr << "SocketChannel:: received unknown NCP command (" << a << ")" << endl;
        a.init();
        a.addByte(RFSV::E_PSI_GEN_NSUP);
        socket_->sendBufferStore(a);
    }
    return ok;
}

void SocketChannel::ncpConnectAck() {
    BufferStore a;
    a.addStringT("Ok");
    socket_->sendBufferStore(a);
    isConnected_ = true;
    connectTry_ = 3;
}

void SocketChannel::ncpConnectTerminate() {
    BufferStore a;
    a.addStringT("NAK");
    socket_->sendBufferStore(a);
    ncpDisconnect();
}

void SocketChannel::ncpRegisterAck() {
    connectTry_++;
    ncpConnect();
}

void SocketChannel::ncpConnectNak() {
    if (!registerName_ || (connectTry_ > 1))
        ncpConnectTerminate();
    else {
        connectTry_++;
        connectTryTimestamp_ = time(0);
        ncpRegister();
    }
}

void SocketChannel::socketPoll() {
    int res;

    if (registerName_ == 0) {
        BufferStore a;
        res = socket_->getBufferStore(a, false);
        switch (res) {
            case 1:
                // A client has isConnected_, and is announcing who it
                // is...  e.g.  "SYS$RFSV.*"
                //
                // An NCP Channel can be in 'Control' or 'Data' mode.
                // Initially, it is in 'Control' mode, and can accept
                // certain commands.
                //
                // When a command is received that ncpd does not
                // understand, this is assumed to be a request to
                // connect to the remote service of that name, and enter
                // 'data' mode.
                //
                // Later, there might be an explicit command to enter
                // 'data' mode, and also a challenge-response protocol
                // before any connection can be made.
                //
                // All commands begin with "NCP$".

                if (memchr(a.getString(), 0, a.getLen()) == 0) {
                    // Not 0 terminated, -> invalid
                    lerr << "ncpd: command " << a << " unrecognized."
                         << endl;
                    return;
                }

                // There is a magic process name called "NCP$INFO.*"
                // which is announced by the rfsvfactory. This causes a
                // response to be issued containing the NCP version
                // number. The rfsvfactory will create the correct type
                // of RFSV protocol handler, which will then announce
                // itself. So, first time in here, we might get the
                // NCP$INFO.*
                if (a.getLen() > 8 && !strncmp(a.getString(), "NCP$", 4)) {
                    if (!ncpCommand(a))
                        lerr << "ncpd: command " << a << " unrecognized."
                             << endl;
                    return;
                }

                // This isn't a command, it's a remote process. Connect.
                registerName_ = strdup(a.getString());
                connectTry_++;

                // If this is SYS$RFSV, we immediately connect. In all
                // other cases, we first perform a registration. Connect
                // is then triggered by RegisterAck and uses the name
                // we received from the Psion.
                connectTryTimestamp_ = time(0);
                if (strncmp(registerName_, "SYS$RFSV", 8) == 0)
                    ncpConnect();
                else
                    ncpRegister();
                break;
            case -1:
                terminateWhenAsked();
                break;
        }
    } else if (isConnected_) {
        BufferStore a;
        res = socket_->getBufferStore(a, false);
        if (res == -1) {
            ncpDisconnect();
            socket_->closeSocket();
        } else if (res == 1) {
            if (a.getLen() > 8 && !strncmp(a.getString(), "NCP$", 4)) {
                if (!ncpCommand(a))
                    lerr << "ncpd: command " << a << " unrecognized."
                         << endl;
                return;
            }
            ncpSend(a);
        }
    } else if (time(0) > (connectTryTimestamp_ + 15))
        terminateWhenAsked();
}

bool SocketChannel::isConnected() const {
    return isConnected_;
}
