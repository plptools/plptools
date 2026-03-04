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
#ifndef _channel_h_
#define _channel_h_

#include "config.h"
#include <stdio.h>

class NCP;
class bufferStore;
class PcServer;
class ppsocket;

class channel {
public:
    channel(NCP *ncpController);
    virtual ~channel() = 0;
    void newNcpController(NCP *ncpController);

    void setNcpChannel(int chan);
    int getNcpChannel(void);
    void ncpSend(bufferStore &a);
    void setVerbose(short int _verbose);
    short int getVerbose();
    virtual void ncpDataCallback(bufferStore &a) = 0;
    virtual const char *getNcpRegisterName() = 0;
    void ncpConnect();
    void ncpRegister();
    void ncpDoRegisterAck(int ch, const char *name);
    virtual void ncpConnectAck() = 0;
    virtual void ncpConnectTerminate() = 0;
    virtual void ncpConnectNak() = 0;
    virtual void ncpRegisterAck() = 0;
    void ncpDisconnect();
    short int ncpProtocolVersion();
    const char *getNcpConnectName();
    void setNcpConnectName(const char *);

    // The following two calls are used for destructing an instance
    bool terminate(); // Mainloop will terminate this class if true
    void terminateWhenAsked();

    PcServer *ncpFindPcServer(const char *name);
    void ncpRegisterPcServer(ppsocket *skt, const char *name);
    void ncpUnregisterPcServer(PcServer *server);
    int ncpGetSpeed();

protected:
    short int verbose;
    const char *connectName;

private:
    NCP *ncpController;
    int ncpChannel;
    bool _terminate;
};

#endif
