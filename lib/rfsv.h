/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
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
#ifndef _RFSV_H_
#define _RFSV_H_

#include <deque>
#include <string>

#include <Enum.h>
#include <plpdirent.h>
#include <bufferstore.h>

typedef std::deque<class PlpDirent> PlpDir;

class ppsocket;
class PlpDrive;

inline const int RFSV_SENDLEN = 2000;

/**
 * Defines the callback procedure for
 * progress indication of copy operations.
 */
typedef int (*cpCallback_t)(void *, uint32_t);

class rfsv16;
class rfsv32;

/**
 * A helper class for storing
 * intermediate internal information in rfsv16 and
 * rfsv32 .
 * @internal
 */
class rfsvDirhandle {
    friend class rfsv16;
    friend class rfsv32;

private:
    uint32_t h;
    bufferStore b;
};

/**
 * Access remote file services of a Psion.
 *
 * rfsv provides an API for accessing file services
 * of a Psion connected via ncpd. This class defines the
 * interface and a small amount of common constants and
 * methods. The majority of implementation is provided
 * by @ref rfsv32 and @ref rfsv16 , which implement the
 * variations of the protocol for EPOC and SIBO respectively.
 * Usually, the class @ref rfsvfactory is used to instantiate
 * the correct variant depending on the remote machine,
 * currently connected.
 */
class rfsv {
public:
    /**
    * The kown modes for seek.
    */
    enum seek_mode {
	PSI_SEEK_SET = 1,
	PSI_SEEK_CUR = 2,
	PSI_SEEK_END = 3
    };

    /**
    * The known modes for file open.
    */
    enum open_flags {
	PSI_O_RDONLY = 0000,
	PSI_O_WRONLY = 0001,
	PSI_O_RDWR   = 0002
    };

    /**
    * The known modes for file creation.
    */
    enum open_mode {
	PSI_O_CREAT  = 00100,
	PSI_O_EXCL   = 00200,
	PSI_O_TRUNC  = 01000,
	PSI_O_APPEND = 02000,
	PSI_O_SHARE  = 04000
    };

