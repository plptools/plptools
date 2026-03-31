/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2026 Fabrice Cappaert <captfab@somesite.net>
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

#include <cli_utils.h>
#include <tcpsocket.h>
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

const char *spooldir = SPOOLDIR;
const char *printcmd = PRINTCMD;
wprt *wPrt;
bool serviceLoop;
bool debug = false;
int verbose = 0;

__attribute__((__format__ (__printf__, 2, 0)))
static void _log(int priority, const char *fmt, va_list ap) {
    char *buf;
    if (vasprintf(&buf, fmt, ap) == -1) {
        return;
    }
    if (debug) {
        std::cout << buf << std::endl;
    } else {
        syslog(priority, "%s", buf);
    }
    free(buf);
}

__attribute__((__format__ (__printf__, 1, 0)))
void debuglog(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _log(LOG_DEBUG, fmt, ap);
    va_end(ap);
}

__attribute__((__format__ (__printf__, 1, 0)))
void errorlog(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _log(LOG_ERR, fmt, ap);
    va_end(ap);
}

__attribute__((__format__ (__printf__, 1, 0)))
void infolog(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _log(LOG_INFO, fmt, ap);
    va_end(ap);
}

static unsigned char fakePage[6] = { 0x2a, 0x2a, 0x09, 0x00, 0x00, 0x00 };

// static unsigned long minx, maxx, miny, maxy;
std::string usedfonts;

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

static void init_fontmap() {
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
            // int ne;
            if ((p = strchr(buf, '#'))) {
                *p = '\0';
            }
            if ((p = strchr(buf, '\n'))) {
                *p = '\0';
            }

            psifont = strsep(&bp, ":");
            if (!psifont || !(*psifont)) {
                continue;
            }
            tmp = strsep(&bp, ":");
            if (!tmp || !(*tmp) || (sscanf(tmp, "%d", &bold) != 1)) {
                continue;
            }
            tmp = strsep(&bp, ":");
            if (!tmp || !(*tmp) || (sscanf(tmp, "%d", &italic) != 1)) {
                continue;
            }
            psfont = strsep(&bp, ":");
            if (!psfont || !(*psfont)) {
                continue;
            }
            fe = static_cast<fontmap_entry *>(malloc(sizeof(fontmap_entry)));
            if (!fe) {
                break;
            }
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
    if (new_fontmap && (new_fontmap->psifont)) {
        fontmap = new_fontmap;
    } else {
        errorlog("No fontmap found in %s/fontmap, using builtin mapping",
                 PKGDATADIR);
        for (const_fontmap_entry *cfe = default_fontmap; cfe->psifont; cfe++) {
            fontmap_entry *nfe = static_cast<fontmap_entry *>(malloc(sizeof(fontmap_entry)));
            if (!nfe) {
                break;
            }
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

static void ps_setfont(FILE *f, const char *fname, bool bold, bool italic, unsigned long fsize) {
    fontmap_entry *fe = fontmap;
    const char *psf = NULL;
    while (fe) {
        if ((!strcmp(fe->psifont, fname)) && (fe->bold == bold) && (fe->italic == italic)) {
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
    fprintf(f, "%lu /%s F\n", fsize, psf);
}

static void ps_escape(std::string &text) {
    size_t pos = 0;
    while ((pos = text.find_first_of("()\\", pos)) != text.npos) {
        text.insert(pos, "\\");
        pos += 2;
    }
}

static void ps_bitmap(FILE *f, unsigned long left, unsigned long bottom, const char *buf) {
    BufferStore out;
    int width, height;
    if (decodeBitmap(reinterpret_cast<const unsigned char *>(buf), width, height, out)) {
        fprintf(f, "%lu %lu %lu %lu %d %d I\n", left, bottom, left+width, bottom+height, width, height);
        const unsigned char *p = reinterpret_cast<const unsigned char *>(out.getString(0));
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                fprintf(f, "%02x", *p++);
            }
            fprintf(f, "\n");
        }
    } else {
        errorlog("Corrupted bitmap data");
    }
}

static void ps_bitmap(FILE *f, unsigned long left, unsigned long bottom, unsigned long right, unsigned long top,
        const char *buf) {
    BufferStore out;
    int width, height;
    if (decodeBitmap(reinterpret_cast<const unsigned char *>(buf), width, height, out)) {
        fprintf(f, "%lu %lu %lu %lu %d %d I\n", left, bottom, right, top, width, height);
        const unsigned char *p = reinterpret_cast<const unsigned char *>(out.getString(0));
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                fprintf(f, "%02x", *p++);
            }
            fprintf(f, "\n");
        }
    } else {
        errorlog("Corrupted bitmap data");
    }
}

static void ps_bitmap(FILE *f, unsigned long left, unsigned long bottom, unsigned long right, unsigned long top,
        unsigned long srcLeft, unsigned long srcBottom, unsigned long srcRight, unsigned long srcTop,
        const char *buf) {
    BufferStore out;
    int width, height;
    if (decodeBitmap(reinterpret_cast<const unsigned char *>(buf), width, height, out)) {
        fprintf(f, "%lu %lu %lu %lu %d %d %lu %lu %lu %lu IC\n", left, bottom, right, top, width, height,
                srcLeft, srcBottom, srcRight, srcTop);
        const unsigned char *p = reinterpret_cast<const unsigned char *>(out.getString(0));
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                fprintf(f, "%02x", *p++);
            }
            fprintf(f, "\n");
        }
    } else {
        errorlog("Corrupted bitmap data");
    }
}

