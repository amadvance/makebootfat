/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2004, 2005 Andrea Mazzoleni
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

#include "portable.h"

#include "disk.h"
#include "error.h"

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HAVE_SCSI_SCSI_H
#include <scsi/scsi.h>
#endif

#if HAVE_MNTENT_H
#include <mntent.h>
#endif

#if HAVE_LINUX_HDREG_H
#include <linux/hdreg.h>
#endif

#if HAVE_LINUX_FD_H
#include <linux/fd.h>
#endif

/* Configuration */

#if defined(SCSI_IOCTL_PROBE_HOST) && defined(SCSI_IOCTL_GET_IDLUN) && defined(SCSI_IOCTL_GET_BUS_NUMBER)
#define USE_FIND
#endif

#if HAVE_SETMNTENT && HAVE_GETMNTENT && HAVE_ENDMNTENT
#define USE_CHECKMOUNT
#endif

/* Endian safe read/write */

unsigned le_uint16_read(const void* ptr)
{
	const unsigned char* ptr8 = (const unsigned char*)ptr;
	return (unsigned)ptr8[0] | (unsigned)ptr8[1] << 8;
}

unsigned le_uint32_read(const void* ptr)
{
	const unsigned char* ptr8 = (const unsigned char*)ptr;
	return (unsigned)ptr8[0] | (unsigned)ptr8[1] << 8 | (unsigned)ptr8[2] << 16 | (unsigned)ptr8[3] << 24;
}

void le_uint16_write(void* ptr, unsigned v)
{
	unsigned char* ptr8 = (unsigned char*)ptr;
	ptr8[0] = v & 0xFF;
	ptr8[1] = (v >> 8) & 0xFF;
}

void le_uint32_write(void* ptr, unsigned v)
{
	unsigned char* ptr8 = (unsigned char*)ptr;
	ptr8[0] = v & 0xFF;
	ptr8[1] = (v >> 8) & 0xFF;
	ptr8[2] = (v >> 16) & 0xFF;
	ptr8[3] = (v >> 24) & 0xFF;
}

/* Disk */

#ifdef __WIN32__

#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <setupapi.h>

/* Declaration of IOCTL_DISK_GET_PARTITION_INFO_EX */
#ifndef IOCTL_DISK_GET_PARTITION_INFO_EX

#define IOCTL_DISK_GET_PARTITION_INFO_EX CTL_CODE(IOCTL_DISK_BASE, 0x0012, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _PARTITION_INFORMATION_GPT {
	GUID PartitionType;
	GUID PartitionId;
	DWORD64 Attributes;
	WCHAR Name[36];
} PARTITION_INFORMATION_GPT;

typedef enum _PARTITION_STYLE {
	PARTITION_STYLE_MBR,
	PARTITION_STYLE_GPT,
	PARTITION_STYLE_RAW
} PARTITION_STYLE;

typedef struct _PARTITION_INFORMATION_MBR {
	BYTE PartitionType;
	BOOLEAN BootIndicator;
	BOOLEAN RecognizedPartition;
	DWORD HiddenSectors;
} PARTITION_INFORMATION_MBR;

typedef struct _PARTITION_INFORMATION_EX {
	PARTITION_STYLE PartitionStyle;
	LARGE_INTEGER StartingOffset;
	LARGE_INTEGER PartitionLength;
	DWORD PartitionNumber;
	BOOLEAN RewritePartition;
	union {
		PARTITION_INFORMATION_MBR Mbr;
		PARTITION_INFORMATION_GPT Gpt;
	};
} PARTITION_INFORMATION_EX;
#endif

/* Declaration of IOCTL_DISK_GET_DRIVE_GEOMETRY_EX */
#ifndef IOCTL_DISK_GET_DRIVE_GEOMETRY_EX

#define IOCTL_DISK_GET_DRIVE_GEOMETRY_EX CTL_CODE(IOCTL_DISK_BASE, 0x0028, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _DISK_GEOMETRY_EX {
	DISK_GEOMETRY Geometry;
	LARGE_INTEGER DiskSize;
	BYTE Data[1];
} DISK_GEOMETRY_EX;

