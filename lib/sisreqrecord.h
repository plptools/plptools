/*
 * This file is part of plptools.
 *
 *  Copyright (C) 2002 Daniel Brahneborg <basic.chello@se>
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
 */

#ifndef _SISREQRECORD_H
#define _SISREQRECORD_H

#include "sistypes.h"

class SISFile;

/**
 * Information about an application that must be installed prior to the
 * current one.
 */
class SISReqRecord
{
public:

	/**
	 * Populate the fields.
	 *
	 * @param buf The buffer to read from.
	 * @param base The index to start reading from, which is updated
	 *   when the record is successfully read.
	 * @param len The length of the buffer.
	 * @param sisFile The container SISFile.
	 */
	SisRC fillFrom(uint8_t* buf, int* base, off_t len, SISFile* sisFile);

	uint32_t m_uid;
	uint16_t m_major;
	uint16_t m_minor;
	uint32_t m_variant;
	uint32_t* m_nameLengths;
	uint32_t* m_namePtrs;
};

#endif