static void write_job_header(FILE *f, BufferStore buffer) {
    unsigned long width  = 0;
    unsigned long height = 0;
    bool isLandscape = 0;

    time_t now = time(NULL);

    width = buffer.getDWord(0);
    height = buffer.getDWord(4);
    isLandscape = (buffer.getByte(8) != 0);
    buffer.discardFirstBytes(9);
    fputs(
        "%!PS-Adobe-3.0\n"
        "%%Creator: plpprintd " VERSION "\n"
        "%%CreationDate: ", f);
    fputs(ctime(&now), f);
    fputs(
        "%%Pages: (atend)\n"
        // "%%BoundingBox: (atend)\n"
        "%%DocumentNeededResources: (atend)\n"
        "%%LanguageLevel: 2\n"
        "%%EndComments\n"
        "%%BeginProlog\n", f);
    char prologBuffer[1024];
    FILE *pf = fopen(PKGDATADIR PSDICT, "r");
    while (fgets(prologBuffer, sizeof(prologBuffer), pf)) {
        fputs(prologBuffer, f);
    }
    fclose(pf);
    fputs(
        "%%EndProlog\n"
        "%%BeginSetup\n", f);
    if (isLandscape) {
        fprintf(f, "<< /PageSize [%ld %ld] >> setpagedevice\n", lround(height / 20.0), lround(width / 20.0));
    } else {
        fprintf(f, "<< /PageSize [%ld %ld] >> setpagedevice\n", lround(width / 20.0), lround(height / 20.0));
    }
    fputs(
        "currentpagedevice /PageSize get 0 get /pagewidth exch def\n"
        "currentpagedevice /PageSize get 1 get /pageheight exch def\n"
        "ip 1 1 TH 32 DM RC 1 PS\n"
        "%%EndSetup\n", f);
    // minx = width / 20.0;
    // miny = height / 20.0;
    // maxx = maxy = 0;
    usedfonts = "";
}

static void write_job_footer(FILE *f, int pageCount) {
        fputs(
            "%%Trailer\n"
            "%%DocumentNeededResources: ", f);
        if (usedfonts.empty()) {
            fputs("none\n", f);
        } else {
            usedfonts.erase(0, 4);
            fputs(usedfonts.c_str(), f);
        }
        fprintf(f, "%%%%Pages: %d\n", pageCount);
        // fprintf(f, "%%%%BoundingBox: %ld %ld %ld %ld\n",
        //         lround(minx / 20.0), lround(miny / 20.0), lround(maxx / 20.0), lround(maxy / 20.0));
        fputs("%%EOF\n", f);
}