#endif

/* Declaration of setupapi */
#ifndef SPDRP_ENUMERATOR_NAME
#define SPDRP_ENUMERATOR_NAME 0x00000016
#endif

static GUID GUID_INTERFACE_DISK = { 0x53f56307L, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b } };

/**
 * Get a device property.
 */
static BOOL SetupDiGetProperty(HDEVINFO dev, SP_DEVINFO_DATA* data, DWORD prop, unsigned char** buf, DWORD* buf_size)
{
	DWORD PropertyRegDataType;
	DWORD RequiredSize;

	if (!SetupDiGetDeviceRegistryProperty(dev, data, prop, &PropertyRegDataType, 0, 0, &RequiredSize)) {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			return FALSE; 
	}

	*buf_size = RequiredSize;
	*buf = (unsigned char*)malloc(RequiredSize);
	if (!buf)
		return FALSE;

	if (!SetupDiGetDeviceRegistryProperty(dev, data, prop, &PropertyRegDataType, *buf, RequiredSize, 0))
		return FALSE;

	return TRUE;
}

/**
 * Change the state of a device.
 */
static BOOL SetupDiChangeDeviceState(HDEVINFO dev, SP_DEVINFO_DATA* data, DWORD state)
{
	SP_PROPCHANGE_PARAMS params;

	memset(&params, 0, sizeof(SP_PROPCHANGE_PARAMS));

	params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
	params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
	params.StateChange = state;
	params.Scope = DICS_FLAG_CONFIGSPECIFIC;
	params.HwProfile = 0;

	if (!SetupDiSetClassInstallParams(dev, data, (PSP_CLASSINSTALL_HEADER)&params, sizeof(SP_PROPCHANGE_PARAMS))) {
		return FALSE;
	}

	if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, dev, data)) {
		return FALSE;
	}

	return TRUE;
}

/**
 * Restart a device.
 */
static BOOL SetupDiRestartDevice(HDEVINFO dev, SP_DEVINFO_DATA* data)
{
	if (!SetupDiChangeDeviceState(dev, data, DICS_PROPCHANGE))
		return FALSE;

	return TRUE;
}

/**
 * Device enumeration callback.
 */
typedef BOOL SetupDiForEachCallBack(void* void_context, HDEVINFO h, SP_DEVINFO_DATA* data, SP_DEVICE_INTERFACE_DATA* idata, SP_DEVICE_INTERFACE_DETAIL_DATA* ddata, const char* enumerator);

/**
 * Device enumerator for all the disk devices.
 */
