/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 2000, 2001 Fritz Elfert <felfert@to.com>
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

#include "bufferstore.h"

#include <cstring>
#include <cstdlib>

// Should be iostream.h, but won't build on Sun WorkShop C++ 5.0
#include <iomanip>
#include <string>

#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

using namespace std;

BufferStore::BufferStore()
: len(0)
, lenAllocd(0)
, start(0)
, buff(0) {
}

BufferStore::BufferStore(const BufferStore &a)
: start(0) {
    lenAllocd = (a.getLen() > MIN_LEN) ? a.getLen() : MIN_LEN;
    buff = (unsigned char *)malloc(lenAllocd);
    assert(buff);
    len = a.getLen();
    memcpy(buff, a.getString(0), len);
}

BufferStore::BufferStore(const unsigned char *_buff, long _len)
: start(0) {
    lenAllocd = (_len > MIN_LEN) ? _len : MIN_LEN;
    buff = (unsigned char *)malloc(lenAllocd);
    assert(buff);
    len = _len;
    memcpy(buff, _buff, len);
}

BufferStore &BufferStore::operator =(const BufferStore &a) {
    if (this != &a) {
        checkAllocd(a.getLen());
        len = a.getLen();
        memcpy(buff, a.getString(0), len);
        start = 0;
    }
    return *this;
}

void BufferStore::init() {
    start = 0;
    len = 0;
}

void BufferStore::init(const unsigned char *_buff, long _len) {
    checkAllocd(_len);
    start = 0;
    len = _len;
    memcpy(buff, _buff, len);
}

BufferStore::~BufferStore() {
    if (buff) {
        ::free(buff);
    }
}

unsigned long BufferStore::getLen() const {
    return (start > len) ? 0 : len - start;
}

unsigned char BufferStore::getByte(long pos) const {
    return buff[pos+start];
}

uint16_t BufferStore::getWord(long pos) const {
    return buff[pos+start] + (buff[pos+start+1] << 8);
}

uint32_t BufferStore::getDWord(long pos) const {
    return buff[pos+start] +
        (buff[pos+start+1] << 8) +
        (buff[pos+start+2] << 16) +
        (buff[pos+start+3] << 24);
}

int32_t BufferStore::getSDWord(long pos) const {
    return buff[pos+start] +
        (buff[pos+start+1] << 8) +
        (buff[pos+start+2] << 16) +
        (buff[pos+start+3] << 24);
}

const char * BufferStore::getString(long pos) const {
    return (const char *)buff + pos + start;
}

ostream &operator<<(std::ostream &s, const BufferStore &m) {
    // save stream flags
    ostream::fmtflags old = s.flags();

    for (int i = m.start; i < m.len; i++)
        s << hex << setw(2) << setfill('0') << (int)m.buff[i] << " ";

    // restore stream flags
    s.flags(old);
    s << "(";

    for (int i = m.start; i < m.len; i++) {
        unsigned char c = m.buff[i];
        s << (unsigned char)(isprint(c) ? c : '.');
    }

    return s << ")";
}

void BufferStore::discardFirstBytes(int n) {
    start += n;
    if (start > len) start = len;
}

void BufferStore::checkAllocd(long newLen) {
    if (newLen >= lenAllocd) {
        do {
            lenAllocd = (lenAllocd < MIN_LEN) ? MIN_LEN : (lenAllocd * 2);
        } while (newLen >= lenAllocd);
        assert(lenAllocd);
        buff = (unsigned char *)realloc(buff, lenAllocd);
        assert(buff);
    }
}

void BufferStore::addByte(unsigned char cc) {
    checkAllocd(len + 1);
    buff[len++] = cc;
}

void BufferStore::addString(const char *s) {
    int l = strlen(s);
    checkAllocd(len + l);
    memcpy(&buff[len], s, l);
    len += l;
}

void BufferStore::addStringT(const char *s) {
    addString(s);
    addByte(0);
}

void BufferStore::addBytes(const unsigned char *s, int l) {
    checkAllocd(len + l);
    memcpy(&buff[len], s, l);
    len += l;
}

void BufferStore::addBuff(const BufferStore &s, long maxLen) {
    long l = s.getLen();
    checkAllocd(len + l);
    if ((maxLen >= 0) && (maxLen < l))
        l = maxLen;
    if (l > 0) {
        memcpy(&buff[len], s.getString(0), l);
        len += l;
    }
}

void BufferStore::addWord(int a) {
    checkAllocd(len + 2);
    buff[len++] = a & 0xff;
    buff[len++] = (a>>8) & 0xff;
}

void BufferStore::addDWord(long a) {
    checkAllocd(len + 4);
    buff[len++] = a & 0xff;
    buff[len++] = (a>>8) & 0xff;
    buff[len++] = (a>>16) & 0xff;
    buff[len++] = (a>>24) & 0xff;
}

void BufferStore::truncate(long newLen) {
    if (newLen < len) {
        len = newLen;
    }
}

void BufferStore::prependByte(unsigned char cc) {
    checkAllocd(len + 1);
    memmove(&buff[1], buff, len++);
    buff[0] = cc;
}

void BufferStore::prependWord(int a) {
    checkAllocd(len + 2);
    memmove(&buff[2], buff, len);
    len += 2;
    buff[0] = a & 0xff;
    buff[1] = (a>>8) & 0xff;
}
