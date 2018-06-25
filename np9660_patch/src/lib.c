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

#include <pspkernel.h>
#include <psputilsforkernel.h>
#include <string.h>
#include <stdio.h>
#include "lib.h"
#include "sctrl.h"

void ClearCaches(void)
{
	sceKernelDcacheWritebackInvalidateAll();
	sceKernelIcacheInvalidateAll();
}

SceModule2 *FindModuleByName(const char *module)
{
	u32 kaddr;
	SceModule2 *mod = (SceModule2 *)0;

	//search for base module struct, then iterate from there.
	for (kaddr = 0x88000000; kaddr < 0x88300000; kaddr += 4) {
		if (!strcmp((const char *)kaddr, "sceSystemMemoryManager")) {
			if ((*(u32*)(kaddr + 0x64) == *(u32*)(kaddr + 0x78)) && \
				(*(u32*)(kaddr + 0x68) == *(u32*)(kaddr + 0x88))) {
				if (*(u32*)(kaddr + 0x64) && *(u32*)(kaddr + 0x68)) {
					mod = (SceModule2 *)(kaddr - 8);
					break;
				}
			}
		}
	}

	while (mod) {
		if (!strcmp(mod->modname, module))
			break;

		mod = mod->next;
	}

	return mod;
}

SceModule2 *FindModuleByAddress(u32 address)
{
	SceModule2 *mod = FindModuleByName("sceSystemMemoryManager"); //iterate starting from the base module

	while (mod) {
		if ((address >= mod->text_addr) && (address < (mod->text_addr + mod->text_size)))
			break;

		mod = mod->next;
	}

	return mod;
}

u32 FindExport(const char *module, const char *library, u32 nid)
{
	SceModule2 *mod = FindModuleByName(module);

	if (mod) {
		u32 addr = mod->text_addr;
		u32 maxaddr = addr + mod->text_size;

		for (; addr < maxaddr; addr += 4) {
			if (strcmp(library, (const char *)addr) == 0) {
				u32 libaddr = addr;

				while (*(u32*)(addr -= 4) != libaddr);

				u32 exports = (u32)(*(u16*)(addr + 10) + *(u8*)(addr + 9));
				u32 jump = exports * 4;

				addr = *(u32*)(addr + 12);

				while (exports--) {
					if (*(u32*)addr == nid)
						return *(u32*)(addr + jump);

					addr += 4;
				}

				return 0;
			}
		}
	}

	return 0;
}

u32 FindImportByModule(const char *module, const char *lib, u32 nid)
{
	SceModule2 *mod = FindModuleByName(module);

	if (mod) {
		int i;
		SceLibStubTable *stub = (SceLibStubTable *)mod->stub_top;

		while (stub->libname) {
			if (!strcmp(stub->libname, lib)) {
				for (i = 0; i < stub->stubcount; i++) {
					if (stub->nidtable[i] == nid)
						return (u32)&stub->stubtable[i * 2];
				}
			}

			stub = (SceLibStubTable *)((u32)stub + (stub->len << 2));
		}
	}

	return 0;
}

int sceKernelHookJalSyscall(void *func, u32 addr, SceModule2 *mod)
{
	int ret = sceKernelQuerySystemCall(func);

	if (ret > 0) {
		u32 ofs, data, jal = (((addr >> 2) & 0x03FFFFFF) | 0x0C000000);
		for (ofs = mod->text_addr; ofs < (mod->text_addr + mod->text_size); ofs += 4) {
			data = _lw(ofs);

			if (data == jal) {
				_sw(_lw(ofs + 4), ofs);
				_sw(((ret << 6) | 12) & 0x03FFFFFF, ofs + 4);
				sceKernelDcacheWritebackInvalidateRange((const void *)ofs, 8);
				sceKernelIcacheInvalidateRange((const void *)ofs, 8);
			}
		}
	}

	return ret;
}

