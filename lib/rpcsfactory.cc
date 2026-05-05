/*
 * This file is part of plptools.
 *
 *  Copyright (C) 2000-2001 Fritz Elfert <felfert@to.com>
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

#include "ncpclient.h"
#include "rpcs.h"
#include "rpcs16.h"
#include "rpcs32.h"
#include "rpcsfactory.h"
#include "Enum.h"

#include <stdlib.h>
#include <time.h>

RPCSFactory::RPCSFactory(const std::string &host, int port)
: host_(host)
, port_(port) {}

RPCSFactory::~RPCSFactory() {}

RPCS *RPCSFactory::create(bool reconnect, Enum<ConnectionError> *error) {
    return ncp_client::connect<RPCS, RPCS16, RPCS32>(host_, port_, reconnect, error);
}
