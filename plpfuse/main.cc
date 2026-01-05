/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2007 Reuben Thomas <rrt@sc3d.org>
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
#include <rpcs.h>
#include <rfsvfactory.h>
#include <rpcsfactory.h>
#include <bufferstore.h>
#include <bufferarray.h>
#include <ppsocket.h>

#include <iostream>
#include <string>

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "rfsv_api.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

#include <fuse3/fuse_lowlevel.h>

using namespace std;

static rfsv *a;
static rfsvfactory *rf;

static rpcs *r;
static rpcsfactory *rp;
static bufferStore owner;

/* Translate EPOC/SIBO error to UNIX error code, leaving positive
   numbers alone */
int epocerr_to_errno(long epocerr) {
  int unixerr = (int)epocerr;

  if (epocerr < 0) {
    switch (epocerr) {
    case rfsv::E_PSI_GEN_NONE:
      unixerr = 0;
      break;
    case rfsv::E_PSI_FILE_EXIST:
      unixerr = -EEXIST;
      break;
    case rfsv::E_PSI_FILE_NXIST:
    case rfsv::E_PSI_FILE_DIR:
      unixerr = -ENOENT;
      break;
    case rfsv::E_PSI_FILE_WRITE:
    case rfsv::E_PSI_FILE_READ:
    case rfsv::E_PSI_FILE_EOF: // Can't err = EOF as it's not an error code
    case rfsv::E_PSI_FILE_ALLOC: // FIXME: No idea what this is
    case rfsv::E_PSI_FILE_UNKNOWN:
    case rfsv::E_PSI_FILE_DIRFULL:
      unixerr = -EPERM;
      break;
    case rfsv::E_PSI_FILE_FULL:
      unixerr = -ENOSPC;
      break;
    case rfsv::E_PSI_FILE_NAME:
    case rfsv::E_PSI_FILE_RECORD:
    case rfsv::E_PSI_FILE_VOLUME:
      unixerr = -EINVAL;
      break;
    case rfsv::E_PSI_FILE_ACCESS:
    case rfsv::E_PSI_FILE_LOCKED:
    case rfsv::E_PSI_FILE_RDONLY:
    case rfsv::E_PSI_FILE_PROTECT:
      unixerr = -EACCES;
      break;
    case rfsv::E_PSI_GEN_INUSE:
    case rfsv::E_PSI_FILE_DEVICE:
    case rfsv::E_PSI_FILE_PENDING:
    case rfsv::E_PSI_FILE_NOTREADY:
      unixerr = -EBUSY;
      break;
    case rfsv::E_PSI_FILE_INV:
    case rfsv::E_PSI_FILE_RETRAN:
    case rfsv::E_PSI_FILE_LINE:
    case rfsv::E_PSI_FILE_INACT:
    case rfsv::E_PSI_FILE_PARITY:
    case rfsv::E_PSI_FILE_FRAME:
    case rfsv::E_PSI_FILE_OVERRUN:
    case rfsv::E_PSI_FILE_CORRUPT:
    case rfsv::E_PSI_FILE_INVALID:
    case rfsv::E_PSI_FILE_ABORT:
    case rfsv::E_PSI_FILE_ERASE:
    case rfsv::E_PSI_FILE_NDISC:
    case rfsv::E_PSI_FILE_DRIVER:
    case rfsv::E_PSI_FILE_COMPLETION:
    default:
      unixerr = -EIO;
      break;
    case rfsv::E_PSI_FILE_CANCEL:
      unixerr = -EINTR;
      break;
    case rfsv::E_PSI_FILE_DISC:
    case rfsv::E_PSI_FILE_CONNECT:
      unixerr = -ENODEV;
      break;
    case rfsv::E_PSI_FILE_TOOBIG:
      unixerr = -EFBIG;
      break;
    case rfsv::E_PSI_FILE_HANDLE:
      unixerr = -EBADF;
      break;
    }
  }

  debuglog("EPOC error %ld became UNIX code %d", epocerr, unixerr);
  return unixerr;
}

int rfsv_isalive(void) {
    if (!a) {
	if (!(a = rf->create(true)))
	    return 0;
    }
    return a->getStatus() == rfsv::E_PSI_GEN_NONE;
}

