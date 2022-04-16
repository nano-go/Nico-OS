-include $(TOP_DIR)/config.mk

BOOT_BINS = mbr.bin setup.bin 
AS_FLAGS += -I ./common -w-zeroing

INCLUDE_FILES = common/config.asm common/disk_read.asm

.PHONY: all clean

all: $(BOOT_BINS)

mbr.bin: mbr.asm $(INCLUDE_FILES)
	$(AS) $(AS_FLAGS) -f bin -o $@ $<

setup.bin: setup.asm $(INCLUDE_FILES)
	$(AS) $(AS_FLAGS) -f bin -o $@ $<

clean:
	$(RM) $(BOOT_BINS)