static void convert_page(FILE *f, int pageNumber, BufferStore buffer) {
    int bufferLength = buffer.getLen();
    int bufferIndex = 0;
    long baselineOffset = 0;

    if (debug) {
        char dumpFileName[128];
        sprintf(dumpFileName, "/tmp/pagedump_%d", pageNumber);
        FILE *df = fopen(dumpFileName, "w");
        fwrite(buffer.getString(0), 1, bufferLength, df);
        fclose(df);
        debuglog("Saved page input to %s", dumpFileName);
    }

    fprintf(f, "%%%%Page: %d %d\n", pageNumber, pageNumber);

    // Ignore what seems to be an hardcoded page header, until someone finds
    // out that it is not a page header and that it could contain useful data.
    // For now, it seems to always be `e8 03 00 00  e8 03 00 00` (1000 1000).
    bufferIndex += 8;

    while (bufferIndex < bufferLength) {
        unsigned char opcode = buffer.getByte(bufferIndex);
        debuglog("%% @%d: OPCODE %02x", bufferIndex, opcode);
        switch (opcode) {
            case 0x00: {
                // Start of section
                unsigned long section = buffer.getDWord(bufferIndex+1) & 3;
                unsigned long page = buffer.getDWord(bufferIndex+1) >> 2;
                fprintf(f, "%% @%d: Section %lu, Page %lu\n", bufferIndex, section, page);
                fprintf(f, "1 1 TH 32 DM RC 1 PS CC\n");
                // (section & 3) =
                // 0 = Header, 1 = Body, 2 = Footer, 3 = Footer
                bufferIndex += 5;
                break;
            }
            case 0x01: {
                // End of page
                fprintf(f, "showpage\n");
                bufferIndex = (bufferLength + 1);
                break;
            }
            case 0x02: {
                // Set origin
                // Not implemented
                unsigned long x = buffer.getDWord(bufferIndex+1);
                unsigned long y = buffer.getDWord(bufferIndex+5);
                fprintf(f, "%% @%d: UNIMPLEMENTED OPCODE %02x %lu %lu\n", bufferIndex, opcode, x, y);
                debuglog("%% @%d: UNIMPLEMENTED OPCODE %02x %lu %lu", bufferIndex, opcode, x, y);
                bufferIndex += 9;
                break;
            }
            case 0x03: {
                // Set drawing mode
                unsigned char drawMode = buffer.getByte(bufferIndex+1);
                fprintf(f, "%% @%d: Drawing mode %02x\n", bufferIndex, drawMode);
                switch (drawMode) {
                    case 0x01: {
                        // ~screen
                        break;
                    }
                    case 0x02: {
                        // colour ^ screen
                        break;
                    }
                    case 0x03: {
                        // ~colour ^ screen
                        break;
                    }
                    case 0x04: {
                        // colour | screen
                        break;
                    }
                    case 0x05: {
                        // colour | ~screen
                        break;
                    }
                    case 0x08: {
                        // colour & screen
                        break;
                    }
                    case 0x09: {
                        // colour & ~screen
                        break;
                    }
                    case 0x14: {
                        // ~colour | screen
                        break;
                    }
                    case 0x15: {
                        // ~colour | ~screen
                        break;
                    }
                    case 0x18: {
                        // ~colour & screen
                        break;
                    }
                    case 0x19: {
                        // ~colour & ~screen
                        break;
                    }
                    case 0x20: {
                        // colour
                        break;
                    }
                    case 0x30: {
                        // ~colour
                        break;
                    }
                    default: {
                    }
                }
                fprintf(f, "%d DM\n", drawMode);
                bufferIndex += 2;
                break;
            }
            case 0x04: {
                // Set clipping rect
                unsigned long left = buffer.getDWord(bufferIndex+1);
                unsigned long top = buffer.getDWord(bufferIndex+5);
                unsigned long right = buffer.getDWord(bufferIndex+9);
                unsigned long bottom = buffer.getDWord(bufferIndex+13);
                // minx = std::min(minx, left);
                // maxx = std::max(maxx, right);
                // miny = std::min(miny, top);
                // maxy = std::max(maxy, bottom);
                fprintf(f, "%% @%d: bbox %lu %lu %lu %lu\n", bufferIndex, left, top, right, bottom);
                fprintf(f, "%lu %lu %lu %lu CB\n", left, top, right, bottom);
                bufferIndex += 17;
                break;
            }
            case 0x05: {
                // Cancel clipping rect
                fprintf(f, "%% @%d: Cancel Cliprect\n", bufferIndex);
                fprintf(f, "CC\n");
                bufferIndex += 1;
                break;
            }
            case 0x06: {
                // Reset
                fprintf(f, "1 1 TH 32 DM RC 1 PS CC\n");
                bufferIndex += 1;
                break;
            }
            case 0x07: {
                // Use font
                int fontNameLength;
                int tempIndex;
                if (buffer.getByte(bufferIndex+1) & 1) {
                    fontNameLength = buffer.getWord(bufferIndex+1) >> 3;
                    tempIndex = bufferIndex + 3;
                } else {
                    fontNameLength = buffer.getByte(bufferIndex+1) >> 2;
                    tempIndex = bufferIndex + 2;
                }
                std::string fontName(buffer.getString(tempIndex), fontNameLength);
                tempIndex += fontNameLength;
                // Unused for now, keep for reference
                // unsigned char screenFont = buffer.getByte(tempIndex);
                // int baseSize = buffer.getWord(tempIndex+1);
                unsigned long style = buffer.getDWord(tempIndex+3);
                bool italic = ((style & 1) != 0);
                bool bold = ((style & 2) != 0);
                unsigned long fontSize = buffer.getDWord(tempIndex+7);
                baselineOffset = buffer.getDWord(tempIndex+11);
                fprintf(f, "%% @%d: Font '%s' %lu %s%s%s\n", bufferIndex, fontName.c_str(), fontSize,
                        bold ? "Bold" : "", italic ? "Italic" : "", (bold || italic) ? "" : "Regular");
                ps_setfont(f, fontName.c_str(), bold, italic, fontSize);
                bufferIndex = (tempIndex + 15);
                break;
            }
            case 0x08: {
                // Discard Font
                fprintf(f, "%% @%d: End Font\n", bufferIndex);
                bufferIndex += 1;
                break;
            }
            case 0x09: {
                // Set underline style
                fprintf(f, "%% @%d: Underline %d\n", bufferIndex, buffer.getByte(bufferIndex+1));
                fprintf(f, "%d UL\n", buffer.getByte(bufferIndex+1));
                bufferIndex += 2;
                break;
            }
            case 0x0a: {
                // Set strikethrough style
                fprintf(f, "%% @%d: Strikethrough %d\n", bufferIndex, buffer.getByte(bufferIndex+1));
                fprintf(f, "%d ST\n", buffer.getByte(bufferIndex+1));
                bufferIndex += 2;
                break;
            }
            case 0x0b: {
                // Set word justification
                // Not implemented
                unsigned long excessWidth = buffer.getDWord(bufferIndex+1);
                unsigned long gapsCount = buffer.getDWord(bufferIndex+5);
                fprintf(f, "%% @%d: UNIMPLEMENTED OPCODE %02x %lu %lu\n", bufferIndex, opcode, excessWidth, gapsCount);
                debuglog("%% @%d: UNIMPLEMENTED OPCODE %02x %lu %lu", bufferIndex, opcode, excessWidth, gapsCount);
                bufferIndex += 9;
                break;
            }
            case 0x0c: {
                // Set character justification
                // Not implemented
                unsigned long excessWidth = buffer.getDWord(bufferIndex+1);
                unsigned long charsCount = buffer.getDWord(bufferIndex+5);
                fprintf(f, "%% @%d: UNIMPLEMENTED OPCODE %02x %lu %lu\n", bufferIndex, opcode, excessWidth, charsCount);
                debuglog("%% @%d: UNIMPLEMENTED OPCODE %02x %lu %lu", bufferIndex, opcode, excessWidth, charsCount);
                bufferIndex += 9;
                break;
            }
            case 0x0d: {
                // Set pen color
                unsigned char red = buffer.getByte(bufferIndex+1);
                unsigned char green = buffer.getByte(bufferIndex+2);
                unsigned char blue = buffer.getByte(bufferIndex+3);
                fprintf(f, "%% @%d: Pen Color %d %d %d\n", bufferIndex, red, green, blue);
                fprintf(f, "%d %d %d FG\n", red, green, blue);
                bufferIndex += 4;
                break;
            }
            case 0x0e: {
                // Set pen style
                unsigned char penStyle = buffer.getByte(bufferIndex+1);
                switch (penStyle) {
                    case 0x00: {
                        // Don't draw
                        break;
                    }
                    case 0x01: {
                        // Solid
                        break;
                    }
                    case 0x02: {
                        // Dotted line
                        break;
                    }
                    case 0x03: {
                        // Dashed line
                        break;
                    }
                    case 0x04: {
                        // Dash Dot
                        break;
                    }
                    case 0x05: {
                        // Dash Dot Dot
                        break;
                    }
                    default: {
                    }
                }
                fprintf(f, "%% @%d: Pen Style %d\n", bufferIndex, penStyle);
                fprintf(f, "%d PS\n", penStyle);
                bufferIndex += 2;
                break;
            }
            case 0x0f: {
                // Set pen size
                // Postscript only supports a single line width value so line height will be ignored
                unsigned long penWidth = buffer.getDWord(bufferIndex+1);
                unsigned long penHeight = buffer.getDWord(bufferIndex+5);
                fprintf(f, "%% @%d: Pen thickness %lu %lu\n", bufferIndex, penWidth, penHeight);
                fprintf(f, "%lu %lu TH\n", penWidth, penHeight);
                bufferIndex += 9;
                break;
            }
            case 0x10: {
                // Set brush color
                unsigned char red = buffer.getByte(bufferIndex+1);
                unsigned char green = buffer.getByte(bufferIndex+2);
                unsigned char blue = buffer.getByte(bufferIndex+3);
                fprintf(f, "%% @%d: Brush Color %d %d %d\n", bufferIndex, red, green, blue);
                fprintf(f, "%d %d %d BG\n", red, green, blue);
                bufferIndex += 4;
                break;
            }
            case 0x11: {
                // Set brush style
                unsigned char brushStyle = buffer.getByte(bufferIndex+1);
                switch (brushStyle) {
                    case 0x00: {
                        // No brush
                        break;
                    }
                    case 0x01: {
                        // Solid brush
                        break;
                    }
                    case 0x02: {
                        // Patterned brush
                        break;
                    }
                    case 0x03: {
                        // Vertical hatch brush
                        break;
                    }
                    case 0x04: {
                        // Diagonal hatch brush (bottom left to top right)
                        break;
                    }
                    case 0x05: {
                        // Horizontal hatch brush
                        break;
                    }
                    case 0x06: {
                        // Rev. diagonal hatch brush (top left to bottom right)
                        break;
                    }
                    case 0x07: {
                        // Square cross hatch (horizontal and vertical)
                        break;
                    }
                    case 0x08: {
                        // Diamond cross hatch (both diagonals)
                        break;
                    }
                    default: {
                    }
                }
                fprintf(f, "%% @%d: Brush style %d\n", bufferIndex, brushStyle);
                fprintf(f, "%d BS\n", brushStyle);
                bufferIndex += 2;
                break;
            }
            case 0x12: {
                // Set brush origin
                // Not implemented
                unsigned long x = buffer.getDWord(bufferIndex+1);
                unsigned long y = buffer.getDWord(bufferIndex+5);
                fprintf(f, "%% @%d: UNIMPLEMENTED OPCODE %02x %lu %lu\n", bufferIndex, opcode, x, y);
                debuglog("%% @%d: UNIMPLEMENTED OPCODE %02x %lu %lu", bufferIndex, opcode, x, y);
                bufferIndex += 9;
                break;
            }
            case 0x13: {
                // Use brush pattern
                // Not implemented
                unsigned long bitmapLength = buffer.getDWord(bufferIndex+17);
                // const char* bitmapData = buffer.getString(bufferIndex+17);
                fprintf(f, "%% @%d: UNIMPLEMENTED OPCODE %02x %lu ...\n", bufferIndex, opcode, bitmapLength);
                debuglog("%% @%d: UNIMPLEMENTED OPCODE %02x %lu ...", bufferIndex, opcode, bitmapLength);
                bufferIndex += (17 + bitmapLength);
                break;
            }
            case 0x14: {
                // Discard brush pattern
                // Not implemented
                fprintf(f, "%% @%d: UNIMPLEMENTED OPCODE %02x\n", bufferIndex, opcode);
                debuglog("%% @%d: UNIMPLEMENTED OPCODE %02x", bufferIndex, opcode);
                bufferIndex += 1;
                break;
            }
            case 0x15: {
                // Move to
                unsigned long x = buffer.getDWord(bufferIndex+1);
                unsigned long y = buffer.getDWord(bufferIndex+5);
                fprintf(f, "%% @%d: Move to %lu %lu\n", bufferIndex, x, y);
                fprintf(f, "%lu %lu MT\n", x, y);
                bufferIndex += 9;
                break;
            }
            case 0x16: {
                // Move by
                long x = buffer.getSDWord(bufferIndex+1);
                long y = buffer.getSDWord(bufferIndex+5);
                fprintf(f, "%% @%d: Move by %ld %ld", bufferIndex, x, y);
                fprintf(f, "%ld %ld MB\n", x, y);
                bufferIndex += 9;
                break;
            }
            case 0x17: {
                // Draw point
                unsigned long x = buffer.getDWord(bufferIndex+1);
                unsigned long y = buffer.getDWord(bufferIndex+5);
                fprintf(f, "%% @%d: Point %lu %lu", bufferIndex, x, y);
                fprintf(f, "%lu %lu PT\n", x, y);
                bufferIndex += 9;
                break;
            }
            case 0x18: {
                // Draw arc
                unsigned long left = buffer.getDWord(bufferIndex+1);
                unsigned long top = buffer.getDWord(bufferIndex+5);
                unsigned long right = buffer.getDWord(bufferIndex+9);
                unsigned long bottom = buffer.getDWord(bufferIndex+13);
                unsigned long startX = buffer.getDWord(bufferIndex+17);
                unsigned long startY = buffer.getDWord(bufferIndex+21);
                unsigned long endX = buffer.getDWord(bufferIndex+25);
                unsigned long endY = buffer.getDWord(bufferIndex+29);
                fprintf(f, "%% @%d: Arc %lu %lu %lu %lu %lu %lu %lu %lu", bufferIndex,
                        left, top, right, bottom, startX, startY, endX, endY);
                fprintf(f, "%lu %lu %lu %lu %lu %lu %lu %lu A\n", left, top, right, bottom,
                        startX, startY, endX, endY);
                bufferIndex += 33;
                break;
            }
            case 0x19: {
                // Draw line
                unsigned long startX = buffer.getDWord(bufferIndex+1);
                unsigned long startY = buffer.getDWord(bufferIndex+5);
                unsigned long endX = buffer.getDWord(bufferIndex+9);
                unsigned long endY = buffer.getDWord(bufferIndex+13);
                fprintf(f, "%% @%d: Line %lu %lu %lu %lu\n", bufferIndex, startX, startY, endX, endY);
                fprintf(f, "%lu %lu %lu %lu L\n", startX, startY, endX, endY);
                bufferIndex += 17;
                break;
            }
            case 0x1a: {
                // Draw line to
                unsigned long x = buffer.getDWord(bufferIndex+1);
                unsigned long y = buffer.getDWord(bufferIndex+5);
                fprintf(f, "%% @%d: Line to %lu %lu\n", bufferIndex, x, y);
                fprintf(f, "%lu %lu LT\n", x, y);
                bufferIndex += 9;
                break;
            }
            case 0x1b: {
                // Draw line by
                long x = buffer.getSDWord(bufferIndex+1);
                long y = buffer.getSDWord(bufferIndex+5);
                fprintf(f, "%% @%d: Line by %ld %ld\n", bufferIndex, x, y);
                fprintf(f, "%ld %ld LB\n", x, y);
                bufferIndex += 9;
                break;
            }
            case 0x1c:
                // Draw polyline (array mode)
            case 0x1d: {
                // Draw polyline (list mode)
                unsigned long verticesCount = buffer.getDWord(bufferIndex+1);
                unsigned long x, y;
                unsigned long tempIndex = bufferIndex + 5;
                fprintf(f, "%% @%d: Polyline (%lu segments)\n", bufferIndex, verticesCount-1);
                fprintf(f, "[\n");
                for (unsigned long i = 0; i < verticesCount; i++) {
                    x = buffer.getDWord(tempIndex);
                    y = buffer.getDWord(tempIndex+4);
                    fprintf(f, "%lu %lu\n", x, y);
                    tempIndex += 8;
                }
                fprintf(f, "] PL\n");
                bufferIndex += (5 + verticesCount * 2 * 4);
                break;
            }
            case 0x1e: {
                // Draw pie
                unsigned long left = buffer.getDWord(bufferIndex+1);
                unsigned long top = buffer.getDWord(bufferIndex+5);
                unsigned long right = buffer.getDWord(bufferIndex+9);
                unsigned long bottom = buffer.getDWord(bufferIndex+13);
                unsigned long startX = buffer.getDWord(bufferIndex+17);
                unsigned long startY = buffer.getDWord(bufferIndex+21);
                unsigned long endX = buffer.getDWord(bufferIndex+25);
                unsigned long endY = buffer.getDWord(bufferIndex+29);
                fprintf(f, "%% @%d: Pie %lu %lu %lu %lu %lu %lu %lu %lu\n", bufferIndex,
                        left, top, right, bottom, startX, startY, endX, endY);
                fprintf(f, "%lu %lu %lu %lu %lu %lu %lu %lu AP\n", left, top, right, bottom,
                        startX, startY, endX, endY);
                bufferIndex += 33;
                break;
            }
            case 0x1f: {
                // Draw ellipse
                unsigned long left = buffer.getDWord(bufferIndex+1);
                unsigned long top = buffer.getDWord(bufferIndex+5);
                unsigned long right = buffer.getDWord(bufferIndex+9);
                unsigned long bottom = buffer.getDWord(bufferIndex+13);
                fprintf(f, "%% @%d: Ellipse %lu %lu %lu %lu\n", bufferIndex, left, top, right, bottom);
                fprintf(f, "%lu %lu %lu %lu E\n", left, top, right, bottom);
                bufferIndex += 17;
                break;
            }
            case 0x20: {
                // Draw rectangle
                unsigned long left = buffer.getDWord(bufferIndex+1);
                unsigned long top = buffer.getDWord(bufferIndex+5);
                unsigned long right = buffer.getDWord(bufferIndex+9);
                unsigned long bottom = buffer.getDWord(bufferIndex+13);
                fprintf(f, "%% @%d: Rectangle %lu %lu %lu %lu\n", bufferIndex, left, top, right, bottom);
                // Rarely, right==left (likely an EPOC bug, no idea if this can also happen with top/bottom)
                // In this case, the few samples tend to show right is to be substracted from the page size
                if (right == left) {
                    fprintf(f, "%lu %lu pagewidth 1 twips div %lu sub %lu R\n", left, top, right, bottom);
                } else {
                    // Rarely, right<left (likely an EPOC bug, no idea if this can also happen with top/bottom)
                    // In this case, the few samples tend to show right is a width and not an absolute coordinate
                    if (right < left) {
                        right += left;
                    }
                    fprintf(f, "%lu %lu %lu %lu R\n", left, top, right, bottom);
                }
                bufferIndex += 17;
                break;
            }
            case 0x21: {
                // Draw rounded rectangle
                unsigned long left = buffer.getDWord(bufferIndex+1);
                unsigned long top = buffer.getDWord(bufferIndex+5);
                unsigned long right = buffer.getDWord(bufferIndex+9);
                unsigned long bottom = buffer.getDWord(bufferIndex+13);
                unsigned long cornerWidth = buffer.getDWord(bufferIndex+17);
                unsigned long cornerHeight = buffer.getDWord(bufferIndex+21);
                fprintf(f, "%% @%d: Rounded rectangle %lu %lu %lu %lu %lu %lu\n", bufferIndex, left, top, right, bottom,
                        cornerWidth, cornerHeight);
                fprintf(f, "%lu %lu %lu %lu R\n", left, top, right, bottom);
                bufferIndex += 25;
                break;
            }
            case 0x22:
                // Draw polygon (array mode)
            case 0x23: {
                // Draw polygon (list mode)
                unsigned long verticesCount = buffer.getDWord(bufferIndex+1);
                unsigned long x, y;
                unsigned long tempIndex = bufferIndex + 5;
                fprintf(f, "%% @%d: Polygon (%lu segments)\n", bufferIndex, verticesCount);
                fprintf(f, "[\n");
                for (unsigned long i = 0; i < verticesCount; i++) {
                    x = buffer.getDWord(tempIndex);
                    y = buffer.getDWord(tempIndex+4);
                    fprintf(f, "%lu %lu\n", x, y);
                    tempIndex += 8;
                }
                unsigned char fillRule = buffer.getByte(tempIndex);
                fprintf(f, "] %s PG\n", fillRule ? "false" : "true");
                bufferIndex += (5 + verticesCount * 2 * 4 + 1);
                break;
            }
            case 0x24: {
                // Draw bitmap at point
                unsigned long x = buffer.getDWord(bufferIndex+1);
                unsigned long y = buffer.getDWord(bufferIndex+5);
                unsigned long bitmapLength = buffer.getDWord(bufferIndex+9);
                fprintf(f, "%% @%d: Bitmap at %lu %lu ...\n", bufferIndex, x, y);
                ps_bitmap(f, x, y, buffer.getString(bufferIndex+9));
                bufferIndex += (9 + bitmapLength);
                break;
            }
            case 0x25: {
                // Draw bitmap in rect
                unsigned long left = buffer.getDWord(bufferIndex+1);
                unsigned long top = buffer.getDWord(bufferIndex+5);
                unsigned long right = buffer.getDWord(bufferIndex+9);
                unsigned long bottom = buffer.getDWord(bufferIndex+13);
                unsigned long bitmapLength = buffer.getDWord(bufferIndex+17);
                fprintf(f, "%% @%d: Bitmap in rect %lu %lu %lu %lu\n", bufferIndex, left, bottom, right, top);
                ps_bitmap(f, left, bottom, right, top, buffer.getString(bufferIndex+17));
                bufferIndex += (17 + bitmapLength);
                break;
            }
            case 0x26: {
                // Draw bitmap in rect from source rect
                unsigned long left = buffer.getDWord(bufferIndex+1);
                unsigned long top = buffer.getDWord(bufferIndex+5);
                unsigned long right = buffer.getDWord(bufferIndex+9);
                unsigned long bottom = buffer.getDWord(bufferIndex+13);
                unsigned long bitmapLength = buffer.getDWord(bufferIndex+17);
                unsigned long sourceLeft = buffer.getDWord(bufferIndex+17+bitmapLength);
                unsigned long sourceBottom = buffer.getDWord(bufferIndex+17+bitmapLength+4);
                unsigned long sourceRight = buffer.getDWord(bufferIndex+17+bitmapLength+8);
                unsigned long sourceTop = buffer.getDWord(bufferIndex+17+bitmapLength+12);
                fprintf(f, "%% @%d: Bitmap in rect %lu %lu %lu %lu from %lu %lu %lu %lu\n", bufferIndex,
                        left, bottom, right, top, sourceLeft, sourceBottom, sourceRight, sourceTop);
                ps_bitmap(f, left, bottom, right, top, sourceLeft, sourceBottom, sourceRight, sourceTop,
                        buffer.getString(bufferIndex+17));
                bufferIndex += (17 + bitmapLength + 16);
                break;
            }
            case 0x27: {
                // Draw text
                int textLength;
                int tempIndex;
                if (buffer.getByte(bufferIndex+1) & 1) {
                    textLength = buffer.getWord(bufferIndex+1) >> 3;
                    tempIndex = bufferIndex + 3;
                } else {
                    textLength = buffer.getByte(bufferIndex+1) >> 2;
                    tempIndex = bufferIndex + 2;
                }
                std::string text(buffer.getString(tempIndex), textLength);
                tempIndex += textLength;
                unsigned long left = buffer.getDWord(tempIndex);
                unsigned long baseline = buffer.getDWord(tempIndex+4);
                ps_escape(text);
                fprintf(f, "%% @%d: Text '%s' %lu %lu\n", bufferIndex, text.c_str(), left, baseline);
                fprintf(f, "(%s) %lu %lu 0 0 -1 T\n", text.c_str(), left, baseline + baselineOffset);
                bufferIndex = (tempIndex + 8);
                break;
            }
            case 0x28: {
                // Draw justified text
                int textLength;
                int tempIndex;
                if (buffer.getByte(bufferIndex+1) & 1) {
                    textLength = buffer.getWord(bufferIndex+1) >> 3;
                    tempIndex = bufferIndex + 3;
                } else {
                    textLength = buffer.getByte(bufferIndex+1) >> 2;
                    tempIndex = bufferIndex + 2;
                }
                std::string text(buffer.getString(tempIndex), textLength);
                tempIndex += textLength;
                unsigned long left = buffer.getDWord(tempIndex);
                unsigned long top = buffer.getDWord(tempIndex+4);
                unsigned long right = buffer.getDWord(tempIndex+8);
                unsigned long bottom = buffer.getDWord(tempIndex+12);
                unsigned long baseline = buffer.getDWord(tempIndex+16);
                unsigned char align = buffer.getByte(tempIndex+20);
                unsigned long amargin = buffer.getDWord(tempIndex+21);
                ps_escape(text);
                if (align == 2) {
                    right -= amargin;
                } else {
                    left += amargin;
                }
                bottom -= lround((bottom - top) / 4.0);
                fprintf(f, "%% @%d: JText '%s' %lu %lu %lu %lu %lu %d %lu\n", bufferIndex, text.c_str(),
                        left, bottom + baselineOffset, top, right, baseline, align, amargin);
                fprintf(f, "(%s) %lu %lu %lu %lu %d T\n", text.c_str(),
                        left, bottom + baselineOffset, top, right, align);
                bufferIndex = (tempIndex + 25);
                break;
            }
            default: {
                fprintf(f, "@%d: UNHANDLED OPCODE %02x\n", bufferIndex, opcode);
                debuglog("@%d: UNHANDLED OPCODE %02x", bufferIndex, opcode);
                bufferIndex += 1;
            }
        }
    }
}

