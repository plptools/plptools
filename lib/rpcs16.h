/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
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
#ifndef _RPCS16_H_
#define _RPCS16_H_

#include "rpcs.h"

class ppsocket;
class bufferStore;
class rpcsfactory;

/**
 * This is the implementation of the @ref rpcs protocol for
 * Psion series 3 (SIBO) variant.  You normally never create
 * objects of this class directly. Thus the constructor is
 * private. Use @ref rpcsfactory for creating an instance of
 * @ref rpcs . For a complete documentation, see @ref rpcs .
 */
class rpcs16 : public rpcs {
    friend class rpcsfactory;

 public:
    Enum<rfsv::errs> getCmdLine(const char *, std::string &);

 private:
    rpcs16(ppsocket *);
};

#endif