static BOOL SetupDiForEach(SetupDiForEachCallBack* callback, void* context)
{
	unsigned index;
	HDEVINFO h;
	GUID guid = GUID_INTERFACE_DISK;

	h = SetupDiGetClassDevs(&guid, 0, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (h == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	index = 0;
	while (1) {
		unsigned char* prop;
		DWORD size;
		SP_DEVINFO_DATA data;
		SP_DEVICE_INTERFACE_DATA idata;
		SP_DEVICE_INTERFACE_DETAIL_DATA* ddata;

		idata.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		if (!SetupDiEnumDeviceInterfaces(h, 0, &guid, index, &idata)) {
			if (GetLastError() != ERROR_NO_MORE_ITEMS)
				return FALSE;
			break;
		}

		if (!SetupDiGetDeviceInterfaceDetail(h, &idata, 0, 0, &size, 0)) {
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
				return FALSE;
		}

		ddata = malloc(size);
		if (!ddata)
			return FALSE;

		ddata->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		data.cbSize = sizeof(SP_DEVINFO_DATA);
		if (!SetupDiGetDeviceInterfaceDetail(h, &idata, ddata, size, 0, &data)) {
			return FALSE;
		}

		if (SetupDiGetProperty(h, &data, SPDRP_ENUMERATOR_NAME, &prop, &size)) {
			if (!callback(context, h, &data, &idata, ddata, prop))
				return FALSE;
			free(prop);
		}

		free(ddata);

		++index;
	}

	SetupDiDestroyDeviceInfoList(h);

	return TRUE;
}

/**
 * Callback for restarting the specified USB device.
 */
static BOOL CallBackUSBReconnect(void* void_context, HDEVINFO h, SP_DEVINFO_DATA* data, SP_DEVICE_INTERFACE_DATA* idata, SP_DEVICE_INTERFACE_DETAIL_DATA* ddata, const char* enumerator)
{
	if (stricmp(enumerator, "USBSTOR") == 0) {
		if (strcmp(void_context, ddata->DevicePath) == 0) {
			if (!SetupDiRestartDevice(h, data))
				return FALSE;
		}
	}

	return TRUE;
}

struct find_context {
	char device[512];
	int count;
};

/**
 * Callback for finding a USB device.
 */
static BOOL CallBackUSBFind(void* void_context, HDEVINFO h, SP_DEVINFO_DATA* data, SP_DEVICE_INTERFACE_DATA* idata, SP_DEVICE_INTERFACE_DETAIL_DATA* ddata, const char* enumerator)
{
	struct find_context* context = (struct find_context*)void_context;

	if (stricmp(enumerator, "USBSTOR") == 0) {
		++context->count;

		memset(context->device, 0, sizeof(context->device));
		strncpy(context->device, ddata->DevicePath, sizeof(context->device) - 1);
	}

	return TRUE;
}

struct disk_handle* disk_find(void)
{
	struct find_context context;

	context.count = 0;

	if ((GetVersion() & 0x80000000) != 0) {
		error_set("This program run only in Windows 2000/XP.");
		return 0;
	}

	if (!SetupDiForEach(CallBackUSBFind, &context)) {
		error_set("Error searching for usb disks.");
		return 0;
	}

	if (context.count > 1) {
		error_set("Please insert ONLY ONE usb disk.");
		return 0;
	}

	if (context.count == 0) {
		error_set("Please insert one usb disk.");
		return 0;
	}

	return disk_open(context.device);
}

struct disk_handle* disk_open(const char* dev)
{
	DISK_GEOMETRY dg;
	DISK_GEOMETRY_EX dge;
	PARTITION_INFORMATION pi;
	PARTITION_INFORMATION_EX pie;
	DWORD size;
	struct disk_handle* h;

	if ((GetVersion() & 0x80000000) != 0) {
		error_set("This program run only in Windows 2000/XP.");
		return 0;
	}

	h = malloc(sizeof(struct disk_handle));
	if (!h) {
		error_set("Low memory.");
		return 0;
	}

	memset(h->device, 0, sizeof(h->device));
	strncpy(h->device, dev, sizeof(h->device) - 1);

	/* restart the usb device to flush the cache */
	/* this is required because the LOCK doesn't work on disks but only on volumes */
	if (!SetupDiForEach(CallBackUSBReconnect, h->device)) {
		error_set("Error %d restarting the device %s.", GetLastError(), h->device);
		return 0;
	}

	h->handle = CreateFile(h->device, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, 0);
	if (h->handle == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_ACCESS_DENIED) {
			error_set("You must be Administrator to access the device %s.", h->device);
			return 0;
		} else {
			error_set("Error %d accessing the device %s.", GetLastError(), h->device);
			return 0;
		}
	}

	if (!DeviceIoControl(h->handle, FSCTL_LOCK_VOLUME , 0, 0, 0, 0, &size, 0)) {
		if (GetLastError() == ERROR_INVALID_FUNCTION) {
			/* the LOCK function is supported only on volumes and not on disks */
			/* ignore the error */
		} else if (GetLastError() == ERROR_ACCESS_DENIED) {
			error_set("The device %s is busy. Close all the applications and retry.", h->device);
			CloseHandle(h->handle);
			return 0;
		} else {
			error_set("Error %d locking the volume %s.", GetLastError(), h->device);
			CloseHandle(h->handle);
			return 0;
		}
	}

	/* get the geometry */
	if (!DeviceIoControl(h->handle, IOCTL_DISK_GET_DRIVE_GEOMETRY, 0, 0, &dg, sizeof(dg), &size, 0)) {
		if (GetLastError() == ERROR_INVALID_FUNCTION) {
			/* IOCTL_DISK_GET_DRIVE_GEOMETRY is supported only on disks and not on volumes */
			if (!DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, 0, 0, &dge, sizeof(dge), &size, 0)) {
				if (GetLastError() == ERROR_INVALID_FUNCTION) {
					/* IOCTL_DISK_GET_DRIVE_GEOMETRY_EX is supported only in Windows XP */
					error_set("You need Windows XP to format this partition.");
					CloseHandle(h->handle);
					return 0;
				} else {
					error_set("Error %d getting the device geometry.", GetLastError());
					CloseHandle(h->handle);
					return 0;
				}
			}
			dg = dge.Geometry;
		} else {
			error_set("Error %d getting the device geometry.", GetLastError());
			CloseHandle(h->handle);
			return 0;
		}
	}

	/* get the size */
	if (!DeviceIoControl(h->handle, IOCTL_DISK_GET_PARTITION_INFO, 0, 0, &pi, sizeof(pi), &size, 0)) {
		if (GetLastError() == ERROR_INVALID_FUNCTION) {
			if (!DeviceIoControl(h, IOCTL_DISK_GET_PARTITION_INFO_EX, 0, 0, &pie, sizeof(pie), &size, 0)) {
				if (GetLastError() == ERROR_INVALID_FUNCTION) {
					error_set("You need Windows XP to format this partition.");
					CloseHandle(h->handle);
					return 0;
				} else {
					error_set("Error %d getting the device size.", GetLastError());
					CloseHandle(h->handle);
					return 0;
				}
			}
			pi.StartingOffset = pie.StartingOffset;
			pi.PartitionLength = pie.PartitionLength;
		} else {
			error_set("Error %d getting the device size.", GetLastError());
			CloseHandle(h->handle);
			return 0;
		}
	}

	if (pi.PartitionLength.QuadPart == 0 || (pi.PartitionLength.QuadPart % SECTOR_SIZE) != 0) {
		error_set("Invalid device size.");
		CloseHandle(h->handle);
		return 0;
	}

	h->geometry.size = pi.PartitionLength.QuadPart / SECTOR_SIZE;
	h->geometry.start = pi.StartingOffset.QuadPart / SECTOR_SIZE;
	h->geometry.sectors = dg.SectorsPerTrack;
	h->geometry.heads = dg.TracksPerCylinder;
	h->geometry.cylinders = dg.Cylinders.QuadPart;
	if (dg.MediaType == FixedMedia || dg.MediaType == RemovableMedia) {
		h->geometry.drive = 0x80;
	} else {
		h->geometry.drive = 0x0;
	}

	return h;
}

