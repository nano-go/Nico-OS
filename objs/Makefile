-include $(TOP_DIR)/config.mk

OBJ_FILES := $(wildcard *.o)
ENTRY_SYMBOL := _start

.PHONY: all clean

# The kernel_elf and the kernel_bin are in the current directory.
all: $(KERNEL_ELF) $(KERNEL_BIN)

$(KERNEL_ELF): $(OBJ_FILES) kernel.lds
	# $(LD) $(LD_FLAGS) -e $(ENTRY_SYMBOL) -Ttext 0xc0030000 -o $@ $(OBJ_FILES)
	$(LD) $(LD_FLAGS) -N -T ./kernel.lds -o $@ $(OBJ_FILES) $(LIB_LIBC)

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -I elf32-i386 -S -R ".comment" -O binary $< $@

clean:
	$(RM) *.o *.d $(KERNEL_ELF) $(KERNEL_BIN)
