/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#ifndef _D_COMMON_H_
#define _D_COMMON_H_

#ifdef __MINGW32__
# undef SIZE_MAX
# ifndef _OFF_T_
#  define _OFF_T_
typedef long long _off_t;
#  ifndef _NO_OLDNAMES
typedef _off_t off_t;
#  endif // !_NO_OLDNAMES
# endif // !_OFF_T_
#endif // __MINGW32__

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef __MINGW32__
#ifdef malloc
#       undef malloc
#endif
#ifdef realloc
#       undef realloc
#endif
#endif // __MINGW32__

#ifdef ENABLE_NLS
# include "gettext.h"
# define _(String) gettext(String)
#else // ENABLE_NLS
# define _(String) String
#endif

// use C99 limit macros
#define __STDC_LIMIT_MACROS
// included here for compatibility issues with old compiler/libraries.
#include <stdint.h>

#endif // _D_COMMON_H_