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

#include "sisfile.h"
#include "sisinstaller.h"
#include "psion.h"
#include "fakepsion.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

static void error(int line)
{
        fprintf(stderr, _("Error %d on line %d: %s\n"), errno, line,
                strerror(errno));
        exit(1);
}

static struct option opts[] = {
        { "help",     no_argument,       0, 'h' },
        { "version",  no_argument,       0, 'V' },
        { "verbose", required_argument, 0, 'v' },
        { "dry-run",  no_argument,       0, 'n' },
        { NULL,       0,                 0, 0 },
};

void printHelp()
{
        printf("%s",
        _("Usage: sisinstall [OPTIONS]... SISFILE\n"
        "\n"
        "Supported options:\n"
        "\n"
        " -h, --help              Display this text.\n"
        " -V, --version           Print version and exit.\n"
        " -v, --verbose=LEVEL     Set the verbosity level, by default 0.\n"
        " -n, --dry-run           Just parse the file.\n"
        ));
}

int main(int argc, char* argv[])
{
        char* filename = 0;
        char option;
        bool dryrun = false;

#ifdef LC_ALL
        setlocale(LC_ALL, "");
#endif
        textdomain(PACKAGE);

        while (1)
                {
                option = getopt_long(argc, argv,
                                                         "hnv:V"
                                                         , opts, NULL);
                if (option == -1)
                        break;
                switch (option)
                        {
                        case 'h':
                        case '?':
                                printHelp();
                                exit(0);
                        case 'v':
                                logLevel = atoi(optarg);
                                break;
                        case 'n':
                                dryrun = true;
                                break;
                        case 'V':
                                printf("%s %s\n", _("sisinstall Version"), VERSION);
                                exit(0);
                        }
                }
        if (optind < argc)
        {
                filename = argv[optind];
                printf(_("Installing sis file %s%s.\n"), filename,
                       dryrun ? _(", not really") : "");
        }
        else
        {
                fprintf(stderr, "%s", _("Missing SIS filename\n"));
                exit(1);
        }
        struct stat st;
        if (-1 == stat(filename, &st))
                error(__LINE__);
        off_t len = st.st_size;
        if (logLevel >= 2)
                printf(_("File is %jd bytes long\n"), (intmax_t)len);
        uint8_t* buf = new uint8_t[len];
        int fd = open(filename, O_RDONLY);
        if (-1 == fd)
                error(__LINE__);
        if (-1 == read(fd, buf, len))
                error(__LINE__);
        close(fd);
        Psion* psion;
        if (dryrun)
                psion = new FakePsion();
        else
                psion = new Psion();
        if (!psion->connect())
                {
                        printf("%s", _("Couldn't connect with the Psion\n"));
                }
        else
                {
                createCRCTable();
                SISFile sisFile;
                SisRC rc = sisFile.fillFrom(buf, len);
                if (rc == SIS_OK)
                        {
                                SISInstaller installer;
                                installer.setPsion(psion);
                                installer.run(&sisFile, buf, len);
                        }
                else
                        printf("%s", _("Could not parse the sis file.\n"));
                psion->disconnect();
                }

        return 0;
}
