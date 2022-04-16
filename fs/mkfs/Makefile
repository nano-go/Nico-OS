-include $(TOP_DIR)/config.mk

MKFS_SRC := $(wildcard *.c)
MKFS_OBJS := $(MKFS_SRC:.c=.o)

.PHONY: all clean
all: $(MKFS) $(MKFS_SRC)

clean:
	$(RM) *.o *.d $(MKFS)

$(MKFS): $(MKFS_OBJS)
	$(CC) -o $@ $^
	rbin $(MKFS) || exit; # Ignore this line.

%.o: %.c
	$(CC) -O2 -MMD -MP -I ./include -Werror -Wall -std=gnu99 -c -o $@ $<
	
-include *.d
