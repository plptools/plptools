/*
 * This file is part of plptools.
 *
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

#include "psiprocess.h"

#include <sstream>
#include <iomanip>

using namespace std;

PsiProcess::PsiProcess()
    : pid(0), name(""), args(""), s5mx(false) {
}

PsiProcess::PsiProcess(const PsiProcess &p) {
    pid  = p.pid;
    name = p.name;
    args = p.args;
    s5mx = p.s5mx;
}

PsiProcess::PsiProcess(int _pid, const char * const _name,
                       const char * const _args, bool _s5mx) {
    pid  = _pid;
    name = _name;
    args = _args;
    s5mx = _s5mx;
}

int PsiProcess::
getPID() {
    return pid;
}

const char *PsiProcess::
getName() {
    return name.c_str();
}

const char *PsiProcess::
getArgs() {
    return args.c_str();
}

const char *PsiProcess::
getProcId() {
    ostringstream tmp;

    if (s5mx)
        tmp << name << ".$" << setw(2) << setfill('0') << pid << '\0';
    else
        tmp << name << ".$" << pid << '\0';
    return tmp.str().c_str();
}

void PsiProcess::
setArgs(string _args) {
    args = _args;
}

PsiProcess &PsiProcess::
operator=(const PsiProcess &p) {
    pid  = p.pid;
    name = p.name;
    args = p.args;
    s5mx = p.s5mx;
    return *this;
}

ostream &
operator<<(ostream &o, const PsiProcess &p) {
    ostream::fmtflags old = o.flags();

    o << dec << setw(5) << setfill(' ') << p.pid << " " << setw(12)
      << setfill(' ') << setiosflags(ios::left) << p.name.c_str()
      << resetiosflags(ios::left) << " " << p.args;
    o.flags(old);
    return o;
}
