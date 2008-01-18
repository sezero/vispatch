/*
 * VisPatch :  Quake level patcher for water visibility.
 *
 * Copyright (C) 1997-2006  Andy Bay <IMarvinTPA@bigfoot.com>
 * Copyright (C) 2006-2007  O. Sezer <sezero@users.sourceforge.net>
 *
 * $Id: vispatch.h,v 1.2 2008-01-18 09:57:01 sezero Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to:
 *
 * Free Software Foundation, Inc.
 * 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
*/

#ifndef __QVISPATCH_H
#define __QVISPATCH_H

/* Version numbers */
#define	VP_VER_MAJ	1
#define	VP_VER_MID	4
#define	VP_VER_MIN	3

/* NOTE: We actually need int32_t, not int, as the types
   for sizes and offsets. If someone ever wants to compile
   this on a 16 bit system like DOS, expect trouble if
   the compiler is not djgpp.
 */

// on-disk bsp file structures

typedef struct			// A Directory entry
{	int	offset;		// Offset to entry, in bytes, from start of file
	int	size;		// Size of entry in file, in bytes
} dentry_t;

typedef struct			// The BSP file header
{	int	version;	// Model version, must be 0x17 (23).
	dentry_t entities;	// List of Entities.
	dentry_t planes;	// Map Planes.
				// numplanes = size/sizeof(plane_t)
	dentry_t miptex;	// Wall Textures.
	dentry_t vertices;	// Map Vertices.
				// numvertices = size/sizeof(vertex_t)
	dentry_t visilist;	// Leaves Visibility lists.
	dentry_t nodes;		// BSP Nodes.
				// numnodes = size/sizeof(node_t)
	dentry_t texinfo;	// Texture Info for faces.
				// numtexinfo = size/sizeof(texinfo_t)
	dentry_t faces;		// Faces of each surface.
				// numfaces = size/sizeof(face_t)
	dentry_t lightmaps;	// Wall Light Maps.
	dentry_t clipnodes;	// clip nodes, for Models.
				// numclips = size/sizeof(clipnode_t)
	dentry_t leaves;	// BSP Leaves.
				// numlaves = size/sizeof(leaf_t)
	dentry_t lface;		// List of Faces.
	dentry_t edges;		// Edges of faces.
				// numedges = Size/sizeof(edge_t)
	dentry_t ledges;	// List of Edges.
	dentry_t models;	// List of Models.
				// nummodels = Size/sizeof(model_t)
} dheader_t;


// on-disk pak file structures

#define	MAX_FILES_IN_PACK	2048

typedef struct
{	char magic[4];		// Pak Name of the new WAD format
	int	diroffset;	// Position of WAD directory from start of file
	int	dirsize;	// Number of entries * 0x40 (64 char)
} pakheader_t;

typedef struct
{	char filename[56];	// Name of the file, Unix style, with extension,
				// 50 chars, padded with '\0'.
	int	offset;		// Position of the entry in PACK file
	int	size;		// Size of the entry in PACK file
} pakentry_t;


// on-disk vis data structure:  stored in little endian format

#define VISPATCH_IDLEN		32

typedef struct
{
	char File[VISPATCH_IDLEN];
	int	len;
	int	vislen;
	unsigned char	*visdata;
	int	leaflen;
	unsigned char	*leafdata;
} visdat_t;


#endif	/* __QVISPATCH_H */