static void service_loop() {
    bool jobLoop = true;
    while (jobLoop) {
        bool spoolOpen = false;
        bool pageStart = true;
        bool cancelled = false;
        bool jobEnd;
        long pageLength;
        long pageBufferLength;
        int pageCount = 0;
        BufferStore buffer;
        BufferStore pageBuffer;
        int fd;
        FILE *f = nullptr;
        unsigned char packetType;
        char *jname = static_cast<char *>(malloc(strlen(spooldir) + strlen(TEMPLATE) + 2));

        while (jobLoop) {
            /* Job loop */
            buffer.init();
            switch (wPrt->getData(buffer)) {
                case rfsv::E_PSI_FILE_DISC: {
                    jobLoop = false;
                    break;
                }
                case rfsv::E_PSI_GEN_NONE: {
                    if ((buffer.getLen() == 15) && (!memcmp(buffer.getString(0), fakePage, 6))) {
                        buffer.discardFirstBytes(6);
                        if (spoolOpen) {
                            fclose(f);
                            infolog("Cancelled job %s", jname);
                            unlink(jname);
                            spoolOpen = false;
                        }
                        sprintf(jname, "%s/%s", spooldir, TEMPLATE);
                        if ((fd = mkstemp(jname)) != -1) {
                            f = fdopen(fd, "w");
                            infolog("Receiving new job %s", jname);
                            cancelled = false;
                            spoolOpen = true;
                            pageStart = true;
                            pageCount = 0;
                            write_job_header(f, buffer);
                            continue;
                        } else {
                            errorlog("Could not create spool file.");
                            cancelled = true;
                            spoolOpen = false;
                            wPrt->cancelJob();
                        }
                        pageLength = 0; // FCAP: to be moved?
                    }
                    packetType = buffer.getByte(0);
                    if ((packetType != 0x2a) && (packetType != 0xff)) {
                        errorlog("Invalid packet type 0x%02x.", packetType);
                        cancelled = true;
                        wPrt->cancelJob();
                    }
                    jobEnd = (packetType == 0xff);
                    if (!cancelled) {
                        buffer.discardFirstBytes(1);
                        if (pageStart) {
                            pageLength = buffer.getDWord(1);
                            buffer.discardFirstBytes(5);
                            pageBuffer.init();
                            pageBufferLength = 0;
                            pageStart = false;
                        }
                        pageBuffer.addBuff(buffer);
                        pageBufferLength += buffer.getLen();
                        if (pageBufferLength >= pageLength) {
                            pageCount += 1;
                            convert_page(f, pageCount, pageBuffer);
                            pageBuffer.init();
                            pageBufferLength = 0;
                            pageStart = true;
                        }
                    }
                    if (jobEnd) {
                        if (spoolOpen) {
                            write_job_footer(f, pageCount);
                            fclose(f);
                        }
                        if (!cancelled) {
                            if (pageCount > 0) {
                                if (!printcmd) {
                                    infolog("Output stored in %s", jname);
                                } else {
                                    int r;
                                    char pipeBuffer[4096];

                                    infolog("Spooling %d pages", pageCount);
                                    FILE *pipe = popen(printcmd, "w");
                                    if (!pipe) {
                                        errorlog("Could not execute %s: %m", printcmd);
                                        unlink(jname);
                                    }
                                    f = fopen(jname, "r");
                                    if (!f) {
                                        errorlog("Could not read %s: %m", jname);
                                        pclose(pipe);
                                        unlink(jname);
                                    }
                                    while ((r = fread(pipeBuffer, 1, sizeof(pipeBuffer), f)) > 0) {
                                        fwrite(pipeBuffer, 1, r, pipe);
                                    }
                                    pclose(pipe);
                                    fclose(f);
                                    // unlink(jname); // FCAP
                                }
                            }
                        } else {
                            unlink(jname);
                        }
                        spoolOpen = false;
                    }
                    break;
                }
                default: {
                    jobLoop = false;
                }
            }
        }
        free(jname);
    }
}

