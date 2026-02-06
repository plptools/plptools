/* plpfuse/Makefile.am
 *
 * This file is part of plptools.
 *
 * Copyright (C) 2001 Fritz Elfert <felfert@to.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <linux/ttytap.h>
#include <linux/kdev_t.h>

static void usage(void) {
    fprintf(stderr, "usage: ttytap -q|<ttyDevice>\n");
    exit(1);
}

int main(int argc, char **argv) {
    int fd;
    tapheader th;
    int i;
    int r;
    int major;
    int minor;
    int lastdir = -1;
    struct stat stbuf;
    char tbuf[256];
    unsigned char buf[4096];


    if (argc != 2)
        usage();
    if (strcmp(argv[1], "-q")) {
        if (stat(argv[1], &stbuf)) {
            perror(argv[1]);
            exit(-1);
        }
        if (!S_ISCHR(stbuf.st_mode)) {
            fprintf(stderr, "%s is not a char device\n", argv[1]);
            exit(1);
        }
        major = MAJOR(stbuf.st_rdev);
        minor = MINOR(stbuf.st_rdev);
    } else {
        minor = 0;
        major = 0;
    }
    fd = open("/proc/ttytap", O_RDONLY);
    if (fd == -1) {
        perror("/proc/ttytap");
        exit(-1);
    }
    if (ioctl(fd, TTYTAP_SETDEV, (major << 8) | minor)) {
        perror("ioctl /proc/ttytap");
        exit(-1);
    }
    if ((minor == 0) && (major == 0)) {
        printf("Serial tapping switched off\n");
        exit(0);
    }
    if (ioctl(fd, TTYTAP_GETQLEN, &i)) {
        perror("ioctl /proc/ttytap");
        exit(-1);
    }
    fprintf(stderr, "Queue length is %d\n", i);
    while (1) {
        r = read(fd, &th, sizeof(th));
        if (r != sizeof(th)) {
            perror("read /proc/ttytap");
            exit(-1);
        }
        strftime(tbuf, 255, "%H:%M:%S", localtime(&th.stamp.tv_sec));
        printf("%s.%06ld %c (%04d) ", tbuf,
            (unsigned long)th.stamp.tv_usec,
            (th.io) ? '<' : '>', th.len);
        r = read(fd, buf, th.len);
        if (r != th.len) {
            perror("read /proc/ttytap");
            exit(-1);
        }
        for (i = 0; i < th.len; i++)
            printf("%02x ", buf[i] & 255);
        for (i = 0; i < th.len; i++)
            printf("%c", isprint(buf[i]) ? buf[i] : '.');
        printf("\n"); fflush(stdout);
    }
    return 0;
}
