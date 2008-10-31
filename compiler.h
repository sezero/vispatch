/*
	compiler.h
	compiler specific definitions and settings

	$Id: compiler.h,v 1.3 2008-10-31 16:40:52 sezero Exp $

	Copyright (C) 2007  O.Sezer <sezero@users.sourceforge.net>

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		51 Franklin St, Fifth Floor,
		Boston, MA  02110-1301  USA
*/

#ifndef __VP_COMPILER_H
#define __VP_COMPILER_H

#if !defined(__GNUC__)
#define	__attribute__(x)
#endif	/* __GNUC__ */

/* Some compilers, such as OpenWatcom, and possibly other compilers
 * from the DOS universe, define __386__ instead of __i386__
 */
#if defined(__386__) && !defined(__i386__)
#define __i386__	1
#endif

/* Provide a substitute for offsetof() if we don't have one.
 * This variant works on most (but not *all*) systems...
 */
#ifndef offsetof
#define offsetof(t,m) ((size_t)&(((t *)0)->m))
#endif


#endif	/* __VP_COMPILER_H */