int rfsv_dir(const char *file, dentry **e) {
    PlpDir entries;
    dentry *tmp;
    long ret;

    if (!a)
	return -ENODEV;
    ret = a->dir(file, entries);

    for (int i = 0; i < entries.size(); i++) {
	PlpDirent pe = entries[i];
	tmp = *e;
	*e = (dentry *)calloc(1, sizeof(dentry));
	if (!*e)
	    return -ENODEV;
	(*e)->time = pe.getPsiTime().getTime();
	(*e)->size = pe.getSize();
	(*e)->attr = pe.getAttr();
	(*e)->name = strdup(pe.getName());
	(*e)->next = tmp;
    }
    return epocerr_to_errno(ret);
}

int rfsv_dircount(const char *file, uint32_t *count) {
    if (!a)
	return -ENODEV;
    return epocerr_to_errno(a->dircount(file, *count));
}

int rfsv_rmdir(const char *name) {
    if (!a)
	return -ENODEV;
    return epocerr_to_errno(a->rmdir(name));
}

int rfsv_mkdir(const char *file) {
    if (!a)
	return -ENODEV;
    return epocerr_to_errno(a->mkdir(file));
}

int rfsv_remove(const char *file) {
    if (!a)
	return -ENODEV;
    return epocerr_to_errno(a->remove(file));
}

int rfsv_fclose(long handle) {
    if (!a)
	return -ENODEV;
    return epocerr_to_errno(a->fclose(handle));
}

int rfsv_fcreate(long attr, const char *file, uint32_t *handle) {
    uint32_t ph;
    long ret;

    if (!a)
	return -ENODEV;
    ret = a->fcreatefile(attr, file, ph);
    *handle = ph;
    return epocerr_to_errno(ret);
}

int rfsv_open(const char *name, long mode, uint32_t *handle) {
    long ret, retry;

    if (!a)
	return -ENODEV;
    if (mode == O_RDONLY)
        mode = rfsv::PSI_O_RDONLY;
    else
        mode = rfsv::PSI_O_RDWR;
    for (retry = 100; retry > 0 && (ret = a->fopen(a->opMode(mode), name, *handle)) != rfsv::E_PSI_GEN_NONE; retry--)
        usleep(20000);
    return epocerr_to_errno(ret);
}

int rfsv_read(char *buf, long offset, long len, const char *name) {
    uint32_t ret = 0, r_offset, handle;

    if (!a)
	return -ENODEV;
    if ((ret = rfsv_open(name, O_RDONLY, &handle)))
        return ret;
    if (a->fseek(handle, offset, rfsv::PSI_SEEK_SET, r_offset) != rfsv::E_PSI_GEN_NONE ||
        offset != r_offset ||
        a->fread(handle, (unsigned char *)buf, len, ret) != rfsv::E_PSI_GEN_NONE)
	ret = -1;
    rfsv_fclose(handle);
    return epocerr_to_errno(ret);
}

int rfsv_write(const char *buf, long offset, long len, const char *name) {
    uint32_t ret = 0, r_offset, handle;

    if (!a)
	return -ENODEV;
    if ((ret = rfsv_open(name, O_RDWR, &handle)))
        return ret;
    if (a->fseek(handle, offset, rfsv::PSI_SEEK_SET, r_offset) != rfsv::E_PSI_GEN_NONE ||
        offset != r_offset ||
        a->fwrite(handle, (unsigned char *)buf, len, ret) != rfsv::E_PSI_GEN_NONE)
	ret = -1;
    rfsv_fclose(handle);
    return epocerr_to_errno(ret);
}

int rfsv_setmtime(const char *name, long time) {
    if (!a)
	return -ENODEV;
    return epocerr_to_errno(a->fsetmtime(name, PsiTime(time)));
}

int rfsv_setsize(const char *name, long size) {
    uint32_t ph;
    long ret;

    if (!a)
	return -ENODEV;
    ret = a->fopen(a->opMode(rfsv::PSI_O_RDWR), name, ph);
    if (!ret) {
	ret = a->fsetsize(ph, size);
	a->fclose(ph);
    }
    return epocerr_to_errno(ret);
}

int rfsv_setattr(const char *name, long sattr, long dattr) {
    if (!a)
	return -ENODEV;
    return epocerr_to_errno(a->fsetattr(name, sattr, dattr));
}

int rfsv_getattr(const char *name, long *attr, long *size, long *time) {
    long res;
    PlpDirent e;

    if (!a)
	return -ENODEV;
    res = a->fgeteattr(name, e);
    *attr = e.getAttr();
    *size = e.getSize();
    *time = e.getPsiTime().getTime();
    return epocerr_to_errno(res);
}

int rfsv_rename(const char *oldname, const char *newname) {
    if (!a)
	return -ENODEV;
    return epocerr_to_errno(a->rename(oldname, newname));
}

