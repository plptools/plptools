/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2026 Jason Morley <hello@jbmorley.co.uk>
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

#include <string>
#include <cstring>
#include <iostream>

#include <bufferstore.h>
#include <ppsocket.h>
#include <iowatch.h>
#include <log.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <plpintl.h>
#include <unistd.h>

#include "ignore-value.h"

#include "ncp_log.h"
#include "ncp_session.h"
#include "packet.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

using namespace std;

// Global session state specific to the `ncpd` process. This exists as a global solely for the purpose of accessing it
// from the interrupt handlers.
static NCPSession *sharedSession;

static void
term_handler(int)
{
    linf << _("Got SIGTERM") << endl;
    sharedSession->cancel();
    signal(SIGTERM, term_handler);
};

static void
int_handler(int)
{
    linf << _("Got SIGINT") << endl;
    sharedSession->cancel();
    signal(SIGINT, int_handler);
};

static void
help()
{
    cout << _(
        "Usage: ncpd [OPTIONS]...\n"
        "\n"
        "Supported options:\n"
        "\n"
        " -d, --dontfork          Run in foreground, don't fork\n"
        " -h, --help              Display this text.\n"
        " -V, --version           Print version and exit.\n"
        " -e, --autoexit          Exit after device is disconnected.\n"
        " -v, --verbose=LOGCLASS  Enable logging of LOGCLASS events\n"
        "                         Valid log classes are:\n"
        "                           m   - main program\n"
        "                           nl  - NCP protocol log\n"
        "                           nd  - NCP protocol data dump\n"
        "                           ll  - PLP protocol log\n"
        "                           ld  - PLP protocol data dump\n"
        "                           pl  - physical I/O log\n"
        "                           ph  - physical I/O handshake\n"
        "                           pd  - physical I/O data dump\n"
        "                           all - All of the above\n"
        " -s, --serial=DEV        Use serial device DEV.\n"
        " -b, --baudrate=RATE     Set serial speed to BAUD.\n"
        );
    cout <<
#if DSPEED > 0
#define SPEEDSTR(x) #x
        _("                         Default: ") << DSPEED << ".\n";
#else
    _("                         Default: Autocycle 115.2k, 57.6k 38.4k, 19.2k\n");
#endif
    cout << _(
        " -p, --port=[HOST:]PORT  Listen on host HOST, port PORT.\n"
        "                         Default for HOST: 127.0.0.1\n"
        "                         Default for PORT: "
        ) << DPORT << "\n\n";
}

static void
usage() {
    cerr << _("Try `ncpd --help' for more information") << endl;
}

static struct option opts[] = {
    {"dontfork",   no_argument,       0, 'd'},
    {"autoexit",   no_argument,       0, 'e'},
    {"help",       no_argument,       0, 'h'},
    {"version",    no_argument,       0, 'V'},
    {"verbose",    required_argument, 0, 'v'},
    {"port",       required_argument, 0, 'p'},
    {"serial",     required_argument, 0, 's'},
    {"baudrate",   required_argument, 0, 'b'},
    {NULL,         0,                 0,  0 }
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
    int pid;
    bool dofork = true;

    int sockNum = DPORT;
    int baudRate = DSPEED;
    const char *host = "127.0.0.1";
    const char *serialDevice = NULL;
    unsigned short nverbose = 0;
    bool autoexit = false;

    struct servent *se = getservbyname("psion", "tcp");
    dlog.useFileDescriptor();
    elog.useFileDescriptor();
    ilog.useFileDescriptor();
    if (se != 0L) {
        sockNum = ntohs(se->s_port);
    }
    endservent();

    while (1) {
        int c = getopt_long(argc, argv, "hdeVb:s:p:v:", opts, NULL);
        if (c == -1)
            break;
        switch (c) {
            case '?':
                usage();
                return -1;
            case 'V':
                cout << _("ncpd Version ") << VERSION << endl;
                return 0;
            case 'h':
                help();
                return 0;
            case 'v':
                if (!strcmp(optarg, "nl"))
                    nverbose |= NCP_DEBUG_LOG;
                if (!strcmp(optarg, "nd"))
                    nverbose |= NCP_DEBUG_DUMP;
                if (!strcmp(optarg, "ll"))
                    nverbose |= LNK_DEBUG_LOG;
                if (!strcmp(optarg, "ld"))
                    nverbose |= LNK_DEBUG_DUMP;
                if (!strcmp(optarg, "pl"))
                    nverbose |= PKT_DEBUG_LOG;
                if (!strcmp(optarg, "pd"))
                    nverbose |= PKT_DEBUG_DUMP;
                if (!strcmp(optarg, "ph"))
                    nverbose |= PKT_DEBUG_HANDSHAKE;
                if (!strcmp(optarg, "m"))
                    nverbose |= NCP_SESSION_LOG;
                if (!strcmp(optarg, "all")) {
                    nverbose = NCP_DEBUG_LOG | NCP_DEBUG_DUMP |
                        LNK_DEBUG_LOG | LNK_DEBUG_DUMP |
                        PKT_DEBUG_LOG | PKT_DEBUG_DUMP | PKT_DEBUG_HANDSHAKE |
                        NCP_SESSION_LOG;
                }
                break;
            case 'd':
                dofork = 0;
                break;
            case 'e':
                autoexit = true;
                break;
            case 'b':
                if (!strcmp(optarg, "auto"))
                    baudRate = -1;
                else
                    baudRate = atoi(optarg);
                break;
            case 's':
                serialDevice = optarg;
                break;
            case 'p':
                parse_destination(optarg, &host, &sockNum);
                break;
        }
    }
    if (optind < argc) {
        usage();
        return -1;
    }

    if (serialDevice == NULL)
        serialDevice = DDEV;

    if (dofork)
        pid = fork();
    else
        pid = 0;
    switch (pid) {
        case 0:
            {
            signal(SIGTERM, term_handler);
            signal(SIGINT, int_handler);

            // Configure the daemon process before starting up.
            if (dofork) {
                openlog("ncpd", LOG_CONS|LOG_PID, LOG_DAEMON);
                dlog.useSyslog();
                elog.useSyslog();
                ilog.useSyslog();
                setsid();
                ignore_value(chdir("/"));
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

            // Once our process is fully set up, we can create and start the session.
            sharedSession = new NCPSession(sockNum,
                                           baudRate,
                                           host,
                                           serialDevice,
                                           autoexit,
                                           nverbose);
            sharedSession->start();
            sharedSession->wait();
            delete sharedSession;

            break;
            }
        case -1:
            lerr << "fork: " << strerror(errno) << endl;
            break;
        default:
            exit(0);
    }
    linf << _("normal exit") << endl;
    return 0;
}
