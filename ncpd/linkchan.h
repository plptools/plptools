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
#ifndef _linkchan_h_
#define _linkchan_h_

#include "channel.h"
#include "bufferarray.h"

#define LINKCHAN_DEBUG_LOG  1
#define LINKCHAN_DEBUG_DUMP 2

class linkChan : public channel {
public:
    linkChan(ncp *ncpController, int ncpChannel = -1);

    void ncpDataCallback(bufferStore &a);
    const char *getNcpRegisterName();
    void ncpConnectAck();
    void ncpConnectTerminate();
    void ncpConnectNak();
    void ncpRegisterAck() {}
    void Register(channel *);
private:
    int registerSer;
    bufferArray registerStack;
};
#endif
