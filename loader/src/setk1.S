	.set noreorder

	.global pspSdkSetK1

	.ent pspSdkSetK1
pspSdkSetK1:
	move       $v0, $k1
	jr         $ra
	move       $k1, $a0
	.end pspSdkSetK1

	.set reorder
