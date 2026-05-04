/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
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

#include "cli_utils.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdlib.h>

#include <netdb.h>
#include <arpa/inet.h>

bool cli_utils::is_number(const std::string &s) {
    if (s.empty()) {
        return false;
    }
    return std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return ::isdigit(c);
    });
}

int cli_utils::lookup_default_port() {
    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se == nullptr) {
        return DPORT;
    }
    return ntohs(se->s_port);
}

bool cli_utils::parse_port(const std::string &arg, std::string *host, int *port) {

    if (host == nullptr || port == nullptr) {
        return false;
    }

    if (arg.empty()) {
        return true;
    }

    size_t pos = arg.find(':');
    if (pos != std::string::npos) {

        // host.domain:400
        // 10.0.0.1:400

        std::string hostComponent = arg.substr(0, pos);
        std::string portComponent = arg.substr(pos + 1);
        if (hostComponent.empty() || portComponent.empty() || !cli_utils::is_number(portComponent)) {
            return false;
        }

        *host = hostComponent;
        *port = atoi(portComponent.c_str());

    } else if (cli_utils::is_number(arg)) {

        // 400

        *port = atoi(arg.c_str());

    } else {

        // host.domain
        // host
        // 10.0.0.1

        *host = arg;

    }

    return true;
}
