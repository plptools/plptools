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
#include "config.h"

#include "connectionerror.h"
#include "Enum.h"

ENUM_DEFINITION_BEGIN(ConnectionError, ConnectionError::FACERR_NONE)
    stringRep.add(ConnectionError::FACERR_NONE,               N_("no error"));
    stringRep.add(ConnectionError::FACERR_COULD_NOT_SEND,     N_("could not send version request"));
    stringRep.add(ConnectionError::FACERR_AGAIN,              N_("try again"));
    stringRep.add(ConnectionError::FACERR_NOPSION,            N_("no EPOC device connected"));
    stringRep.add(ConnectionError::FACERR_PROTVERSION,        N_("wrong protocol version"));
    stringRep.add(ConnectionError::FACERR_NORESPONSE,         N_("no response from ncpd"));
    stringRep.add(ConnectionError::FACERR_CONNECTION_FAILURE, N_("could not connect to ncpd"));
ENUM_DEFINITION_END(ConnectionError)