    /**
    * The known error codes.
    */
    enum errs {
	E_PSI_GEN_NONE	= 0,
	E_PSI_GEN_FAIL = -1,
	E_PSI_GEN_ARG = -2,
	E_PSI_GEN_OS = -3,
	E_PSI_GEN_NSUP = -4,
	E_PSI_GEN_UNDER = -5,
	E_PSI_GEN_OVER = -6,
	E_PSI_GEN_RANGE = -7,
	E_PSI_GEN_DIVIDE = -8,
	E_PSI_GEN_INUSE = -9,
	E_PSI_GEN_NOMEMORY = - 10,
	E_PSI_GEN_NOSEGMENTS = -11,
	E_PSI_GEN_NOSEM = -12,
	E_PSI_GEN_NOPROC = -13,
	E_PSI_GEN_OPEN = -14,
	E_PSI_GEN_NOTOPEN = -15,
	E_PSI_GEN_IMAGE = -16,
	E_PSI_GEN_RECEIVER = -17,
	E_PSI_GEN_DEVICE = -18,
	E_PSI_GEN_FSYS = -19,
	E_PSI_GEN_START = -20,
	E_PSI_GEN_NOFONT = -21,
	E_PSI_GEN_TOOWIDE = -22,
	E_PSI_GEN_TOOMANY = -23,
	E_PSI_FILE_EXIST = -32,
	E_PSI_FILE_NXIST = -33,
	E_PSI_FILE_WRITE = -34,
	E_PSI_FILE_READ = -35,
	E_PSI_FILE_EOF = -36,
	E_PSI_FILE_FULL = -37,
	E_PSI_FILE_NAME = -38,
	E_PSI_FILE_ACCESS = -39,
	E_PSI_FILE_LOCKED = -40,
	E_PSI_FILE_DEVICE = -41,
	E_PSI_FILE_DIR = -42,
	E_PSI_FILE_RECORD = -43,
	E_PSI_FILE_RDONLY = -44,
	E_PSI_FILE_INV = -45,
	E_PSI_FILE_PENDING = -46,
	E_PSI_FILE_VOLUME = -47,
	E_PSI_FILE_CANCEL = -48,
	E_PSI_FILE_ALLOC = -49,
	E_PSI_FILE_DISC = -50,
	E_PSI_FILE_CONNECT = -51,
	E_PSI_FILE_RETRAN = -52,
	E_PSI_FILE_LINE = -53,
	E_PSI_FILE_INACT = -54,
	E_PSI_FILE_PARITY = -55,
	E_PSI_FILE_FRAME = -56,
	E_PSI_FILE_OVERRUN = -57,
	E_PSI_MDM_CONFAIL = -58,
	E_PSI_MDM_BUSY = -59,
	E_PSI_MDM_NOANS = -60,
	E_PSI_MDM_BLACKLIST = -61,
	E_PSI_FILE_NOTREADY = -62,
	E_PSI_FILE_UNKNOWN = -63,
	E_PSI_FILE_DIRFULL = -64,
	E_PSI_FILE_PROTECT = -65,
	E_PSI_FILE_CORRUPT = -66,
	E_PSI_FILE_ABORT = -67,
	E_PSI_FILE_ERASE = -68,
	E_PSI_FILE_INVALID = -69,
	E_PSI_GEN_POWER = -100,
	E_PSI_FILE_TOOBIG = -101,
	E_PSI_GEN_DESCR = -102,
	E_PSI_GEN_LIB = -103,
	E_PSI_FILE_NDISC = -104,
	E_PSI_FILE_DRIVER = -105,
	E_PSI_FILE_COMPLETION = -106,
	E_PSI_GEN_BUSY = -107,
	E_PSI_GEN_TERMINATED = -108,
	E_PSI_GEN_DIED = -109,
	E_PSI_FILE_HANDLE = -110,

	// Special error code for "Operation not permitted in RFSV16"
	E_PSI_NOT_SIBO = -200,
	// Special error code for "internal library error"
	E_PSI_INTERNAL = -201
    };

    /**
    * The known file attributes
    */
    enum file_attribs {
	/** Attributes, valid on both <em>EPOC</em> and <em>SIBO</em>. */
	PSI_A_RDONLY     = 0x0001,
	PSI_A_HIDDEN     = 0x0002,
	PSI_A_SYSTEM     = 0x0004,
	PSI_A_DIR        = 0x0008,
	PSI_A_ARCHIVE    = 0x0010,
	PSI_A_VOLUME     = 0x0020,

	/** Attributes, valid on EPOC <em>only</em>. */
	PSI_A_NORMAL     = 0x0040,
	PSI_A_TEMP       = 0x0080,
	PSI_A_COMPRESSED = 0x0100,

	/** Attributes, valid on SIBO <em>only</em>. */
	PSI_A_READ       = 0x0200,
	PSI_A_EXEC       = 0x0400,
	PSI_A_STREAM     = 0x0800,
	PSI_A_TEXT       = 0x1000
    };

    virtual ~rfsv();
    void reset();
    void reconnect();

    /**
    * Retrieves the current connection status.
    *
    * @returns The status of the connection.
    */
    Enum<errs> getStatus();

