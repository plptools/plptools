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

#include <ppsocket.h>
#include <wprt.h>
#include <psibitmap.h>

#include <iostream>
#include <string>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ignore-value.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

#define TEMPLATE "plpprint_XXXXXX"
#define PRINTCMD "lpr -Ppsion"
#define SPOOLDIR "/var/spool/plpprint"
#define PSDICT   "/prolog.ps"

using namespace std;

const char *spooldir = SPOOLDIR;
const char *printcmd = PRINTCMD;
wprt *wPrt;
bool serviceLoop;
bool debug = false;
int verbose = 0;

static void
_log(int priority, const char *fmt, va_list ap)
{
    char *buf;
    if (vasprintf(&buf, fmt, ap) == -1)
        return;
    if (debug)
        cout << buf << endl;
    else
        syslog(priority, "%s", buf);
    free(buf);
}

void
debuglog(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _log(LOG_DEBUG, fmt, ap);
    va_end(ap);
}

void
errorlog(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _log(LOG_ERR, fmt, ap);
    va_end(ap);
}

void
infolog(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _log(LOG_INFO, fmt, ap);
    va_end(ap);
}

static int minx, maxx, miny, maxy;
string usedfonts;

typedef struct fontmap_entry_s {
    struct fontmap_entry_s *next;
    char *psifont;
    bool bold;
    bool italic;
    char *psfont;
} fontmap_entry;

typedef struct const_fontmap_entry_s {
    const char *psifont;
    bool bold;
    bool italic;
    const char *psfont;
} const_fontmap_entry;

#define FALLBACK_FONT "Courier"

static const_fontmap_entry default_fontmap[] = {
    // PsionFontname, bold, italic, PSFontName
    { "Times New Roman", false, false, "Times-Roman"},
    { "Times New Roman", true,  false, "Times-Bold"},
    { "Times New Roman", false, true,  "Times-Italic"},
    { "Times New Roman", true,  true,  "Times-BoldItalic"},
    { "Arial",           false, false, "Helvetica"},
    { "Arial",           true,  false, "Helvetica-Bold"},
    { "Arial",           false, true,  "Helvetica-Oblique"},
    { "Arial",           true,  true,  "Helvetica-BoldOblique"},
    { "Courier New",     false, false, "Courier"},
    { "Courier New",     true,  false, "Courier-Bold"},
    { "Courier New",     false, true,  "Courier-Oblique"},
    { "Courier New",     true,  true,  "Courier-BoldOblique"},
    { "Swiss",           false, false, "Courier"},
    { "Swiss",           true,  false, "Courier-Bold"},
    { "Swiss",           false, true,  "Courier-Oblique"},
    { "Swiss",           true,  true,  "Courier-BoldOblique"},
    { NULL,              false, false, NULL}
};

static fontmap_entry *fontmap = NULL;

