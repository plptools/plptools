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

#include <stdint.h>
#include "psibitmap.h"

void
encodeBitmap(int width, int height, getPixelFunction_t getPixel, bool /*rle*/,
             bufferStore &out) {
    bufferStore ib;

    ib.addDWord(0x00000028);   // hdrlen
    ib.addDWord(width);        // xPixels
    ib.addDWord(height);       // yPixels
    ib.addDWord(0);            // xTwips (unspecified)
    ib.addDWord(0);            // yTwips (unspecified)
    ib.addDWord(2);            // bitsPerPixel
    ib.addDWord(0);            // unknown1
    ib.addDWord(0);            // unknown2
    ib.addDWord(0);            // RLEflag

    bufferStore rawBuf;
    for (int y = 0; y < height; y++) {
        int ov = 0;
        int shift = 0;
        int bc = 0;

        for (int x = 0; x < width; x++) {
            int v = getPixel(x, y) / 85;
            ov |= (v << shift);
            if (shift == 6) {
                rawBuf.addByte(ov);
                bc++;
                shift = 0;
                ov = 0;
            } else
                shift += 2;
        }
        if (shift != 0) {
            rawBuf.addByte(ov);
            shift = 0;
            ov = 0;
            bc++;
        }
        while (bc % 4) {
            rawBuf.addByte(0);
            bc++;
        }
    }

#if 1
    ib.addBuff(rawBuf);
#else
    //TODO: RLE encoding

    int rawLen = rawBuf.getLen();
    int eqCount = 1;
    int lastByte = rawBuf.getByte(0);
    bufferStore diBuf;

    for (int i = 1; i <= rawLen; i++) {
        int v;
        if (i < rawLen)
            v = rawBuf.getByte(i);
        else
            v = lastByte + 1;
        if (v == lastByte) {
            if (diBuf.getLen()) {
                ib.addByte(0x100 - diBuf.getLen());
                ib.addBuff(diBuf);
                diBuf.init();
            }
            eqCount++;
            if (eqCount > 0x7f) {
                ib.addByte(0x7f);
                ib.addByte(v);
                eqCount = 1;
            }
        } else {
            if (eqCount > 1) {
                ib.addByte(eqCount);
                ib.addByte(lastByte);
                eqCount = 1;
            } else {
                diBuf.addByte(lastByte);
                if ((diBuf.getLen() > 0x7f) || (i == rawLen)) {
                    ib.addByte(0x100 - diBuf.getLen());
                    ib.addBuff(diBuf);
                    diBuf.init();
                }
            }
        }
        lastByte = v;
    }
#endif

    out.addDWord(ib.getLen() + 4);
    out.addBuff(ib);
}

#define splitByte(v)                                \
do {                                                \
    int j;                                          \
                                                    \
    if (x < bytesPerLine)                           \
        for (j = 0; j < pixelsPerByte; j++) {       \
            if (j && ((oidx % xPixels) == 0))       \
                break;                              \
            else                                    \
              if (oidx >= picsize)                  \
                return false;                       \
            else {                                  \
                out.addByte((v & mask) * grayVal);  \
                v >>= bitsPerPixel;                 \
                oidx++;                             \
            }                                       \
        }                                           \
    if (++x >= linelen)                             \
        x = 0;                                      \
} while (0)

bool
decodeBitmap(const unsigned char *p, int &width, int &height, bufferStore &out)
{
    uint32_t totlen = *((const uint32_t*)p); p += 4;
    uint32_t hdrlen = *((const uint32_t*)p); p += 4;
    uint32_t datlen = totlen - hdrlen;
    uint32_t xPixels = *((const uint32_t*)p); p += 4;
    uint32_t yPixels = *((const uint32_t*)p); p += 4;
    uint32_t xTwips = *((const uint32_t*)p); p += 4;
    uint32_t yTwips = *((const uint32_t*)p); p += 4;
    uint32_t bitsPerPixel = *((const uint32_t*)p); p += 4;
    uint32_t unknown1 = *((const uint32_t*)p); p += 4;
    uint32_t unknown2 = *((const uint32_t*)p); p += 4;
    uint32_t RLEflag = *((const uint32_t*)p); p += 4;

    width = xPixels;
    height = yPixels;

    uint32_t picsize = xPixels * yPixels;
    uint32_t linelen;
    int pixelsPerByte = (8 / bitsPerPixel);
    int nColors = 1 << bitsPerPixel;
    int grayVal = 255 / (nColors - 1);
    int bytesPerLine = (xPixels + pixelsPerByte - 1) / pixelsPerByte;
    int mask = (bitsPerPixel << 1) - 1;

    int oidx = 0;
    int x = 0;
    int y = 0;
    int offset = 0;

    if (RLEflag) {
        int i = 0;
        while (offset < datlen) {
            unsigned char b = *(p + offset);
            if (b >= 0x80) {
                offset += 0x100 - b + 1;
                i += 0x100 - b;
            } else {
                offset += 2;
                i += b + 1;
            }
        }
        linelen = i / yPixels;
        offset = 0;
        while (offset < datlen) {
            unsigned char b = *(p + offset++);
            if (b >= 0x80) {
                for (i = 0; i < 0x100 - b; i++, offset++) {
                    if (offset >= datlen)
                        return false; // data corrupted
                    unsigned char b2 = *(p + offset);
                    splitByte(b2);
                }
            } else {
                if (offset >= datlen)
                    return false;
                else {
                    unsigned char b2 = *(p + offset);
                    unsigned char bs = b2;
                    for (i = 0; i <= b; i++) {
                        splitByte(b2);
                        b2 = bs;
                    }
                }
                offset++;
            }
        }
    } else {
        linelen = datlen / yPixels;
        while (offset < datlen) {
            unsigned char b = *(p + offset++);
            splitByte(b);
        }
    }
    return true;
}
