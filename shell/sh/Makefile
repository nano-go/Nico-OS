-include $(TOP_DIR)/config.mk

COPTIMIZE = -O0
CFLAGS += -I ./include

SH_SRC := $(wildcard *.c)
SH_OBJS := $(patsubst %.c, objs/%.o, $(SH_SRC))

.PHONY: all clean
all: objs $(SH_SRC) sh

clean:
	$(RM) ./objs

objs:
	@if [ ! -d $@ ];\
		then mkdir $@;\
	fi

sh: $(SH_OBJS) $(LIB_LIBC)
	$(LD) -N -s -Ttext 0x4048000 -o $@ $^

objs/%.o: %.c $(LIB_LIBC)
	$(CC) $(CFLAGS) -m32 -c -o $@ $<

-include *.d
