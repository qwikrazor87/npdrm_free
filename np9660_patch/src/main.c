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
#include <pspiofilemgr.h>
#include <pspinit.h>
#include <stdio.h>
#include <string.h>
#include "lib.h"
#include "sctrl.h"
#include "pgd.h"

PSP_MODULE_INFO("npdrm_free", PSP_MODULE_KERNEL, 1, 0);
PSP_HEAP_SIZE_KB(0);

static STMOD_HANDLER previous = NULL;
char ebootpath[256], g_pgd_path[256];
u8 pgdbuf[0x90], g_eboot_key[16];
SceModule2 *npmod;
int licensed_eboot = 0, g_is_key = 0;
int applicationType;
#define FAKEFD 0x12345678

u32 tou32(u8 *buf)
{
	return (u32)(buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
}

int is_licensed_eboot(const char *path)
{
	int ret = 0;
	u8 buf[0x28];
	SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0);
	sceIoRead(fd, buf, sizeof(buf));

	if (!memcmp(buf, "\x00PBP", 4)) {
		sceIoLseek(fd, tou32(buf + 0x24), 0);
		sceIoRead(fd, buf, 0xC);

		if (!memcmp(buf, "NPUMDIMG", 8))
			ret = memcmp(buf + 8, "\x03\x00\x00\x01", 4); //disable patch if fixed key version is detected.
	}

	sceIoClose(fd);

	return ret;
}

SceUID userIoOpen(const char *path, int flags, SceMode mode)
{
	if (flags & 0x40000000)
		strcpy(g_pgd_path, path);

	return sceIoOpen(path, flags, mode);
}

SceUID userIoOpenAsync(const char *path, int flags, SceMode mode)
{
	if (flags & 0x40000000)
		strcpy(g_pgd_path, path);

	return sceIoOpenAsync(path, flags, mode);
}

//patches sceNpDrmEdataSetupKey and sceNpDrmGetModuleKey
int (* setup_edat_version_key)(u8 *vkey, u8 *edat, int size);
int setup_edat_version_key_hook(u8 *vkey, u8 *edat, int size) //variable EDAT/SPRX vkey per game, do not backup vkey.
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

int (* setup_eboot_version_key)(u8 *vkey, u8 *cid, u32 type, u8 *act);
int setup_eboot_version_key_hook(u8 *vkey, u8 *cid, u32 type, u8 *act)
{
	if (g_is_key) { //prevent SceNpUmdMount thread from crashing during suspend/resume process if rif/act.dat fails.
		memcpy(vkey, g_eboot_key, 16); //copy generated key from first call since there is only one version key per eboot.
		return 0;
	}

	int ret = setup_eboot_version_key(vkey, cid, type, act);

	if (ret < 0) //generate key from mac if official method fails.
		ret = get_version_key(vkey, ebootpath);

	if (ret >= 0) {
		memcpy(g_eboot_key, vkey, 16); //backup eboot vkey for later calls.
		g_is_key = 1;
	}

	return ret;
}

void patch_drm()
{
	u32 addr, data;

	for (addr = npmod->text_addr; addr < (npmod->text_addr + npmod->text_size); addr += 4) {
		data = _lw(addr);

		if (data == 0x3C118021) { //lui        $s1, 0x8021
			_sw(0x1021, addr - 4); //move $v0, $zr //patch memcmp, ensures the kernel continues to decrypt the eboot.
		} else if (data == 0x3C098055) { //lui        $t1, 0x8055
			HIJACK_FUNCTION(addr - 4, setup_eboot_version_key_hook, setup_eboot_version_key);
			break;
		}
	}

	SceModule2 *mod = FindModuleByName("scePspNpDrm_Driver");

	for (addr = mod->text_addr; addr < (mod->text_addr + mod->text_size); addr += 4) {
		if (_lw(addr) == 0x2CC60080) { //sltiu      $a2, $a2, 128
			HIJACK_FUNCTION(addr - 8, setup_edat_version_key_hook, setup_edat_version_key);
			break;
		}
	}

	ClearCaches();
}

