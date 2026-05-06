/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2006-2025 Reuben Thomas <rrt@sc3d.org>
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
#pragma once

#include "config.h"

#include <memory>

#include "rfsv.h"

class DeviceConfiguration;
class rclip;
class RFSV;
class RPCS;

namespace device {

class DeviceEndpoint {
public:
    DeviceEndpoint(const std::string &id,
                   std::unique_ptr<RFSV> rfsv,
                   std::unique_ptr<RPCS> rpcs,
                   std::unique_ptr<rclip> clip);

    const std::string id_;
    const std::unique_ptr<RFSV> rfsv_;
    const std::unique_ptr<RPCS> rpcs_;
    const std::unique_ptr<rclip> clip_;
};

extern std::unique_ptr<DeviceEndpoint> connect(const std::string host, int port, Enum<ConnectionError> *error);

extern Enum<RFSV::errs> write_configuration(RFSV &rfsv, const DeviceConfiguration &deviceConfiguration);

extern std::unique_ptr<DeviceConfiguration> read_configuration(RFSV &rfsv, Enum<RFSV::errs> &error);

};
