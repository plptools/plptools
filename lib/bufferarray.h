/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
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
#ifndef _BUFFERARRAY_H_
#define _BUFFERARRAY_H_

#include "config.h"

class BufferStore;

/**
 * An array of BufferStores
 */
class BufferArray {
public:
    /**
    * constructs a new BufferArray.
    * A minimum of @ref ALLOC_MIN
    * elements is allocated.
    */
    BufferArray();

    /**
    * Constructs a new BufferArray.
    *
    * @param a The initial contents for this array.
    */
    BufferArray(const BufferArray &a);

    /**
    * Destroys the BufferArray.
    */
    ~BufferArray();

    /**
    * Copys the BufferArray.
    */
    BufferArray &operator =(const BufferArray &a);

    /**
    * Checks if this BufferArray is empty.
    *
    * @return true if the BufferArray is empty.
    */
    bool empty() const;

    /**
    * Retrieves the BufferStore at given index.
    *
    * @return The BufferStore at index.
    */
    BufferStore &operator [](const unsigned long index);

    /**
    * Appends a BufferStore to a BufferArray.
    *
    * @param s The BufferStore to be appended.
    *
    * @returns A new BufferArray with BufferStore appended to.
    */
    BufferArray operator +(const BufferStore &s);

    /**
    * Concatenates two BufferArrays.
    *
    * @param a The BufferArray to be appended.
    *
    * @returns A new BufferArray consisting with a appended.
    */
    BufferArray operator +(const BufferArray &a);

    /**
    * Appends a BufferStore to current instance.
    *
    * @param s The BufferStore to append.
    *
    * @returns A reference to the current instance with s appended.
    */
    BufferArray &operator +=(const BufferStore &s);

    /**
    * Appends a BufferArray to current instance.
    *
    * @param a The BufferArray to append.
    *
    * @returns A reference to the current instance with a appended.
    */
    BufferArray &operator +=(const BufferArray &a);

    /**
    * Removes the first BufferStore.
    *
    * @return The removed BufferStore.
    */
    BufferStore pop(void);

    /**
    * Inserts a BufferStore at index 0.
    *
    * @param b The BufferStore to be inserted.
    */
    void push(const BufferStore& b);

    /**
    * Appends a BufferStore.
    *
    * @param b The BufferStore to be appended.
    */
    void append(const BufferStore& b);

    /**
    * Evaluates the current length.
    *
    * @return The current number of BufferStores
    */
    long length(void);

    /**
    * Empties the BufferArray.
    */
    void clear(void);

private:
    /**
    * Minimum number of BufferStores to
    * allocate.
    */
    static const long ALLOC_MIN = 5;

    /**
    * The current number of BufferStores in
    * this BufferArray.
    */
    long len;

    /**
    * The current number of BufferStores
    * allocated.
    */
    long lenAllocd;

    /**
    * The content.
    */
    BufferStore* buff;
};

inline bool BufferArray::empty() const { return len == 0; }

#endif
