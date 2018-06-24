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

#include "sctrl.h"

#include <psptypes.h>

void ClearCaches(void);

SceModule2 *FindModuleByName(const char *module);
u32 FindExport(const char *module, const char *library, u32 nid);
u32 pspSdkSetK1(u32 k1);
int sceKernelQuerySystemCall(void *func);

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

typedef struct {
	u16 label_offset;
	u16 type;
	u32 size;
	u32 size_padded;
	u32 data_offset;
} SFOtable;

typedef struct {
	u32 magic;
	u32 version;
	u32 label;
	u32 data;
	int entries;
	SFOtable sfotable[];
} SFO;

#endif /* LIB_H_ */