    /**
    * Opens a file.
    *
    * @param attr   The open mode. Use @ref opMode to convert a combination of @ref open_flags
    *               and @ref open_mode to the machine-specific representation.
    * @param name   The name of the file to open.
    * @param handle The handle for usage with @ref fread ,
    *               @ref fwrite , @ref fseek or @ref fclose is returned here.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> fopen(const uint32_t attr, const char * const name, uint32_t &handle) = 0;

    /**
    * Creates a unique temporary file.
    * The file is opened for reading and writing.
    *
    * @param handle The handle for usage with @ref fread ,
    *               @ref fwrite , @ref fseek or @ref fclose is returned here.
    * @param name   The name of the temporary file is returned here.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> mktemp(uint32_t &handle, std::string &name) = 0;

    /**
    * Creates a named file.
    *
    * @param attr   The open mode. Use @ref opMode to convert a combination of @ref open_flags
    *               and @ref open_mode to the machine-specific representation.
    * @param name   The name of the file to create.
    * @param handle The handle for usage with @ref fread ,
    *               @ref fwrite , @ref fseek or @ref fclose is returned here.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> fcreatefile(const uint32_t attr, const char * const name, uint32_t &handle) = 0;

    /**
    * Creates an named file, overwriting an existing file.
    *
    * @param attr   The open mode. Use @ref opMode to convert a combination of @ref open_flags
    *               and @ref open_mode to the machine-specific representation.
    * @param name   The name of the file to create.
    * @param handle The handle for usage with @ref fread ,
    *               @ref fwrite , @ref fseek or @ref fclose is returned here.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> freplacefile(const uint32_t attr, const char * const name, uint32_t &handle) = 0;

    /**
    * Close a file on the Psion whih was previously opened/created by using
    * @ref fopen , @ref fcreatefile , @ref freplacefile or @ref mktemp .
    *
    * @param handle A valid file handle.
    */
    virtual Enum<errs> fclose(const uint32_t handle) = 0;

    /**
    * Reads a directory on the Psion.
    * The returned STL deque of @ref PlpDirent contains all
    * requested directory entries.
    *
    * @param name The name of the directory
    * @param ret  An STL deque of @ref PlpDirent entries.
    *
    * @returns A Psion error code (One of enum @ref rfsv::errs ).
    */
    virtual Enum<errs> dir(const char * const name, PlpDir &ret) = 0;

    /**
    * Retrieves the modification time of a file on the Psion.
    *
    * @param name Name of the file.
    * @param mtime Modification time is returned here.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> fgetmtime(const char * const name, PsiTime &mtime) = 0;

    /**
    * Sets the modification time of a file on the Psion.
    *
    * @param name Name of the file whose modification time should be set.
    * @param mtime The desired modification time.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> fsetmtime(const char * const name, const PsiTime mtime) = 0;

    /**
    * Retrieves attributes of a file on the Psion.
    *
    * @param name Name of the file whose attributes ar to be retrieved.
    * @param attr The file's attributes are returned here.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> fgetattr(const char * const name, uint32_t &attr) = 0;

    /**
     * Checks to see if the directory component of a path or file name exists
     * and is valid. Returns `E_PSI_GEN_NONE` if is valid.
     *
     * Only paths up to the last trailing backslash are tested; trailing
     * filenames are ignored. For example,
     *
     * - "C:\\" tests the validity of "C:\\"
     * - "C:\\System" tests the validity of "C:\\"
     * - "C:\\System\\" tests the validity of "C:\\System\\"
     * - "D:\\Documents\\hello.txt" tests the validity of "D:\\Documents\\"
     *
     * This can be safely used to test for the existence of directories and
     * drives on EPOC16 and EPOC32; check for `E_PSI_FILE_NXIST`,
     * `E_PSI_FILE_DIR`, `E_PSI_FILE_DEVICE`, and `E_PSI_FILE_NOTREADY` to
     * determine non-existence.
     *
     * @returns A Psion error code (One of enum @ref #errs ).
     */
    virtual Enum<errs> pathtest(const char * const name) = 0;

