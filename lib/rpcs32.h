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
#ifndef _RPCS32_H_
#define _RPCS32_H_

#include "rpcs.h"

class ppsocket;
class rpcsfactory;

/**
 * This is the implementation of the @ref rpcs protocol for
 * Psion series 5 (EPOC) variant. You normally never create
 * objects of this class directly. Thus the constructor is
 * private. Use @ref rpcsfactory for creating an instance of
 * @ref rpcs . For a complete documentation, see @ref rpcs .
 */
class rpcs32 : public rpcs {
    friend class rpcsfactory;

 public:
    Enum<rfsv::errs> getCmdLine(const char *, std::string &);
    Enum<rfsv::errs> getMachineInfo(machineInfo &);
    Enum<rfsv::errs> configRead(uint32_t, bufferStore &);
    Enum<rfsv::errs> configWrite(bufferStore);
    Enum<rfsv::errs> closeHandle(uint16_t);
    Enum<rfsv::errs> regOpenIter(uint32_t uid, char *match, uint16_t &handle);
    Enum<rfsv::errs> regReadIter(uint16_t handle);
    Enum<rfsv::errs> setTime(time_t time);
#if 0
    Enum<rfsv::errs> regWrite(void);
    Enum<rfsv::errs> regRead(void);
    Enum<rfsv::errs> regDelete(void);
    Enum<rfsv::errs> queryOpen(void);
    Enum<rfsv::errs> queryRead(void);
    Enum<rfsv::errs> quitServer(void);
#endif

protected:
    Enum<rfsv::errs> configOpen(uint16_t &, uint32_t);

 private:
    rpcs32(ppsocket *);
};

#endif
