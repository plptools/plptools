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
#ifndef _RFSV16_H_
#define _RFSV16_H_

#include "rfsv.h"

class rfsvfactory;

/**
 * This is the implementation of the @ref rfsv protocol for
 * Psion series 3 (SIBO) variant. You normally never create
 * objects of this class directly. Thus the constructor is
 * private. Use @ref rfsvfactory for creating an instance of
 * @ref rfsv . For a complete documentation, see @ref rfsv .
 */
class rfsv16 : public rfsv {

    /**
     * rfsvfactory may call our constructor.
     */
    friend class rfsvfactory;

public:
    Enum<rfsv::errs> fopen(const uint32_t, const char * const, uint32_t &);
    Enum<rfsv::errs> mktemp(uint32_t &, std::string &);
    Enum<rfsv::errs> fcreatefile(const uint32_t, const char * const, uint32_t &);
    Enum<rfsv::errs> freplacefile(const uint32_t, const char * const, uint32_t &);
    Enum<rfsv::errs> fclose(const uint32_t);
    Enum<rfsv::errs> dir(const char * const, PlpDir &);
    Enum<rfsv::errs> pathtest(const char * const);
    Enum<rfsv::errs> fgetmtime(const char * const, PsiTime &);
    Enum<rfsv::errs> fsetmtime(const char * const, const PsiTime);
    Enum<rfsv::errs> fgetattr(const char * const, uint32_t &);
    Enum<rfsv::errs> fgeteattr(const char * const, PlpDirent &);
    Enum<rfsv::errs> fsetattr(const char * const, const uint32_t seta, const uint32_t unseta);
    Enum<rfsv::errs> dircount(const char * const, uint32_t &);
    Enum<rfsv::errs> devlist(uint32_t &);
    Enum<rfsv::errs> devinfo(const char, PlpDrive &);
    Enum<rfsv::errs> fread(const uint32_t, unsigned char * const, const uint32_t, uint32_t &);
    Enum<rfsv::errs> fwrite(const uint32_t, const unsigned char * const, const uint32_t, uint32_t &);
    Enum<rfsv::errs> copyFromPsion(const char * const, const char * const, void *, cpCallback_t);
    Enum<rfsv::errs> copyFromPsion(const char *from, int fd, cpCallback_t cb);
    Enum<rfsv::errs> copyToPsion(const char * const, const char * const, void *, cpCallback_t);
    Enum<rfsv::errs> copyOnPsion(const char *, const char *, void *, cpCallback_t);
    Enum<rfsv::errs> fsetsize(const uint32_t, const uint32_t);
    Enum<rfsv::errs> fseek(const uint32_t, const int32_t, const uint32_t, uint32_t &);
    Enum<rfsv::errs> mkdir(const char * const);
    Enum<rfsv::errs> rmdir(const char * const);
    Enum<rfsv::errs> rename(const char * const, const char * const);
    Enum<rfsv::errs> remove(const char * const);
    Enum<rfsv::errs> opendir(const uint32_t, const char * const, rfsvDirhandle &);
    Enum<rfsv::errs> readdir(rfsvDirhandle &, PlpDirent &);
    Enum<rfsv::errs> closedir(rfsvDirhandle &);
    Enum<rfsv::errs> setVolumeName(const char, const char * const);

    uint32_t opMode(const uint32_t);
    int getProtocolVersion() { return 3; }

private:
    enum commands {
	SIBO_FOPEN = 0, // File Open
	SIBO_FCLOSE = 2, // File Close
	SIBO_FREAD = 4, // File Read
	SIBO_FDIRREAD = 6, // Read Directory entries
	SIBO_FDEVICEREAD = 8, // Device Information
	SIBO_FWRITE = 10, // File Write
	SIBO_FSEEK = 12, // File Seek
	SIBO_FFLUSH = 14, // Flush
	SIBO_FSETEOF = 16,
	SIBO_RENAME = 18,
	SIBO_DELETE = 20,
	SIBO_FINFO = 22,
	SIBO_SFSTAT = 24,
	SIBO_PARSE = 26,
	SIBO_MKDIR = 28,
	SIBO_OPENUNIQUE = 30,
	SIBO_STATUSDEVICE = 32,
	SIBO_PATHTEST = 34,
	SIBO_STATUSSYSTEM = 36,
	SIBO_CHANGEDIR = 38,
	SIBO_SFDATE = 40,
	SIBO_RESPONSE = 42
    };

    enum fopen_attrib {
	P_FOPEN = 0x0000, /* Open file */
	P_FCREATE = 0x0001, /* Create file */
	P_FREPLACE = 0x0002, /* Replace file */
	P_FAPPEND = 0x0003, /* Append records */
	P_FUNIQUE = 0x0004, /* Unique file open */
	P_FSTREAM = 0x0000, /* Stream access to a binary file */
	P_FSTREAM_TEXT = 0x0010, /* Stream access to a text file */
	P_FTEXT = 0x0020, /* Record access to a text file */
	P_FDIR = 0x0030, /* Record access to a directory file */
	P_FFORMAT = 0x0040, /* Format a device */
	P_FDEVICE = 0x0050, /* Record access to device name list */
	P_FNODE = 0x0060, /* Record access to node name list */
	P_FUPDATE = 0x0100, /* Read and write access */
	P_FRANDOM = 0x0200, /* Random access */
	P_FSHARE = 0x0400 /* File can be shared */
    };

    enum status_enum {
	P_FAWRITE  = 0x0001, /* can the file be written to? */
	P_FAHIDDEN = 0x0002, /* set if file is hidden */
	P_FASYSTEM = 0x0004, /* set if file is a system file */
	P_FAVOLUME = 0x0008, /* set if the name is a volume name */
	P_FADIR    = 0x0010, /* set if file is a directory file */
	P_FAMOD    = 0x0020, /* has the file been modified? */
	P_FAREAD   = 0x0100, /* can the file be read? */
	P_FAEXEC   = 0x0200, /* is the file executable? */
	P_FASTREAM = 0x0400, /* is the file a byte stream file? */
	P_FATEXT   = 0x0800, /* is it a text file? */
	P_FAMASK   = 0x0f3f  /* All of the above */
    };

    /**
    * Private constructor. Shall be called by
    * rfsvfactory only.
    */
    rfsv16(ppsocket *);

    // Miscellaneous
    Enum<rfsv::errs> fopendir(const char * const, uint32_t &);
    uint32_t attr2std(const uint32_t);
    uint32_t std2attr(const uint32_t);

    // Communication
    bool sendCommand(enum commands, bufferStore &);
    Enum<rfsv::errs> getResponse(bufferStore &);
};

#endif
