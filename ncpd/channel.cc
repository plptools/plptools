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

Channel::Channel(NCP * _ncpController) {
    verbose = 0;
    ncpChannel = 0;
    connectName = 0;
    ncpController = _ncpController;
    _terminate = false;
}

Channel::~Channel() {
    if (connectName) {
        free((void *)connectName);
    }
}

void Channel::ncpSend(BufferStore & a) {
    ncpController->send(ncpChannel, a);
}

bool Channel::
shouldTerminate() const
{
    return _terminate;
}

void Channel::
terminateWhenAsked()
{
    _terminate = true;
}

void Channel::ncpConnect() {
    ncpController->connect(this);
}

void Channel::ncpRegister() {
    ncpController->Register(this);
}

void Channel::ncpDoRegisterAck(int ch, const char *name) {
    ncpController->RegisterAck(ch, name);
}

void Channel::ncpDisconnect() {
    ncpController->disconnect(ncpChannel);
}

PcServer *Channel::ncpFindPcServer(const char *name) {
    return ncpController->findPcServer(name);
}

void Channel::ncpRegisterPcServer(TCPSocket *skt, const char *name) {
    ncpController->registerPcServer(skt, name);
}

void Channel::ncpUnregisterPcServer(PcServer *server) {
    ncpController->unregisterPcServer(server);
}

int Channel::ncpGetSpeed() {
    return ncpController->getSpeed();
}

short int Channel::ncpProtocolVersion() {
    return ncpController->getProtocolVersion();
}

void Channel::setNcpChannel(int chan) {
    ncpChannel = chan;
}

int Channel::getNcpChannel() {
    return ncpChannel;
}

void Channel::newNcpController(NCP * _ncpController) {
    ncpController = _ncpController;
}

void Channel::setVerbose(short int _verbose) {
    verbose = _verbose;
}

short int Channel::getVerbose() {
    return verbose;
}

const char * Channel::getNcpConnectName() {
    return connectName;
}

void Channel::setNcpConnectName(const char *name) {
    if (name) {
        if (connectName) {
            free((void *)connectName);
        }
        connectName = strdup(name);
    }
}
