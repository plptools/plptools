/*
 * This file is part of plptools.
 *
 *  Copyright (C) 2002 Daniel Brahneborg <basic@chello.se>
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

#ifndef _PSION_H
#define _PSION_H

#include <Enum.h>
#include "rfsv.h"

class ppsocket;
class rfsvfactory;
class rpcsfactory;
class rpcs;

/**
 * Semi smart proxy for communicating with a Psion.
 */
class Psion
{
public:

    virtual ~Psion();

    virtual bool connect();

    virtual Enum<rfsv::errs> copyFromPsion(const char * const from, int fd,
                                           cpCallback_t func);

    virtual Enum<rfsv::errs> copyToPsion(const char * const from,
                                         const char * const to,
                                         void *, cpCallback_t func);

    virtual Enum<rfsv::errs> devinfo(const char drive, PlpDrive& plpDrive);

    virtual Enum<rfsv::errs> devlist(uint32_t& devbits);

    virtual Enum<rfsv::errs> dir(const char* dir, PlpDir& files);

    virtual bool dirExists(const char* name);

    virtual void disconnect();

    virtual Enum<rfsv::errs> mkdir(const char* dir);

    virtual void remove(const char* name);

private:

    ppsocket* m_skt;
    ppsocket* m_skt2;
    rfsvfactory* m_rfsvFactory;
    rpcsfactory* m_rpcsFactory;
    rpcs* m_rpcs;
    rfsv* m_rfsv;

};

#endif

