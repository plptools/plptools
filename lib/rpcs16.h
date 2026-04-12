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
#pragma once

#include "rpcs.h"

class TCPSocket;
class BufferStore;
class RPCSFactory;

/**
 * This is the implementation of the @ref RPCS protocol for
 * Psion series 3 (SIBO) variant.  You normally never create
 * objects of this class directly. Thus the constructor is
 * private. Use @ref RPCSFactory for creating an instance of
 * @ref RPCS . For a complete documentation, see @ref RPCS .
 */
class RPCS16 : public RPCS {
    friend class RPCSFactory;

 public:
    Enum<RFSV::errs> getCmdLine(const char *, std::string &);
    Enum<RFSV::errs> getOwnerInfo(BufferArray &owner);


 private:
    RPCS16(TCPSocket *);
};
