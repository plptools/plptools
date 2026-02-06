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
#include "config.h"

#include <rfsv.h>
#include <rfsvfactory.h>
#include <rpcs.h>
#include <rpcsfactory.h>
#include <rclip.h>
#include <plpintl.h>
#include <ppsocket.h>
#include <bufferstore.h>

#include <iostream>
#include <string>
#include <vector>

#include <stdlib.h>
#include <stdio.h>

#include "ftp.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

using namespace std;

static void
help()
{
    cout << _(
        "Usage: plpftp [OPTIONS]... [FTPCOMMAND]\n"
        "\n"
        "If FTPCOMMAND is given, connect; run FTPCOMMAND and\n"
        "terminate afterwards. If no FTPCOMMAND is given, start up\n"
        "in interactive mode. For help on supported FTPCOMMANDs,\n"
        "use `?' or `help' as FTPCOMMAND.\n"
        "\n"
        "Supported options:\n"
        "\n"
        " -h, --help              Display this text.\n"
        " -V, --version           Print version and exit.\n"
        " -p, --port=[HOST:]PORT  Connect to port PORT on host HOST.\n"
        "                         Default for HOST is 127.0.0.1\n"
        "                         Default for PORT is "
        ) << DPORT << "\n\n";
}

static void
usage() {
    cerr << _("Try `plpftp --help' for more information") << endl;
}

static struct option opts[] = {
    {"help",     no_argument,       0, 'h'},
    {"version",  no_argument,       0, 'V'},
    {"port",     required_argument, 0, 'p'},
    {NULL,       0,                 0,  0 }
};

void
ftpHeader()
{
    cout << _("PLPFTP Version ") << VERSION;
    cout << _(" Copyright (C) 1999 Philip Proudman") << endl;
    cout << _(" Additions Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>") << endl;
    cout << _("                   & (C) 1999 Matt Gumbley <matt@gumbley.demon.co.uk>") << endl;
    cout << _("                   & (C) 2006-2025 Reuben Thomas <rrt@sc3d.org>") << endl;
    cout << _("PLPFTP comes with ABSOLUTELY NO WARRANTY.") << endl;
    cout << _("This is free software, and you are welcome to redistribute it") << endl;
    cout << _("under GPL conditions; see the COPYING file in the distribution.") << endl;
    cout << endl;
    cout << _("FTP like interface started. Type \"?\" for help.") << endl;
}

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
            pp = 0L;
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
    ppsocket *skt2;
    rfsv *a;
    rpcs *r;
    ppsocket *rclipSocket;
    rclip *rc;
    ftp f;
    const char *host = "127.0.0.1";
    int status = 0;
    int sockNum = DPORT;

    setlocale (LC_ALL, "");
    textdomain(PACKAGE);

    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se != 0L)
        sockNum = ntohs(se->s_port);

    while (1) {
        int c = getopt_long(argc, argv, "hVp:", opts, NULL);
        if (c == -1)
            break;
        switch (c) {
            case '?':
                usage();
                return -1;
            case 'V':
                cout << _("plpftp Version ") << VERSION << endl;
                return 0;
            case 'h':
                help();
                return 0;
            case 'p':
                parse_destination(optarg, &host, &sockNum);
                break;
        }
    }
    if (optind == argc)
        ftpHeader();

    skt = new ppsocket();
    if (!skt->connect(host, sockNum)) {
        cout << _("plpftp: could not connect to ncpd") << endl;
        return 1;
    }
    skt2 = new ppsocket();
    if (!skt2->connect(host, sockNum)) {
        cout << _("plpftp: could not connect to ncpd") << endl;
        return 1;
    }
    rfsvfactory *rf = new rfsvfactory(skt);
    rpcsfactory *rp = new rpcsfactory(skt2);
    a = rf->create(false);
    r = rp->create(false);
    rclipSocket = new ppsocket();
    rclipSocket->connect(NULL, sockNum);
    if (rclipSocket)
        rc = new rclip(rclipSocket);
    f.canClip = rclipSocket && rc ? true : false;
    if ((a != NULL) && (r != NULL)) {
        vector<char *> args(argv + optind, argv + argc);
        status = f.session(*a, *r, *rc, *rclipSocket, args);
        delete r;
        delete a;
        delete skt;
        delete skt2;
        if (rclipSocket)
            delete rclipSocket;
        if (rc)
            delete rc;
    } else {
        cerr << "plpftp: " << rf->getError() << endl;
        status = 1;
    }
    delete rf;
    delete rp;
    return status;
}
