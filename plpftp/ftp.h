/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2025 Reuben Thomas <rrt@sc3d.org>
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
#ifndef _ftp_h_
#define _ftp_h_

#include "config.h"

#include <vector>

#include "rfsv.h"
#include "Enum.h"

class rpcs;
class bufferStore;
class bufferArray;

class ftp {
    public:
    ftp();
    ~ftp();
    int session(rfsv & a, rpcs & r, rclip & rc, ppsocket & rclipSocket, std::vector<char *> argv);
    bool canClip;

    private:
    std::vector<char *> getCommand();
    void initReadline(void);
    int putClipText(rpcs & r, rfsv & a, rclip & rc, ppsocket & rclipSocket, const char *data);
    int getClipData(rpcs & r, rfsv & a, rclip & rc, ppsocket & rclipSocket, const char *file);
    bool checkClipConnection(rfsv &a, rclip & rc, ppsocket & rclipSocket);

    // utilities
    void resetUnixWd();
    void usage();

    char defDrive[9];
    char *localDir;
};

#endif
