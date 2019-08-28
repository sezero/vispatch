/*
	compiler.h
	compiler specific definitions and settings

	Copyright (C) 2007-2011  O.Sezer <sezero@users.sourceforge.net>

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
#define __i386__		1
#endif

/* inline keyword: */
#if defined(_MSC_VER) && !defined(__cplusplus)
#define inline __inline
#endif	/* _MSC_VER */


#endif	/* __VP_COMPILER_H */

