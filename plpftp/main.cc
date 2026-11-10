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
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include <sys/ioctl.h>

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
#include <ncpsession.h>
#include <semaphore.h>

#include <stdlib.h>
#include <stdio.h>

#include "ftp.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

using namespace std;

static std::string wrapText(const std::string &text, int width) {
    std::string output;
    std::stringstream lines(text);
    std::string line;
    int yPos = 0;
    while (std::getline(lines, line, '\n')) {
        if (yPos > 0) {
            output += "\n";
        }
        int xPos = 0;
        std::stringstream words(line);
        std::string word;
        while (std::getline(words, word, ' ')) {
            if (xPos + 1 + static_cast<int>(word.length()) > width) {
                yPos += 1;
                output += "\n";
                xPos = word.length();
            } else if (xPos > 0) {
                output += " ";
                xPos += 1;
            }
            output += word;
            xPos += word.length();
        }
        yPos += 1;
    }
    return output;
}

static int terminalWidth(std::size_t fallback = 80) {
    winsize w{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        return w.ws_col;
    }
    return fallback;
}

struct wrap {
    int width = terminalWidth();
};

struct Wrapper {
public:
    Wrapper(std::ostream &os, int width)
    : os_(os)
    , width_(width) {

    }

    std::ostream &operator<<(const std::string &text) const {
        return os_ << wrapText(text, width_);
    }

private:
    std::ostream &os_;
    int width_;
};

inline Wrapper operator<<(std::ostream &os, struct wrap w) {
    return Wrapper(os, w.width);
}

static void help() {
    cout << _(
        "Usage: plpftp [OPTIONS]... [FTPCOMMAND]\n"
        "\n"
        "Options:\n"
        "\n"
        " -h, --help              Display this text.\n"
        " -V, --version           Print version and exit.\n"
        " -s, --serial=DEV        Use serial device DEV (optional).\n"
        " -b, --baudrate=RATE     Set serial speed to BAUD (optional).\n"
        " -p, --port=[HOST:]PORT  Connect to port PORT on host HOST.\n"
        "                         Default for HOST is 127.0.0.1\n"
        "                         Default for PORT is "
        ) << DPORT << endl;
    cout << endl;
    cout << wrap() << _("If FTPCOMMAND is given, connect, run FTPCOMMAND, and terminate afterwards. If no FTPCOMMAND is given, start up in interactive mode. For help on supported FTPCOMMANDs, use `?' or `help' as FTPCOMMAND.") << endl << endl;
    cout << wrap() << _("By default, plpftp connects to a pre-existing ncpd instance on the specified TCP port (-p|--port). To connect directly without running a separate ncpd instance, provide a serial port (-s|--serial) and optional the baud rate (-b|--baud).") << endl << endl;
}

static void usage() {
    cerr << _("Try `plpftp --help' for more information") << endl;
}

static struct option opts[] = {
    {"help",     no_argument,       nullptr, 'h'},
    {"version",  no_argument,       nullptr, 'V'},
    {"port",     required_argument, nullptr, 'p'},
    {"serial",   required_argument, nullptr, 's'},
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
    int baudRate = DSPEED;
    const char *serialDevice = NULL;

    setlocale (LC_ALL, "");
    textdomain(PACKAGE);

    while (1) {
        int c = getopt_long(argc, argv, "hVps:", opts, NULL);
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
            case 's':
                serialDevice = optarg;
                break;
            case 'b':
                if (!strcmp(optarg, "auto"))
                    baudRate = -1;
                else
                    baudRate = atoi(optarg);
                break;

        }
    }
    if (optind == argc) {
        ftpHeader();
    }

    Semaphore *sem = new Semaphore();

    // Optionally start ncpd.
    std::unique_ptr<NCPSession> session = nullptr;
    if (serialDevice) {
        session = std::make_unique<NCPSession>(
            port,
            baudRate,
            host,
            serialDevice,
            false,
            false,
            0,
            [](void *context, bool connected, int /* version */) {
                if (!connected) {
                    return;
                }
                static_cast<Semaphore *>(context)->signal();
            },
            sem);
        session->start();
        sem->wait();
    }

    Enum<ConnectionError> error;
    auto deviceEndpoint = DeviceEndpoint::connect(host, port, &error);
    if (!deviceEndpoint) {
        std::cerr << "plpftp: " << error << std::endl;
        return EXIT_FAILURE;
    }
    vector<char *> args(argv + optind, argv + argc);
    return ftp.session(*deviceEndpoint, *deviceEndpoint->rfsv_, *deviceEndpoint->rpcs_, *deviceEndpoint->clip_, args);
    session->cancel();

    // Clean up the session if there is one.
    // TODO: We could perhaps move this into the session destructor?
    if (session) {
        session->wait();
    }

}
