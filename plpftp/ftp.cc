/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2006-2025 Reuben Thomas <rrt@sc3d.org>
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

#include <rfsv.h>
#include <rpcs.h>
#include <rclip.h>
#include <plpintl.h>
#include <ppsocket.h>
#include <bufferarray.h>
#include <bufferstore.h>
#include <Enum.h>

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <netdb.h>
#include <fnmatch.h>

#include "ignore-value.h"
#include "string-buffer.h"
#include "xalloc.h"
#include "xvasprintf.h"

#include "ftp.h"

extern "C"  {
#include "yesno.h"

#if defined(HAVE_READLINE_READLINE_H)
#  include <readline/readline.h>
#elif defined(HAVE_READLINE_H)
#  include <readline.h>
#else /* !defined(HAVE_READLINE_H) */
extern char *readline ();
#endif /* !defined(HAVE_READLINE_H) */
#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  elif defined(HAVE_HISTORY_H)
#    include <history.h>
#  endif /* !defined(HAVE_READLINE_HISTORY_H) */
#endif /* !defined(HAVE_READLINE_HISTORY) */
}

using namespace std;

static char *psionDir;
static rfsv *comp_a;
static int continueRunning;

#define CLIPFILE "C:/System/Data/Clpboard.cbd"


void ftp::
resetUnixWd()
{
    localDir = getcwd(NULL, 0);
    assert(localDir != NULL);
}

ftp::ftp()
{
    resetUnixWd();
}

ftp::~ftp()
{
    free(localDir);
}

void ftp::usage() {
    cout << _("Known FTP commands:") << endl << endl;
    cout << "  pwd" << endl;
    cout << "  ren <oldname> <newname>" << endl;
    cout << "  touch <psionfile>" << endl;
    cout << "  gtime <psionfile>" << endl;
    cout << "  test <psionfile>" << endl;
    cout << "  gattr <psionfile>" << endl;
    cout << "  sattr [[-|+]rwhsa] <psionfile>" << endl;
    cout << "  devs" << endl;
    cout << "  dir|ls" << endl;
    cout << "  dircnt" << endl;
    cout << "  cd <dir>" << endl;
    cout << "  lcd <dir>" << endl;
    cout << "  !<system command>" << endl;
    cout << "  get <psionfile>" << endl;
    cout << "  put <unixfile>" << endl;
    cout << "  mget <shellpattern>" << endl;
    cout << "  mput <shellpattern>" << endl;
    cout << "  cp <psionfile> <psionfile>" << endl;
    cout << "  del|rm <psionfile>" << endl;
    cout << "  mkdir <psiondir>" << endl;
    cout << "  rmdir <psiondir>" << endl;
    cout << "  volname <drive> <name>" << endl;
    cout << "  prompt" << endl;
    cout << "  hash" << endl;
    cout << "  bye" << endl;
    cout << endl << _("Known RPC commands:") << endl << endl;
    cout << "  ps" << endl;
    cout << "  kill <pid|'all'>" << endl;
    cout << "  getclip <unixfile>" << endl;
    cout << "  putclip <unixfile>" << endl;
    cout << "  run <psionfile> [args]" << endl;
    cout << "  killsave <unixfile>" << endl;
    cout << "  runrestore <unixfile>" << endl;
    cout << "  machinfo" << endl;
    cout << "  ownerinfo" << endl;
    cout << "  settime" << endl;
    cout << "  setupinfo" << endl;
}

static char *join_string_vector(vector<char *> argv, const char *sep)
{
    struct string_buffer sb;
    sb_init(&sb);
    int argc = argv.size();
    for (int i = 0; i < argc; i++) {
        sb_append(&sb, argv[i]);
        if (i < argc - 1)
            sb_append(&sb, sep);
    }
    return sb_dupfree(&sb);
}

static int
checkAbortNoHash(void *, uint32_t)
{
    return continueRunning;
}

static int
checkAbortHash(void *, uint32_t)
{
    if (continueRunning) {
        printf("#"); fflush(stdout);
    }
    return continueRunning;
}

static void
sigint_handler(int i) {
    continueRunning = 0;
    signal(SIGINT, sigint_handler);
}

static void
sigint_handler2(int i) {
    continueRunning = 0;
    fclose(stdin);
    signal(SIGINT, sigint_handler2);
}

static int
stopPrograms(rpcs & r, const char *file) {
    Enum<rfsv::errs> res;
    processList tmp;
    FILE *fp = fopen(file, "w");

    if (fp == NULL) {
        cerr << _("Could not open command list file ") << file << endl;
        return 1;
    }
    fputs("#plpftp processlist\n", fp);
    if ((res = r.queryPrograms(tmp)) != rfsv::E_PSI_GEN_NONE) {
        cerr << _("Could not get process list, Error: ") << res << endl;
        return 1;
    }
    for (processList::iterator i = tmp.begin(); i != tmp.end(); i++) {
        fputs(i->getArgs(), fp);
        fputc('\n', fp);
    }
    fclose(fp);
    time_t tstart = time(0) + 5;
    while (!tmp.empty()) {
        for (processList::iterator i = tmp.begin(); i != tmp.end(); i++) {
            r.stopProgram(i->getProcId());
        }
        usleep(100000);
        if (time(0) > tstart) {
            cerr << _(
                      "Could not stop all processes. Please stop running\n"
                      "programs manually on the Psion, then hit return.") << flush;
            cin.getline((char *)&tstart, 1);
            tstart = time(0) + 5;
        }
        if ((res = r.queryPrograms(tmp)) != rfsv::E_PSI_GEN_NONE) {
            cerr << _("Could not get process list, Error: ") << res << endl;
            return 1;
        }
    }
    return 0;
}

static char *
get_upto(FILE *fp, const char *term, size_t *final_len)
{
    size_t len = 256;
    int c;
    char *l = (char *)malloc(len), *s = l;

    assert(l);
    for (c = getc(fp); c != EOF && strchr(term, c) == NULL; c = getc(fp)) {
        if (s == l + len) {
            l = (char *)realloc(l, len * 2);
            assert(l);
            len *= 2;
        }
        *s++ = c;
    }
    if (s == l + len) {
        l = (char *)realloc(l, len + 1);
        assert(l);
    }

    if (final_len)
        *final_len = s - l;

    *s++ = '\0';
    l = (char *)realloc(l, s - l);
    assert(l);
    return l;
}