static void help() {
    std::cout <<
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

static void usage() {
    std::cerr << "Usage: plpprintd [OPTIONS]" << std::endl
         << "Use --help for more information" << std::endl;
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

int main(int argc, char **argv) {
    TCPSocket *skt;
    std::string host = "127.0.0.1";
    int sockNum = cli_utils::lookup_default_port();
    int ret = 0;
    int c;

    while (1) {
        c = getopt_long(argc, argv, "dhVvp:s:c:", opts, NULL);
        if (c == -1) {
            break;
        }
        switch (c) {
            case '?': {
                usage();
                return -1;
            }
            case 'd': {
                debug = true;
                break;
            }
            case 'v': {
                verbose++;
                break;
            }
            case 'V': {
                std::cout << "plpprintd Version " << VERSION << std::endl;
                return 0;
            }
            case 'h': {
                help();
                return 0;
            }
            case 'p': {
                if (!cli_utils::parse_port(optarg, &host, &sockNum)) {
                    std::cout << _("Invalid port definition.") << std::endl;
                    return 1;
                }
                break;
            }
            case 's': {
                spooldir = strdup(optarg);
                break;
            }
            case 'c': {
                if (!strcmp(optarg, "-")) {
                    printcmd = NULL;
                } else {
                    printcmd = strdup(optarg);
                }
                break;
            }
            default: {

            }
        }
    }
    if (optind < argc) {
        usage();
        return -1;
    }

    skt = new TCPSocket();
    if (!skt->connect(host.c_str(), sockNum)) {
        std::cout << _("plpprintd: could not connect to ncpd") << std::endl;
        return 1;
    }
    if (!debug) {
        ret = fork();
    }
    switch (ret) {
        case 0: {
            /* child */
            setsid();
            ignore_value(chdir("/"));
            if (!debug) {
                openlog("plpprintd", LOG_PID|LOG_CONS, LOG_DAEMON);
                int devnull = open("/dev/null", O_RDWR, 0);
                if (devnull != -1) {
                    dup2(devnull, STDIN_FILENO);
                    dup2(devnull, STDOUT_FILENO);
                    dup2(devnull, STDERR_FILENO);
                    if (devnull > 2) {
                        close(devnull);
                    }
                }
            }
            init_fontmap();
            infolog("started, waiting for requests.");
            serviceLoop = true;
            while (serviceLoop) {
                wPrt = new wprt(skt);
                if (wPrt) {
                    Enum<rfsv::errs> status;
                    status = wPrt->initPrinter();
                    if (status == rfsv::E_PSI_GEN_NONE) {
                        service_loop();
                    } else {
                        if (debug) {
                            debuglog("plpprintd: could not connect: %s", status.toString().c_str());
                        }
                    }
                    delete wPrt;
                    sleep(1);
                } else {
                    errorlog("plpprintd: Could not create wprt object");
                    exit(1);
                }
            }
            break;
        }
        case -1: {
            std::cerr << "plpprintd: fork failed" << std::endl;
            return 1;
        }
        default: {
            /* parent */
            break;
        }
    }
    return 0;
}
