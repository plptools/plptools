/*
 * This file is part of plptools.
 *
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
#ifndef _WPRT_H_
#define _WPRT_H_

#include "rfsv.h"
#include "Enum.h"

class ppsocket;
class bufferStore;
class bufferArray;

/**
 * Remote Print services via PLP
 *
 */
class wprt {
public:
    wprt(ppsocket *);

    /**
    * Virtual destructor.
    */
    ~wprt();

    /**
    * Initializes a connection to the remote
    * machine.
    */
    void reset();

    /**
    * Attempts to re-establish a remote
    * connection by first closing the socket,
    * then connecting again to the ncpd daemon
    * and finally calling @ref reset.
    */
    void reconnect();

    /**
    * Retrieves the current status of the
    * connection.
    *
    * @returns The connection status.
    */
    Enum<rfsv::errs> getStatus();

    /**
    * Get Print Data
    */
    Enum<rfsv::errs> getData(bufferStore &buf);

    /**
    * Init Printer
    */
    Enum<rfsv::errs> initPrinter();

    /**
    * Cancels a running job.
    */
    Enum<rfsv::errs> cancelJob();

    /**
    * Stops the WPRT server.
    */
    bool stop();

protected:
    /**
    * The possible commands.
    */
    enum commands {
	WPRT_INIT    = 0x00,
	WPRT_GET     = 0xf0,
	WPRT_CANCEL  = 0xf1,
	WPRT_STOP    = 0xff
    };

    /**
    * The socket, used for communication
    * with ncpd.
    */
    ppsocket *skt;

    /**
    * The current status of the connection.
    */
    Enum<rfsv::errs> status;

   /**
    * Sends a command to the remote side.
    *
    * If communication fails, a reconnect is triggered
    * and a second attempt to transmit the request
    * is attempted. If that second attempt fails,
    * the function returns an error an sets rpcs::status
    * to E_PSI_FILE_DISC.
    *
    * @param cc The command to execute on the remote side.
    * @param data Additional data for this command.
    *
    * @returns true on success, false on failure.
    */
    bool sendCommand(enum commands cc, bufferStore &data);

    Enum<rfsv::errs> getResponse(bufferStore &data);
    const char *getConnectName();

};

#endif
