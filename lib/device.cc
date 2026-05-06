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

#include "config.h"

#include "device.h"

#include <cstdint>
#include <memory>

#include "connectionerror.h"
#include "deviceconfiguration.h"
#include "pathutils.h"
#include "rclip.h"
#include "rfsv.h"
#include "rpcs.h"
#include "uuid.h"

device::DeviceEndpoint::DeviceEndpoint(const std::string &id,
                                       std::unique_ptr<RFSV> rfsv,
                                       std::unique_ptr<RPCS> rpcs,
                                       std::unique_ptr<rclip> clip)
: id_(id)
, rfsv_(std::move(rfsv))
, rpcs_(std::move(rpcs))
, clip_(std::move(clip)) {}

std::unique_ptr<device::DeviceEndpoint> device::connect(const std::string host,
                                                        int port,
                                                        Enum<ConnectionError> *error) {
    Enum<ConnectionError> internalError = ConnectionError::FACERR_NONE;
    auto rfsv = std::unique_ptr<RFSV>(RFSV::connect(host, port, &internalError));
    if (!rfsv) {
        if (error) {
            *error = internalError;
        }
        return nullptr;
    }

    // Get the device configuration.
    Enum<RFSV::errs> result;
    auto deviceConfiguration = device::read_configuration(*rfsv, result);
    if (!deviceConfiguration) {
        // Create and write a new device configuration if it doesn't exist.
        // We ignore errors here as we want failures to write the device configuration to be non-fatal. For example., if
        // the device is out of memory, it's acceptable for it to appear as a new device on each connection.
        deviceConfiguration = std::make_unique<DeviceConfiguration>(uuid::uuid4(), _("My Psion"));
        device::write_configuration(*rfsv, *deviceConfiguration);
    }

    auto rpcs = std::unique_ptr<RPCS>(RPCS::connect(host, port, &internalError));
    if (!rpcs) {
        if (error) {
            *error = internalError;
        }
        return nullptr;
    }

    auto clip = std::unique_ptr<rclip>(rclip::connect(host, port, &internalError));
    if (!clip) {
        if (error) {
            *error = internalError;
        }
        return nullptr;
    }

    return std::make_unique<device::DeviceEndpoint>(deviceConfiguration->id(), std::move(rfsv), std::move(rpcs), std::move(clip));
}

Enum<RFSV::errs> device::write_configuration(RFSV &rfsv, const DeviceConfiguration &deviceConfiguration) {
    std::string configurationPath = rfsv.deviceConfigurationPath();

    Enum<RFSV::errs> error = RFSV::E_PSI_GEN_NONE;

    // Ensure the directory exists by trying to create it and ignoring exists errors.
    std::string directoryPath = pathutils::epoc_dirname(configurationPath.c_str());
    error = rfsv.mkdir(directoryPath.c_str());
    if (error != RFSV::E_PSI_GEN_NONE && error != RFSV::E_PSI_FILE_EXIST) {
        return error;
    }

    // Write the file.
    uint32_t handle;
    uint32_t mode = rfsv.opMode(RFSV::PSI_O_WRONLY);
    error = rfsv.fcreatefile(mode, configurationPath.c_str(), handle);
    if (error != RFSV::E_PSI_GEN_NONE) {
        if (error != RFSV::E_PSI_FILE_EXIST) {
            return error;
        }
        error = rfsv.freplacefile(mode, configurationPath.c_str(), handle);
        if (error != RFSV::E_PSI_GEN_NONE) {
            return error;
        }
    }

    std::string contents = deviceConfiguration.serialize();
    uint32_t count = 0;
    error = rfsv.fwrite(handle, reinterpret_cast<const unsigned char *>(contents.data()), contents.length(), count);
    if (error != RFSV::E_PSI_GEN_NONE) {
        rfsv.fclose(handle);
        return error;
    }

    return rfsv.fclose(handle);
}

std::unique_ptr<DeviceConfiguration> device::read_configuration(RFSV &rfsv, Enum<RFSV::errs> &error) {
    std::string configurationPath = rfsv.deviceConfigurationPath();
    uint32_t handle;

    // Get the file size.
    PlpDirent dirent;
    error = rfsv.fgeteattr(configurationPath.c_str(), dirent);
    if (error != RFSV::E_PSI_GEN_NONE) {
        return nullptr;
    }

    // Open the file.
    error = rfsv.fopen(RFSV::PSI_O_RDONLY, configurationPath.c_str(), handle);
    if (error != RFSV::E_PSI_GEN_NONE) {
        return nullptr;
    }

    // Read the contents.
    uint32_t size = dirent.getSize();
    std::vector<unsigned char> buffer(size);
    uint32_t count;
    error = rfsv.fread(handle, buffer.data(), size, count);
    if (error != RFSV::E_PSI_GEN_NONE) {
        rfsv.fclose(handle);
        return nullptr;
    }

    // Close the file.
    error = rfsv.fclose(handle);
    if (error != RFSV::E_PSI_GEN_NONE) {
        return nullptr;
    }

    // Parse the contents.
    std::string contents(buffer.begin(), buffer.end());
    auto configuration = DeviceConfiguration::deserialize(contents);
    if (!configuration) {
        error = RFSV::E_PSI_FILE_CORRUPT;
        return nullptr;
    }

    return configuration;
}
