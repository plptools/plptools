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

#include <iomanip>
#include <string>
#include <cstring>

#include "bufferstore.h"
#include "bufferarray.h"

#include "linkchan.h"
#include "ncp.h"
#include "main.h"

using namespace std;

linkChan::linkChan(ncp * _ncpController, int _ncpChannel):channel(_ncpController)
{
    registerSer = 0x1234;
    if (_ncpChannel != -1)
	setNcpChannel(_ncpChannel);
    ncpConnect();
}

void linkChan::
ncpDataCallback(bufferStore & a)
{
    int len = a.getLen();
    if (verbose & LINKCHAN_DEBUG_LOG) {
	lout << "linkchan: << msg ";
	if (verbose & LINKCHAN_DEBUG_DUMP)
	    lout << a << endl;
	else
	    lout << len << endl;
    }

    if ((len >= 5) && (a.getByte(0) == 1)) {
	char srvName[20];
	unsigned int ser = a.getWord(1);
	int res = a.getWord(3);
	// int dontknow = a.getWord(5);
	bufferArray newStack;
	bufferStore se;


	strncpy(srvName, a.getString(7), 17);
	if (verbose & LINKCHAN_DEBUG_LOG)
	    lout << "linkchan: received registerAck: ser=0x" << hex << setw(4)
		 << setfill('0') << ser << " res=" << res << " srvName=\""
		 << srvName << "\"" << endl;

	while (!registerStack.empty()) {
	    se = registerStack.pop();
	    if (se.getWord(0) == ser) {
		if (verbose & LINKCHAN_DEBUG_LOG)
		    lout << "linkchan: found ser=0x" << hex << setw(4) <<
			setfill('0') << se.getWord(0) <<
			" on stack -> callBack to waiting chan" << endl;
		if (strlen(srvName) < 4)
		    strcat(srvName, ".*");
		ncpDoRegisterAck((int)se.getWord(2), srvName);
	    } else
		newStack += se;
	}
	registerStack = newStack;
	return;
    }
    lerr << "linkchan: unknown message " << a.getByte(0) << endl;
}

const char *linkChan::
getNcpRegisterName()
{
    return "LINK";
}

void linkChan::
ncpConnectAck()
{
    if (verbose & LINKCHAN_DEBUG_LOG)
	lout << "linkchan: << cack" << endl;
}

void linkChan::
ncpConnectTerminate()
{
    if (verbose & LINKCHAN_DEBUG_LOG)
	lout << "linkchan: << ctrm" << endl;
    terminateWhenAsked();
}

void linkChan::
ncpConnectNak()
{
    ncpConnectTerminate();
}

void linkChan::
Register(channel *ch)
{
    bufferStore a;
    bufferStore stack;

    stack.addWord(registerSer);
    stack.addWord(ch->getNcpChannel());
    registerStack += stack;
    a.addByte(0);
    a.addWord(registerSer++);
    a.addString(ch->getNcpRegisterName());
    a.addByte(0);
    ncpSend(a);
}
