/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999 Matt J. Gumbley <matt@gumbley.demon.co.uk>
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2026 Jason Morley <hello@jbmorley.co.uk>
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

#include <cstddef>

#include "connectionerror.h"
#include "rfsv.h"

class TCPSocket;

/**
 * A factory for automatically instantiating the correct
 * @ref RFSV protocol variant depending on the connected Psion.
 */
class RFSVFactory final {

public:

    /**
    * Constructs a RFSVFactory.
    *
    * @param host The host be used for connecting to the ncpd daemon.
    * @param port The port be used for connecting to the ncpd daemon.
    */
    RFSVFactory(const std::string &host, int port);

    /**
     * Delete the RFSVFactory, cleaning up any resources.
     */
    ~RFSVFactory();

    /**
    * Creates a new @ref RFSV instance.
    *
    * @param reconnect Set to true, if automatic reconnect should be performed on failure.
    * @param error Out parameter; set to the error on failure if non-NULL.
    *
    * @returns A pointer to a newly created @ref RFSV instance or NULL on failure.
    */
    RFSV* create(bool, Enum<ConnectionError> *error = nullptr);

private:
    std::string host_;
    int port_;
};
