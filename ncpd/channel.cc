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
#include <cstdlib>
#include <iostream>
#include <string>
#include <cstring>

#include "channel.h"
#include "ncp.h"

channel::channel(ncp * _ncpController)
{
    verbose = 0;
    ncpChannel = 0;
    connectName = 0;
    ncpController = _ncpController;
    _terminate = false;
}

channel::~channel()
{
    if (connectName)
        free((void *)connectName);
}

void channel::
ncpSend(bufferStore & a)
{
    ncpController->send(ncpChannel, a);
}

bool channel::
terminate()
{
    return _terminate;
}

void channel::
terminateWhenAsked()
{
    _terminate = true;
}

void channel::
ncpConnect()
{
    ncpController->connect(this);
}

void channel::
ncpRegister()
{
    ncpController->Register(this);
}

void channel::
ncpDoRegisterAck(int ch, const char *name)
{
    ncpController->RegisterAck(ch, name);
}

void channel::
ncpDisconnect()
{
    ncpController->disconnect(ncpChannel);
}

PcServer *channel::
ncpFindPcServer(const char *name)
{
    return ncpController->findPcServer(name);
}

void channel::
ncpRegisterPcServer(ppsocket *skt, const char *name)
{
    ncpController->registerPcServer(skt, name);
}

void channel::
ncpUnregisterPcServer(PcServer *server)
{
    ncpController->unregisterPcServer(server);
}

int channel::
ncpGetSpeed()
{
    return ncpController->getSpeed();
}

short int channel::
ncpProtocolVersion()
{
    return ncpController->getProtocolVersion();
}

void channel::
setNcpChannel(int chan)
{
    ncpChannel = chan;
}

int channel::
getNcpChannel()
{
    return ncpChannel;
}

void channel::
newNcpController(ncp * _ncpController)
{
    ncpController = _ncpController;
}

void channel::
setVerbose(short int _verbose)
{
    verbose = _verbose;
}

short int channel::
getVerbose()
{
    return verbose;
}

const char * channel::
getNcpConnectName()
{
    return connectName;
}

void channel::
setNcpConnectName(const char *name)
{
    if (name) {
        if (connectName)
            free((void *)connectName);
        connectName = strdup(name);
    }
}