static void
init_fontmap() {
    FILE *f;
    fontmap_entry *new_fontmap = NULL;
    fontmap_entry *fe;

    if ((f = fopen(PKGDATADIR "/fontmap", "r"))) {
        char *p;
        int bold;
        int italic;
        char *psifont;
        char *psfont;
        char *tmp;
        char buf[1024];
        while (fgets(buf, sizeof(buf), f)) {
            char *bp = buf;
            int ne;
            if ((p = strchr(buf, '#')))
                *p = '\0';
            if ((p = strchr(buf, '\n')))
                *p = '\0';

            psifont = strsep(&bp, ":");
            if (!psifont || !(*psifont))
                continue;
            tmp = strsep(&bp, ":");
            if (!tmp || !(*tmp) || (sscanf(tmp, "%d", &bold) != 1))
                continue;
            tmp = strsep(&bp, ":");
            if (!tmp || !(*tmp) || (sscanf(tmp, "%d", &italic) != 1))
                continue;
            psfont = strsep(&bp, ":");
            if (!psfont || !(*psfont))
                continue;
            fe = (fontmap_entry *)malloc(sizeof(fontmap_entry));
            if (!fe)
                break;
            if (!(fe->psifont = strdup(psifont))) {
                free(fe);
                break;
            }
            if (!(fe->psfont = strdup(psfont))) {
                free(fe->psifont);
                free(fe);
                break;
            }
            fe->bold = bold ? true : false;
            fe->italic = italic ? true : false;
            fe->next = new_fontmap;
            new_fontmap = fe;
        }
        fclose(f);
    }
    if (new_fontmap && (new_fontmap->psifont))
        fontmap = new_fontmap;
    else {
        errorlog("No fontmap found in %s/fontmap, using builtin mapping",
                 PKGDATADIR);
        for (const_fontmap_entry *cfe = default_fontmap; cfe->psifont; cfe++) {
            fontmap_entry *nfe = (fontmap_entry *)malloc(sizeof(fontmap_entry));
            if (!nfe)
                break;
            nfe->next = fontmap;
            nfe->psifont = strdup(cfe->psifont);
            nfe->bold = cfe->bold;
            nfe->italic = cfe->italic;
            nfe->psfont = strdup(cfe->psfont);
            fontmap = nfe;
        }
    }
    if (debug) {
        debuglog("Active Font-Mapping:");
        debuglog("%-20s%-7s%-7s%-20s", "Psion", "Bold", "Italic", "PS-Font");
        fe = fontmap;
        while (fe) {
            debuglog("%-20s%-7s%-7s%-20s", fe->psifont,
                     fe->bold ? "true" : "false",
                     fe->italic ? "true" : "false",
                     fe->psfont);
            fe = fe->next;
        }
    }
}

static void
ps_setfont(FILE *f, const char *fname, bool bold, bool italic,
           unsigned long fsize)
{
    fontmap_entry *fe = fontmap;
    const char *psf = NULL;
    while (fe) {
        if ((!strcmp(fe->psifont, fname)) &&
            (fe->bold == bold) &&
            (fe->italic == italic)) {
            psf = fe->psfont;
            break;
        }
        fe = fe->next;
    }
    if (!psf) {
        psf = FALLBACK_FONT;
        errorlog("No font mapping for '%s' (%s%s%s); fallback to %s",
                 fname, (bold) ? "Bold" : "", (italic) ? "Italic" : "",
                 (bold || italic) ? "" : "Regular", psf);
    }
    if (usedfonts.find(psf) == usedfonts.npos) {
        usedfonts += "%%+ font ";
        usedfonts += psf;
        usedfonts += "\n";
    }
    fprintf(f, "%ld /%s F\n", fsize, psf);
}

static void
ps_escape(string &text)
{
    int pos = 0;
    while ((pos = text.find_first_of("()", pos)) != text.npos) {
        text.insert(pos, "\\");
        pos += 2;
    }
}

static void
ps_bitmap(FILE *f, unsigned long llx, unsigned long lly, unsigned long urx,
          unsigned long ury, const char *buf)
{
    bufferStore out;
    int width, height;
    if (decodeBitmap((const unsigned char *)buf, width, height, out)) {
        fprintf(f, "%ld %ld %ld %ld %d %d I\n", llx, lly, urx, ury, width, height);
        const unsigned char *p = (const unsigned char *)out.getString(0);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++)
                fprintf(f, "%02x", *p++);
            fprintf(f, "\n");
        }
    } else
        errorlog("Corrupted bitmap data");
}

