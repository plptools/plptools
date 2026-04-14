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
#include "config.h"

#include <string>
#include <unordered_map>

#include "deviceconfiguration.h"

#include "ini.h"
#include "plpintl.h"
#include "uuid.h"

std::unique_ptr<DeviceConfiguration> DeviceConfiguration::deserialize(const std::string &stringRepresentation) {
    auto contents = ini::deserialize(stringRepresentation);
    if (!contents) {
        return nullptr;
    }
    if (contents->find("id") == contents->end() || contents->find("name") == contents->end()) {
        return nullptr;
    }
    return std::unique_ptr<DeviceConfiguration>(new DeviceConfiguration((*contents)["id"], (*contents)["name"]));
}

DeviceConfiguration::DeviceConfiguration()
: id_(uuid::uuid4())
, name_(_("My Psion")) {
}

DeviceConfiguration::DeviceConfiguration(std::string const &id, std::string const &name)
: id_(id)
, name_(name) {
}

std::string DeviceConfiguration::serialize() const {
    return ini::serialize({
        {"id", id_},
        {"name", name_},
    });
}
