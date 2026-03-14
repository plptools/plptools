/*
 * This file is part of plptools.
 *
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

#pragma once

#include "config.h"

#include "transport.h"

class SerialTransport: public Transport {
public:

    SerialTransport(const char *dev, int speed, int debug);
    ~SerialTransport() override;
    int open() override;
    void close() override;
    bool isConnected() const override;
    LinkStatus refreshLink() override;
    int getFd() const override;

private:
    std::string dev_;
    int speed_;
    int debug_;
    int fd_;
};
