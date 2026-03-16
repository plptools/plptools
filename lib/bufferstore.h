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
#ifndef _BUFFERSTORE_H_
#define _BUFFERSTORE_H_

#include "config.h"

#include <iostream>
#include <cstdint>

#include <sys/types.h>

/**
 * A generic container for an array of bytes.
 *
 * BufferStore provides an array of bytes which
 * can be accessed using various types.
 */
class BufferStore {
public:
    /**
    * Constructs a new BufferStore.
    */
    BufferStore();

    /**
    * Constructs a new BufferStore and
    * initializes its content.
    *
    * @param buf Pointer to data for initialization.
    * @param len Length of data for initialization.
    */
    BufferStore(const unsigned char *buf, long len);

    /**
    * Destroys a BufferStore instance.
    */
    ~BufferStore();

    /**
    * Constructs a new BufferStore and
    * initializes its content.
    *
    * @param b A BufferStore, whose content is
    * used for initialization.
    */
    BufferStore(const BufferStore &b);

    /**
    * Copies a BufferStore.
    */
    BufferStore &operator =(const BufferStore &);

    /**
    * Retrieves the length of a BufferStore.
    *
    * @returns The current length of the contents in bytes.
    */
    unsigned long getLen() const;

    /**
    * Retrieves the byte at index <em>pos</em>.
    *
    * @param pos The index of the byte to retrieve.
    *
    * @returns The value of the byte at index <em>pos</em>
    */
    unsigned char getByte(long pos = 0) const;

    /**
    * Retrieves the word at index <em>pos</em>.
    *
    * @param pos The index of the word to retrieve.
    *
    * @returns The value of the word at index <em>pos</em>
    */
    uint16_t getWord(long pos = 0) const;

    /**
    * Retrieves the dword at index <em>pos</em>.
    *
    * @param pos The index of the dword to retrieve.
    *
    * @returns The value of the dword at index <em>pos</em>
    */
    uint32_t getDWord(long pos = 0) const;

    /**
     * Retrieves the signed dword at index <em>pos</em>.
     *
     * @param pos The index of the signed dword  to retrieve.
     *
     * @returns The value of the signed dword at index <em>pos</em>
     */
    int32_t getSDWord(long pos = 0) const;

    /**
    * Retrieves the characters at index <em>pos</em>.
    *
    * @param pos The index of the characters to retrieve.
    *
    * @returns A pointer to characters at index <em>pos</em>
    */
    const char * getString(long pos = 0) const;

    /**
    * Removes bytes from the start of the buffer.
    *
    * @param len Number of bytes to remove.
    */
    void discardFirstBytes(int len = 0);

    /**
    * Prints a dump of the content.
    *
    * @param s The stream to write to.
    * @param m The BufferStore do be dumped.
    *
    * @returns The stream.
    */
    friend std::ostream &operator<<(std::ostream &s, const BufferStore &m);

    /**
    * Tests if the BufferStore is empty.
    *
    * @returns true, if the BufferStore is empty; false, if it contains data.
    */
    bool empty() const;

    /**
    * Initializes the BufferStore.
    *
    * All data is removed, the length is
    * reset to 0.
    */
    void init();

    /**
    * Initializes the BufferStore with
    * a given data.
    *
    * @param buf Pointer to data to initialize from.
    * @param len Length of data.
    */
    void init(const unsigned char * buf, long len);

    /**
    * Appends a byte to the content of this instance.
    *
    * @param c The byte to append.
    */
    void addByte(unsigned char c);

    /**
    * Appends a word to the content of this instance.
    *
    * @param w The word to append.
    */
    void addWord(int w);

    /**
    * Appends a dword to the content of this instance.
    *
    * @param dw The dword to append.
    */
    void addDWord(long dw);

    /**
    * Appends a string to the content of this instance.
    *
    * The trailing zero byte is <em>not</em> copied
    * to the content.
    *
    * @param s The string to append.
    */
    void addString(const char *s);

    /**
    * Appends a string to the content of this instance.
    *
    * The trailing zero byte <em>is</em> copied
    * to the content.
    *
    * @param s The string to append.
    */
    void addStringT(const char *s);

    /**
    * Appends data to the content of this instance.
    *
    * @param buf The data to append.
    * @param len Length of data.
    */
    void addBytes(const unsigned char *buf, int len);

    /**
    * Appends data to the content of this instance.
    *
    * @param b The BufferStore whose content to append.
    * @param maxLen Length of content to append. If
    *               @p maxLen is less than 0 or greater than
    *               the current length of @p b , then the
    *               whole content of @p b is appended.
    */
    void addBuff(const BufferStore &b, long maxLen = -1);

    /**
    * Truncates the buffer.
    * If the buffer is smaller, does nothing.
    *
    * @param newLen The new length of the buffer.
    */
    void truncate(long newLen);

    /**
    * Prepends a byte to the content of this instance.
    *
    * @param c The byte to append.
    */
    void prependByte(unsigned char c);

    /**
    * Prepends a word to the content of this instance.
    *
    * @param w The word to append.
    */
    void prependWord(int w);

private:
    void checkAllocd(long newLen);

    long len;
    long lenAllocd;
    long start;
    unsigned char * buff;

    enum c { MIN_LEN = 300 };
};

inline bool BufferStore::empty() const {
    return (len - start) == 0;
}

#endif
