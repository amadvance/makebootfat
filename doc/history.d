Name
	makebootfat - History For makebootfat

makebootfat Version 1.6 2024/10
	) Use /proc/mounts instead of /etc/mtab
	) Updated wth new autoconf.

makebootfat Version 1.5 2012/12
	) Added a new -G option to recompute the geometry values to standard
		values overriding the system reported values.
	) Minor fixes at the documentation.
	) Now in git repository.

makebootfat Version 1.4 2005/03
	) Fixed a memory leak.

makebootfat Version 1.3 2005/02
	) Minor fixes at the documentation.

makebootfat Version 1.2 2005/02
	) Added support for syslinux 3.xx with the new
		-Y option. This removes the limitation of 1 GB
		on FAT filesystems.
	) The Linux version now suggests to switch to root if
		searching for usb devices results in "Access Denied"
		errors.
	) Added a new -Z option to force ZIP-Disk compatibility.
	) Added a minimal write cache to speedup the writing process.

makebootfat Version 1.1 2004/12
	) The mbrfat sector now prints the type and geometry of
		the drive.
	) Minor fixes at the documentation.
	) Fixed a minor problem in the Windows SetupDiGetProperty()
		function.
	) Removed the 4GB limit creating FAT32 partition in 32 bit
		Linux systems.

makebootfat Version 1.0 2004/06
	) First version.

