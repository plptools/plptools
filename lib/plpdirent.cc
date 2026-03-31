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

#include "plpdirent.h"

#include <cstdint>
#include <iomanip>

using namespace std;

PlpUID::PlpUID() {
    memset(uid, 0, sizeof(uid));
}

PlpUID::PlpUID(const uint32_t u1, const uint32_t u2, const uint32_t u3) {
    uid[0] = u1; uid[1] = u2; uid[2] = u3;
}

uint32_t PlpUID::
operator[](int idx) {
    assert ((idx > -1) && (idx < 3));
    return uid[idx];
}

PlpDirent::PlpDirent()
: size(0)
, attr(0)
, time(time_t(0))
, attrstr("")
, name("") {
}

PlpDirent::PlpDirent(const PlpDirent &e) {
    size    = e.size;
    attr    = e.attr;
    time    = e.time;
    UID     = e.UID;
    name    = e.name;
    attrstr = e.attrstr;
}

PlpDirent::PlpDirent(const uint32_t _size, const uint32_t _attr,
                     const uint32_t tHi, const uint32_t tLo,
                     const char * const _name) {
    size = _size;
    attr = _attr;
    time = PsiTime(tHi, tLo);
    UID  = PlpUID();
    name = _name;
    attrstr = "";
}

uint32_t PlpDirent::getSize() const {
    return size;
}

uint32_t PlpDirent::getAttr() const {
    return attr;
}

bool PlpDirent::isDirectory() const {
    return (attr & RFSV::PSI_A_DIR) > 0;
}

uint32_t PlpDirent::getUID(int uididx) {
    if ((uididx >= 0) && (uididx < 4))
        return UID[uididx];
    return 0;
}

PlpUID &PlpDirent::getUID() {
    return UID;
}

const char *PlpDirent::getName() const {
    return name.c_str();
}

PsiTime PlpDirent::getPsiTime() {
    return time;
}

void PlpDirent::setName(const char *str) {
    name = str;
}

PlpDirent &PlpDirent::operator=(const PlpDirent &e) {
    size    = e.size;
    attr    = e.attr;
    time    = e.time;
    UID     = e.UID;
    name    = e.name;
    attrstr = e.attrstr;
    return *this;
}

ostream &operator<<(ostream &o, const PlpDirent &e) {
    ostream::fmtflags old = o.flags();

    o << e.attrstr << " " << dec << setw(10)
      << setfill(' ') << e.size << " " << e.time
      << " " << e.name;
    o.flags(old);
    return o;
}

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

Drive::Drive(const Drive &other)
: mediaType_(other.mediaType_)
, driveAttributes_(other.driveAttributes_)
, mediaAttributes_(other.mediaAttributes_)
, uid_(other.uid_)
, size_(other.size_)
, space_(other.space_)
, driveLetter_(other.driveLetter_)
, name_(other.name_) {
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