static char*
getln(FILE *fp)
{
    return get_upto(fp, "\n", NULL);
}

static int
startPrograms(rpcs & r, rfsv & a, const char *file) {
    Enum<rfsv::errs> res;
    FILE *fp = fopen(file, "r");
    string cmd;

    if (fp == NULL) {
        cerr << _("Could not open command list file ") << file << endl;
        return 1;
    }
    cmd = string(getln(fp));
    if (strcmp(cmd.c_str(), "#plpftp processlist")) {
        fclose(fp);
        cerr << _("Error: ") << file <<
            _(" is not a process list saved with killsave") << endl;
        return 1;
    }
    for (cmd = string(getln(fp)); cmd.length() > 0; cmd = string(getln(fp))) {
        int firstBlank = cmd.find(' ');
        string prog = string(cmd, 0, firstBlank);
        string arg = string(cmd, firstBlank + 1);

        if (!prog.empty()) {
            // Workaround for broken programs like Backlite. These do not store
            // the full program path. In that case we try running the arg1 which
            // results in starting the program via recog. facility.
            if ((arg.size() > 2) && (arg[1] == ':') && (arg[0] >= 'A') &&
                (arg[0] <= 'Z'))
                res = r.execProgram(arg.c_str(), "");
            else
                res = r.execProgram(prog.c_str(), arg.c_str());
            if (res != rfsv::E_PSI_GEN_NONE) {
                // If we got an error here, that happened probably because
                // we have no path at all (e.g. Macro5) and the program is
                // not registered in the Psion's path properly. Now try
                // the usual \System\Apps\<AppName>\<AppName>.app
                // on all drives.
                if (prog.find('\\') == prog.npos) {
                    uint32_t devbits;
                    if ((res = a.devlist(devbits)) == rfsv::E_PSI_GEN_NONE) {
                        int i;
                        for (i = 0; i < 26; i++) {
                            if (devbits & (1 << i)) {
                                string tmp = string();
                                tmp += ('A' + i);
                                tmp += ":\\System\\Apps\\" + prog + "\\" + prog + ".app";
                                res = r.execProgram(tmp.c_str(), "");
                                if (res == rfsv::E_PSI_GEN_NONE)
                                    break;
                            }
                        }
                    }
                }
            }
            if (res != rfsv::E_PSI_GEN_NONE) {
                cerr << _("Could not start ") << cmd << endl;
                cerr << _("Error: ") << res << endl;
            }
        }
    }
    return 0;
}

bool
ftp::checkClipConnection(rfsv &a, rclip & rc, ppsocket & rclipSocket)
{
    if (canClip == false)
        return false;

    if (a.getProtocolVersion() == 3) {
        cerr << _("Clipboard protocol not supported by Psion Series 3.") << endl;
        return false;
    }

    Enum<rfsv::errs> ret;
    if ((ret = rc.initClipbd()) == rfsv::E_PSI_GEN_NONE)
        return true;
    else if (ret == rfsv::E_PSI_GEN_NSUP)
        cerr << _("Your Psion does not support the clipboard protocol.\n\
                The reason for that is usually a missing server library.\n\
                Make sure that C:\\System\\Libs\\clipsvr.rsy exists.\n\
                This file is part of PsiWin and usually gets copied to\n\
                your Psion when you enable CopyAnywhere in PsiWin.\n\
                You can also get it from a PsiWin installation directory\n\
                and copy it to your Psion manually.") << endl;
    return false;
}

static void
psiText2ascii(char *buf, int len) {
    char *p;

    for (p = buf; len; len--, p++)
        switch (*p) {
            case 6:
            case 7:
                *p = '\n';
                break;
            case 8:
                *p = '\f';
                break;
            case 10:
                *p = '\t';
                break;
            case 11:
            case 12:
                *p = '-';
                break;
            case 15:
            case 16:
                *p = ' ';
                break;
        }
}

static void
ascii2PsiText(char *buf, int len) {
    char *p;

    for (p = buf; len; len--, p++)
        switch (*p) {
            case '\0':
                *p = ' ';
            case '\n':
                *p = 6;
                break;
            case '\f':
                *p = 8;
                break;
            case '-':
                *p = 11;
                break;
        }
}

static char *
slurp(FILE *fp, size_t *final_len)
{
    return get_upto(fp, "", final_len);
}

int
ftp::putClipText(rpcs & r, rfsv & a, rclip & rc, ppsocket & rclipSocket, const char *file)
{
    Enum<rfsv::errs> res;
    uint32_t fh;
    uint32_t l;
    const unsigned char *p;
    bufferStore b;
    char *data;
    FILE *fp;

    if (!checkClipConnection(a, rc, rclipSocket))
        return 1;

    if ((fp = fopen(file, "r")) == NULL)
        return 1;

    size_t len;
    data = slurp(fp, &len);
    fclose(fp);
    ascii2PsiText(data, len);

    res = a.freplacefile(0x200, CLIPFILE, fh);
    if (res == rfsv::E_PSI_GEN_NONE) {
        // Base Header
        b.addDWord(0x10000037);   // @00 UID 0
        b.addDWord(0x1000003b);   // @04 UID 1
        b.addDWord(0);            // @08 UID 3
        b.addDWord(0x4739d53b);   // @0c Checksum the above

        // Section Table
        b.addDWord(0x00000014);   // @10 Offset of Section Table
        b.addByte(2);             // @14 Section Table, length in DWords
        b.addDWord(0x10000033);   // @15 Section Type (ASCII)
        b.addDWord(0x0000001d);   // @19 Section Offset

        // Data
        b.addDWord(strlen(data)); // @1e Section (String) length
        b.addStringT(data);       // @22 Data (Psion Word seems to need a
                                  //     terminating 0.

        p = (const unsigned char *)b.getString(0);
        a.fwrite(fh, p, b.getLen(), l);
        a.fclose(fh);
        a.fsetattr(CLIPFILE, 0x20, 0x07);
    }

    return 0;
}

// FIXME: Make this work as putclipimg

