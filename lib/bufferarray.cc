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

bufferArray::bufferArray()
{
    len = 0;
    lenAllocd = ALLOC_MIN;
    buff = new bufferStore[lenAllocd];
}

bufferArray::bufferArray(const bufferArray & a)
{
    len = a.len;
    lenAllocd = a.lenAllocd;
    buff = new bufferStore[lenAllocd];
    for (int i = 0; i < len; i++)
        buff[i] = a.buff[i];
}

bufferArray::~bufferArray()
{
    delete []buff;
}

bufferStore bufferArray::
pop()
{
    bufferStore ret;
    if (len > 0) {
        ret = buff[0];
        len--;
        for (long i = 0; i < len; i++) {
            buff[i] = buff[i + 1];
        }
    }
    return ret;
}

void bufferArray::
append(const bufferStore & b)
{
    if (len == lenAllocd) {
        lenAllocd += ALLOC_MIN;
        bufferStore *nb = new bufferStore[lenAllocd];
        for (long i = 0; i < len; i++) {
            nb[i] = buff[i];
        }
        delete []buff;
        buff = nb;
    }
    buff[len++] = b;
}

void bufferArray::
push(const bufferStore & b)
{
    if (len == lenAllocd)
        lenAllocd += ALLOC_MIN;
    bufferStore *nb = new bufferStore[lenAllocd];
    for (long i = len; i > 0; i--) {
        nb[i] = buff[i - 1];
    }
    nb[0] = b;
    delete[]buff;
    buff = nb;
    len++;
}

long bufferArray::
length(void)
{
    return len;
}

void bufferArray::
clear(void)
{
    len = 0;
    lenAllocd = ALLOC_MIN;
    delete []buff;
    buff = new bufferStore[lenAllocd];
}

bufferArray &bufferArray::
operator =(const bufferArray & a)
{
    delete []buff;
    len = a.len;
    lenAllocd = a.lenAllocd;
    buff = new bufferStore[lenAllocd];
    for (int i = 0; i < len; i++)
        buff[i] = a.buff[i];
    return *this;
}

bufferStore &bufferArray::
operator [](const unsigned long index)
{
    return buff[index];
}

bufferArray bufferArray::
operator +(const bufferStore &s)
{
    bufferArray res = *this;
    res += s;
    return res;
}

bufferArray bufferArray::
operator +(const bufferArray &a)
{
    bufferArray res = *this;
    res += a;
    return res;
}

bufferArray &bufferArray::
operator +=(const bufferArray &a)
{
    lenAllocd += a.lenAllocd;
    bufferStore *nb = new bufferStore[lenAllocd];
    for (int i = 0; i < len; i++)
        nb[len + i] = buff[i];
    for (int i = 0; i < a.len; i++)
        nb[len + i] = a.buff[i];
    len += a.len;
    delete []buff;
    buff = nb;
    return *this;
}

bufferArray &bufferArray::
operator +=(const bufferStore &s)
{
    append(s);
    return *this;
}
