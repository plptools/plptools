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

#ifndef _PSIBITMAP_H_
#define _PSIBITMAP_H_

#include "bufferstore.h"

/**
 * This function is used by encodeBitmap for retrieving image data.
 * It must return a gray value between 0 and 255 where 0 is black and
 * 255 is white.
 *
 * @param x The x coordinate of the pixel to get (0 = left)
 * @param y The y coordinate of the pixel to get (0 = top)
 */
typedef int (*getPixelFunction_t)(int x, int y);

/**
 * Convert an image into a bitmap in Psion format.
 *
 * @param width    The width of the image to convert.
 * @param height   The height of the image to convert.
 * @param getPixel Pointer to a function for retrieving pixel values.
 * @param rle      Flag: Perform RLE compression (currently ignored).
 * @param out      Output buffer; gets filled with the Psion representation
 *                 of the converted image.
 */
extern void
encodeBitmap(int width, int height, getPixelFunction_t getPixel,
	     bool rle, bufferStore &out);

/**
 * Convert a Psion bitmap to a 8bit/pixel grayscale image.
 *
 * @param p Pointer to an input buffer which contains the Psion-formatted
 *          bitmap to convert. Must start with a Psion bitmap header.
 * @param width  On return, the image width in pixels is returned here.
 * @param height On return, the image height in pixels is returned here.
 * @param out    Buffer which gets filled with the raw image data. Each pixel
 *               is represented by a byte. The image data is organized in
 *               height scanlines of width bytes, starting with the topmost
 *               scanline.
 *
 * @returns      true on success, false if input data is inconsistent.
 */
extern bool
decodeBitmap(const unsigned char *p, int &width, int &height, bufferStore &out);

#endif // !_PSIBITMAP_H_