    /**
    * Retrieves attributes, size and modification time of a file on the Psion.
    *
    * @param name The name of the file.
    * @param e @ref PlpDirent object, filled with the information on return.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> fgeteattr(const char * const name, PlpDirent &e) =0;

    /**
    * @param name
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> fsetattr(const char * const name, const uint32_t seta, const uint32_t unseta) = 0;

    /**
    * Counts number of entries in a directory.
    *
    * @param name The directory whose entries are to be counted.
    * @param count The number of entries is returned here.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> dircount(const char * const name, uint32_t &count) = 0;

    /**
    * Retrieves available drives on the Psion.
    * @p devbits On return, for every exiting drive, a bit is set in this
    *                variable. The lowest bit represents drive A:.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> devlist(uint32_t &devbits) = 0;

    /**
    * Retrieves details about a drive.
    *
    * @param drive The drive character of the drive to get details from
    *              (e.g: 'C', 'D' etc.).
    *              (0 represents A:, 1 is B: and so on ...)
    * @param dinfo A @ref PlpDrive object which is filled with the drive's
    *              information upon return.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> devinfo(const char drive, PlpDrive &dinfo) = 0;

    /**
    * Reads from a file on the Psion.
    *
    * @param handle Handle of the file to read from.
    * @param buffer The area where to store the data read.
    * @param len The number of bytes to read.
    * @param count The number of bytes actually read is returned here.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> fread(const uint32_t handle, unsigned char * const buffer, const uint32_t len, uint32_t &count) = 0;

    /**
    * Write to a file on the Psion.
    *
    * @param handle Handle of the file to read from.
    * @param buffer The area to be written.
    * @param len The number of bytes to write.
    * @param count The number of bytes actually written is returned here.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> fwrite(const uint32_t handle, const unsigned char * const buffer, const uint32_t len, uint32_t &count) = 0;

    /**
    * Copies a file from the Psion to the local machine.
    *
    * @param from Name of the file on the Psion to be copied.
    * @param to Name of the destination file on the local machine.
    * @param func Pointer to a function which gets called on every read.
    * 	This function can be used to show some progress etc. May be set
    * 	to NULL, where no callback is performed. If the callback function
    * 	returns 0, the operation is aborted and E_PSI_FILE_CANCEL is returned.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> copyFromPsion(const char *from, const char *to, void *, cpCallback_t func) = 0;

    /**
    * Copies a file from the Psion to the local machine.
    */
    virtual Enum<rfsv::errs> copyFromPsion(const char *from, int fd, cpCallback_t cb) = 0;

    /**
    * Copies a file from local machine to the Psion.
    *
    * @param from Name of the file on the local machine to be copied.
    * @param to Name of the destination file on the Psion.
    * @param func Pointer to a function which gets called on every read.
    * 	This function can be used to show some progress etc. May be set
    * 	to NULL, where no callback is performed. If the callback function
    * 	returns 0, the operation is aborted and E_PSI_FILE_CANCEL is returned.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> copyToPsion(const char * const from, const char * const to, void *, cpCallback_t func) = 0;

    /**
    * Copies a file from the Psion to the Psion.
    * On the EPOC variants, this runs much faster than reading
    * data from the Psion and then writing it back to the Psion, since
    * data transfer is handled locally on the Psion.
    *
    * @param from Name of the file to be copied.
    * @param to Name of the destination file.
    * @param func Pointer to a function which gets called on every read.
    * 	This function can be used to show some progress etc. May be set
    * 	to NULL, where no callback is performed. If the callback function
    * 	returns 0, the operation is aborted and E_PSI_FILE_CANCEL is returned.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> copyOnPsion(const char * const from, const char * const to, void *, cpCallback_t func) = 0;

    /**
    * Resizes an open file on the Psion.
    * If the new size is greater than the file's
    * current size, the contents of the added
    * data is undefined. If The new size is smaller,
    * the file is truncated.
    *
    * @param handle Handle of the file to be resized.
    * @param size New size for that file.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> fsetsize(const uint32_t handle, const uint32_t size) = 0;

    /**
    * Sets the current file position of a file on the Psion.
    *
    * @param handle The file handle.
    * @param offset Position to be seeked to.
    * @param mode The mode for seeking.
    * @param resultpos The final file position after seeking is returned here.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> fseek(const uint32_t handle, const int32_t offset, const uint32_t mode, uint32_t &resultpos) = 0;

    /**
    * Creates a directory on the Psion.
    *
    * @param name Name of the directory to be created.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> mkdir(const char * const name) = 0;

    /**
    * Removes a directory on the Psion.
    *
    * @param name Name of the directory to be removed.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> rmdir(const char * const name) = 0;

    /**
    * Renames a file on the Psion.
    *
    * @param oldname Name of the file to be renamed.
    * @param newname New Name for that file.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> rename(const char * const oldname, const char * const newname) = 0;

    /**
    * Removes a file on the Psion.
    *
    * @param name Name of the file to be removed.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> remove(const char * const name) = 0;

    /**
    * Open a directory for reading with readdir.
    *
    * @param attr   A combination of PSI_A_.. flags, representing the desired types
    *               of entries to be returned when calling @ref readdir .
    * @param name   The name of the directory
    * @param handle A handle to be used with @ref readdir and @ref closedir .
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> opendir(const uint32_t attr, const char * const name, rfsvDirhandle &handle) = 0;

    /**
    * Read directory entries.
    * This method reads entries of a directory, previously
    * opened with @ref opendir .
    *
    * @param handle A handle, obtained by calling @ref opendir .
    * @param entry  The entry information is returned here.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> readdir(rfsvDirhandle &handle, PlpDirent &entry) = 0;

    /**
    * Close a directory, previously opened with @ref opendir.
    *
    * @param handle A handle, obtained by calling @ref opendir .
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> closedir(rfsvDirhandle &handle) = 0;

    /**
    * Set the name of a Psion Volume (Drive).
    *
    * @param drive The drive character of the Volume, whose name should be set.
    * @param name  The new name for that drive.
    *
    * @returns A Psion error code (One of enum @ref #errs ).
    */
    virtual Enum<errs> setVolumeName(const char drive, const char * const name) = 0;