int rfsv_drivelist(int *cnt, device **dlist) {
    *dlist = NULL;
    uint32_t devbits;
    long ret;
    int i;

    if (!a)
	return -ENODEV;
    ret = a->devlist(devbits);
    if (ret == 0)
	for (i = 0; i < 26; i++) {
	    PlpDrive drive;

	    if ((devbits & 1) &&
		((a->devinfo(i + 'A', drive) == rfsv::E_PSI_GEN_NONE))) {

		device *next = *dlist;
		*dlist = (device *)malloc(sizeof(device));
		(*dlist)->next = next;
		(*dlist)->name = strdup(drive.getName().c_str());
		(*dlist)->total = drive.getSize();
		(*dlist)->free = drive.getSpace();
		(*dlist)->letter = 'A' + i;
		(*dlist)->attrib = drive.getMediaType();
		(*cnt)++;
	    }
	    devbits >>= 1;
	}
    return epocerr_to_errno(ret);
}

static void
help()
{
    cerr << _(
	"Usage: plpfuse [OPTION...] MOUNTPOINT\n"
	"\n"
	"plpfuse options:\n"
	"\n"
	"    -d, --debug             Increase debugging level\n"
	"    -h, --help              Display this text\n"
	"    -V, --version           Print version and exit\n"
	"    -p, --port=[HOST:]PORT  Connect to port PORT on host HOST\n"
	"                            Default for HOST is 127.0.0.1\n"
	"                            Default for PORT is "
	) << DPORT << "\n\n";
}

static struct option opts[] = {
    {"help",       no_argument,       nullptr, 'h'},
    {"debug",      no_argument,       nullptr, 'd'},
    {"version",    no_argument,       nullptr, 'V'},
    {"port",       required_argument, nullptr, 'p'},
    {nullptr,      0,                 nullptr,  0 }
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
	    pp = nullptr;
	} else
	    pp = argcpy;
    }
    if (pp)
	*port = atoi(pp);
}

int fuse(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_cmdline_opts opts;
    struct fuse *fp;
    int err = -1, foreground;

    if (fuse_parse_cmdline(&args, &opts) != -1) {
        if (fuse_daemonize(opts.foreground) != -1) {
            fp = fuse_new(&args, &plp_oper, sizeof(plp_oper), NULL);
            if (fp != NULL && (fuse_mount(fp, opts.mountpoint)) != 0)
                err = fuse_loop(fp);
        }
        fuse_unmount(fp);
    }
    fuse_opt_free_args(&args);

    return err ? 1 : 0;
}

int main(int argc, char**argv) {
    ppsocket *skt, *skt2;
    const char *host = "127.0.0.1";
    int sockNum = DPORT, i, c, oldoptind = 1;

    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se != nullptr)
	sockNum = ntohs(se->s_port);

    /* N.B. Option handling is kludged. Most of the options are shared
       with FUSE, except for -p/--port, which has to be removed from
       argv so that FUSE doesn't see it. Hence, we don't complain
       about unknown options, but leave that to FUSE, and similarly we
       don't quit after issuing a version or help message. */
    opterr = 0; // Suppress errors from unknown options
    while ((c = getopt_long(argc, argv, "hVp:d", opts, NULL)) != -1) {
	switch (c) {
        case 'V':
            cout << _("plpfuse version ") << VERSION << endl;
            return 0;
        case 'h':
            help();
            return 0;
        case 'd':
            debug++;
            break;
        case 'p':
            parse_destination(optarg, &host, &sockNum);
            argc -= optind - oldoptind;
            for (i = oldoptind; i < argc; i++)
              argv[i] = argv[i + (optind - oldoptind)];
            break;
	}
        if (optind >= argc)
            break;
    }

    skt = new ppsocket();
    if (!skt->connect(host, sockNum)) {
        cerr << _("plpfuse: could not connect to ncpd") << endl;
	return 1;
    }
    skt2 = new ppsocket();
    if (!skt2->connect(host, sockNum)) {
        cerr << _("plpfuse: could not connect to ncpd") << endl;
        return 1;
    }

    rf = new rfsvfactory(skt);
    rp = new rpcsfactory(skt2);
    a = rf->create(true);
    r = rp->create(true);
    if (a != NULL && r != NULL)
        debuglog("plpfuse: connected");
    else
        debuglog("plpfuse: could not create rfsv or rpcs object, connect delayed");
    return fuse(argc, argv);
}