static void
convertPage(FILE *f, int page, bool last, bufferStore buf)
{
    int len = buf.getLen();
    int i = 0;
    long boffset = 0;
    unsigned long left   = 0;
    unsigned long top    = 0;
    unsigned long right  = 0;
    unsigned long bottom = 0;

    if (debug) {
        char dumpname[128];
        sprintf(dumpname, "/tmp/pdump_%d", page);
        FILE *df = fopen(dumpname, "w");
        fwrite(buf.getString(0), 1, len, df);
        fclose(df);
        debuglog("Saved page input to %s", dumpname);
    }
    if (page == 0) {
        time_t now = time(NULL);
        fputs(
            "%!PS-Adobe-3.0\n"
            "%%Creator: plpprintd " VERSION "\n"
            "%%CreationDate: ", f);
        fputs(ctime(&now), f);
        fputs(
            "%%Pages: (atend)\n"
            "%%BoundingBox: (atend)\n"
            "%%DocumentNeededResources: (atend)\n"
            "%%LanguageLevel: 2\n"
            "%%EndComments\n"
            "%%BeginProlog\n", f);
        char pbuf[1024];
        FILE *pf = fopen(PKGDATADIR "/prolog.ps", "r");
        while (fgets(pbuf, sizeof(pbuf), pf))
            fputs(pbuf, f);
        fclose(pf);
        fputs(
            "%%EndProlog\n"
            "%%BeginSetup\n"
            "currentpagedevice /PageSize get 1 get /top exch def\n"
            "ip 1 1 TH 32 DM RC 1 PS\n"
            "%%EndSetup\n", f);
        minx = miny = 9999;
        maxx = maxy = 0;
        usedfonts = "";
    }
    fprintf(f, "%%%%Page: %d %d\n", page+1, page+1);
    while (i < len) {
        unsigned char opcode = buf.getByte(i);
        switch (opcode) {
            case 0x00: {
                // Start of section
                unsigned long section = buf.getDWord(i+1) & 3;
                unsigned long pagenr = buf.getDWord(i+1) >> 2;
                fprintf(f, "%% @%d: Section %ld, Page %ld\n", i, section, pagenr);
                fprintf(f, "1 1 TH 32 DM RC 1 PS CC\n");
                // (section & 3) =
                // 0 = Header, 1 = Body, 2 = Footer, 3 = Footer
                i += 5;
            }
                break;
            case 0x01: {
                // End of page
                i = len + 1;
            }
                break;
            case 0x03: {
                // Set drawing mode
                unsigned char drwmode = buf.getByte(i+1);
                fprintf(f, "%% @%d: Drawing mode %02x\n", i, drwmode);
                switch (drwmode) {
                    case 0x01:
                        // ~screen
                        break;
                    case 0x02:
                        // colour ^ screen
                        break;
                    case 0x03:
                        // ~colour ^ screen
                        break;
                    case 0x04:
                        // colour | screen
                        break;
                    case 0x05:
                        // colour | ~screen
                        break;
                    case 0x08:
                        // colour & screen
                        break;
                    case 0x09:
                        // colour & ~screen
                        break;
                    case 0x14:
                        // ~colour | screen
                        break;
                    case 0x15:
                        // ~colour | ~screen
                        break;
                    case 0x18:
                        // ~colour & screen
                        break;
                    case 0x19:
                        // ~colour & ~screen
                        break;
                    case 0x20:
                        // colour
                        break;
                    case 0x30:
                        // ~colour
                        break;
                }
                fprintf(f, "%d DM\n", drwmode);
                i += 2;
            }
                break;
            case 0x04: {
                // Bounding box (clipping rectangle)
                left   = buf.getDWord(i+1);
                top    = buf.getDWord(i+5);
                right  = buf.getDWord(i+9);
                bottom = buf.getDWord(i+13);
                if (left < minx)
                    minx = left;
                if (right > maxx)
                    maxx = right;
                if (top < miny)
                    miny = top;
                if (bottom > maxy)
                    maxy = bottom;
                i += 17;
                fprintf(f, "%% @%d: bbox %ld %ld %ld %ld\n", i, left, top, right,
                        bottom);
                fprintf(f, "%ld %ld %ld %ld CB\n", left, top, right, bottom);
            }
                break;
            case 0x05: {
                // Cancel clipping rect
                left = top = right = bottom = 0;
                fprintf(f, "%% @%d: Cancel Cliprect\n", i);
                fprintf(f, "CC\n");
                i++;
            }
                break;
            case 0x06: {
                // ???
                fprintf(f, "%% @%d: U06 %d 0x%08x\n", i,
                         buf.getByte(i+1), buf.getDWord(i+2));
                i += 6;
            }
                break;
            case 0x07: {
                // Font
                int namelen;
                int ofs;
                if (buf.getByte(i+1) & 1) {
                    namelen = buf.getWord(i+1) >> 3;
                    ofs = i + 3;
                } else {
                    namelen = buf.getByte(i+1) >> 2;
                    ofs = i + 2;
                }
                string fname(buf.getString(ofs), namelen);
                ofs += namelen;
                int screenfont = buf.getByte(ofs);
                int basesize = buf.getWord(ofs+1);
                unsigned long style = buf.getDWord(ofs+3);
                bool italic = ((style & 1) != 0);
                bool bold = ((style & 2) != 0);
                unsigned long fontsize = buf.getDWord(ofs+7);
                boffset = (long)buf.getDWord(ofs+11);
                fprintf(f, "%% @%d: Font '%s' %ld %s%s%s\n", i, fname.c_str(),
                        fontsize, bold ? "Bold" : "", italic ? "Italic" : "",
                        (bold || italic) ? "" : "Regular");
                ps_setfont(f, fname.c_str(), bold, italic, fontsize);
                i = ofs + 15;
            }
                break;
            case 0x08: {
                // End Font
                fprintf(f, "%% @%d: End Font\n", i);
                i++;
            }
                break;
            case 0x09: {
                // underline
                fprintf(f, "%% @%d: Underline %d\n", i, buf.getByte(i+1));
                fprintf(f, "%d UL\n", buf.getByte(i+1));
                i += 2;
            }
                break;
            case 0x0a: {
                // strikethru
                fprintf(f, "%% @%d: Strikethru %d\n", i, buf.getByte(i+1));
                fprintf(f, "%d ST\n", buf.getByte(i+1));
                i += 2;
            }
                break;
            case 0x0b: {
                // newline
                fprintf(f, "%% @%d: Newline %d %d\n", i, buf.getDWord(i+1),
                        buf.getDWord(i+5));
                i += 9;
            }
                break;
            case 0x0c: {
                // cr
                fprintf(f, "%% @%d: CR %d %d\n", i, buf.getDWord(i+1),
                        buf.getDWord(i+5));
                i += 9;
            }
                break;
            case 0x0d: {
                // foreground color
                fprintf(f, "%% @%d: Foreground %d %d %d\n", i, buf.getByte(i+1),
                        buf.getByte(i+2), buf.getByte(i+3));
                fprintf(f, "%d %d %d FG\n", buf.getByte(i+1),
                        buf.getByte(i+2), buf.getByte(i+3));
                i += 4;
            }
                break;
            case 0x0e: {
                // Set pen style
                unsigned char pstyle = buf.getByte(i+1);
                switch (pstyle) {
                    case 0x00:
                        // Don't draw
                        break;
                    case 0x01:
                        // Solid
                        break;
                    case 0x02:
                        // Dotted line
                        break;
                    case 0x03:
                        // Dashed line
                        break;
                    case 0x04:
                        // Dash Dot
                        break;
                    case 0x05:
                        // Dash Dot Dot
                        break;
                }
                fprintf(f, "%% @%d: Pen Style %d\n", i, pstyle);
                fprintf(f, "%d PS\n", pstyle);
                i += 2;
            }
                break;
            case 0x0f: {
                // Pen thickness x, y
                fprintf(f, "%% @%d: Pen thickness %d %d\n", i, buf.getDWord(i+1),
                         buf.getDWord(i+5));
                fprintf(f, "%d %d TH\n", buf.getDWord(i+1), buf.getDWord(i+5));
                i += 9;
            }
                break;
            case 0x10: {
                // background color
                fprintf(f, "%% @%d: Background %d %d %d\n", i, buf.getByte(i+1),
                        buf.getByte(i+2), buf.getByte(i+3));
                fprintf(f, "%d %d %d BG\n", buf.getByte(i+1),
                        buf.getByte(i+2), buf.getByte(i+3));
                i += 4;
            }
                break;
            case 0x11: {
                // Brush style
                unsigned char bstyle = buf.getByte(i+1);
                switch (bstyle) {
                    case 0x00:
                        // No brush
                        break;
                    case 0x01:
                        // Solid brush
                        break;
                    case 0x02:
                        // Patterned brush
                        break;
                    case 0x03:
                        // Vertical hatch brush
                        break;
                    case 0x04:
                        // Diagonal hatch brush (bottom left to top right)
                        break;
                    case 0x05:
                        // Horizontal hatch brush
                        break;
                    case 0x06:
                        // Rev. diagonal hatch brush (top left to bottom right)
                        break;
                    case 0x07:
                        // Square cross hatch (horizontal and vertical)
                        break;
                    case 0x08:
                        // Diamond cross hatch (both diagonals)
                        break;
                }
                fprintf(f, "%% @%d: Brush style %d\n", i, bstyle);
                fprintf(f, "%d BS\n", bstyle);
                i += 2;
            }
                break;
            case 0x17: {
                // ???
                fprintf(f, "%% @%d: U17 %d %d\n", i, buf.getDWord(i+1),
                         buf.getDWord(i+5));
                i += 9;
            }
                break;
            case 0x19: {
                // Draw line
                fprintf(f, "%% @%d: Line %d %d %d %d\n", i,
                        buf.getDWord(i+1), buf.getDWord(i+5),
                        buf.getDWord(i+9), buf.getDWord(i+13));
                fprintf(f, "%d %d %d %d L\n",
                        buf.getDWord(i+1), buf.getDWord(i+5),
                        buf.getDWord(i+9), buf.getDWord(i+13));
                i += 17;
            }
                break;
            case 0x1b: {
                // ???
                fprintf(f, "%% @%d: U1b %d %d\n", i, buf.getDWord(i+1),
                         buf.getDWord(i+5));
                i += 9;
            }
                break;
            case 0x1f: {
                // Draw ellipse
                fprintf(f, "%% @%d: Ellipse %d %d %d %d\n", i,
                        buf.getDWord(i+1), buf.getDWord(i+5),
                        buf.getDWord(i+9), buf.getDWord(i+13));
                fprintf(f, "%d %d %d %d E\n",
                        buf.getDWord(i+1), buf.getDWord(i+5),
                        buf.getDWord(i+9), buf.getDWord(i+13));
                i += 17;
            }
                break;
            case 0x20: {
                // Draw rectangle
                fprintf(f, "%% @%d: Rectangle %d %d %d %d\n", i,
                        buf.getDWord(i+1), buf.getDWord(i+5),
                        buf.getDWord(i+9), buf.getDWord(i+13));
                fprintf(f, "%d %d %d %d R\n",
                        buf.getDWord(i+1), buf.getDWord(i+5),
                        buf.getDWord(i+9), buf.getDWord(i+13));
                i += 17;
            }
                break;

            case 0x23: {
                // Draw polygon
                unsigned long count = buf.getDWord(i+1);
                int o = i + 5;
                fprintf(f, "%% @%d: Polygon (%ld segments)\n", i, count);
                fprintf(f, "[\n");
                for (int j = 0; j < count; j++) {
                    fprintf(f, "%d %d\n", buf.getDWord(o),
                            buf.getDWord(o+4));
                    o += 8;
                }
                unsigned char frule = buf.getByte(o);
                fprintf(f, "] %s P\n", frule ? "false" : "true");
                i = o + 1;
            }
                break;
            case 0x25: {
                // Draw bitmap
                unsigned long llx  = buf.getDWord(i+1);
                unsigned long lly  = buf.getDWord(i+13);
                unsigned long urx  = buf.getDWord(i+9);
                unsigned long ury  = buf.getDWord(i+5);
                unsigned long blen = buf.getDWord(i+17);
                fprintf(f, "%% @%d: Bitmap\n", i);
                ps_bitmap(f, llx, lly, urx, ury, buf.getString(i+17));
                i += (17 + blen);
            }
                break;
            case 0x26: {
                // Draw bitmap
                unsigned long llx  = buf.getDWord(i+1);
                unsigned long lly  = buf.getDWord(i+13);
                unsigned long urx  = buf.getDWord(i+9);
                unsigned long ury  = buf.getDWord(i+5);
                unsigned long blen = buf.getDWord(i+17);
                unsigned long u1   = buf.getDWord(i+17+blen);
                unsigned long u2   = buf.getDWord(i+17+blen+4);
                unsigned long u3   = buf.getDWord(i+17+blen+8);
                unsigned long u4   = buf.getDWord(i+17+blen+12);
                fprintf(f, "%% @%d: Bitmap %ld %ld %ld %ld\n", i, u1, u2, u3, u4);
                ps_bitmap(f, llx, lly, urx, ury, buf.getString(i+17));
                i += (17 + blen + 16);
            }
                break;
            case 0x27: {
                // Draw label
                int tlen;
                int ofs;
                if (buf.getByte(i+1) & 1) {
                    tlen = buf.getWord(i+1) >> 3;
                    ofs = i + 3;
                } else {
                    tlen = buf.getByte(i+1) >> 2;
                    ofs = i + 2;
                }
                string text(buf.getString(ofs), tlen);
                ofs += tlen;
                ps_escape(text);
                fprintf(f, "%% @%d: Text '%s' %d %d\n", i,
                        text.c_str(), buf.getDWord(ofs), buf.getDWord(ofs+4));
                fprintf(f, "(%s) %d %ld 0 0 -1 T\n", text.c_str(),
                        buf.getDWord(ofs), buf.getDWord(ofs+4) + boffset);
                i = ofs + 8;
            }
                break;
            case 0x28: {
                // Draw justified text
                int tlen;
                int ofs;
                if (buf.getByte(i+1) & 1) {
                    tlen = buf.getWord(i+1) >> 3;
                    ofs = i + 3;
                } else {
                    tlen = buf.getByte(i+1) >> 2;
                    ofs = i + 2;
                }
                string text(buf.getString(ofs), tlen);
                ofs += tlen;
                int left = buf.getDWord(ofs);
                int top = buf.getDWord(ofs+4);
                int right = buf.getDWord(ofs+8);
                int bottom = buf.getDWord(ofs+12);
                int baseline = buf.getDWord(ofs+16);
                unsigned char align = buf.getByte(ofs+20);
                int amargin = buf.getDWord(ofs+21);
                fprintf(f, "%% @%d: JText '%s' %d %ld %d %d %d %d %d\n", i,
                        text.c_str(), left, bottom + boffset, top, right,
                        baseline, align, amargin);
                ps_escape(text);
                if (align == 2)
                    right -= amargin;
                else
                    left += amargin;
                bottom -= ((bottom - top) / 4);
                fprintf(f, "(%s) %d %ld %d %d %d T\n", text.c_str(),
                        left, bottom + boffset, top, right, align);
                i = ofs + 25;
            }
                break;
            default:
                fprintf(f, "@%d: UNHANDLED OPCODE %02x\n", i, opcode);
                debuglog("@%d: UNHANDLED OPCODE %02x", i, opcode);
                i++;
                break;
        }
    }
    fprintf(f, "showpage\n");
    if (last) {
        fputs(
            "%%Trailer\n"
            "%%DocumentNeededResources: ", f);
        if (usedfonts.empty())
            fputs("none\n", f);
        else {
            usedfonts.erase(0, 4);
            fputs(usedfonts.c_str(), f);
        }
        fprintf(f, "%%%%Pages: %d\n", page + 1);
        fprintf(f, "%%%%BoundingBox: %d %d %d %d\n",
                minx / 20, miny / 20, maxx / 20, maxy / 20);
        fputs("%%EOF\n", f);
    }
}

