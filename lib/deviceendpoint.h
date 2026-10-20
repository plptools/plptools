/*
 * This file is part of plptools.
 *
 *  Copyright (c) 2026 Jason Morley <hello@jbmorley.co.uk>
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

#include "config.h"

#include <memory>
#include <string>

#include "connectionerror.h"
#include "Enum.h"
#include "device.h"

class rclip;
class RFSV;
class RPCS;

class DeviceEndpoint {
public:

    static std::unique_ptr<DeviceEndpoint> connect(const std::string host, int port, Enum<ConnectionError> *error);

    /**
    * Device identifier.
    *
    * The device identifier is guaranteed to be session-stable but isn't guaranteed to be persisted to disk. Use
    * @ref hasPersistentId to check if the identifier is persistent. The identifier can be persisted by setting the
    * device name using @ref setName.
    *
    * @return String containing the device identifier.
    */
    std::string id() const;

    bool hasPersistentId() const;

    /**
    * Get the device name.
    *
    * @param name Out-param for the device name.
    *
    * A return value of @ref RFSV::E_PSI_FILE_RECORD indicates that the device name has not been set.
    *
    * @result @ref RFSV::E_PSI_GEN_NONE on success; error otherwise.
    */
    Enum<RFSV::errs> getName(std::string &name) const;

    /**
    * Set the device name.
    *
    * @param name New device name.
    *
    * @result @ref RFSV::E_PSI_GEN_NONE on success; error otherwise.
    */
    Enum<RFSV::errs> setName(const std::string &name);

    const std::unique_ptr<RFSV> rfsv_;
    const std::unique_ptr<RPCS> rpcs_;
    const std::unique_ptr<rclip> clip_;

private:

    DeviceEndpoint(const std::string &id,
                   bool hasPersistentConfiguration,
                   std::unique_ptr<RFSV> rfsv,
                   std::unique_ptr<RPCS> rpcs,
                   std::unique_ptr<rclip> clip);

    const std::string id_;
    bool hasPersistentConfiguration_;
};
