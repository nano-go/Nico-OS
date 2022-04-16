-include $(TOP_DIR)/config.mk

OBJ_FILES := $(wildcard *.o)

.PHONY: all clean

all: $(LIB_LIBC)

$(LIB_LIBC): $(OBJ_FILES)
	$(AR) crv -o $@ $(OBJ_FILES)
	
clean: 
	$(RM) *.o *.d $(LIB_LIBC)
