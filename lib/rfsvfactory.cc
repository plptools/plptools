/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Matt J. Gumbley <matt@gumbley.demon.co.uk>
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

#include "rfsvfactory.h"

#include "ncpclient.h"
#include "rfsv.h"
#include "rfsv16.h"
#include "rfsv32.h"

RFSVFactory::RFSVFactory(const std::string &host, int port)
: host_(host)
, port_(port) {}

RFSVFactory::~RFSVFactory() {}

RFSV* RFSVFactory::create(bool reconnect, Enum<ConnectionError> *error) {
    return ncp_client::connect<RFSV, RFSV16, RFSV32>(host_, port_, reconnect, error);
}
