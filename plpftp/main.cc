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

#include <cli_utils.h>
#include <rfsv.h>
#include <rfsvfactory.h>
#include <rpcs.h>
#include <rpcsfactory.h>
#include <rclip.h>
#include <plpintl.h>
#include <tcpsocket.h>
#include <bufferstore.h>

#include <iostream>
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
    {"help",     no_argument,       nullptr, 'h'},
    {"version",  no_argument,       nullptr, 'V'},
    {"port",     required_argument, nullptr, 'p'},
    {NULL,       0,                 nullptr,  0 }
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

int
main(int argc, char **argv)
{
    TCPSocket *skt;
    TCPSocket *skt2;
    RFSV *a;
    RPCS *r;
    TCPSocket *rclipSocket;
    rclip *rc;
    ftp f;
    string host = "127.0.0.1";
    int status = 0;
    int sockNum = cli_utils::lookup_default_port();

    setlocale (LC_ALL, "");
    textdomain(PACKAGE);

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
                if (!cli_utils::parse_port(optarg, &host, &sockNum)) {
                    cout << _("Invalid port definition.") << endl;
                    return 1;
                }
                break;
        }
    }
    if (optind == argc)
        ftpHeader();

    skt = new TCPSocket();
    if (!skt->connect(host.c_str(), sockNum)) {
        cout << _("plpftp: could not connect to ncpd") << endl;
        return 1;
    }
    skt2 = new TCPSocket();
    if (!skt2->connect(host.c_str(), sockNum)) {
        cout << _("plpftp: could not connect to ncpd") << endl;
        return 1;
    }
    RFSVFactory *rf = new RFSVFactory(skt);
    rpcsfactory *rp = new rpcsfactory(skt2);
    a = rf->create(false);
    r = rp->create(false);
    rclipSocket = new TCPSocket();
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
