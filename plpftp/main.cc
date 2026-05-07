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

#include <iostream>
#include <memory>
#include <vector>

#include <bufferstore.h>
#include <cliutils.h>
#include <device.h>
#include <deviceendpoint.h>
#include <deviceconfiguration.h>
#include <plpintl.h>
#include <rclip.h>
#include <rfsv.h>
#include <rfsvfactory.h>
#include <rpcs.h>
#include <rpcsfactory.h>
#include <tcpsocket.h>

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

static void usage() {
    cerr << _("Try `plpftp --help' for more information") << endl;
}

static struct option opts[] = {
    {"help",     no_argument,       nullptr, 'h'},
    {"version",  no_argument,       nullptr, 'V'},
    {"port",     required_argument, nullptr, 'p'},
    {NULL,       0,                 nullptr,  0 }
};

void ftpHeader() {
    cout << _("PLPFTP Version ") << VERSION << endl;
    cout << _("FTP like interface started. Type \"?\" for help.") << endl;
    cout << endl;
}

int main(int argc, char **argv) {
    FTP ftp;
    string host = "127.0.0.1";
    int port = cli_utils::lookup_default_port();

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
                if (!cli_utils::parse_port(optarg, &host, &port)) {
                    cout << _("Invalid port definition.") << endl;
                    return 1;
                }
                break;
        }
    }
    if (optind == argc) {
        ftpHeader();
    }

    Enum<ConnectionError> error;
    auto deviceEndpoint = DeviceEndpoint::connect(host, port, &error);
    if (!deviceEndpoint) {
        std::cerr << "plpftp: " << error << std::endl;
        return EXIT_FAILURE;
    }
    vector<char *> args(argv + optind, argv + argc);
    return ftp.session(*deviceEndpoint->rfsv_, *deviceEndpoint->rpcs_, *deviceEndpoint->clip_, args);
}
