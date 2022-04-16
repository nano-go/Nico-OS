-include $(TOP_DIR)/build/param.mk

SH = sh
CC = clang
AS = nasm
LD = ld.lld
DD = dd
AR = ar
RUBY = ruby
OBJCOPY = llvm-objcopy

MKDIR = mkdir
RM    = rm -rf

# $(CC) Flags
CWARNING = -Wall -Wextra -Werror -Wno-unused-parameter

COPTIMIZE = -O2
CFLAGS =  -MMD -MP $(COPTIMIZE) -m32 -std=gnu99\
					--target=i386 -march=i386\
					-static\
					-nostdinc\
					-fno-pic\
					-fno-builtin\
					-fno-stack-protector\
					-fno-omit-frame-pointer\
					$(CWARNING)

#-ffunction-sections -fdata-sections

CFLAGS += -I $(TOP_DIR)/include -I $(TOP_DIR)/lib/include
ifdef KERNEL_TEST
	CFLAGS += -D KERNEL_TEST
endif
ifndef KERNEL_DEBUG
	CFLAGS += -D NDEBUG
endif

# $(LD) Flags
LD_FLAGS := --gc-sections --static -nostdlib -O3

# $(AS) Flags
AS_FLAGS = 
AS_FLAGS_OUTFORMAT = -f elf
AS_FLAGS += $(AS_FLAGS_OUTFORMAT)

# $(DD) Flags
DD_FLAGS = conv=notrunc bs=512

OBJ_DIR := $(TOP_DIR)/objs
LIB_DIR := $(TOP_DIR)/lib
LIB_OBJ_DIR := $(LIB_DIR)/objs
LIB_LIBC := $(LIB_OBJ_DIR)/libc
BUILD_DIR := $(TOP_DIR)/build
MODULE := $(BUILD_DIR)/module.mk
MKFS := $(TOP_DIR)/mmkfs
CMD_OUT_DIR := $(TOP_DIR)/shell/cmds
CMD_OUT_FILES := $(CMD_OUT_DIR)/ls\
								 $(CMD_OUT_DIR)/mkdir\
								 $(CMD_OUT_DIR)/cat\
								 $(CMD_OUT_DIR)/echo\
								 $(CMD_OUT_DIR)/grep\
								 $(CMD_OUT_DIR)/rm
USER_PROCS := $(CMD_OUT_FILES) $(TOP_DIR)/shell/sh/sh $(TOP_DIR)/init/init

# Image File
OS_IMAGE_FILE := $(TOP_DIR)/nico_os.img
FS_IMAGE_FILE := $(TOP_DIR)/hd80MB.img

# Kernel Files
KERNEL_ELF := $(OBJ_DIR)/kernel.elf
KERNEL_BIN := $(OBJ_DIR)/kernel.bin

define MK_FILE
	truncate -s $2 $1
endef
