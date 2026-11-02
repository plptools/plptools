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
#pragma once

#include "psion.h"

/**
 * A dummy version of the Psion proxy, mainly for testing the installer.
 */
class FakePsion : public Psion {
public:

    virtual ~FakePsion() override;

    virtual bool connect() override;

    virtual Enum<RFSV::errs> copyToPsion(const char * const from,
                                         const char * const to,
                                         void *, cpCallback_t func) override;

    virtual Enum<RFSV::errs> devinfo(const char drive, Drive& plpDrive) override;

    virtual Enum<RFSV::errs> devlist(uint32_t& devbits) override;

    virtual Enum<RFSV::errs> dir(const char* dir, PlpDir& files) override;

    virtual bool dirExists(const char* name) override;

    virtual void disconnect() override;

    virtual Enum<RFSV::errs> mkdir(const char* dir) override;

    virtual void remove(const char* name) override;

};