    /**
    * Converts a file attribute @ref rfsv::file_attribs to
    * human readable format, usable for showing them in directory
    * listings. The first 7 characters are common to all
    * machine types:
    * <pre>
    * 	Char Nr. Value
    * 	0        'd' if a directory,                     '-' otherwise.
    * 	1        'r' if file is readable,                '-' otherwise.
    * 	2        'w' if file is writeable,               '-' otherwise.
    * 	3        'h' if file is hidden,                  '-' otherwise.
    * 	4        's' if file is a system file,           '-' otherwise.
    * 	5        'a' if file is modified (archive flag), '-' otherwise.
    * 	6        'v' if file is a volume name,           '-' otherwise.
    * </pre>
    * The rest (3 characters) are machine specific:
    * <pre>
    * 	Char Nr. EPOC Value          SIBO Value
    * 	7        'n' if normal,      'x' if executable, '-' otherwise.
    * 	8        't' if temporary,   'b' if a stream,   '-' otherwise.
    * 	8        'c' if compressed,  't' if a textfile, '-' otherwise.
    * </pre>
    *
    * @param attr the generic file attribute.
    *
    * @returns Pointer to static textual representation of file attributes.
    *
    */
    std::string attr2String(const uint32_t attr);

    /**
    * Converts an open-mode (A combination of the PSI_O_ constants.)
    * from generic representation to the machine-specific representation.
    *
    * @param mode The generic open mode.
    *
    * @returns The machine specific representation for use with
    *          @ref fopen , @ref fcreatefile and @freplacefile.
    */
    virtual uint32_t opMode(const uint32_t mode) = 0;

    /**
    * Utility method, converts '/' to '\'.
    */
    static std::string convertSlash(const std::string &name);

    /**
     * Retrieve speed of serial link.
     *
     * @returns The speed of the serial link in baud or -1 on error.
     */
    int getSpeed();

    /**
     * Retrieves the protocol version.
     *
     * @returns Either 3 or 5 representing Series 3 (SIBO) or Series 5 (EPOC)
     */
    virtual int getProtocolVersion() = 0;

protected:
    /**
    * Retrieves the PLP protocol name. Mainly internal use.
    *
    * @returns The connection name always "SYS$RFSV"
    */
    const char *getConnectName();

    ppsocket *skt;
    Enum<errs> status;
    int32_t serNum;
};

#endif