static unsigned char fakePage[15] = {
    0x2a, 0x2a, 0x09, 0x00, 0x00, 0x00, 0x82, 0x2e,
    0x00, 0x00, 0xc6, 0x41, 0x00, 0x00, 0x00,
};

const
static void
service_loop()
{
    bool jobLoop = true;
    while (jobLoop) {
        bool spoolOpen = false;
        bool pageStart = true;
        bool cancelled = false;
        bool jobEnd;
        unsigned long plen;
        int pageCount;
        bufferStore buf;
        bufferStore pageBuf;
        int fd;
        FILE *f;
        unsigned char b;
        char *jname =
            (char *)malloc(strlen(spooldir) +
                           strlen(TEMPLATE) + 2);

        while (jobLoop) {
            /* Job loop */
            buf.init();
            switch (wPrt->getData(buf)) {
                case rfsv::E_PSI_FILE_DISC:
                    jobLoop = false;
                    break;
                case rfsv::E_PSI_GEN_NONE:
                    if ((buf.getLen() == 15) &&
                        (!memcmp(buf.getString(0), fakePage, 15))) {
                        cancelled = false;
                        if (spoolOpen) {
                            fclose(f);
                            infolog("Cancelled job %s", jname);
                            unlink(jname);
                            break;
                        }
                        continue;
                    }
                    if (!spoolOpen && !cancelled) {
                        sprintf(jname, "%s/%s", spooldir, TEMPLATE);
                        if ((fd = mkstemp(jname)) != -1) {
                            infolog("Receiving new job %s", jname);
                            spoolOpen = true;
                            pageStart = true;
                            pageCount = 0;
                        } else {
                            errorlog("Could not create spool file.");
                            cancelled = true;
                            wPrt->cancelJob();
                        }
                        f = fdopen(fd, "w");
                        plen = 0;
                    }
                    b = buf.getByte(0);
                    if ((b != 0x2a) && (b != 0xff)) {
                        errorlog("Invalid packet type 0x%02x.", b);
                        cancelled = true;
                        wPrt->cancelJob();
                    }
                    jobEnd = (b == 0xff);
                    if (!cancelled) {
                        buf.discardFirstBytes(1);
                        if (pageStart) {
                            b = buf.getByte(0);
                            plen = buf.getDWord(1) - 8;
                            buf.discardFirstBytes(5+8);
                            pageStart = false;
                            pageBuf.init();
                        }
                        pageBuf.addBuff(buf);
                        plen -= buf.getLen();
                        if (plen <= 0) {
                            convertPage(f, pageCount++, jobEnd, pageBuf);
                            pageBuf.init();
                            pageStart = true;
                        }
                    }
                    if (jobEnd) {
                        if (spoolOpen)
                            fclose(f);
                        if (!cancelled) {
                            if (pageCount > 0) {
                                if (!printcmd)
                                    infolog("Output stored in %s", jname);
                                else {
                                    int r;
                                    char cbuf[4096];

                                    infolog("Spooling %d pages", pageCount);
                                    FILE *pipe = popen(printcmd, "w");
                                    if (!pipe) {
                                        errorlog("Could not execute %s: %m",
                                                 printcmd);
                                        unlink(jname);
                                    }
                                    f = fopen(jname, "r");
                                    if (!f) {
                                        errorlog("Could not read %s: %m",
                                                 jname);
                                        pclose(pipe);
                                        unlink(jname);
                                    }
                                    while ((r = fread(cbuf, 1,
                                                      sizeof(cbuf), f)) > 0)
                                        fwrite(cbuf, 1, r, pipe);
                                    pclose(pipe);
                                    fclose(f);
                                    unlink(jname);
                                }
                            }
                        } else
                            unlink(jname);
                        spoolOpen = false;
                    }
                    break;
            }
        }
        free(jname);
    }
}

