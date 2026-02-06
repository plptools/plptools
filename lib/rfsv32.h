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

#ifndef _RFSV32_H_
#define _RFSV32_H_

#include "rfsv.h"
#include "plpdirent.h"

class rfsvfactory;

/**
 * This is the implementation of the @ref rfsv protocol for
 * Psion series 5 (EPOC) variant. You normally never create
 * objects of this class directly. Thus the constructor is
 * private. Use @ref rfsvfactory for creating an instance of
 * @ref rfsv . For a complete documentation, see @ref rfsv .
 */
class rfsv32 : public rfsv {

    /**
     * rfsvfactory may call our constructor.
     */
    friend class rfsvfactory;

public:
    Enum<rfsv::errs> dir(const char * const, PlpDir &);
    Enum<rfsv::errs> dircount(const char * const, uint32_t &);
    Enum<rfsv::errs> copyFromPsion(const char * const, const char * const, void *, cpCallback_t);
    Enum<rfsv::errs> copyFromPsion(const char *from, int fd, cpCallback_t cb);
    Enum<rfsv::errs> copyToPsion(const char * const, const char * const, void *, cpCallback_t);
    Enum<rfsv::errs> copyOnPsion(const char * const, const char * const, void *, cpCallback_t);
    Enum<rfsv::errs> pathtest(const char * const);
    Enum<rfsv::errs> mkdir(const char * const);
    Enum<rfsv::errs> rmdir(const char * const);
    Enum<rfsv::errs> remove(const char * const);
    Enum<rfsv::errs> rename(const char * const, const char * const);
    Enum<rfsv::errs> mktemp(uint32_t &, std::string &);
    Enum<rfsv::errs> fgeteattr(const char * const, PlpDirent &);
    Enum<rfsv::errs> fgetattr(const char * const, uint32_t &);
    Enum<rfsv::errs> fsetattr(const char * const, const uint32_t, const uint32_t);
    Enum<rfsv::errs> fgetmtime(const char * const, PsiTime &);
    Enum<rfsv::errs> fsetmtime(const char * const, PsiTime const);
    Enum<rfsv::errs> fopen(const uint32_t, const char * const, uint32_t &);
    Enum<rfsv::errs> fcreatefile(const uint32_t, const char * const, uint32_t &);
    Enum<rfsv::errs> freplacefile(const uint32_t, const char * const, uint32_t &);
    Enum<rfsv::errs> fseek(const uint32_t, const int32_t, const uint32_t, uint32_t &);
    Enum<rfsv::errs> fread(const uint32_t, unsigned char * const, const uint32_t, uint32_t &);
    Enum<rfsv::errs> fwrite(const uint32_t, const unsigned char * const, const uint32_t, uint32_t &);
    Enum<rfsv::errs> fsetsize(uint32_t, uint32_t);
    Enum<rfsv::errs> fclose(const uint32_t);

    Enum<rfsv::errs> devlist(uint32_t &);
    Enum<rfsv::errs> devinfo(const char, PlpDrive&);
    Enum<rfsv::errs> opendir(const uint32_t, const char * const, rfsvDirhandle &);
    Enum<rfsv::errs> readdir(rfsvDirhandle &, PlpDirent &);
    Enum<rfsv::errs> closedir(rfsvDirhandle &);
    Enum<rfsv::errs> setVolumeName(const char, const char * const);
    uint32_t opMode(const uint32_t);
    int getProtocolVersion() { return 5; }

private:

    enum file_attrib {
        EPOC_ATTR_RONLY      = 0x0001,
        EPOC_ATTR_HIDDEN     = 0x0002,
        EPOC_ATTR_SYSTEM     = 0x0004,
        EPOC_ATTR_DIRECTORY  = 0x0010,
        EPOC_ATTR_ARCHIVE    = 0x0020,
        EPOC_ATTR_VOLUME     = 0x0040,
        EPOC_ATTR_NORMAL     = 0x0080,
        EPOC_ATTR_TEMPORARY  = 0x0100,
        EPOC_ATTR_COMPRESSED = 0x0800,
        EPOC_ATTR_MASK       = 0x09f7,  /* All of the above */
        EPOC_ATTR_GETUID     = 0x10000000 /* Deliver UIDs on dir listing */
    };

    enum open_mode {
        EPOC_OMODE_SHARE_EXCLUSIVE = 0x0000,
        EPOC_OMODE_SHARE_READERS = 0x0001,
        EPOC_OMODE_SHARE_ANY = 0x0002,
        EPOC_OMODE_BINARY = 0x0000,
        EPOC_OMODE_TEXT = 0x0020,
        EPOC_OMODE_READ_WRITE = 0x0200
    };