// static QImage *putImage;

// static int
// getGrayPixel(int x, int y)
// {
//     return qGray(putImage->pixel(x, y));
// }

// void TopLevel::
// putClipImage(QImage &img) {
//     Enum<rfsv::errs> res;
//     uint32_t fh;
//     uint32_t l;
//     const unsigned char *p;
//     bufferStore b;

//     res = rf->freplacefile(0x200, CLIPFILE, fh);
//     if (res == rfsv::E_PSI_GEN_NONE) {
//         while ((res = rc->checkNotify()) != rfsv::E_PSI_GEN_NONE) {
//             if (res != rfsv::E_PSI_FILE_EOF) {
//                 rf->fclose(fh);
//                 closeConnection();
//                 return;
//             }
//         }

//         // Base Header
//         b.addDWord(0x10000037);   // @00 UID 0
//         b.addDWord(0x1000003b);   // @04 UID 1
//         b.addDWord(0);            // @08 UID 3
//         b.addDWord(0x4739d53b);   // @0c Checksum the above

//         // Section Table
//         b.addDWord(0x00000014);   // @10 Offset of Section Table
//         b.addByte(2);             // @14 Section Table, length in DWords
//         b.addDWord(0x1000003d);   // @15 Section Type (Image)
//         b.addDWord(0x0000001d);   // @19 Section Offset

//         // Data
//         bufferStore ib;
//         putImage = &img;
//         encodeBitmap(img.width(), img.height(), getGrayPixel, false, ib);
//         b.addBuff(ib);

//         p = (const unsigned char *)b.getString(0);
//         rf->fwrite(fh, p, b.getLen(), l);
//         rf->fclose(fh);
//         rf->fsetattr(CLIPFILE, 0x20, 0x07);
//     } else
//         closeConnection();
// }

// QImage *TopLevel::
// decode_image(const unsigned char *p)
// {
//     bufferStore out;
//     bufferStore hout;
//     QImage *img = 0L;
//     int xPixels;
//     int yPixels;

//     if (!decodeBitmap(p, xPixels, yPixels, out))
//         return img;

//     QString hdr = QString("P5\n%1 %2\n255\n").arg(xPixels).arg(yPixels);
//     hout.addString(hdr.latin1());
//     hout.addBuff(out);

//     img = new QImage(xPixels, yPixels, 8);
//     if (!img->loadFromData((const uchar *)hout.getString(0), hout.getLen())) {
//         delete img;
//         img = 0L;
//     }
//     return img;
// }

int
ftp::getClipData(rpcs & r, rfsv & a, rclip & rc, ppsocket & rclipSocket, const char *file) {
    Enum<rfsv::errs> res;
    PlpDirent de;
    uint32_t fh;
    string clipText;

    res = a.fgeteattr(CLIPFILE, de);
    if (res == rfsv::E_PSI_GEN_NONE) {
        if (de.getAttr() & rfsv::PSI_A_ARCHIVE) {
            uint32_t len = de.getSize();
            char *buf = (char *)malloc(len);

            if (!buf) {
                cerr << "Out of memory in getClipData" << endl;
                return 1;
            }
            res = a.fopen(a.opMode(rfsv::PSI_O_RDONLY | rfsv::PSI_O_SHARE),
                           CLIPFILE, fh);
            if (res == rfsv::E_PSI_GEN_NONE) {
                uint32_t tmp;
                res = a.fread(fh, (unsigned char *)buf, len, tmp);
                a.fclose(fh);

                if ((res == rfsv::E_PSI_GEN_NONE) && (tmp == len)) {
                    char *p = buf;
                    int lcount;
                    uint32_t     *ti = (uint32_t*)buf;

                    // Check base header
                    if (*ti++ != 0x10000037) {
                        free(buf);
                        return 1;
                    }
                    if (*ti++ != 0x1000003b) {
                        free(buf);
                        return 1;
                    }
                    if (*ti++ != 0) {
                        free(buf);
                        return 1;
                    }
                    if (*ti++ != 0x4739d53b) {
                        free(buf);
                        return 1;
                    }

                    // Start of section table
                    p = buf + *ti;
                    // Length of section table (in DWords)
                    lcount = *p++;

                    uint32_t *td = (uint32_t*)p;
                    while (lcount > 0) {
                        uint32_t sType = *td++;
                        if (sType == 0x10000033) {
                            // An ASCII section
                            p = buf + *td;
                            len = *((uint32_t*)p);
                            p += 4;
                            psiText2ascii(p, len);
                            clipText += (char *)p;
                        }
                        if (sType == 0x1000003d) {
// FIXME: Implement this
//                             // A paint data section
//                             p = buf + *td;
//                             if (clipImg)
//                                 delete clipImg;
//                             clipImg = decode_image((const unsigned char *)p);
                        }
                        td++;
                        lcount -= 2;
                    }
                }

            }
            free(buf);
        }
    }

    FILE *fp = fopen(file, "w");
    if (fp == NULL)
        return 1;
    fwrite(clipText.c_str(), 1, clipText.length(), fp);
    return 0;
}

/* Compute parent directory of an EPOC directory. */
static char *epoc_dirname(const char *path) {
    char *f1 = xstrdup(path);
    char *p = f1 + strlen(f1);

    /* Skip trailing slash. */
    if (p > f1)
        *--p = '\0';

    /* Skip backwards to next slash. */
    while ((p > f1) && (*p != '/') && (*p != '\\'))
        p--;

    /* If we have just a drive letter, colon and slash, keep exactly that. */
    if (p - f1 < 3)
        p = f1 + 3;

    /* Truncate the string at the current point, and return it. */
    *p = '\0';

    return f1;
}

/* Compute new directory from path, which may be absolute or relative, and
   cwd. */
