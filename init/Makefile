-include $(TOP_DIR)/config.mk

.PHONY: all clean
	
all: init

clean:
	$(RM) *.o *.d *.bin init

init.o: main.c $(LIB_LIBC)
	$(CC) -MMD -MP -I $(LIB_DIR)/include\
		-I $(TOP_DIR)/include\
		$(CWARNING) -m32 -fno-builtin -nostdinc --target=i386 -march=i386\
		-c -o $@ $<

init: init.o $(LIB_LIBC)
	$(LD) -N -s -Ttext 0x4048000 -o $@ $^
	
-include *.d