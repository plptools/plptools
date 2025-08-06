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

#ifndef _SISCOMPONENTRECORD_H
#define _SISCOMPONENTRECORD_H

#include "sistypes.h"

class SISFile;

/**
 * The name of the component in this SIS file.
 * A single instance holds the names for all languages.
 *
 * There is one name record for each language.
 * First comes the lengths of all the names, as 32 bit integers.
 * Second comes pointers to the names, as an index in the complete
 * SISFile.
 */
class SISComponentNameRecord
{
public:

	SISComponentNameRecord();
	virtual ~SISComponentNameRecord();

	/**
	 * Populate the fields.
	 *
	 * @param buf The buffer to read data from.
	 * @param base The index where we start reading data.
	 * @param len The length of the buffer, for range checking.
	 * @param sisFile The container sis file.
	 */
	SisRC fillFrom(uint8_t* buf, int* base, off_t len, SISFile* sisFile);

	/**
	 * Find out the end position for the last name in the file.
	 */
	uint32_t getLastEnd();

	/**
	 * Return the name for the given language.
	 *
	 * @param no The sequence number in the list of language records in
	 * the sis file.
	 */
	uint8_t* getName(int no);

private:

	uint32_t* m_nameLengths;
	uint32_t* m_namePtrs;

	/**
	 * The extracted names, as zero terminated strings.
	 */
	uint8_t** m_names;

	/**
	 * The number of names, so we know how much to delete.
	 */
	int m_nameCount;

};

#endif