int (* init_eboot)(const char *eboot, u32 a1) = (void *)NULL;
int init_eboot_hook(const char *eboot, u32 a1)
{
	if ((licensed_eboot = is_licensed_eboot(eboot))) {
		strcpy(ebootpath, eboot);
		patch_drm();
	}

	return init_eboot(eboot, a1);
}

void patch_np9660(SceModule2 *mod)
{
	u32 addr, jalcount = 0, data;
	npmod = mod;

	//do not patch in pops mode
	if (applicationType == PSP_INIT_KEYCONFIG_POPS)
		return;

	//prevent from patching again, should fix suspend issue in most cases.
	if (init_eboot)
		return;

	for (addr = mod->text_addr; addr < (mod->text_addr + mod->text_size); addr += 4) {
		data = _lw(addr);

		if (((data & 0xFC000000) == 0x0C000000) && (jalcount < 2))
			jalcount++;

		if (jalcount == 2) {
			init_eboot = (void *)(((data & 0x03FFFFFF) << 2) | 0x80000000);
			_sw(MAKE_CALL(init_eboot_hook), addr);
			break;
		}
	}

	ClearCaches();
}

void patch_game_module(SceModule2 *mod)
{
	if (!licensed_eboot)
		return;

	u32 user_sceIoOpen = FindImportByModule(mod->modname, "IoFileMgrForUser", 0x109F50BC);
	if (user_sceIoOpen)
		sceKernelHookJalSyscall(userIoOpen, user_sceIoOpen, mod);

	u32 user_sceIoOpenAsync = FindImportByModule(mod->modname, "IoFileMgrForUser", 0x89AA9906);
	if (user_sceIoOpenAsync)
		sceKernelHookJalSyscall(userIoOpenAsync, user_sceIoOpenAsync, mod);

	ClearCaches();
}

void patch_popsman()
{
	//patch only in pops mode
	if (applicationType != PSP_INIT_KEYCONFIG_POPS)
		return;

	//just a patch to generate KEYS.BIN if necessary
	strcpy(ebootpath, sceKernelInitFileName());
	SceUID fd = sceIoOpen(ebootpath, PSP_O_RDONLY, 0);
	sceIoRead(fd, pgdbuf, 0x28);

	if (!memcmp(pgdbuf, "\x00PBP", 4)) {
		sceIoLseek(fd, tou32(pgdbuf + 0x24), 0);
		sceIoRead(fd, pgdbuf, 16);

		if (!memcmp(pgdbuf, "PSTITLE", 7))
			sceIoLseek(fd, 0x1F0, 1);
		else if (!memcmp(pgdbuf, "PSISO", 5))
			sceIoLseek(fd, 0x3F0, 1);

		sceIoRead(fd, pgdbuf, 0x90);

		if (!memcmp(pgdbuf, "\x00PGD", 4))
			dumpPS1key(ebootpath, pgdbuf);
	}

	sceIoClose(fd);
}

int modflag = 0;
int module_start_handler(SceModule2 *module)
{
	int ret = previous ? previous(module) : 0;

	if (modflag == 1) { //next module after sceKernelLibrary should be the main game module.
		modflag = 2;
		patch_game_module(module);
	}

	if (!strcmp(module->modname, "sceNp9660_driver")) {
		patch_np9660(module);
	} else if (!strcmp(module->modname, "sceMediaSync")) {
		patch_popsman();
	} else if (!strcmp(module->modname, "sceKernelLibrary")) {
		if (applicationType != PSP_INIT_KEYCONFIG_POPS)
			modflag = 1;
	}

	return ret;
}

int thread_start(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
	previous = sctrlHENSetStartModuleHandler(module_start_handler);

	return sceKernelExitDeleteThread(0);
}

int module_start(SceSize args, void *argp)
{
	applicationType = sceKernelInitKeyConfig();

	SceUID thid = sceKernelCreateThread("npdrm_free", thread_start, 0x22, 0x2000, 0, NULL);

	if (thid >= 0)
		sceKernelStartThread(thid, args, argp);

	return 0;
}

int module_stop(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
	return 0;
}
