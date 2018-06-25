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
#include "lib.h"

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

