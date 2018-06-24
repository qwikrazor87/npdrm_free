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

#ifndef LIB_H_
#define LIB_H_

#include <psptypes.h>
#include "sctrl.h"

void ClearCaches(void);

SceModule2 *FindModuleByName(const char *module);
SceModule2 *FindModuleByAddress(u32 address);
u32 FindExport(const char *module, const char *library, u32 nid);
u32 FindImportByModule(const char *module, const char *lib, u32 nid);
u32 pspSdkSetK1(u32 k1);
int sceKernelHookJalSyscall(void *func, u32 addr, SceModule2 *mod);

int sceKernelQuerySystemCall(void *);

#define U_EXTRACT_IMPORT(x) ((((u32)_lw((u32)x)) & ~0x08000000) << 2)
#define K_EXTRACT_IMPORT(x) (((((u32)_lw((u32)x)) & ~0x08000000) << 2) | 0x80000000)
#define U_EXTRACT_CALL(x) ((((u32)_lw((u32)x)) & ~0x0C000000) << 2)
#define K_EXTRACT_CALL(x) (((((u32)_lw((u32)x)) & ~0x0C000000) << 2) | 0x80000000)

#define MAKE_JUMP(f) (0x08000000 | (((u32)(f) >> 2)  & 0x03FFFFFF))
#define MAKE_CALL(f) (0x0C000000 | (((u32)(f) >> 2)  & 0x03FFFFFF))

#define MAKE_JUMP_PATCH(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a);

#define HIJACK_FUNCTION(a, f, ptr) \
{ \
	u32 func = a; \
	static u32 patch_buffer[3]; \
	_sw(_lw(func), (u32)patch_buffer); \
	_sw(_lw(func + 4), (u32)patch_buffer + 8);\
	MAKE_JUMP_PATCH((u32)patch_buffer + 4, func + 8); \
	_sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), func); \
	_sw(0, func + 4); \
	ptr = (void *)patch_buffer; \
}

static inline u32 pspGetRa(void)
{
	u32 ra;
	__asm("nop");
	__asm("move %0, $ra" : "=r" (ra));
	__asm("nop");
	return ra;
}

typedef struct {
	const char *libname;
	u8 version[2];
	u16 attribute;
	u8 len;
	u8 vstubcount;
	u16 stubcount;
	u32 *nidtable;
	u32 *stubtable;
} SceLibStubTable;

#endif /* LIB_H_ */
