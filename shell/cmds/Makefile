-include $(TOP_DIR)/config.mk

COPTIMIZE = -O0

.PHONY: all clean
	
all: $(CMD_OUT_FILES) $(LIB_LIBC)

clean:
	$(RM) $(CMD_OUT_FILES) *.o *.d

%.o: %.c $(LIB_LIBC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(CMD_OUT_FILES):%: %.o $(LIB_LIBC)
	$(LD) -N -s -Ttext 0x4048000 -o $@ $< $(LIB_LIBC)
	
-include *.d
