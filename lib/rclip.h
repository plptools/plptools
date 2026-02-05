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
#ifndef _RCLIP_H_
#define _RCLIP_H_

#include "rfsv.h"
#include "Enum.h"

class ppsocket;
class bufferStore;
class bufferArray;

/**
 * Remote ClipBoard services via PLP
 *
 * This class implements access to the remote clipboard notification
 * feature of the Psion. The Psion uses a file C:\System\Data\Clpboard.cbd
 * for storing the content of its clipboard. This file can be accessed like
 * any other regular file on the Psion using the @ref rfsv implementation.
 * This class handles notification about changes of this file.
 * There are two methods of notification implemented. Using @ref waitNotify ,
 * a blocking method can be used and using @ref sendListen followed by
 * @ref checkNotify , a polling approach (usable for GUI programs) can
 * be implemented.
 */
class rclip {
public:
    /**
    * Constructs a new rclip object.
    *
    * @param skt The socket to be used by this object.
    */
    rclip(ppsocket *skt);

    /**
    * destructor.
    */
    ~rclip();

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
    * Send initialization frame.
    *
    * Must be called once after a new rclip object has
    * be called. It sends an initialzation frame to the
    * Psion's server and returns its status.
    *
    * @returns The connection status.
    */
    Enum<rfsv::errs> initClipbd();

    /**
    * Send listen request.
    *
    * Calling this method arms the Psion's clipboard server.
    * After that, every change of the Psion's clipboard file
    * will be signaled. To poll the signal, subsequent calls
    * to @ref checkNotify should be made.
    *
    * @returns The connection status.
    */
    Enum<rfsv::errs> sendListen();

    /**
    * Check for clipboard notification.
    *
    * If the Psion has sent a notification, this method returns
    * @ref rfsv::E_PSI_GEN_NONE . If there is no notification
    * pending, this method returns @ref rfsv::E_PSI_FILE_EOF
    * All other return values are to be treated as errors
    *
    * @returns The connection status.
    */
    Enum<rfsv::errs> checkNotify();

    /**
    * Send listen request and wait for notification.
    *
    * This method is the blocking version of the two above methods.
    * It first sends a listen request and then blocks until a
    * notification has sent by the Psion or an error occured.
    *
    * @returns The connection status, rfsv::E_PSI_GEN_NONE if a
    *          notification has been received.
    */
    Enum<rfsv::errs> waitNotify();

    /**
    * Send a notification to the Psion.
    *
    * If the application wishes to notify the Psion after changing the
    * clipboard file, this method can be used.
    *
    * @returns The connection status.
    */
    Enum<rfsv::errs> notify();

protected:
    /**
    * The possible commands.
    */
    enum commands {
        RCLIP_INIT   = 0x00,
        RCLIP_NOTIFY = 0x08,
        RCLIP_LISTEN = 0x04
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
    *
    * @returns true on success, false on failure.
    */
    bool sendCommand(enum commands cc);
    Enum<rfsv::errs> getResponse(bufferStore &data);
    const char *getConnectName();

};

#endif
