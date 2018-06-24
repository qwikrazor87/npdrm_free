all:
	@cd np9660_patch; make; \
		bin2c npdrm_free.prx np9660_patch.h np9660_patch; \
		mv np9660_patch.h ../loader/src/np9660_patch.h;
	@cd loader; make; mv npdrm_free.prx ../npdrm_free.prx;

clean:
	@cd np9660_patch; make clean;
	@cd loader; make clean; rm -rf src/np9660_patch.h;
	rm -rf npdrm_free.prx