static void
help() {
    cout <<
        "Options of plpprintd:\n"
        "\n"
        " -d, --debug            Debugging, do not fork.\n"
        " -h, --help             Display this text.\n"
        " -v, --verbose          Increase verbosity.\n"
        " -V, --version          Print version and exit.\n"
        " -p, --port=[HOST:]PORT Connect to port PORT on host HOST.\n"
        " -s, --spooldir=DIR     Specify spooldir DIR.\n"
        "                        Default: " SPOOLDIR "\n"
        " -c, --printcmd=CMD     Specify print command.\n"
        "                        Default: " PRINTCMD "\n";
}

static void
usage() {
    cerr << "Usage: plpprintd [OPTIONS]" << endl
         << "Use --help for more information" << endl;
}

static struct option opts[] = {
    {"debug",    no_argument,       nullptr, 'd'},
    {"help",     no_argument,       nullptr, 'h'},
    {"version",  no_argument,       nullptr, 'V'},
    {"verbose",  no_argument,       nullptr, 'v'},
    {"port",     required_argument, nullptr, 'p'},
    {"spooldir", required_argument, nullptr, 's'},
    {"printcmd", required_argument, nullptr, 'c'},
    {nullptr,    0,                 nullptr,  0 }
};

static void
parse_destination(const char *arg, const char **host, int *port)
{
    if (!arg)
        return;
    // We don't want to modify argv, therefore copy it first ...
    char *argcpy = strdup(arg);
    char *pp = strchr(argcpy, ':');

    if (pp) {
        // host.domain:400
        // 10.0.0.1:400
        *pp ++= '\0';
        *host = argcpy;
    } else {
        // 400
        // host.domain
        // host
        // 10.0.0.1
        if (strchr(argcpy, '.') || !isdigit(argcpy[0])) {
            *host = argcpy;
            pp = nullptr;
        } else
            pp = argcpy;
    }
    if (pp)
        *port = atoi(pp);
}

