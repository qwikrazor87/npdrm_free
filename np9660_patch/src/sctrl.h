/*
 *  Copyright (C) 2014-2018 qwikrazor87
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SCTRL_H
#define _SCTRL_H

#include <pspkernel.h>

typedef struct SceModule2
{
	struct SceModule2*   next; //0, 0x00
	u16             attribute; //4, 0x04
	u8              version[2]; //6, 0x06
	char            modname[27]; //8, 0x08
	char            terminal; //35, 0x23
	u16             status; //36, 0x24 (Used in modulemgr for stage identification)
	u16             padding; //38, 0x26
	u32             unk_28; //40, 0x28
	SceUID          modid; //44, 0x2C
	SceUID          usermod_thid; //48, 0x30
	SceUID          memid; //52, 0x34
	SceUID          mpidtext; //56, 0x38
	SceUID          mpiddata; //60, 0x3C
	void *          ent_top; //64, 0x40
	u32             ent_size; //68, 0x44
	void *          stub_top; //72, 0x48
	u32             stub_size; //76, 0x4C
	int             (* module_start)(SceSize, void *); //80, 0x50
	int             (* module_stop)(SceSize, void *); //84, 0x54
	int             (* module_bootstart)(SceSize, void *); //88, 0x58
	int             (* module_reboot_before)(void *); //92, 0x5C
	int             (* module_reboot_phase)(SceSize, void *); //96, 0x60
	u32             entry_addr; //100, 0x64(seems to be used as a default address)
	u32             gp_value; //104, 0x68
	u32             text_addr; //108, 0x6C
	u32             text_size; //112, 0x70
	u32             data_size; //116, 0x74
	u32             bss_size; //120, 0x78
	u8              nsegment; //124, 0x7C
	u8              padding2[3]; //125, 0x7D
	u32             segmentaddr[4]; //128, 0x80
	u32             segmentsize[4]; //144, 0x90
	int             module_start_thread_priority; //160, 0xA0
	SceSize         module_start_thread_stacksize; //164, 0xA4
	SceUInt         module_start_thread_attr; //168, 0xA8
	int             module_stop_thread_priority; //172, 0xAC
	SceSize         module_stop_thread_stacksize; //176, 0xB0
	SceUInt         module_stop_thread_attr; //180, 0xB4
	int             module_reboot_before_thread_priority; //184, 0xB8
	SceSize         module_reboot_before_thread_stacksize; //188, 0xBC
	SceUInt         module_reboot_before_thread_attr; //192, 0xC0
} SceModule2;

enum BootLoadFlags
{
	BOOTLOAD_VSH = 1,
	BOOTLOAD_GAME = 2,
	BOOTLOAD_UPDATER = 4,
	BOOTLOAD_POPS = 8,
	BOOTLOAD_UMDEMU = 64, /* for original NP9660 */
};

typedef int (* STMOD_HANDLER)(SceModule2 *);
STMOD_HANDLER sctrlHENSetStartModuleHandler(STMOD_HANDLER handler);
void sctrlHENLoadModuleOnReboot(char *module_before, void *buf, int size, int flags);

#endif /* _SCTRL_H */
