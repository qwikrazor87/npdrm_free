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
#include <pspsysmem_kernel.h>
#include <psputilsforkernel.h>
#include <stdio.h>
#include <string.h>
#include "lib.h"
#include "sctrl.h"
#include "np9660_patch.h"
#include "pgd.h"

PSP_MODULE_INFO("npdrm_free_loader", PSP_MODULE_KERNEL, 1, 0);
PSP_HEAP_SIZE_KB(0);

static STMOD_HANDLER previous = NULL;

char g_pgd_path[256];
u8 pgdbuf[0x90];

void *vshCheckBootable(void *dst, const void *src, int size)
{
	SFO *sfo = (SFO *)src;

	int i;

	for (i = 0; i < sfo->entries; i++) {
		if (!strcmp((char *)((u32)src + sfo->label + sfo->sfotable[i].label_offset), "BOOTABLE")) {
			if (_lw((u32)src + sfo->data + sfo->sfotable[i].data_offset) == 2)
				_sw(1, (u32)src + sfo->data + sfo->sfotable[i].data_offset);

			break;
		}
	}

	return memcpy(dst, src, size);
}

void patch_vsh_module(SceModule2 *mod)
{
	u32 addr;
	int syscall = sceKernelQuerySystemCall(vshCheckBootable);

	for (addr = mod->text_addr; addr < (mod->text_addr + mod->text_size); addr += 4) {
		if ((_lw(addr) == 0x10400004) && (_lw(addr + 16) == 0x02003021)) {
			_sw(_lw(addr + 16), addr + 12);
			_sw(((syscall << 6) | 12) & 0x03FFFFFF, addr + 16);
			sceKernelDcacheWritebackInvalidateRange((const void *)(addr + 12), 8);
			sceKernelIcacheInvalidateRange((const void *)(addr + 12), 8);
			break;
		}
	}
}

u32 tou32(u8 *buf)
{
	return (u32)(buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
}

//patches sceNpDrmEdataSetupKey and sceNpDrmGetModuleKey
int (* setup_edat_version_key)(u8 *vkey, u8 *edat, int size);
int setup_edat_version_key_hook(u8 *vkey, u8 *edat, int size)
{
	int ret = setup_edat_version_key(vkey, edat, size);

	if (ret < 0) { //generate key from mac if official method fails.
		ret = sceIoOpen(g_pgd_path, 1, 0);
		sceIoRead(ret, pgdbuf, 16);
		sceIoLseek(ret, tou32(pgdbuf + 0xC) & 0xFFFF, 0);
		sceIoRead(ret, pgdbuf, 0x90);
		sceIoClose(ret);

		ret = get_edat_key(vkey, pgdbuf);
	}

	return ret;
}

int (* do_open)(const char *path, int flags, SceMode mode, int async, int retAddr, int oldK1);
int do_open_hook(const char *path, int flags, SceMode mode, int async, int retAddr, int oldK1)
{
	if (flags & 0x40000000)
		strcpy(g_pgd_path, path);

	return do_open(path, flags, mode, async, retAddr, oldK1);
}

void patch_drm()
{
	u32 addr;
	SceModule2 *mod = FindModuleByName("scePspNpDrm_Driver");

	for (addr = mod->text_addr; addr < (mod->text_addr + mod->text_size); addr += 4) {
		if (_lw(addr) == 0x2CC60080) { //sltiu      $a2, $a2, 128
			HIJACK_FUNCTION(addr - 8, setup_edat_version_key_hook, setup_edat_version_key);
			break;
		}
	}

	addr = K_EXTRACT_IMPORT(sceIoOpen) + 4;

	while (1) {
		if ((_lw(addr) & 0xFC000000) == 0x0C000000) {
			do_open = (void *)(((_lw(addr) & 0x03FFFFFF) << 2) | 0x80000000);
			_sw(MAKE_CALL(do_open_hook), addr);
			break;
		}

		addr += 4;
	}

	ClearCaches();
}

void patch_sysconf(SceModule2 *mod)
{
	//patch sysconf act/rif check, call official function first, then patch to return 0 if it fails.
	u32 addr;
	for (addr = mod->text_addr; addr < (mod->text_addr + mod->text_size); addr += 4) {
		if (_lw(addr) == 0x0062200B) { //movn       $a0, $v1, $v0
			_sw(MAKE_CALL(0x08802000), addr + 4);
			_sw(0x27BDFFF0, 0x08802000); //addiu      $sp, $sp, -16
			_sw(0xAFBF0000, 0x08802004); //sw         $ra, 0($sp)
			_sw(MAKE_CALL(mod->text_addr + 0x0000A1D0), 0x08802008); //jal        sub_0000A1D0 (sysconf)
			_sw(0, 0x0880200C); //nop
			_sw(0x0002100B, 0x08802010); //movn       $v0, $zr, $v0
			_sw(0x8FBF0000, 0x08802014); //lw         $ra, 0($sp)
			_sw(0x03E00008, 0x08802018); //jr         $ra
			_sw(0x27BD0010, 0x0880201C); //addiu      $sp, $sp, 16
			ClearCaches();
			break;
		}
	}
}

int module_start_handler(SceModule2 *module)
{
	int ret = previous ? previous(module) : 0;

	if (!strcmp(module->modname, "vsh_module")) {
		patch_vsh_module(module);
		patch_drm();
	} else if (!strcmp(module->modname, "sysconf_plugin_module")) {
		patch_sysconf(module);
	}

	return ret;
}

int thread_start(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
	previous = sctrlHENSetStartModuleHandler(module_start_handler);

	SceUID blockid = sceKernelAllocPartitionMemory(1, "npdrm_free_module", PSP_SMEM_Low, size_np9660_patch, NULL);
	void *modbuf = sceKernelGetBlockHeadAddr(blockid);

	if (blockid >= 0) {
		memcpy(modbuf, np9660_patch, size_np9660_patch);
		sctrlHENLoadModuleOnReboot("/kd/iofilemgr_dnas.prx", modbuf, size_np9660_patch, BOOTLOAD_UMDEMU | BOOTLOAD_POPS);
	}

	return sceKernelExitDeleteThread(0);
}

int module_start(SceSize args, void *argp)
{
	SceUID thid = sceKernelCreateThread("npdrm_free_loader", thread_start, 0x22, 0x2000, 0, NULL);

	if (thid >= 0)
		sceKernelStartThread(thid, args, argp);

	return 0;
}

int module_stop(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
	return 0;
}
