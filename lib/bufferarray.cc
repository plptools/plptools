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
#include "bufferarray.h"

BufferArray::BufferArray()
{
    len = 0;
    lenAllocd = ALLOC_MIN;
    buff = new BufferStore[lenAllocd];
}

BufferArray::BufferArray(const BufferArray & a)
{
    len = a.len;
    lenAllocd = a.lenAllocd;
    buff = new BufferStore[lenAllocd];
    for (int i = 0; i < len; i++)
        buff[i] = a.buff[i];
}

BufferArray::~BufferArray()
{
    delete []buff;
}

BufferStore BufferArray::
pop()
{
    BufferStore ret;
    if (len > 0) {
        ret = buff[0];
        len--;
        for (long i = 0; i < len; i++) {
            buff[i] = buff[i + 1];
        }
    }
    return ret;
}

void BufferArray::
append(const BufferStore & b)
{
    if (len == lenAllocd) {
        lenAllocd += ALLOC_MIN;
        BufferStore *nb = new BufferStore[lenAllocd];
        for (long i = 0; i < len; i++) {
            nb[i] = buff[i];
        }
        delete []buff;
        buff = nb;
    }
    buff[len++] = b;
}

void BufferArray::
push(const BufferStore & b)
{
    if (len == lenAllocd)
        lenAllocd += ALLOC_MIN;
    BufferStore *nb = new BufferStore[lenAllocd];
    for (long i = len; i > 0; i--) {
        nb[i] = buff[i - 1];
    }
    nb[0] = b;
    delete[]buff;
    buff = nb;
    len++;
}

long BufferArray::
length(void)
{
    return len;
}

void BufferArray::
clear(void)
{
    len = 0;
    lenAllocd = ALLOC_MIN;
    delete []buff;
    buff = new BufferStore[lenAllocd];
}

BufferArray &BufferArray::
operator =(const BufferArray & a)
{
    delete []buff;
    len = a.len;
    lenAllocd = a.lenAllocd;
    buff = new BufferStore[lenAllocd];
    for (int i = 0; i < len; i++)
        buff[i] = a.buff[i];
    return *this;
}

BufferStore &BufferArray::
operator [](const unsigned long index)
{
    return buff[index];
}

BufferArray BufferArray::
operator +(const BufferStore &s)
{
    BufferArray res = *this;
    res += s;
    return res;
}

BufferArray BufferArray::
operator +(const BufferArray &a)
{
    BufferArray res = *this;
    res += a;
    return res;
}

BufferArray &BufferArray::
operator +=(const BufferArray &a)
{
    lenAllocd += a.lenAllocd;
    BufferStore *nb = new BufferStore[lenAllocd];
    for (int i = 0; i < len; i++)
        nb[len + i] = buff[i];
    for (int i = 0; i < a.len; i++)
        nb[len + i] = a.buff[i];
    len += a.len;
    delete []buff;
    buff = nb;
    return *this;
}

BufferArray &BufferArray::
operator +=(const BufferStore &s)
{
    append(s);
    return *this;
}
