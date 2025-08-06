/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999 Matt J. Gumbley <matt@gumbley.demon.co.uk>
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
#ifndef _RFSVFACTORY_H_
#define _RFSVFACTORY_H_

#include "rfsv.h"

class ppsocket;

/**
 * A factory for automatically instantiating the correct
 * @ref rfsv protocol variant depending on the connected Psion.
 */
class rfsvfactory {

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
	FACERR_NORESPONSE = 5
    };

    /**
    * Constructs a rfsvfactory.
    *
    * @param skt The socket to be used for connecting
    * to the ncpd daemon.
    */
    rfsvfactory(ppsocket * skt);

    /**
    * Creates a new @ref rfsv instance.
    *
    * @param reconnect Set to true, if automatic reconnect
    * should be performed on failure.
    *
    * @returns A pointer to a newly created rfsv instance or
    * NULL on failure.
    */
    virtual rfsv * create(bool);

    /**
    * Retrieve an error code.
    *
    * @returns The error code, in case @ref create has
    * failed, 0 otherwise.
    */
    virtual Enum<errs> getError() { return err; }

private:
    /**
    * The socket to be used for connecting to the
    * ncpd daemon.
    */
    ppsocket *skt;
    int serNum;
    Enum<errs> err;
};

#endif
