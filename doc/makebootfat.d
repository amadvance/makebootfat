Name
	makebootfat - create bootable FAT filesystems

Synopsis
	:makebootfat [options] IMAGE

Description
	This utility creates a bootable FAT filesystem and
	populates it with files and boot tools.

	It is mainly designed to create bootable USB and
	Fixed disk for the AdvanceCD project.

	The official site of AdvanceCD and makebootfat is:

		+http://advancemame.sourceforge.net/

Options
	-o, --output DEVICE
		Specify the output device. It must be the device 
		where you want to setup the filesystem.
		You can use the special "usb" value to automatically
		select the USB Mass Storage device connected at
		the system.
		This option is always required.

	-b, --boot FILE
	-1, --boot-fat12 FILE
	-2, --boot-fat16 FILE
	-3, --boot-fat32 FILE
		Specify the FAT boot sector images to use. The -b option
		uses the same sector for all the FAT types. The other
		options can be used to specify a different sector for
		different FAT types. The FAT types for which a boot sector
		is not specified are not used.
		This option is always required.

	-m, --mbr FILE
		Specify the MBR sector image to use.
		If this option is specified a partition table is
		created on the disk like a harddisk. Otherwise
		the disk is filled without a partition table like
		a floppy disk.

	-F, --mbrfat
		Change the MBR image specified with the -m option to pretend
		to be a FAT filesystem starting from the first sector of
		the disk. This allows booting from USB-FDD (Floppy Disk Drive)
		also using a partition table generally required by USB-HDD
		(Hard Disk Drive).
		The MBR image specified with the -m option must have
		executable code positioned like a FAT boot sector. You
		can use the included `mbrfat.bin' file.

	-c, --copy FILE
		Copy the specified file in the root directory of the disk.
		The file is copied using the readonly attribute.

	-x, --exclude FILE
		Exclude the specified files and subdirectories in the
		IMAGE directory to copy. The path must be specified using
		the same format used in the IMAGE directory specification.

	-X, --syslinux
		Enforce the syslinux FAT limitations. Syslinux doesn't
		support FAT32 at all, and FAT16 with 64 and 128 sectors
		per cluster formats.
		This option exclude all the FAT formats not supported
		by syslinux. Please note that it limits the maximum
		size of filesystem to 1 GB.
		Syslinux version 2.20 (and higher) supports all
		FAT types and doesn't require this option.

	-P, --partition
		Ensure to operate on a partition and not on a disk.

	-D, --disk
		Ensure to operate on a disk and not on a partition.

	-L, --label LABEL
		Set the FAT label. The label is a string of 11 chars.

	-O, --oem OEM
		Set the FAT OEM name. The OEM name is a string of 11 chars.

	-S, --serial SERIAL
		Set the FAT serial number. The serial number is a 32 bit
		unsigned integer.

	-E, --drive DRIVE
		Set the BIOS drive to setup in the FAT boot sector.
		Generally this value is ignored by boot sectors, with 
		the exception of the FAT12 and FAT16 FreeDOS boot sectors 
		that require the correct value or the value 255 to force
		auto detection.

	-v, --verbose
		Print some information on the device and on the filesystem 
		created.

	-i, --interactive
		Show the errors in a message box. Only for Windows.

	-h, --help
		Print a short help.

	-V, --version
		Print the version number.

	=IMAGE
		Directory image to copy on the disk. All the files
		and subdirectories present in this directory
		are copied on the disk.

Disks and Partitions Names
	In Linux disk devices are named /dev/hdX or /dev/sdX where X
	is a letter. Partition devices are named /dev/hdXN or /dev/sdXN
	where X is a letter and N a digit.

	In Windows disk devices are named \\.\PhysicalDriveN where N is
	a digit. Partition devices are named \\.\X: where X is a letter,
	but sometimes \\.\X: is a disk and not a partition, for example on 
	floppies and on all the USB Mass Storage devices without a
	partition table.

Syslinux
	To make a bootable FAT using syslinux you must use
	the -X option and copy in the root directory of the disk
	the files:

	ldlinux.sys - The syslinux loader.
	syslinux.cfg - The syslinux configuration file.
	linux - The Linux kernel image  (the file name may be different).
	initrd.img - The initrd filesystem (the file name may be different
		or missing).

	You must also specify the `ldlinux.bss' boot sector with the -b
	option and eventually the `mbr.bin' MBR sector with the -m option.
	Both the sector images are present in the syslinux package.

	For example:

		:makebootfat -o usb \
		:	-X \
		:	-b ldlinux.bss -m mbr.bin \
		:	-c ldlinux.sys -c syslinux.cfg \
		:	-c linux -c initrd.img \
		:	image

	To make a bootable FAT using syslinux 2.20 (and higher)
	you must use the syslinux included tools because the
	ldlinux.* files need a special customization not yet
	supported by makebootfat.

Loadlin and FreeDOS
	To make a bootable FAT using loadlin and FreeDOS you must copy
	in the root directory of the disk the files:

	kernel.sys - The FreeDOS kernel. Remember to use the "32" kernel
		version to support FAT32.
	command.com - The FreeDOS shell.
	autoexec.bat - Used to start loadlin.
	loadlin.exe - The loadlin executable.
	linux - The Linux kernel image  (the file name may be different).
	initrd.img - The initrd filesystem (the file name may be different
		or missing).

	You must also specify the FreeDOS boot sectors available on the
	FreeDOS `sys' source package with the -1, -2, -3 option.
	For the MBR you can use the sectors image available on the FreeDOS
	`fdisk' source package.

	For example:

		:makebootfat -o /dev/hda1 \
		:	-E 255 \
		:	-1 fat12com.bin -2 fat16com.bin -3 fat32lba.bin \
		:	-c kernel.sys -c command.com \
		:	-c autoexec.bat -c loadlin.exe \
		:	-c linux -c initrd.img \
		:	image

	For harddisk you can safely use the fat32lba.bin FAT32 boot
	sector. For usb device, it's better to use the fat32chs.bin
	FAT32 boot sector.

Multi Standard USB Booting
	The BIOS USB boot support is generally differentiated in two
	categories: USB-FDD and USB-HDD.

	The USB-FDD (Floppy Disk Drive) standard requires the presence of
	a filesystem starting from the first sector of the disk without
	a partition table.
	You can create this type of disk without using the -m option.

	The USB-HDD (Hard Disk Drive) standard requires the presence of
	a partition table in the first sector of the disk.
	You can create this type of disk using the -m option.

	Generally these standard are incompatible, but using the -m
	and -F options you can create a disk compatible with both the
	BIOS USB-FDD and USB-HDD standards.

	To use the -F option the MBR image specified must follow
	the constrains:

	* It must start with a standard FAT 3 bytes jump instruction.
	* It must have the bytes from address 3 to 89 (included) unused.

	And example of such image is in the `mbrfat.bin' file.

	For example to create a syslinux image:

		:makebootfat -o usb \
		:	-X \
		:	-b ldlinux.bss -m mbrfat.bin -F \
		:	-c ldlinux.sys -c syslinux.cfg \
		:	-c linux -c initrd.img \
		:	image

	and for a FreeDOS and loadlin image:

		:makebootfat -o usb \
		:	-E 255 \
		:	-1 fat12com.bin -2 fat16com.bin -3 fat32chs.bin \
		:	-m mbrfat.bin -F \
		:	-c kernel.sys -c command.com \
		:	-c autoexec.bat -c loadlin.exe \
		:	-c linux -c initrd.img \
		:	image

Exclusion
	To exclude some files or directories in the image copy, you
	can use the -x option using the same path specification
	which are you using for the image directory.

	For example, if you need to exclude the `isolinux' and
	`syslinux' subdirectories from the `image' directory
	you can use the command:

		:makebootfat ... \
		:	-x image/isolinux \
		:	-x image/syslinux \
		:	image

Copyright
	This file is Copyright (C) 2004 Andrea Mazzoleni

See Also
	syslinux(1), mkdosfs(1), dosfsck(1)

