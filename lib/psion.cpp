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

#include "psion.h"

#include <cli_utils.h>
#include <memory>
#include <plpintl.h>
#include <rfsv.h>
#include <rpcs.h>
#include <rfsvfactory.h>
#include <rpcsfactory.h>
#include <bufferarray.h>
#include <bufferstore.h>
#include <tcpsocket.h>

#include <dirent.h>
#include <netdb.h>

#include <stdio.h>

Psion::~Psion() {
    disconnect();
}

bool Psion::connect() {
    int sockNum = cli_utils::lookup_default_port();
    rpcsSocket_ = new TCPSocket();
    if (!rpcsSocket_->connect(NULL, sockNum)) {
        return false;
    }

    auto rfsvFactory = std::make_unique<RFSVFactory>("127.0.0.1", sockNum);
    auto rpcsFactory = std::make_unique<RPCSFactory>(rpcsSocket_);
    rfsv_ = rfsvFactory->create(true);
    rpcs_ = rpcsFactory->create(true);
    if ((rfsv_ != NULL) && (rpcs_ != NULL)) {
        return true;
    }
    return false;
}

Enum<RFSV::errs> Psion::copyFromPsion(const char * const from, int fd, cpCallback_t func) {
    return rfsv_->copyFromPsion(from, fd, func);
}

Enum<RFSV::errs> Psion::copyToPsion(const char * const from, const char * const to, void *, cpCallback_t func) {
    return rfsv_->copyToPsion(from, to, NULL, func);
}

Enum<RFSV::errs> Psion::devinfo(const char drive, Drive& plpDrive) {
    return rfsv_->devinfo(drive, plpDrive);
}

Enum<RFSV::errs> Psion::devlist(uint32_t& devbits) {
    Enum<RFSV::errs> res;
    res = rfsv_->devlist(devbits);
    return res;
}

Enum<RFSV::errs> Psion::dir(const char* dir, PlpDir& files) {
    return rfsv_->dir(dir, files);
}

bool Psion::dirExists(const char* name) {
    RFSVDirHandle handle;
    Enum<RFSV::errs> res;
    bool exists = false;
    res = rfsv_->opendir(RFSV::PSI_A_ARCHIVE, name, handle);
    if (res == RFSV::E_PSI_GEN_NONE) {
        exists = true;
    }
    res = rfsv_->closedir(handle);
    return exists;
}

void Psion::disconnect() {
    if (rfsv_) {
        delete rfsv_;
        rfsv_ = nullptr;
    }
    if (rpcs_) {
        delete rpcs_;
        rpcs_ = nullptr;
    }
    if (rpcsSocket_) {
        delete rpcsSocket_;
        rpcsSocket_ = nullptr;
    }
}

Enum<RFSV::errs> Psion::mkdir(const char* dir) {
    return rfsv_->mkdir(dir);
}

void Psion::remove(const char* name) {
    rfsv_->remove(name);
}
