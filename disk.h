/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2004 Andrea Mazzoleni
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __DISK_H
#define __DISK_H

/**
 * Sector size in bytes.
 */
#define SECTOR_SIZE 512

struct disk_geometry {
	unsigned sectors; /**< Track size in number of sector. */
	unsigned heads; /**< Cylinder size in number of tracks. */
	unsigned cylinders; /**< Number of cylinder. */
	unsigned size; /**< Total size in number of sectors. */
	unsigned start; /**< Number of sectors before the accessible space. */
	unsigned char drive; /**< BIOS drive number. 0x0 for first floppy, 0x80 for first harddisk. */
};

struct disk_handle {
#ifdef __WIN32__
	void* handle; /**< Handle. */
#else
	int handle; /**< Handle. */
#endif
	struct disk_geometry geometry; /**< Device geometry. */
	char device[512]; /**< Device name. */
};

unsigned le_uint16_read(const void* ptr);
unsigned le_uint32_read(const void* ptr);
void le_uint16_write(void* ptr, unsigned v);
void le_uint32_write(void* ptr, unsigned v);

struct disk_handle* disk_open(const char* dev);
int disk_close(struct disk_handle* h);
struct disk_handle* disk_find(void);
int disk_read(struct disk_handle* h, unsigned pos, void* data, unsigned size);
int disk_write(struct disk_handle* h, unsigned pos, const void* data, unsigned size);

#endif