int disk_close(struct disk_handle* h)
{
	BOOL r;
	DWORD size;

	/* signal the change of filesystem */
	if (!DeviceIoControl(h->handle, FSCTL_DISMOUNT_VOLUME , 0, 0, 0, 0, &size, 0)) {
		/* the DISMOUNT function is supported only on volumes and not on disks */
		/* ignore error */
	}

	/* unlock the volume */
	if (!DeviceIoControl(h->handle, FSCTL_UNLOCK_VOLUME , 0, 0, 0, 0, &size, 0)) {
		/* the UNLOCK function is supported only on volumes and not on disks */
		/* ignore error */
	}

	r = CloseHandle(h->handle);
	if (!r) {
		error_set("Error %d closing the device %s.", GetLastError(), h->device);
		return -1;
	}

	/* restart the usb device to commit all the changes and to reload the filesystem */
	/* this is required because the LOCK doesn't work on disks but only on volumes */
	if (!SetupDiForEach(CallBackUSBReconnect, h->device)) {
		error_set("Error %d restarting the device %s.", GetLastError(), h->device);
		return -1;
	}

	/* do it two times, in some Windows XP it seems required to ensure a correct device restart */
	if (!SetupDiForEach(CallBackUSBReconnect, h->device)) {
		error_set("Error %d restarting the device %s.", GetLastError(), h->device);
		return -1;
	}

	free(h);

	return 0;
}

