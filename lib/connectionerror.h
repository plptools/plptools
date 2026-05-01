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

/**
* Errors that can occur when connecting to a PLP server.
*/
enum ConnectionError {
    FACERR_NONE = 0,
    FACERR_COULD_NOT_SEND = 1,
    FACERR_AGAIN = 2,
    FACERR_NOPSION = 3,
    FACERR_PROTVERSION = 4,
    FACERR_NORESPONSE = 5,
    FACERR_CONNECTION_FAILURE = 6,
};
