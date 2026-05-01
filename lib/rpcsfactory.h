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
#pragma once

#include "rpcs.h"

class TCPSocket;

/**
 * A factory for automatically instantiating the correct protocol
 * variant depending on the connected Psion.
 */
class RPCSFactory {
 public:

    /**
    * The known errors which can happen during @ref create .
    */
    enum errs {
        FACERR_NONE = 0,
        FACERR_COULD_NOT_SEND = 1,
        FACERR_AGAIN = 2,
        FACERR_NOPSION = 3,
        FACERR_PROTVERSION = 4,
        FACERR_NORESPONSE = 5,
        FACERR_CONNECTION_FAILURE = 6,
    };

    /**
    * Constructs a RPCSFactory.
    *
    * @param host The host be used for connecting to the ncpd daemon.
    * @param port The port be used for connecting to the ncpd daemon.
    */
    RPCSFactory(const std::string &host, int port);

    /**
     * Delete the RPCSFactory, cleaning up any resources.
     */
    ~RPCSFactory();

    /**
    * Creates a new RPCS instance.
    *
    * @param reconnect Set to true, if automatic reconnect should be performed on failure.
    * @param error Out parameter; set to the error on failure if non-NULL.
    *
    * @returns A pointer to a newly created @ref RPCS instance or NULL on failure.
    */
    RPCS* create(bool, Enum<errs> *error = nullptr);

 private:
    std::string host_;
    int port_;
};