static char *epoc_dir_from(const char *path) {
    char *f1;

    /* If we have asked for parent dir, get dirname of cwd. */
    if (!strcmp(path, "..")) {
        f1 = epoc_dirname(psionDir);
    } else {
        /* If path is relative, append it to cwd. */
        if ((path[0] != '/') && (path[0] != '\\') && (path[1] != ':'))
            f1 = xasprintf("%s%s", psionDir, path);
        /* Otherwise, path is absolute, so duplicate it. */
        else
            f1 = xstrdup(path);
    }

    /* Ensure path ends with a slash. */
    if ((f1[strlen(f1) - 1] != '/') && (f1[strlen(f1) - 1] != '\\')) {
        char *f2 = xasprintf("%s%s", f1, "\\");
        free(f1);
        f1 = f2;
    }

    /* Convert forward slashes in new path to backslashes. */
    for (char *p = f1; *p; p++)
        if (*p == '/')
            *p = '\\';

    return f1;
}

int ftp::
session(rfsv & a, rpcs & r, rclip & rc, ppsocket & rclipSocket, vector<char *> argv)
{
    Enum<rfsv::errs> res;
    bool prompt = true;
    bool hash = false;
    cpCallback_t cab = checkAbortNoHash;
    bool once = false;

    unsigned argc = argv.size();
    if (argc > 0)
        once = true;

    {
        Enum<rpcs::machs> machType;
        bufferArray b;
        if ((res = r.getOwnerInfo(b)) == rfsv::E_PSI_GEN_NONE) {
            r.getMachineType(machType);
            if (!once) {
                int speed = a.getSpeed();
                cout << _("Connected to a ") << machType << _(" at ")
                     << speed << _(" baud, OwnerInfo:") << endl;
                while (!b.empty())
                    cout << "  " << b.pop().getString() << endl;
                cout << endl;
            }
        } else
            cerr << _("OwnerInfo returned error ") << res << endl;
    }

    if (!strcmp(DDRIVE, "AUTO")) {
        uint32_t devbits;
        int i;

        strcpy(defDrive, "::");
        if (a.devlist(devbits) == rfsv::E_PSI_GEN_NONE) {

            for (i = 0; i < 26; i++) {
                PlpDrive drive;
                if ((devbits & 1) && a.devinfo(i + 'A', drive) == rfsv::E_PSI_GEN_NONE) {
                    defDrive[0] = 'A' + i;
                    break;
                }
                devbits >>= 1;
            }
        }
        if (!strcmp(defDrive, "::")) {
            cerr << _("FATAL: Couldn't find default Drive") << endl;
            return -1;
        }
    } else
        strcpy(defDrive, DDRIVE);
    free(psionDir);
    psionDir = xasprintf("%s%s", defDrive, DBASEDIR);
    comp_a = &a;
    if (!once) {
        cout << _("Psion dir is: \"") << psionDir << "\"" << endl;
        initReadline();
    }
    continueRunning = 1;
    signal(SIGINT, sigint_handler);
    do {
        if (!once) {
            argv = getCommand();
            argc = argv.size();
        }

        if (argc == 0) {
            continue;
        }
        if ((!strcmp(argv[0], "help")) || (!strcmp(argv[0], "?"))) {
            usage();
            continue;
        }
        if (!strcmp(argv[0], "prompt")) {
            prompt = !prompt;
            cout << _("Prompting now ") << (prompt? _("on") : _("off")) << endl;
            continue;
        }
        if (!strcmp(argv[0], "hash")) {
            hash = !hash;
            cout << _("Hash printing now ") << (hash? _("on") : _("off")) << endl;
            cab = (hash) ? checkAbortHash : checkAbortNoHash;
            continue;
        }
        if (!strcmp(argv[0], "pwd")) {
            cout << _("Local dir: \"") << localDir << "\"" << endl;
            cout << _("Psion dir: \"") << psionDir << "\"" << endl;
            continue;
        }
        if (!strcmp(argv[0], "volname") && (argc == 3) && (strlen(argv[1]) == 1)) {
            if ((res = a.setVolumeName(toupper(argv[1][0]), argv[2])) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            continue;

        }
        if (!strcmp(argv[0], "ren") && (argc == 3)) {
            char *f1 = xasprintf("%s%s", psionDir, argv[1]);
            char *f2 = xasprintf("%s%s", psionDir, argv[2]);
            if ((res = a.rename(f1, f2)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            free(f1);
            free(f2);
            continue;
        }
        if (!strcmp(argv[0], "cp") && (argc == 3)) {
            char *f1 = xasprintf("%s%s", psionDir, argv[1]);
            char *f2 = xasprintf("%s%s", psionDir, argv[2]);
            if ((res = a.copyOnPsion(f1, f2, NULL, cab)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            free(f1);
            free(f2);
            continue;
        }
        if (!strcmp(argv[0], "touch") && (argc == 2)) {
            char *f1 = xasprintf("%s%s", psionDir, argv[1]);
            PsiTime pt;
            if ((res = a.fsetmtime(f1, pt)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            free(f1);
            continue;
        }
        if (!strcmp(argv[0], "test") && (argc == 2)) {
            PlpDirent e;
            char *f1 = xasprintf("%s%s", psionDir, argv[1]);
            if ((res = a.fgeteattr(f1, e)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            else
                cout << e << endl;
            free(f1);
            continue;
        }
        if (!strcmp(argv[0], "gattr") && (argc == 2)) {
            uint32_t attr;
            char *f1 = xasprintf("%s%s", psionDir, argv[1]);
            if ((res = a.fgetattr(f1, attr)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            else {
                cout << hex << setw(4) << setfill('0') << attr;
                cout << " (" << a.attr2String(attr) << ")" << endl;
            }
            free(f1);
            continue;
        }
        if (!strcmp(argv[0], "gtime") && (argc == 2)) {
            PsiTime mtime;
            char *f1 = xasprintf("%s%s", psionDir, argv[1]);
            if ((res = a.fgetmtime(f1, mtime)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            else
                cout << mtime << "(" << hex
                     << setw(8) << setfill('0') << mtime.getPsiTimeHi()
                     << ":"
                     << setw(8) << setfill('0') << mtime.getPsiTimeLo()
                     << ")" << endl;
            free(f1);
            continue;
        }
        if (!strcmp(argv[0], "sattr") && (argc == 3)) {
            long attr[2];
            int aidx = 0;
            char *p = argv[1];

            char *f1 = xasprintf("%s%s", psionDir, argv[2]);

            attr[0] = attr[1] = 0;
            while (*p) {
                switch (*p) {
                    case '+':
                        aidx = 0;
                        break;
                    case '-':
                        aidx = 1;
                        break;
                    case 'r':
                        attr[aidx] |= rfsv::PSI_A_READ;
                        attr[aidx] &= ~rfsv::PSI_A_READ;
                        break;
                    case 'w':
                        attr[1 - aidx] |= rfsv::PSI_A_RDONLY;
                        attr[aidx] &= ~rfsv::PSI_A_RDONLY;
                        break;
                    case 'h':
                        attr[aidx] |= rfsv::PSI_A_HIDDEN;
                        attr[1 - aidx] &= ~rfsv::PSI_A_HIDDEN;
                        break;
                    case 's':
                        attr[aidx] |= rfsv::PSI_A_SYSTEM;
                        attr[1 - aidx] &= ~rfsv::PSI_A_SYSTEM;
                        break;
                    case 'a':
                        attr[aidx] |= rfsv::PSI_A_ARCHIVE;
                        attr[1 - aidx] &= ~rfsv::PSI_A_ARCHIVE;
                        break;
                }
                p++;
            }
            if ((res = a.fsetattr(f1, attr[0], attr[1])) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            free(f1);
            continue;
        }
        if (!strcmp(argv[0], "dircnt")) {
            uint32_t cnt;
            if ((res = a.dircount(psionDir, cnt)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            else
                cout << cnt << _(" Entries") << endl;
            continue;
        }
        if (!strcmp(argv[0], "devs")) {
            uint32_t devbits;
            if ((res = a.devlist(devbits)) == rfsv::E_PSI_GEN_NONE) {
                cout << _("Drive Type Volname     Total     Free      UniqueID") << endl;
                for (int i = 0; i < 26; i++) {
                    PlpDrive drive;

                    if ((devbits & 1) != 0) {
                        if (a.devinfo(i + 'A', drive) == rfsv::E_PSI_GEN_NONE)
                            cout << (char) ('A' + i) << "     " << hex
                                 << setw(4) << setfill('0')
                                 << drive.getMediaType() << " " << setw(12)
                                 << setfill(' ') << setiosflags(ios::left)
                                 << drive.getName().c_str()
                                 << resetiosflags(ios::left) << dec << setw(9)
                                 << drive.getSize() << setw(9)
                                 << drive.getSpace() << "  " << setw(8)
                                 << setfill('0') << hex << drive.getUID()
                                 << dec << endl;
                    }
                    devbits >>= 1;
                }
            } else
                cerr << _("Error: ") << res << endl;
            continue;
        }
        if (!strcmp(argv[0], "ls") || !strcmp(argv[0], "dir")) {
            PlpDir files;
            char *dname = argc > 1 ? epoc_dir_from(argv[1]) : xstrdup(psionDir);
            if ((res = a.dir(dname, files)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            else
                while (!files.empty()) {
                    cout << files[0] << endl;
                    files.pop_front();
                }
            free(dname);
            continue;
        }
        if (!strcmp(argv[0], "lcd")) {
            if (argc == 1)
                resetUnixWd();
            else {
                if (chdir(argv[1]) == 0) {
                    resetUnixWd();
                } else
                    cerr << _("No such directory") << endl
                         << _("Keeping original directory \"") << localDir << "\"" << endl;
            }
            continue;
        }
        if (!strcmp(argv[0], "cd")) {
            if (argc == 1) {
                free(psionDir);
                psionDir = xasprintf("%s%s", defDrive, DBASEDIR);
            } else {
                char *newDir = epoc_dir_from(argv[1]);
                uint32_t tmp;
                if ((res = a.dircount(newDir, tmp)) != rfsv::E_PSI_GEN_NONE) {
                    cerr << _("Error: ") << res << endl;
                    cerr << _("Keeping original directory \"") << psionDir << "\"" << endl;
                    free(newDir);
                } else {
                    free(psionDir);
                    psionDir = newDir;
                }
            }
            continue;
        }
        if ((!strcmp(argv[0], "get")) && (argc > 1)) {
            struct timeval stime;
            struct timeval etime;
            struct stat stbuf;

            char *f1 = xasprintf("%s%s", psionDir, argv[1]);
            char *f2 = xasprintf("%s%s%s", localDir, "/", argc == 2 ? argv[1] : argv[2]);
            gettimeofday(&stime, 0L);
            if ((res = a.copyFromPsion(f1, f2, NULL, cab)) != rfsv::E_PSI_GEN_NONE) {
                if (hash)
                    cout << endl;
                continueRunning = 1;
                cerr << _("Error: ") << res << endl;
            } else {
                if (hash)
                    cout << endl;
                gettimeofday(&etime, 0L);
                long dsec = etime.tv_sec - stime.tv_sec;
                long dhse = (etime.tv_usec / 10000) -
                    (stime.tv_usec /10000);
                if (dhse < 0) {
                    dsec--;
                    dhse = 100 + dhse;
                }
                float dt = dhse;
                dt /= 100.0;
                dt += dsec;
                stat(f2, &stbuf);
                float cps = (float)(stbuf.st_size) / dt;
                cout << _("Transfer complete, (") << dec << stbuf.st_size
                     << _(" bytes in ") << dsec << "."
                     << dhse << _(" secs = ") << cps << " cps)\n";
            }
            free(f1);
            free(f2);
            continue;
        } else if ((!strcmp(argv[0], "mget")) && (argc == 2)) {
            char *pattern = argv[1];
            PlpDir files;
            if ((res = a.dir(psionDir, files)) != rfsv::E_PSI_GEN_NONE) {
                cerr << _("Error: ") << res << endl;
                continue;
            }
            for (int i = 0; i < files.size(); i++) {
                PlpDirent e = files[i];
                long attr = e.getAttr();

                if (attr & (rfsv::PSI_A_DIR | rfsv::PSI_A_VOLUME))
                    continue;
                if (fnmatch(pattern, e.getName(), FNM_NOESCAPE) == FNM_NOMATCH)
                    continue;
                cout << _("Get \"") << e.getName() << "\" (y,n): ";
                bool yes = false;
                if (prompt) {
                    cout.flush();
                    yes = yesno();
                } else {
                    yes = true;
                    cout << "y ";
                    cout.flush();
                }
                if (yes) {
                    char *f1 = xasprintf("%s%s", psionDir, e.getName());
                    char *f2 = xasprintf("%s%s%s", localDir, "/", e.getName());
                    if ((res = a.copyFromPsion(f1, f2, NULL, cab)) != rfsv::E_PSI_GEN_NONE) {
                        if (hash)
                            cout << endl;
                        continueRunning = 1;
                        cerr << _("Error: ") << res << endl;
                        break;
                    } else {
                        if (hash)
                            cout << endl;
                        cout << _("Transfer complete\n");
                    }
                    free(f1);
                    free(f2);
                }
            }
            continue;
        }
        if (!strcmp(argv[0], "put") && (argc >= 2)) {
            struct timeval stime;
            struct timeval etime;
            struct stat stbuf;

            char *f1 = xasprintf("%s%s%s", localDir, "/", argv[1]);
            char *f2 = xasprintf("%s%s", psionDir, argc == 2 ? argv[1] : argv[2]);
            gettimeofday(&stime, 0L);
            if ((res = a.copyToPsion(f1, f2, NULL, cab)) != rfsv::E_PSI_GEN_NONE) {
                if (hash)
                    cout << endl;
                continueRunning = 1;
                cerr << _("Error: ") << res << endl;
            } else {
                if (hash)
                    cout << endl;
                gettimeofday(&etime, 0L);
                long dsec = etime.tv_sec - stime.tv_sec;
                long dhse = (etime.tv_usec / 10000) -
                    (stime.tv_usec /10000);
                if (dhse < 0) {
                    dsec--;
                    dhse = 100 + dhse;
                }
                float dt = dhse;
                dt /= 100.0;
                dt += dsec;
                stat(f1, &stbuf);
                float cps = (float)(stbuf.st_size) / dt;
                cout << _("Transfer complete, (") << dec << stbuf.st_size
                     << _(" bytes in ") << dsec << "."
                     << dhse << _(" secs = ") << cps << " cps)\n";
            }
            free(f1);
            free(f2);
            continue;
        }
        if ((!strcmp(argv[0], "mput")) && (argc == 2)) {
            char *pattern = argv[1];
            DIR *d = opendir(localDir);
            if (d) {
                struct dirent *de;
                do {
                    de = readdir(d);
                    if (de) {
                        struct stat st;

                        if (fnmatch(pattern, de->d_name, FNM_NOESCAPE) == FNM_NOMATCH)
                            continue;
                        char *f1 = xasprintf("%s%s%s", localDir, "/", de->d_name);
                        if (stat(f1, &st) == 0 && S_ISREG(st.st_mode)) {
                            cout << _("Put \"") << de->d_name << "\" y,n: ";
                            bool yes = false;
                            if (prompt) {
                                cout.flush();
                                yes = yesno();
                            } else {
                                cout << "y ";
                                cout.flush();
                            }
                            if (yes) {
                                char *f2 = xasprintf("%s%s", psionDir, de->d_name);
                                if ((res = a.copyToPsion(f1, f2, NULL, cab)) != rfsv::E_PSI_GEN_NONE) {
                                    if (hash)
                                        cout << endl;
                                    continueRunning = 1;
                                    cerr << _("Error: ") << res << endl;
                                    free(f1);
                                    free(f2);
                                    break;
                                } else {
                                    if (hash)
                                        cout << endl;
                                    free(f2);
                                    cout << _("Transfer complete\n");
                                }
                            }
                        }
                        free(f1);
                    }
                } while (de);
                closedir(d);
            } else
                cerr << _("Error in directory name \"") << localDir << "\"\n";
            continue;
        }
        if ((!strcmp(argv[0], "del") ||
             !strcmp(argv[0], "rm")) && (argc == 2)) {
            char *f1 = xasprintf("%s%s", psionDir, argv[1]);
            if ((res = a.remove(f1)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            free(f1);
            continue;
        }
        if (!strcmp(argv[0], "mkdir") && (argc == 2)) {
            char *f1 = xasprintf("%s%s", psionDir, argv[1]);
            if ((res = a.mkdir(f1)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            free(f1);
            continue;
        }
        if (!strcmp(argv[0], "rmdir") && (argc == 2)) {
            char *f1 = xasprintf("%s%s", psionDir, argv[1]);
            if ((res = a.rmdir(f1)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            free(f1);
            continue;
        }
        if (argv[0][0] == '!') {
            char *cmd = join_string_vector(argv, " ");
            if (strlen(cmd))
                ignore_value(system(cmd));
            else {
                const char *sh;
                cout << _("Starting subshell ...\n");
                sh = getenv("SHELL");
                if (!sh)
                    sh = "/bin/sh";
                ignore_value(system(sh));
            }
            free(cmd);
            continue;
        }
        // RPCS commands
        if (!strcmp(argv[0], "settime")) {
            if ((res = r.setTime(time(NULL))) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            continue;
        }
        if (!strcmp(argv[0], "setupinfo")) {
            Enum<rfsv::errs> res;
            bufferStore db;

            if ((res = r.configRead(0, db)) != rfsv::E_PSI_GEN_NONE) {
                cerr << _("Error: ") << res << endl;
                continue;
            }
            if (db.getLen() < 1152) {
                cerr << _("Unknown setup info received") << endl;
                continue;
            }
            cout << _("Setup information:") << endl;
            cout << _("  Screen contrast:                  ") << dec
                 << db.getDWord(0x4c) + 1 << endl;
            cout << _("  Keyboard click:                   ")
                 << (db.getDWord(0x200) ?
                (db.getDWord(0x204) ? _("high") : _("low")) : _("off")) << endl;
            cout << _("  Screen click:                     ")
                 << (db.getDWord(0x20c) ?
                (db.getDWord(0x210) ? _("high") : _("low")) : _("off")) << endl;
            cout << _("  Error sound:                      ")
                 << (db.getDWord(0x214) ?
                (db.getDWord(0x218) ? _("high") : _("low")) : _("off")) << endl;
            cout << _("  Auto-switch off:                  ");
            switch (db.getDWord(0x228)) {
                case 0:
                    cout << _("never");
                    break;
                case 1:
                    cout << _("if running on battery power");
                    break;
                case 2:
                    cout << _("always");
                    break;
            }
            cout << endl;
            if (db.getDWord(0x228) != 0)
                cout << _("  Switch off after:                 ") << dec
                     << db.getDWord(0x22c) << _(" seconds") << endl;
            cout << _("  Backlight off after:              ") << dec
                 << db.getDWord(0x234) << _(" seconds") << endl;
            cout << _("  Switch on when tapping on screen: ")
                 << (db.getDWord(0x238) ? _("yes") : _("no")) << endl;
            cout << _("  Switch on when opening:           ")
                 << (db.getDWord(0x23c) ? _("yes") : _("no")) << endl;
            cout << _("  Switch off when closing:          ")
                 << (db.getDWord(0x23c) ? _("yes") : _("no")) << endl;
            cout << _("  Ask for password on startup:      ")
                 << (db.getDWord(0x29c) ? _("yes") : _("no")) << endl;
            cout << _("  Show Owner info on startup:       ");
            switch (db.getByte(0x3b0)) {
                case 0x31:
                    cout << _("never");
                    break;
                case 0x32:
                    cout << _("once a day");
                    break;
                case 0x33:
                    cout << _("always");
                    break;
            }
            cout << endl;
            continue;
        }
        if (!strcmp(argv[0], "run") && (argc >= 2)) {
            vector<char *> args = {argv.begin() + 1, argv.end()};
            char *arg = join_string_vector(args, " ");
            char *cmd;
            if (!strchr(argv[1], ':'))
                cmd = xasprintf("%s%s", psionDir, argv[1]);
            else
                cmd = xstrdup(argv[1]);
            r.execProgram(cmd, arg);
            free(arg);
            free(cmd);
            continue;
        }
        if (!strcmp(argv[0], "ownerinfo")) {
            bufferArray b;
            if ((res = r.getOwnerInfo(b)) != rfsv::E_PSI_GEN_NONE) {
                cerr << _("Error: ") << res << endl;
                continue;
            }
            while (!b.empty())
                cout << "  " << b.pop().getString() << endl;
            continue;
        }
        if (!strcmp(argv[0], "machinfo")) {
            rpcs::machineInfo mi;
            if ((res = r.getMachineInfo(mi)) != rfsv::E_PSI_GEN_NONE) {
                cerr << _("Error: ") << res << endl;
                continue;
            }

            cout << _("General:") << endl;
            cout << _("  Machine Type: ") << mi.machineType << endl;
            cout << _("  Machine Name: ") << mi.machineName << endl;
            cout << _("  Machine UID:  ") << hex << mi.machineUID << dec << endl;
            cout << _("  UI Language:  ") << mi.uiLanguage << endl;
            cout << _("ROM:") << endl;
            cout << _("  Version:      ") << mi.romMajor << "." << setw(2) << setfill('0') <<
                mi.romMinor << "(" << mi.romBuild << ")" << endl;
            cout << _("  Size:         ") << mi.romSize / 1024 << "k" << endl;
            cout << _("  Programmable: ") <<
                (mi.romProgrammable ? _("yes") : _("no")) << endl;
            cout << _("RAM:") << endl;
            cout << _("  Size:         ") << mi.ramSize / 1024 << "k" << endl;
            cout << _("  Free:         ") << mi.ramFree / 1024 << "k" << endl;
            cout << _("  Free max:     ") << mi.ramMaxFree / 1024 << "k" << endl;
            cout << _("RAM disk size:  ") << mi.ramDiskSize / 1024 << "k" << endl;
            cout << _("Registry size:  ") << mi.registrySize << endl;
            cout << _("Display size:   ") << mi.displayWidth << "x" <<
                mi.displayHeight << endl;
            cout << _("Time:") << endl;
            PsiTime pt(&mi.time, &mi.tz);
            cout << _("  Current time: ") << pt << endl;
            cout << _("  UTC offset:   ") << mi.tz.utc_offset << _(" seconds") << endl;
            cout << _("  DST:          ") <<
                (mi.tz.dst_zones & PsiTime::PSI_TZ_HOME ? _("yes") : _("no")) << endl;
            cout << _("  Timezone:     ") << mi.tz.home_zone << endl;
            cout << _("  Country Code: ") << mi.countryCode << endl;
            cout << _("Main battery:") << endl;
            pt.setPsiTime(&mi.mainBatteryInsertionTime);
            cout << _("  Changed at:   ") << pt << endl;
            cout << _("  Used for:     ") << mi.mainBatteryUsedTime << endl;
            cout << _("  Status:       ") << mi.mainBatteryStatus << endl;
            cout << _("  Current:      ") << mi.mainBatteryCurrent << " mA" << endl;
            cout << _("  UsedPower:    ") << mi.mainBatteryUsedPower << " mAs" << endl;
            cout << _("  Voltage:      ") << mi.mainBatteryVoltage << " mV" << endl;
            cout << _("  Max. voltage: ") << mi.mainBatteryMaxVoltage << " mV" << endl;
            cout << _("Backup battery:") << endl;
            cout << _("  Status:       ") << mi.backupBatteryStatus << endl;
            cout << _("  Voltage:      ") << mi.backupBatteryVoltage << " mV" << endl;
            cout << _("  Max. voltage: ") << mi.backupBatteryMaxVoltage << " mV" << endl;
            cout << _("External power:") << endl;
            cout << _("  Supplied:     ")
                 << (mi.externalPower ? _("yes") : _("no")) << endl;
            cout << _("  Used for:     ") << mi.externalPowerUsedTime << endl;
            continue;
        }
        if (!strcmp(argv[0], "runrestore") && (argc == 2)) {
            startPrograms(r, a, argv[1]);
            continue;
        }
        if (!strcmp(argv[0], "killsave") && (argc == 2)) {
            stopPrograms(r, argv[1]);
            continue;
        }
        if (!strcmp(argv[0], "putclip") && (argc == 2)) {
            if (putClipText(r, a, rc, rclipSocket, argv[1]))
                cerr << _("Error setting clipboard") << endl;
            continue;
        }
        if (!strcmp(argv[0], "getclip") && (argc == 2)) {
            if (getClipData(r, a, rc, rclipSocket, argv[1]))
                cerr << _("Error getting clipboard") << endl;
            continue;
        }
        if (!strcmp(argv[0], "kill") && (argc >= 2)) {
            processList tmp;
            bool anykilled = false;
            if ((res = r.queryPrograms(tmp)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            else {
                for (int i = 1; i < argc; i++) {
                    int kpid;
                    if (!strcmp(argv[i], "all"))
                        kpid = -1;
                    else
                        sscanf(argv[i], "%d", &kpid);
                    processList::iterator j;
                    for (j = tmp.begin(); j != tmp.end(); j++) {
                        if (kpid == -1 || kpid == j->getPID()) {
                            r.stopProgram(j->getProcId());
                            anykilled = true;
                        }
                    }
                    if (kpid == -1)
                        break;
                }
                if (!anykilled)
                    cerr << _("no such process") << endl;
            }
            continue;
        }
        if (!strcmp(argv[0], "ps")) {
            processList tmp;
            if ((res = r.queryPrograms(tmp)) != rfsv::E_PSI_GEN_NONE)
                cerr << _("Error: ") << res << endl;
            else {
                cout << "PID   CMD          ARGS" << endl;
                for (processList::iterator i = tmp.begin(); i != tmp.end(); i++)
                    cout << *i << endl;
            }
            continue;
        }
        if (strcmp(argv[0], "bye") == 0 || strcmp(argv[0], "quit") == 0)
            continueRunning = 0;
        else
            cerr << _("syntax error. Try \"help\"") << endl;
    } while (!once && continueRunning);
    return a.getStatus();
}

#define MATCHFUNCTION rl_completion_matches

static const char *all_commands[] = {
    "pwd", "ren", "touch", "gtime", "test", "gattr", "sattr", "devs",
    "dir", "ls", "dircnt", "cd", "lcd", "get", "put", "mget", "mput",
    "del", "rm", "mkdir", "rmdir", "prompt", "bye", "cp", "volname",
    "ps", "kill", "killsave", "runrestore", "run", "machinfo",
    "ownerinfo", "help", "settime", "setupinfo", NULL
};

static const char *localfile_commands[] = {
    "lcd ", "put ", "mput ", "killsave ", "runrestore ", NULL
};

static const char *remote_dir_commands[] = {
    "cd ", "rmdir ", NULL
};

static PlpDir comp_files;
static long maskAttr;
static char *cplPath;

static char*
filename_generator(const char *text, int state)
{
    static int len;
    string tmp;

    if (!state) {
        Enum<rfsv::errs> res;
        len = strlen(text);
        tmp = psionDir;
        // cplPath will always be non-NULL here, having been initialized in
        // do_completion.
        tmp += cplPath;
        tmp = rfsv::convertSlash(tmp);
        if ((res = comp_a->dir(tmp.c_str(), comp_files)) != rfsv::E_PSI_GEN_NONE) {
            cerr << _("Error: ") << res << endl;
            return NULL;
        }
    }
    while (!comp_files.empty()) {
        PlpDirent e = comp_files.front();
        long attr = e.getAttr();

        comp_files.pop_front();
        if ((attr & maskAttr) == 0)
            continue;
        tmp = cplPath;
        tmp += e.getName();
        if (!(strncmp(tmp.c_str(), text, len))) {
            if (attr & rfsv::PSI_A_DIR) {
                rl_completion_append_character = '\0';
                tmp += '/';
            }
            return (strdup(tmp.c_str()));
        }
    }
    return NULL;
}

static char *
command_generator(
                  const char *text,
                  int state)
{
    static int idx, len;
    const char *name;

    if (!state) {
        idx = 0;
        len = strlen(text);
    }
    while ((name = all_commands[idx])) {
        idx++;
        if (!strncmp(name, text, len))
            return (strdup(name));
    }
    return NULL;
}

extern "C" {

static char * null_completion(const char *, int) {
    static char null[1] = "";
    return null;
}

static char **
do_completion(const char *text, int start, int end)
{
    char **matches = NULL;

    rl_completion_entry_function = null_completion;
    rl_completion_append_character = ' ';
    rl_attempted_completion_over = 1;
    if (start == 0)
        matches = MATCHFUNCTION(text, command_generator);
    else {
        int idx = 0;
        const char *name;
        char *p;

        rl_filename_quoting_desired = 1;
        while ((name = localfile_commands[idx])) {
            idx++;
            if (!strncmp(name, rl_line_buffer, strlen(name))) {
                rl_completion_entry_function = NULL;
                return NULL;
            }
        }
        maskAttr = 0xffff;
        idx = 0;
        free(cplPath);
        cplPath = xstrdup(text);
        p = strrchr(cplPath, '/');
        if (p)
            *(++p) = '\0';
        else
            cplPath[0] = '\0';
        while ((name = remote_dir_commands[idx])) {
            idx++;
            if (!strncmp(name, rl_line_buffer, strlen(name)))
                maskAttr = rfsv::PSI_A_DIR;
        }

        matches = MATCHFUNCTION(text, filename_generator);
    }
    return matches;
}

}

void ftp::
initReadline(void)
{
    rl_readline_name = "plpftp";
    rl_completion_entry_function = null_completion;
    rl_attempted_completion_function = do_completion;
    rl_basic_word_break_characters = " \t\n\"\\'`@><=;|&{(";
    rl_completer_quote_characters = "\"";
}

vector<char *> ftp::
getCommand()
{
    int ws, quote;
    static char *buf;
    vector<char *> argv;

    // Free existing buffer, and reinitialize it.
    free(buf);
    buf = NULL;

    // Get new command.
    signal(SIGINT, sigint_handler2);
    buf = readline("> ");
    if (buf) {
        add_history(buf);

        // Parse command into argv.
        ws = 1; quote = 0;
        for (char *p = buf; *p; p++)
            switch (*p) {
            case ' ':
            case '\t':
                if (!quote) {
                    ws = 1;
                    *p = 0;
                }
                break;
            case '"':
                quote = 1 - quote;
                if (!quote)
                    *p = 0;
                break;
            default:
                if (ws)
                    argv.push_back(p);
                ws = 0;
            }
    } else {
        cout << "bye" << endl;
    }
    signal(SIGINT, sigint_handler);

    return argv;
}
