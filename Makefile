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
						-drive format=raw,file=$(OS_IMAGE_FILE),media=disk\
						-drive format=raw,file=$(FS_IMAGE_FILE),media=disk
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
	$(MKFS) -i $@ --ish ./init/init.sh $(USER_PROCS)

$(MKFS):
	$(MAKE) -C ./fs/mkfs all || exit 1;

bochs: 
	if [ ! -e bochsrc ];\
		then ! echo "Bochs needs the configuration file: bochsrc" && exit 1;\
	fi
	$(BOCHS) -f bochsrc

qemu:
	$(QEMU) $(QEMUOPT)

qemu-gdb:
	$(QEMU) $(QEMUOPTS) -s -S

qemu-sdl:
	$(QEMU) $(QEMUOPTS) -display sdl

qemu-sdl-gdb:
	$(QEMU) $(QEMUOPTS) -display sdl -s -S

qemu-gtk:
	$(QEMU) $(QEMUOPTS) -display gtk

qemu-gtk-gdb:
	$(QEMU) $(QEMUOPTS) -display gtk -s -S

qemu-curses:
	$(QEMU) $(QEMUOPTS) -display curses

qemu-curses-gdb:
	$(QEMU) $(QEMUOPTS) -display curses -s -S

qemu-vnc: 
	$(QEMU) $(QEMUOPTS) -display vnc=$(QEMU_VNC_PORT)

qemu-vnc-gdb:
	$(QEMU) $(QEMUOPTS) -display vnc=$(QEMU_VNC_PORT) -s -S

clean:
	@for dir in $(SUBMODULES);\
		do $(MAKE) -C $$dir clean;\
	done
	$(MAKE) -C fs/mkfs clean
	$(RM) $(OS_IMAGE_FILE)
	$(RM) $(FS_IMAGE_FILE)
