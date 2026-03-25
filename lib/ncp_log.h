/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
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
#ifndef _ncp_log_h_
#define _ncp_log_h_

#include "config.h"

#include <log.h>

#define NCP_DEBUG_LOG 1
#define NCP_DEBUG_DUMP 2
#define LNK_DEBUG_LOG 4
#define LNK_DEBUG_DUMP 8
#define PKT_DEBUG_LOG 16
#define PKT_DEBUG_DUMP 32
#define PKT_DEBUG_HANDSHAKE 64
#define NCP_SESSION_LOG 128

// Note that these logs are not thread-safe and there's nothing to ensure that log messages from different threads
// aren't interlaced. Since these are ultimately written to using `write` logging shouldn't crash, but it's important to
// understand the limitations.

extern logbuf ilog;
extern logbuf dlog;
extern logbuf elog;

extern std::ostream lout;
extern std::ostream lerr;
extern std::ostream linf;

#endif
