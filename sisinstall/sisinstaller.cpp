/*
 * This file is part of plptools.
 *
 *  Copyright (C) 2002 Daniel Brahneborg <basic@chello.se>
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

#include "sisinstaller.h"

#include "sisfile.h"
#include "sisfilelink.h"
#include "sisfilerecord.h"
#include "sisreqrecord.h"
#include "psion.h"

#include <cstdlib>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>

#include "ignore-value.h"

static int continueRunning;

static int
checkAbortHash(void *, uint32_t)
{
        if (continueRunning)
                {
                if (logLevel >= 1)
                        {
                        printf("#");
                        fflush(stdout);
                        }
                }
        return continueRunning;
}

SISInstaller::SISInstaller()
{
        m_installed = nullptr;
        m_ownInstalled = false;
}

SISInstaller::~SISInstaller()
{
        if (m_ownInstalled)
                {
                SISFileLink* curr = m_installed;
                while (curr)
                        {
                        delete curr->m_file;
                        SISFileLink* next = curr->m_next;
                        delete curr;
                        curr = next;
                        }
                }
}

void
SISInstaller::createDirs(char* filename)
{
        char* end = filename + strlen(filename);
        while (--end > filename)
                {
                char ch = *end;
                if ((ch == '/') || (ch == '\\'))
                        {
                        *end = 0;
                        if (logLevel >= 1)
                                fprintf(stderr, "Checking for existance of %s\n", filename);
//                        if (!m_psion->dirExists(filename))
                                {
                                if (logLevel >= 1)
                                        fprintf(stderr, "Creating dir %s\n", filename);
                                Enum<rfsv::errs> res;
                                res = m_psion->mkdir(filename);
                                if ((res != rfsv::E_PSI_GEN_NONE) &&
                                        (res != rfsv::E_PSI_FILE_EXIST))
                                        {
                                                fprintf(stderr, " -> Failed: %s\n", (const char*)res);
                                        }
                                }
                        *end = ch;
                        return;
                        }
                }
}

void
SISInstaller::copyFile(SISFileRecord* fileRecord)
{
        uint8_t* destptr = fileRecord->getDestPtr();
        if (destptr == nullptr)
                return;
        if (destptr[0] == '!')
                {
                if (m_drive == 0)
                        selectDrive();
                m_file->setDrive(m_drive);
                }
        int len = fileRecord->m_destLength;
        char dest[256];
        memcpy(dest, destptr, len);
        dest[len] = 0;
        if (dest[0] == '!')
                {
                dest[0] = m_drive;
                fileRecord->setMainDrive(m_drive);
                if (logLevel >= 2)
                        fprintf(stderr, "Setting main drive to %c\n", m_drive);
                }

        char msgbuf[1024];
        sprintf(msgbuf,
                        "Copying %u bytes to %s\n",
                        fileRecord->m_fileLengths[m_fileNo], dest);
                        {
                        printf("%s\n", msgbuf);
                        }

        copyBuf(fileRecord->getFilePtr(m_fileNo),
                        fileRecord->m_fileLengths[m_fileNo],
                        dest);

}

void
SISInstaller::copyBuf(const uint8_t* buf, int len, char* name)
{
        createDirs(name);
        char srcName[32];
        strcpy(srcName, "/tmp/plptools-sis-XXXXXX");
        int fd = mkstemp(srcName);
        if (-1 == fd)
                {
                        fprintf(stderr,
                                        "Couldn't create temp file: %s\n", strerror(errno));
                return;
                }
        Enum<rfsv::errs> res;
        if (logLevel >= 2)
                fprintf(stderr, "Storing in %s\n", srcName);
        ssize_t written = write(fd, buf, len);
        int close_res = close(fd);
        bool write_ok = written == len && close_res == 0;
        if (write_ok) {
                continueRunning = 1;
                res = m_psion->copyToPsion(srcName, name, NULL, checkAbortHash);
        }
        if (write_ok && res == rfsv::E_PSI_GEN_NONE)
                {
                if (logLevel >= 1)
                        fprintf(stderr, " -> Success.\n");
                }
        else
                {
                        fprintf(stderr, " -> Fail: %s\n", (const char*)res);
                }
        unlink(srcName);
}

int
SISInstaller::installFile(SISFileRecord* fileRecord)
{
        char readbuf[8];
        switch (fileRecord->m_fileType)
                {
                case 0:
                        copyFile(fileRecord);
                        break;
                case 1:

                                {
                                printf("Info:\n%.*s\n",
                                       (int) fileRecord->m_fileLengths[m_fileNo],
                                           fileRecord->getFilePtr(m_fileNo));
                                switch (fileRecord->m_fileDetails)
                                        {
                                        case 0:
                                                printf("Continue\n");
                                                ignore_value(fgets(readbuf, sizeof(readbuf), stdin));
                                                break;
                                        case 1: {
                                                printf("(Install next file?) [Y]es/No\n");
                                                char *res = fgets(readbuf, sizeof(readbuf), stdin);
                                                if (!res || strchr("Nn", readbuf[0]))
                                                        {
                                                        return FILE_SKIP;
                                                        }
                                                break;
                                        }
                                        case 2: {
                                                printf("(Continue installation?) [Y]es/No\n");
                                                char *res = fgets(readbuf, sizeof(readbuf), stdin);
                                                if (!res || strchr("Nn", readbuf[0]))
                                                        {
                                                        // Watch out if we have copied any files
                                                        // already.
                                                        //
                                                        return FILE_ABORT;
                                                        }
                                                break;
                                        }
                                        }
                                }
                        break;
                case 2:
                        {
                        if (logLevel >= 1)
                                fprintf(stderr, "Recursive sis file...\n");
                        SISFile sisFile;
                        uint8_t* buf2 = fileRecord->getFilePtr(m_fileNo);
                        off_t len = fileRecord->m_fileLengths[m_fileNo];
//                        if (m_lastSisFile < fileptr + len)
//                                m_lastSisFile = fileptr + len;
                        SisRC rc = sisFile.fillFrom(buf2, len);
                        if (rc != SIS_OK)
                                {
                                fprintf(stderr,
                                                "Could not read contained sis file, rc = %d\n", rc);
                                break;
                                }
                        SISInstaller installer;
                        installer.setPsion(m_psion);
                        installer.setInstalled(m_installed);
                        rc = installer.run(&sisFile, buf2, len, m_file);
                        if (0 == m_drive)
                                {
                                m_drive = sisFile.m_header.m_installationDrive;
                                m_file->setDrive(m_drive);
                                if (logLevel >= 1)
                                        fprintf(stderr,
                                                        "Updated drive to %c from recursive sis file\n",
                                                        m_drive);
                                }
                        break;
                        }
                case 3:
                        if (logLevel >= 1)
                                fprintf(stderr, "Run %.*s during installation/remove\n",
                                        (int) fileRecord->m_destLength, fileRecord->getDestPtr());
                        break;
                case 4:
                        if (logLevel >= 2)
                                fprintf(stderr, "Running the app will create %.*s\n",
                                        (int) fileRecord->m_destLength, fileRecord->getDestPtr());
                        break;
                }
        return FILE_OK;
}

#define SYSTEMINSTALL "c:\\system\\install\\"

SisRC
SISInstaller::loadInstalled()
{
        PlpDir files;
        Enum<rfsv::errs> res;

        if ((res = m_psion->dir(SYSTEMINSTALL, files)) != rfsv::E_PSI_GEN_NONE)
                {
                return SIS_FAILED;
                }
        else
                {
                while (!files.empty())
                        {
                        PlpDirent file = files[0];
                        if (logLevel >= 1)
                                fprintf(stderr, "Loading sis file `%s'\n", file.getName());
                        char sisname[256];
                        sprintf(sisname, "%s%s", SYSTEMINSTALL, file.getName());
                        loadPsionSis(sisname);
                        files.pop_front();
                        }
                return SIS_OK;
                }
}

void
SISInstaller::loadPsionSis(const char* name)
{
        char srcName[32];
        strcpy(srcName, "/tmp/plptools-sis-XXXXXX");
        int fd = mkstemp(srcName);
        if (-1 == fd)
                {
                fprintf(stderr, "Couldn't create temp file: %s\n", strerror(errno));
                return;
                }
        Enum<rfsv::errs> res;
        continueRunning = 1;
        if (logLevel >= 2)
                fprintf(stderr, "Copying from %s to temp file %s\n", name, srcName);
        res = m_psion->copyFromPsion(name, fd, checkAbortHash);
        if (res == rfsv::E_PSI_GEN_NONE)
                {
                off_t fileLen = lseek(fd, 0, SEEK_END);
                if (logLevel >= 2)
                        fprintf(stderr, "Read %d bytes from the Psion file %s\n",
                                   (int)fileLen, name);
                lseek(fd, SEEK_SET, 0);
                uint8_t* sisbuf = new uint8_t[fileLen];
                int rc = read(fd, sisbuf, fileLen);
                if (rc == fileLen)
                        {
                        SISFile* sisFile = new SISFile();
                        SisRC rc2 = sisFile->fillFrom(sisbuf, fileLen);
                        if (rc2 == SIS_OK)
                                {
                                if (logLevel >= 1)
                                        fprintf(stderr, " Ok.\n");
                                SISFileLink* link = new SISFileLink(sisFile);
                                link->m_next = m_installed;
                                m_ownInstalled = true;
                                m_installed = link;
                                sisFile->ownBuffer();
                                }
                        else
                                {
                                delete sisFile;
                                delete[] sisbuf;
                                }
                        }
                }
        close(fd);
        unlink(srcName);
}

void
SISInstaller::removeFile(SISFileRecord* fileRecord)
{
        int len = fileRecord->m_destLength;
        char dest[256];
        memcpy(dest, fileRecord->getDestPtr(), len);
        dest[len] = 0;
        if (logLevel >= 1)
                fprintf(stderr, "Removing file component %s.\n", dest);
        m_psion->remove(dest);
}

SisRC
SISInstaller::run(SISFile* file, uint8_t* buf, off_t len)
{
        return run(file, buf, len, 0);
}

SisRC
SISInstaller::run(SISFile* file, uint8_t* buf, off_t len, SISFile* parent)
{
        int n;
        long lang;
        m_file = file;
        m_buf = buf;
        char msgbuf[1024];
        if (parent == 0)
                {
                n = m_file->m_header.m_nlangs;
                if (n == 1)
                        {
                        sprintf(msgbuf,
                                        _("You have only one language: %s"),
                                        m_file->getLanguage(0)->m_name);
                        printf("%s\n", msgbuf);
                        lang = 0;
                        }
                else
                        {
                                printf("Select a language (%d alternatives):\n", n);
                                for (int i = 0; i < n; ++i)
                                        printf(" %d. %s\n", i, m_file->getLanguage(i)->m_name);
                                lang = 0;
                        }
                }
        else
                {
                // This needs to check the _name_ of the language, since the
                // recursive sis file might have a different language list.
                // For now, defalt to 0.
                // lang = parent->getLanguage();
                lang = 0;
                if (logLevel >= 1)
                        fprintf(stderr, "Forcing language to %ld\n", lang);
                }
        m_file->setLanguage(lang);
        uint8_t* compName = m_file->getName();
        sprintf(msgbuf, _("Installing component: `%s'"), compName);
        printf("%s\n", msgbuf);

        // In order to check requisites and previous versions, we need to
        // load all sis files from the c:/system/install directory.
        // This is the only way to find out if a specific application or
        // library has been loaded, since the sis file names could be just
        // about anything.
        //
        if (m_installed == 0)
                loadInstalled();

        // Check Requisites.
        //
        n = m_file->m_header.m_nreqs;
        if (logLevel >= 1)
                fprintf(stderr, "Found %d requisites, of some sort.\n", n);
        for (int i = 0; i < n; ++i)
                {
                SISReqRecord* reqRecord = &m_file->m_reqRecords[i];
                if (logLevel >= 1)
                        fprintf(stderr,
                                " Check if app with uid %08x exists with version >= %d.%d\n",
                                reqRecord->m_uid, reqRecord->m_major, reqRecord->m_minor);
                }

        // Check previous version.
        //
        if (logLevel >= 1)
                fprintf(stderr,
  "Checking if this app (uid %08x) exists with a version less than %d.%d.\n",
                   m_file->m_header.m_uid1,
                   m_file->m_header.m_major,
                   m_file->m_header.m_minor);

        bool uninstallFirst = false;
        SISFileLink* curr = m_installed;
        SISFile* oldFile = 0;
        while (curr)
                {
                SISFile* sisFile = curr->m_file;
                switch (sisFile->compareApp(m_file))
                        {
                        case SIS_VER_EARLIER:
                                uninstallFirst = true;
                                oldFile = sisFile;
                                break;

                        case SIS_SAME_OR_LATER:
                                // Ask for confirmation.
                                uninstallFirst = true;
                                oldFile = sisFile;
                                break;

                        case SIS_OTHER_VARIANT:
                                // Ask for confirmation.
                                uninstallFirst = true;
                                oldFile = sisFile;
                                break;
                        }
                curr = curr->m_next;
                }

        if (uninstallFirst)
                {
//                printf("You should uninstall the previous version first.\n");
//                if (!m_forced)
//                        return SIS_ABORTED;
//                printf("Forced mode... Installing anyway!\n");
                if (oldFile == 0)
                        fprintf(stderr, "Already installed, but 0?\n");
                else
                        {
                        sprintf(msgbuf, "%s", _("Uninstalling the previous version first."));
                        printf("%s\n", msgbuf);
                        uninstall(oldFile);
                        }
                }

        // Install file components.
        //
        n = m_file->m_header.m_nfiles;
        if (logLevel >= 1)
                fprintf(stderr, "Found %d files.\n", n);
        m_drive = (parent == 0) ? 0 : parent->m_header.m_installationDrive;
        int nCopiedFiles = 0;
        m_lastSisFile = 0;
        bool skipnext = false;
        bool aborted = false;
        while (!aborted && (n-- > 0))
                {
                SISFileRecord* fileRecord = &m_file->m_fileRecords[n];
                m_fileNo = (fileRecord->m_flags & 1) ? lang : 0;

                if (skipnext)
                        {
                        skipnext = false;
                        }
                else
                        {
                        switch (installFile(fileRecord))
                                {
                                case FILE_OK:
                                        break;
                                case FILE_SKIP:
                                        skipnext = true;
                                        break;
                                case FILE_ABORT:
                                        aborted = true;
                                        break;
                                }
                        }
                if (!aborted)
                        nCopiedFiles++;
                }
        m_file->setFiles(nCopiedFiles);
        if (logLevel >= 1)
                fprintf(stderr,
                                "Installed %d files of %d, cutting at offset %u.\n",
                                m_file->m_header.m_installationFiles,
                                m_file->m_header.m_nfiles,
                                m_file->getResidualEnd());
        if (nCopiedFiles == 0)
                {
                // There is no need to copy any uninstall information to the
                // psion, unless we've actually copied anything there.
                //
                return SIS_ABORTED;
                }

        // Copy the updated sis file to the epoc machine.
        //
        char resname[256];
        int namelen = 0;
        while (compName[namelen] != 0)
                {
                if (compName[namelen] == ' ')
                        break;
                namelen++;
                }
        sprintf(resname, "C:\\System\\Install\\%.*s.sis", namelen, compName);
        if (logLevel >= 1)
                fprintf(stderr, "Creating residual sis file %s\n", resname);
        copyBuf(buf, m_file->getResidualEnd(), resname);
        if (aborted)
                return SIS_ABORTED;
        return SIS_OK;
}

void
SISInstaller::selectDrive()
{
        uint32_t devbits = 0;
        Enum<rfsv::errs> res;
        char drivelist[26];
        int ndrives = 0;
        if ((res = m_psion->devlist(devbits)) == rfsv::E_PSI_GEN_NONE)
                {
                for (int i = 0; i < 26; i++)
                        {
                        PlpDrive plpdrive;
                        if (((devbits & 1) != 0) &&
                                (m_psion->devinfo(i + 'A', plpdrive) == rfsv::E_PSI_GEN_NONE))
                                {
                                uint32_t mediaType = plpdrive.getMediaType();
                                if ((mediaType == 3) || (mediaType == 5))
                                        {
                                        drivelist[ndrives] = 'A' + i;
                                        printf("%c: %lud bytes free, %lud bytes total\n",
                                               'A' + i,
                                               plpdrive.getSpace(),
                                               plpdrive.getSize());
                                        ++ndrives;
                                        }
                                }
                        devbits >>= 1;
                        }
                }
        drivelist[ndrives] = 0;
        if (ndrives == 0)
                {
                m_drive = 'C';
                if (logLevel >= 1)
                        printf("Selecting the only drive %c\n", m_drive);
                }
        else if (ndrives == 1)
                {
                m_drive = drivelist[0];
                if (logLevel >= 1)
                        printf("Selecting the only drive %c\n", m_drive);
                }
        else
                {

                        {
                        printf("Please select a drive\n");
                        char ch;
                        char readbuf[2];
                        while (m_drive == 0)
                                {
                                if (fgets(readbuf, 2, stdin) == NULL)
                                        {
                                        printf("please enter a drive");
                                        continue;
                                        }
                                ch = readbuf[0];
                                if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
                                        {
                                        m_drive = toupper(ch);
                                        if (!strchr(drivelist, m_drive))
                                                {
                                                m_drive = 0;
                                                printf("Please select a valid drive: %s\n", drivelist);
                                                }
                                        }
                                }
                        }

                }
}

void
SISInstaller::setPsion(Psion* psion)
{
        m_psion = psion;
}

void
SISInstaller::uninstall(SISFile* file)
{
        int n = file->m_header.m_nfiles;
        int fileix = n - file->m_header.m_installationFiles;
        if (logLevel >= 1)
                fprintf(stderr, "Uninstalling %d files, from a total of %d.\n",
                                file->m_header.m_installationFiles,
                                file->m_header.m_nfiles);
        int lang = file->getLanguage();
        while (fileix < n)
                {
                SISFileRecord* fileRecord = &file->m_fileRecords[fileix];
                m_fileNo = (fileRecord->m_flags & 1) ? lang : 0;
                char drive = file->m_header.m_installationDrive;
                fileRecord->setMainDrive(drive);
                uninstallFile(fileRecord);
                ++fileix;
                }
}

void
SISInstaller::uninstallFile(SISFileRecord* fileRecord)
{
        switch (fileRecord->m_fileType)
                {
                case 0:
                case 4:
                        removeFile(fileRecord);
                        break;
                case 2:
                        {
#if 0
                        // This is messy... We can't remove the sis component unless
                        // we've stored the entire component in the residual sis
                        // file on the target machine.
                        if (logLevel >= 1)
                                fprintf(stderr, "Recursive sis file...\n");
                        SISFile sisFile;
                        int fileptr = fileRecord->m_filePtrs[m_fileNo];
                        uint8_t* buf2 = m_buf + fileptr;
                        off_t len = fileRecord->m_fileLengths[m_fileNo];
                        if (m_lastSisFile < fileptr + len)
                                m_lastSisFile = fileptr + len;
                        SisRC rc = sisFile.fillFrom(buf2, len);
                        if (rc != SIS_OK)
                                {
                                fprintf(stderr,
                                                "Could not read contained sis file, rc = %d\n", rc);
                                break;
                                }
                        SISInstaller installer;
                        installer.setPsion(m_psion);
                        installer.setInstalled(m_installed);
                        rc = installer.run(&sisFile, buf2, len, m_file);
                        if (0 == m_drive)
                                {
                                m_drive = sisFile.m_header.m_installationDrive;
                                m_file->setDrive(m_drive);
                                if (logLevel >= 1)
                                        fprintf(stderr,
                                                        "Updated drive to %c from recursive sis file\n",
                                                        m_drive);
                                }
#endif
                        break;
                        }
                }
}