    enum epoc_errs {
        E_EPOC_NONE = 0,
        E_EPOC_NOT_FOUND = -1,
        E_EPOC_GENERAL = -2,
        E_EPOC_CANCEL = -3,
        E_EPOC_NO_MEMORY = -4,
        E_EPOC_NOT_SUPPORTED = -5,
        E_EPOC_ARGUMENT = -6,
        E_EPOC_TOTAL_LOSS_OF_PRECISION = -7,
        E_EPOC_BAD_HANDLE = -8,
        E_EPOC_OVERFLOW = -9,
        E_EPOC_UNDERFLOW = -10,
        E_EPOC_ALREADY_EXISTS = -11,
        E_EPOC_PATH_NOT_FOUND = -12,
        E_EPOC_DIED = -13,
        E_EPOC_IN_USE = -14,
        E_EPOC_SERVER_TERMINATED = -15,
        E_EPOC_SERVER_BUSY = -16,
        E_EPOC_COMPLETION = -17,
        E_EPOC_NOT_READY = -18,
        E_EPOC_UNKNOWN = -19,
        E_EPOC_CORRUPT = -20,
        E_EPOC_ACCESS_DENIED = -21,
        E_EPOC_LOCKED = -22,
        E_EPOC_WRITE = -23,
        E_EPOC_DISMOUNTED = -24,
        E_EPOC_EoF = -25,
        E_EPOC_DISK_FULL = -26,
        E_EPOC_BAD_DRIVER = -27,
        E_EPOC_BAD_NAME = -28,
        E_EPOC_COMMS_LINE_FAIL = -29,
        E_EPOC_COMMS_FRAME = -30,
        E_EPOC_COMMS_OVERRUN = -31,
        E_EPOC_COMMS_PARITY = -32,
        E_EPOC_TIMEOUT = -33,
        E_EPOC_COULD_NOT_CONNECT = -34,
        E_EPOC_COULD_NOT_DISCONNECT = -35,
        E_EPOC_DISCONNECTED = -36,
        E_EPOC_BAD_LIBRARY_ENTRY_POINT = -37,
        E_EPOC_BAD_DESCRIPTOR = -38,
        E_EPOC_ABORT = -39,
        E_EPOC_TOO_BIG = -40,
        E_EPOC_DIVIDE_BY_ZERO = -41,
        E_EPOC_BAD_POWER = -42,
        E_EPOC_DIR_FULL = -43
    };

    enum commands {
        CLOSE_HANDLE     = 0x01,
        OPEN_DIR         = 0x10,
        READ_DIR         = 0x12,
        GET_DRIVE_LIST   = 0x13,
        DRIVE_INFO       = 0x14,
        SET_VOLUME_LABEL = 0x15,
        OPEN_FILE        = 0x16,
        TEMP_FILE        = 0x17,
        READ_FILE        = 0x18,
        WRITE_FILE       = 0x19,
        SEEK_FILE        = 0x1a,
        DELETE           = 0x1b,
        REMOTE_ENTRY     = 0x1c,
        FLUSH            = 0x1d,
        SET_SIZE         = 0x1e,
        RENAME           = 0x1f,
        MK_DIR_ALL       = 0x20,
        RM_DIR           = 0x21,
        SET_ATT          = 0x22,
        ATT              = 0x23,
        SET_MODIFIED     = 0x24,
        MODIFIED         = 0x25,
        SET_SESSION_PATH = 0x26,
        SESSION_PATH     = 0x27,
        READ_WRITE_FILE  = 0x28,
        CREATE_FILE      = 0x29,
        REPLACE_FILE     = 0x2a,
        PATH_TEST        = 0x2b,
        LOCK             = 0x2d,
        UNLOCK           = 0x2e,
        OPEN_DIR_UID     = 0x2f,
        DRIVE_NAME       = 0x30,
        SET_DRIVE_NAME   = 0x31,
        REPLACE          = 0x32
    };

    /**
    * Private constructor. Shall be called by
    * rfsvfactory only.
    */
    rfsv32(ppsocket *);

    Enum<rfsv::errs> err2psierr(int32_t);
    Enum<rfsv::errs> fopendir(const uint32_t, const char *, uint32_t &);
    uint32_t attr2std(const uint32_t);
    uint32_t std2attr(const uint32_t);


    // Communication
    bool sendCommand(enum commands, bufferStore &);
    Enum<rfsv::errs> getResponse(bufferStore &);
};

#endif