int
main(int argc, char **argv)
{
    ppsocket *skt;
    const char *host = "127.0.0.1";
    int status = 0;
    int sockNum = DPORT;
    int ret = 0;
    int c;

    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se != nullptr)
        sockNum = ntohs(se->s_port);

    while (1) {
        c = getopt_long(argc, argv, "dhVvp:s:c:", opts, NULL);
        if (c == -1)
            break;
        switch (c) {
            case '?':
                usage();
                return -1;
            case 'd':
                debug = true;
                break;
            case 'v':
                verbose++;
                break;
            case 'V':
                cout << "plpprintd Version " << VERSION << endl;
                return 0;
            case 'h':
                help();
                return 0;
            case 'p':
                parse_destination(optarg, &host, &sockNum);
                break;
            case 's':
                spooldir = strdup(optarg);
                break;
            case 'c':
                if (!strcmp(optarg, "-"))
                    printcmd = NULL;
                else
                    printcmd = strdup(optarg);
                break;
        }
    }
    if (optind < argc) {
        usage();
        return -1;
    }

    skt = new ppsocket();
    if (!skt->connect(host, sockNum)) {
        cout << _("plpprintd: could not connect to ncpd") << endl;
        return 1;
    }
    if (!debug)
        ret = fork();
    switch (ret) {
        case 0:
            /* child */
            setsid();
            ignore_value(chdir("/"));
            if (!debug) {
                openlog("plpprintd", LOG_PID|LOG_CONS, LOG_DAEMON);
                int devnull =
                    open("/dev/null", O_RDWR, 0);
                if (devnull != -1) {
                    dup2(devnull, STDIN_FILENO);
                    dup2(devnull, STDOUT_FILENO);
                    dup2(devnull, STDERR_FILENO);
                    if (devnull > 2)
                        close(devnull);
                }
            }
            init_fontmap();
            infolog("started, waiting for requests.");
            serviceLoop = true;
            while (serviceLoop) {
                wPrt = new wprt(skt);
                if (wPrt) {
                    Enum<rfsv::errs> ret;
                    ret = wPrt->initPrinter();
                    if (ret == rfsv::E_PSI_GEN_NONE)
                        service_loop();
                    else {
                        if (debug)
                            debuglog("plpprintd: could not connect: %s",
                                     ret.toString().c_str());
                    }
                    delete wPrt;
                    sleep(1);
                } else {
                    errorlog("plpprintd: Could not create wprt object");
                    exit(1);
                }
            }
            break;
        case -1:
            cerr << "plpprintd: fork failed" << endl;
            return 1;
        default:
            /* parent */
            break;
    }
    return 0;
}
