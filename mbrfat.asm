; -----------------------------------------------------------------------
;   
;   Copyright 2003-2004 H. Peter Anvin - All Rights Reserved
;   Copyright 2004 Andrea Mazzoleni
;
;   Permission is hereby granted, free of charge, to any person
;   obtaining a copy of this software and associated documentation
;   files (the "Software"), to deal in the Software without
;   restriction, including without limitation the rights to use,
;   copy, modify, merge, publish, distribute, sublicense, and/or
;   sell copies of the Software, and to permit persons to whom
;   the Software is furnished to do so, subject to the following
;   conditions:
;   
;   The above copyright notice and this permission notice shall
;   be included in all copies or substantial portions of the Software.
;   
;   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
;   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
;   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
;   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
;   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
;   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
;   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
;   OTHER DEALINGS IN THE SOFTWARE.
;
; -----------------------------------------------------------------------

;
; mbr.asm
;
; Simple Master Boot Record
; 
; The MBR lives in front of the boot sector, and is responsible for
; loading the boot sector of the active partition.
; 
; This MBR should be "8086-clean", i.e. not require a 386.
;

		absolute 0462h
BIOS_page	resb 1			; Current video page

;
; Note: The MBR is actually loaded at 0:7C00h, but we quickly move it down to
; 0600h.
;
		section .text
		cpu 8086
		org 0600h

;
; Standard FAT jump signature
;
		jmp short _start
		nop

%macro	zb	1.nolist
	times %1 db 0
%endmacro

;
; Allocate space for the FAT12/16/32 boot sector
;
FATBootSector	zb 87

;
; Real MBR code
;
_start:		cli
		xor ax,ax
		mov ds,ax
		mov es,ax
		mov ss,ax
		mov sp,7C00h
		sti
		cld
		mov si,sp		; Start address
		mov di,0600h		; Destination address
		mov cx,512/2
		rep movsw

;
; Now, jump to the copy at 0600h so we can load the boot sector at 7C00h.
; Since some BIOSes seem to think 0000:7C00h and 07C0:0000h are the same
; thing, use a far jump to canonicalize the address.  This also makes
; sure that it is a code speculation barrier.
;
		jmp 0:next		; Jump to copy at 0600h
next:
		mov [DriveNo], dl

;
; Print FDD or HDD
;
		mov si, hdd_msg
		mov dl, [DriveNo]
		and dl, dl
		js .harddisk
		mov si, fdd_msg
.harddisk
		call print_msg

;
; Print Cylinders/Heads/Sectors
;
		mov dl, [DriveNo]
		mov ah,08h

		int 13h
		jc .no_driveparm
		and ah,ah
		jnz .no_driveparm

		mov bx,cx
		and bx,3fh
		push bx

		mov dl, dh
		xor dh, dh
		inc dx
		push dx

		mov bl,ch
		rol cl,1
		rol cl,1
		and cl,3h
		mov bh,cl
		push bx

		pop si
		call print_num

		mov si,sep_msg
		call print_msg

		pop si
		call print_num

		mov si,sep_msg
		call print_msg

		pop si
		call print_num

.no_driveparm:
		mov si, crlf_msg
		call print_msg

;
; Load the boot sector using CHS.
;
		; The boot sector of disks created with makebootfat
		; is always at 0/0/2 (Cylinder/Head/Sector)
		mov cx, 2			; Sector 2, Cylinder 0
		mov dh, 0			; Head 0
		mov dl, [DriveNo]		; dl Drive Number
		mov ax,ds
		mov es,ax
		mov bx,7C00h
		mov ax,0201h			; Read one sector

		int 13h
		jc disk_error

;
; Verify that we have a boot sector, jump
;
		cmp word [7C00h+510],0AA55h
		jne missing_os

;
; Run it
;
		; DS:SI -> active partition table entry
		mov si, PartitionTable
		mov cx, 4
.partsearch:
		test byte [si],80h
		jnz .partfound
		add si,byte 16
		loop .partsearch
		jmp missing_part
.partfound:

		; DL -> Drive Number
		mov dl, [DriveNo]

		cli
		jmp 0:7C00h			; Jump to boot sector; far
						; jump is speculation barrier
						; (Probably not neecessary, but
						; there is plenty of space.)

print_msg:
		lodsb
		and al,al
		jz .print_end
		mov ah,0Eh
		mov bh,[BIOS_page]
		mov bl,07h
		int 10h
		jmp short print_msg
.print_end:
		ret

print_num:
		mov ax,si
		mov bx,10
		xor cx,cx

.print_num_loop:
		xor dx,dx
		div bx

		add dl,'0'
		push dx
		inc cx

		and ax, ax
		jnz .print_num_loop

.print_num_pop:
		pop ax

		mov ah,0Eh
		mov bh,[BIOS_page]
		mov bl,07h
		int 10h

		dec cx
		jnz .print_num_pop

		ret

missing_os:
		mov si,missing_os_msg
		jmp short print_and_die

missing_part:
		mov si,missing_part_msg
		jmp short print_and_die

disk_error:
		mov si,bad_disk_msg
		jmp short print_and_die

print_and_die:
		call print_msg
die:
		jmp short die

		align 4, db 0			; Begin data area

; Data
DriveNo:	db 0

; Messages
missing_part_msg db 'No partition', 13, 10, 0
missing_os_msg	db 'No operating system', 13, 10, 0
bad_disk_msg	db 'Disk error'
crlf_msg	db 13, 10, 0
fdd_msg		db 'FDD ', 0
hdd_msg		db 'HDD ', 0
sep_msg		db '/', 0

;
; Maximum MBR size: 446 bytes; end-of-boot-sector signature also needed.
; Note that some operating systems (NT, DR-DOS) put additional stuff at
; the end of the MBR, so shorter is better.  Location 440 is known to
; have a 4-byte attempt-at-unique-ID for some OSes.
;

PartitionTable	equ $$+446			; Start of partition table

