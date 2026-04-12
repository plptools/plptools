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
class RPCSFactory;

/**
 * This is the implementation of the @ref RPCS protocol for
 * Psion series 5 (EPOC) variant. You normally never create
 * objects of this class directly. Thus the constructor is
 * private. Use @ref RPCSFactory for creating an instance of
 * @ref RPCS . For a complete documentation, see @ref RPCS .
 */
class RPCS32 : public RPCS {
    friend class RPCSFactory;

 public:
    Enum<RFSV::errs> getCmdLine(const char *, std::string &);
    Enum<RFSV::errs> getMachineInfo(machineInfo &);
    Enum<RFSV::errs> getOwnerInfo(BufferArray &owner);
    Enum<RFSV::errs> configRead(uint32_t, BufferStore &);
    Enum<RFSV::errs> configWrite(BufferStore);
    Enum<RFSV::errs> closeHandle(uint16_t);
    Enum<RFSV::errs> regOpenIter(uint32_t uid, char *match, uint16_t &handle);
    Enum<RFSV::errs> regReadIter(uint16_t handle);
    Enum<RFSV::errs> setTime(time_t time);
#if 0
    Enum<RFSV::errs> regWrite(void);
    Enum<RFSV::errs> regRead(void);
    Enum<RFSV::errs> regDelete(void);
    Enum<RFSV::errs> queryOpen(void);
    Enum<RFSV::errs> queryRead(void);
    Enum<RFSV::errs> quitServer(void);
#endif

protected:
    Enum<RFSV::errs> configOpen(uint16_t &, uint32_t);

 private:
    RPCS32(TCPSocket *);
};
