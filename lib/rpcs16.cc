/*
 * This file is part of plptools.
 *
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
#include "config.h"

#include "rpcs16.h"
#include "bufferstore.h"
#include "bufferarray.h"
#include "ppsocket.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

rpcs16::rpcs16(ppsocket * _skt)
{
    skt = _skt;
    mtCacheS5mx = 0;
    reset();
}

Enum<rfsv::errs> rpcs16::
getCmdLine(const char *process, string &ret)
{
    bufferStore a;
    Enum<rfsv::errs> res;

    a.addStringT(process);
    if (!sendCommand(rpcs::GET_CMDLINE, a))
        return rfsv::E_PSI_FILE_DISC;
    if ((res = getResponse(a, true)) == rfsv::E_PSI_GEN_NONE)
        ret = a.getString(0);
    return res;
}
