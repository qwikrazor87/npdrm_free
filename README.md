# npdrm_free
npdrm_free by qwikrazor87
A plugin to run official PSN content without a valid license or act.dat.

Place npdrm_free.prx at ms0:/seplugins/npdrm_free.prx.
Enable plugin in both ms0:/seplugins/game.txt and ms0:/seplugins/vsh.txt.
Write this line in both files:
	ms0:/seplugins/npdrm_free.prx 1

Restart VSH, now you can run official PSN content without a license.

This plugin now patches the core of npdrm to decrypt content if and only if the official methods fail,
otherwise the official license/act.dat is used.
Now you can run official PSN games and DLC without needing to decrypt them.
Now generates KEYS.BIN for official PSN PS1 eboots on the fly if needed.
ALL content MUST be encrypted and left as is from official files for this plugin to work, otherwise use nodrm engine in PRO CFW.
PSN TG-16/PC Engine games can now be run without first decrypting any content, do NOT decrypt the PSP-KEY.EDAT for it.
PTF themes requiring a license can now be installed.
Suspend issue from previous versions should now be fixed, if not, try disabling all other plugins.

Huge thanks to all involved in the homebrew community, devs and users alike,
without whom I would not be able to accomplish this task. You are too numerous to name.

Contact me for issues if needed.
	@qwikrazor87 on Twitter
	qwikrazor87 on http://wololo.net/talk/index.php
	https://github.com/qwikrazor87/

I hope you enjoy this plugin as much as I did making it and figuring out how npdrm works. :)

