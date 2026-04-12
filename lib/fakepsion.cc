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
#include "config.h"

#include "fakepsion.h"
#include "sistypes.h"

#include <stdio.h>

FakePsion::~FakePsion() {
}

bool FakePsion::connect() {
    return true;
}

Enum<RFSV::errs> FakePsion::copyToPsion(const char * const from, const char * const to, void *, cpCallback_t func) {
    (void)func;
    if (logLevel >= 1) {
        printf(" -- Not really copying %s to %s\n", from, to);
    }
    return RFSV::E_PSI_GEN_NONE;
}

Enum<RFSV::errs> FakePsion::devinfo(const char drive, Drive& plpDrive) {
    (void)drive;
    (void)plpDrive;
    return RFSV::E_PSI_GEN_NONE;
}

Enum<RFSV::errs> FakePsion::devlist(uint32_t& devbits) {
    (void)devbits;
    return RFSV::E_PSI_GEN_FAIL;
}

Enum<RFSV::errs> FakePsion::dir(const char* dir, PlpDir& files) {
    (void) dir;
    (void) files;
    return RFSV::E_PSI_GEN_NONE;
}

bool FakePsion::dirExists(const char* name) {
    (void)name;
    return true;
}

void FakePsion::disconnect() {
}

Enum<RFSV::errs> FakePsion::mkdir(const char* dir) {
    (void)dir;
    if (logLevel >= 1) {
        printf(" -- Not really creating dir %s\n", dir);
    }
    return RFSV::E_PSI_GEN_NONE;
}

void FakePsion::remove(const char* name) {
    (void)name;
}
