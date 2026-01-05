/*
 * This file is part of plptools.
 *
 *  Copyright (C) 2007-2024 Reuben Thomas <rrt@sc3d.org>
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
#ifndef _plpfuse_h_
#define _plpfuse_h_

#include <fuse3/fuse.h>

typedef struct p_inode {
	int inode;
	char *name;
	struct p_inode *nextnam, *nextnum;
} p_inode;

/**
 * Description of a Psion-Device
 */
typedef struct p_device {
  char *name;  /* Volume-Name */
  char letter; /* Drive-Letter */
  long attrib; /* Device-Attribs */
  long total;  /* Total capacity in bytes */
  long free;   /* Free space in bytes */
  struct p_device *next;
} device;

/*
 * Description of a Psion-File/Dir
 */
typedef struct p_dentry
{
  char *name;
  long time;
  long attr;
  long size;
  long links;
  struct p_dentry *next;
} dentry;

extern int debug;

extern void debuglog(const char *fmt, ...);

#define BLOCKSIZE      512
#define FID            7 /* File system id */

#endif

extern struct fuse_operations plp_oper;
