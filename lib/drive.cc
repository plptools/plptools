/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
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
#include "rfsv.h"

#include "drive.h"

#include <cstdint>
#include <iomanip>

using namespace std;

Drive::Drive() {
}

Drive::Drive(MediaType mediaType,
             uint32_t driveAttributes,
             uint32_t mediaAttributes,
             uint32_t uid,
             uint64_t size,
             uint64_t space,
             char driveLetter,
             std::string name)
: mediaType_(mediaType)
, driveAttributes_(driveAttributes)
, mediaAttributes_(mediaAttributes)
, uid_(uid)
, size_(size)
, space_(space)
, driveLetter_(driveLetter)
, name_(name) {
}

void Drive::setMediaType(MediaType type) {
    mediaType_ = type;
}

void Drive::setDriveAttributes(uint32_t attr) {
    driveAttributes_ = attr;
}

void Drive::setMediaAttributes(uint32_t mediaAttribute) {
    mediaAttributes_ = mediaAttribute;
}

void Drive::setUID(uint32_t uid) {
    uid_ = uid;
}

void Drive::setSize(uint32_t sizeLo, uint32_t sizeHi) {
    size_ = (static_cast<unsigned long long>(sizeHi) << 32) + sizeLo;
}

void Drive::setSpace(uint32_t spaceLo, uint32_t spaceHi) {
    space_ = (static_cast<unsigned long long>(spaceHi) << 32) + spaceLo;
}

void Drive::setName(char drive, const char * const volname) {
    driveLetter_ = drive;
    name_ = "";
    name_ += volname;
}

MediaType Drive::getMediaType() const {
    return mediaType_;
}

uint32_t Drive::getDriveAttributes() const {
    return driveAttributes_;
}

uint32_t Drive::getMediaAttributes() const {
    return mediaAttributes_;
}

uint32_t Drive::getUID() const {
    return uid_;
}

uint64_t Drive::getSize() const {
    return size_;
}

uint64_t Drive::getSpace() const {
    return space_;
}

string Drive::getName() const {
    return name_;
}

char Drive::getDriveLetter() const {
    return driveLetter_;
}

std::string Drive::getPath() const {
    std::string path;
    path += driveLetter_;
    path += ":\\";
    return path;
}