int disk_read(struct disk_handle* h, unsigned pos, void* data, unsigned size)
{
	LARGE_INTEGER off;
	DWORD info;
	ULONG lp;
	LONG hp;

	off.QuadPart = SECTOR_SIZE * (LONGLONG)pos;
	lp = off.LowPart;
	hp = off.HighPart;
	lp = SetFilePointer(h->handle, lp, &hp, FILE_BEGIN);
	if (off.LowPart != lp || off.HighPart != hp) {
		error_set("Error %d reading the device.", GetLastError());
		return -1;
	}

	if (!ReadFile(h->handle, data, size * SECTOR_SIZE, &info, 0)) {
		error_set("Error %d reading the device.", GetLastError());
		return -1;
	}
	if (info != size * SECTOR_SIZE) {
		error_set("Error %d reading the device.", GetLastError());
		return -1;
	}

	return 0;
}

int disk_write(struct disk_handle* h, unsigned pos, const void* data, unsigned size)
{
	LARGE_INTEGER off;
	DWORD info;
	ULONG lp;
	LONG hp;

	off.QuadPart = SECTOR_SIZE * (LONGLONG)pos;
	lp = off.LowPart;
	hp = off.HighPart;
	lp = SetFilePointer(h->handle, lp, &hp, FILE_BEGIN);
	if (off.LowPart != lp || off.HighPart != hp) {
		error_set("Error %d writing the device.", GetLastError());
		return -1;
	}

	if (!WriteFile(h->handle, data, size * SECTOR_SIZE, &info, 0)) {
		error_set("Error %d writing the device.", GetLastError());
		return -1;
	}
	if (info != size * SECTOR_SIZE) {
		error_set("Error %d writing the device.", GetLastError());
		return -1;
	}

	return 0;
}

#else

#ifdef USE_FIND
/**
 * Check if the SCSI device is a USB Mass Storage and if it's present.
 */
static int disk_usbmassstorage(const char* dev, int* host, int* channel, int* id, int* lun)
{
	int r, h;
	char hostname[64];
	int idlun[2];
	int bus_number;
	char buf[SECTOR_SIZE];

	h = open(dev, O_RDONLY | O_NONBLOCK);
	if (h < 0) {
		return -1;
	}

	*(int*)hostname = 63;
	r = ioctl(h, SCSI_IOCTL_PROBE_HOST, hostname);
	if (r < 0) {
		close(h);
		return -1;
	}

	/* exclude not USB mass storage device */
	if (strstr(hostname, "USB Mass Storage") == 0) {
		close(h);
		return -1;
	}

	r = ioctl(h, SCSI_IOCTL_GET_IDLUN, &idlun);
	if (r < 0) {
		close(h);
		return -1;
	}

	*channel = (idlun[0] >> 16) & 0xff;
	*lun = (idlun[0] >> 8 ) & 0xff;
	*id = idlun[0] & 0xff;

	/* exclude secondary devices */
	if (*lun != 0) {
		close(h);
		return -1;
	}

	r = ioctl(h, SCSI_IOCTL_GET_BUS_NUMBER, &bus_number);
	if (r < 0) {
		close(h);
		return -1;
	}

	*host = bus_number;

	/* try a read, this exclude removed device on the 2.4 Linux kernel */
	r = read(h, buf, SECTOR_SIZE);
	if (r != SECTOR_SIZE) {
		close(h);
		return -1;
	}

	return 0;
}
#endif

struct disk_handle* disk_find(void)
{
#ifdef USE_FIND
	int i;
	char device[64];
	int count;
	int eaccess;

	eaccess = 0;
	count = 0;
	for(i=0;i<16;++i) {
		int r;
		char buf[64];
		int host;
		int channel;
		int id;
		int lun;

		sprintf(buf, "/dev/sd%c", (char)i + 'a');

		r = disk_usbmassstorage(buf, &host, &channel, &id, &lun);
		if (r != 0) {
			if (errno == EACCES)
				eaccess = 1;
			continue;
		}

		++count;
		strcpy(device, buf);
	}

	if (count > 1) {
		error_set("Please insert ONLY ONE usb disk.");
		return 0;
	}

