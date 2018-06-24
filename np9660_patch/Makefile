TARGET = npdrm_free
OBJS =  src/main.o src/exports.o src/lib.o src/pgd.o src/setk1.o src/imports.o

LIBDIR = 
LIBS = 

CFLAGS = -Wall -Wextra -O2 -G0
ASFLAGS = $(CFLAGS)

USE_KERNEL_LIBC = 1
USE_KERNEL_LIBS = 1

PSP_FW_VERSION=660

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
