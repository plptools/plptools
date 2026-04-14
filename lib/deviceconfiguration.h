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

#include <memory>
#include <string>
#include <unordered_map>

/**
* Class for managing and serializing device details.
*
* Right now this includes the device identifier string (expected to be a UUID4) and the user-assigned device name.
*
* This configuration is generated the first time a device is seen to ensure plptools has a way to uniquely identify it
* on future connections. This is particularly important for backups.
*/
class DeviceConfiguration {
public:

    DeviceConfiguration();

    DeviceConfiguration(std::string const &id, std::string const &name);

    static std::unique_ptr<DeviceConfiguration> deserialize(const std::string &contents);

    std::string id() const {
        return id_;
    }

    std::string name() const {
        return name_;
    }

    void setName(std::string name) {
        name_ = name;
    }

    std::string serialize() const;

private:
    std::string id_;
    std::string name_;
};
