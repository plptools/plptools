/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
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
    return (attr & rfsv::PSI_A_DIR) > 0;
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

PlpDrive::PlpDrive() {
}

PlpDrive::PlpDrive(const PlpDrive &other)
: mediaType_(other.mediaType_)
, driveAttribute_(other.driveAttribute_)
, mediaAttribute_(other.mediaAttribute_)
, uid_(other.uid_)
, size_(other.size_)
, space_(other.space_)
, driveChar_(other.driveChar_)
, name_(other.name_) {
}

void PlpDrive::setMediaType(MediaType type) {
    mediaType_ = type;
}

void PlpDrive::setDriveAttribute(uint32_t attr) {
    driveAttribute_ = attr;
}

void PlpDrive::setMediaAttribute(uint32_t mediaAttribute) {
    mediaAttribute_ = mediaAttribute;
}

void PlpDrive::setUID(uint32_t uid) {
    uid_ = uid;
}

void PlpDrive::setSize(uint32_t sizeLo, uint32_t sizeHi) {
    size_ = ((unsigned long long)sizeHi << 32) + sizeLo;
}

void PlpDrive::setSpace(uint32_t spaceLo, uint32_t spaceHi) {
    space_ = ((unsigned long long)spaceHi << 32) + spaceLo;
}

void PlpDrive::setName(char drive, const char * const volname) {
    driveChar_ = drive;
    name_ = "";
    name_ += volname;
}

MediaType PlpDrive::getMediaType() const {
    return mediaType_;
}

static const char * const media_types[] = {
    N_("Not present"),
    N_("Unknown"),
    N_("Floppy"),
    N_("Disk"),
    N_("CD-ROM"),
    N_("RAM"),
    N_("Flash Disk"),
    N_("ROM"),
    N_("Remote"),
};

void PlpDrive::getMediaType(std::string &ret) const {
    ret = media_types[static_cast<uint32_t>(mediaType_)];
}

uint32_t PlpDrive::getDriveAttribute() {
    return driveAttribute_;
}

static void
appendWithDelim(string &s1, const char * const s2) {
    if (!s1.empty()) {
        s1 += ',';
    }
    s1 += s2;
}

void PlpDrive::
getDriveAttribute(std::string &ret) {
    ret = "";
    if (driveAttribute_ & 1) {
        appendWithDelim(ret, _("local"));
    }
    if (driveAttribute_ & 2) {
        appendWithDelim(ret, _("ROM"));
    }
    if (driveAttribute_ & 4) {
        appendWithDelim(ret, _("redirected"));
    }
    if (driveAttribute_ & 8) {
        appendWithDelim(ret, _("substituted"));
    }
    if (driveAttribute_ & 16) {
        appendWithDelim(ret, _("internal"));
    }
    if (driveAttribute_ & 32) {
        appendWithDelim(ret, _("removable"));
    }
}

uint32_t PlpDrive::getMediaAttribute() {
    return mediaAttribute_;
}

void PlpDrive::getMediaAttribute(std::string &ret) {
    ret = "";

    if (mediaAttribute_ & 1) {
        appendWithDelim(ret, _("variable size"));
    }
    if (mediaAttribute_ & 2) {
        appendWithDelim(ret, _("dual density"));
    }
    if (mediaAttribute_ & 4) {
        appendWithDelim(ret, _("formattable"));
    }
    if (mediaAttribute_ & 8) {
        appendWithDelim(ret, _("write protected"));
    }
}

uint32_t PlpDrive::getUID() {
    return uid_;
}

uint64_t PlpDrive::getSize() {
    return size_;
}

uint64_t PlpDrive::getSpace() {
    return space_;
}

string PlpDrive::getName() const {
    return name_;
}

char PlpDrive::getDriveChar() const {
    return driveChar_;
}

std::string PlpDrive::getPath() const {
    std::string path;
    path += driveChar_;
    path += ":\\";
    return path;
}