	if (count == 0) {
		if (eaccess) {
			error_set("Please insert one usb disk and ensure to be root.");
		} else {
			error_set("Please insert one usb disk.");
		}
		return 0;
	}

	return disk_open(device);
#else
	error_set("USB disk detection not supported in this system.");
	return 0;
#endif
}

#ifdef USE_CHECKMOUNT
/**
 * Check that the device is not mounted.
 */
int disk_checkmount(const char* dev)
{
	FILE* f;
	struct mntent* m;

	f = setmntent("/etc/mtab", "r");
	if (!f) {
		error_set("Error accessing /etc/mtab. %s.", strerror(errno));
		return -1;
	}

	while ((m = getmntent(f)) != 0) {
		if (strncmp(m->mnt_fsname, dev, strlen(dev)) == 0) {
			error_set("You cannot operate on device %s because is mounted as %s.", m->mnt_fsname, m->mnt_dir);
			return -1;
		}
	}

	endmntent(f);

	return 0;
}
#endif

struct disk_handle* disk_open(const char* dev)
{
	off_t o;
	struct hd_geometry hg;
	struct floppy_struct fg;
	struct disk_handle* h = malloc(sizeof(struct disk_handle));
	if (!h) {
		error_set("Low memory.");
		return 0;
	}

	memset(h->device, 0, sizeof(h->device));
	strncpy(h->device, dev, sizeof(h->device) - 1);

#ifdef USE_CHECKMOUNT
	if (disk_checkmount(h->device) != 0) {
		return 0;
	}
#endif

	h->handle = open(h->device, O_RDWR);
	if (h->handle == -1) {
		error_set("Error opening the device %s. %s.", h->device, strerror(errno));
		return 0;
	}

	o = lseek(h->handle, 0, SEEK_END);
	if (o == -1) {
		close(h->handle);
		error_set("Error seeking the device %s. %s.", h->device, strerror(errno));
		return 0;
	}

	if (o == 0 || (o % SECTOR_SIZE) != 0) {
		close(h->handle);
		error_set("Invalid device size.");
		return 0;
	}

	h->geometry.size = o / SECTOR_SIZE;

	memset(&hg, 0, sizeof(hg));
	memset(&fg, 0, sizeof(fg));

	if (ioctl(h->handle, HDIO_GETGEO, &hg) >= 0) {
		h->geometry.start = hg.start;
		h->geometry.sectors = hg.sectors;
		h->geometry.heads = hg.heads;
		h->geometry.cylinders = hg.cylinders;
		h->geometry.drive = 0x80;
	} else if (ioctl(h->handle, FDGETPRM, &fg) >= 0) {
		h->geometry.start = 0;
		h->geometry.sectors = fg.sect;
		h->geometry.heads = fg.head;
		h->geometry.cylinders = fg.track;
		h->geometry.drive = 0x0;
	} else {
		close(h->handle);
		error_set("Error getting the geometry of the device %s.", h->device);
		return 0;
	}

	return h;
}

int disk_close(struct disk_handle* h)
{
	int r;

	r = close(h->handle);
	if (r == -1) {
		error_set("Error closing the device. %s.", strerror(errno));
		return -1;
	}

	/* commit all the changes */
	sync();

	free(h);

	return 0;
}

int disk_read(struct disk_handle* h, unsigned pos, void* data, unsigned size)
{
	off_t o;
	ssize_t s;

	o = SECTOR_SIZE * (off_t)pos;

	s = pread(h->handle, data, size * SECTOR_SIZE, o);

	if (s != size * SECTOR_SIZE) {
		error_set("Error reading the device. %s.", strerror(errno));
		return -1;
	}

	return 0;
}

int disk_write(struct disk_handle* h, unsigned pos, const void* data, unsigned size)
{
	off_t o;
	ssize_t s;

	o = SECTOR_SIZE * (off_t)pos;

	s = pwrite(h->handle, data, size * SECTOR_SIZE, o);

	if (s != size * SECTOR_SIZE) {
		error_set("Error writing the device. %s.", strerror(errno));
		return -1;
	}

	return 0;
}

#endif

