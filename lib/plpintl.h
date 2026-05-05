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
#pragma once

#include "config.h"

/* libintl.h includes locale.h only if optimized.
 * however, we need LC_ALL ...
 */
#include <locale.h>

#if defined(ENABLE_NLS)
#  include <libintl.h>
#  define _(String) gettext (String)
#  define N_(String) (String)
#else
#  define _(String) (String)
#  define N_(String) String
#  define textdomain(Domain)
#  define bindtextdomain(Package, Directory)
#endif
