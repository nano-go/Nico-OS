TOP_DIR = $(CURDIR)
export TOP_DIR

-include config.mk

SUBMODULES = boot lib kernel fs shell init
ifdef KERNEL_TEST
	SUBMODULES += test
endif
SUBMODULES += objs

BOCHS = bochs
QEMU = qemu-system-i386
QEMUOPTS := -m 36 -smp 1\
						-drive format=raw,file=nico_os.img,media=disk\
						-drive format=raw,file=hd80MB.img,media=disk
QEMU_VNC_PORT = 127.0.0.1:1

.PHONY: all qemu qemu-vnc bochs clean

all: $(OS_IMAGE_FILE) $(FS_IMAGE_FILE) 

compile_all_modules:
	@for dir in $(SUBMODULES);\
		do $(MAKE) -C $$dir all || exit 1;\
	done;

$(OS_IMAGE_FILE): compile_all_modules
	# Create 20 MB OS image file.
	$(call MK_FILE, $(OS_IMAGE_FILE), 20M)
	$(DD) $(DD_FLAGS) if=boot/mbr.bin of=$(OS_IMAGE_FILE) count=1 seek=0 
	$(DD) $(DD_FLAGS) if=boot/setup.bin of=$(OS_IMAGE_FILE) count=8 seek=1
	$(DD) $(DD_FLAGS) if=objs/kernel.bin of=$(OS_IMAGE_FILE) count=400 seek=9

# Make file system and write all user procs to the fs image.
$(FS_IMAGE_FILE): compile_all_modules $(MKFS) $(USER_PROCS)
	$(SH) ./make_fs_img.sh || exit 1;
	$(MKFS) $@ $(USER_PROCS)

$(MKFS):
	$(MAKE) -C ./fs/mkfs all || exit 1;

bochs: all
	if [ ! -e bochsrc ];\
		then ! echo "Bochs needs the configuration file: bochsrc" && exit 1;\
	fi
	$(BOCHS) -f bochsrc

qemu: all
	$(QEMU) $(QEMUOPT)

qemu-gdb: all
	$(QEMU) $(QEMUOPTS) -s -S

qemu-sdl: all
	$(QEMU) $(QEMUOPTS) -display sdl

qemu-gtk: all
	$(QEMU) $(QEMUOPTS) -display gtk

qemu-curses: all
	$(QEMU) $(QEMUOPTS) -display curses

qemu-vnc: all
	$(QEMU) $(QEMUOPTS) -display vnc=$(QEMU_VNC_PORT)

qemu-vnc-gdb: all
	$(QEMU) $(QEMUOPTS) -display vnc=$(QEMU_VNC_PORT) -s -S

clean:
	@for dir in $(SUBMODULES);\
		do $(MAKE) -C $$dir clean;\
	done
	$(MAKE) -C fs/mkfs clean
	$(RM) $(OS_IMAGE_FILE)
	$(RM) $(FS_IMAGE_FILE)
