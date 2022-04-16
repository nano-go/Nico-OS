OBJ_OUT_DIR := $(OBJ_DIR)

C_SRCS   := $(wildcard *.c)
ASM_SRCS := $(wildcard *.asm)

C_OBJS   := $(patsubst %.c, $(OBJ_OUT_DIR)/%.o, $(C_SRCS))
ASM_OBJS := $(patsubst %.asm, $(OBJ_OUT_DIR)/%.o, $(ASM_SRCS))

C_DEPS   := $(patsubst %.c, $(OBJ_OUT_DIR)/%.d, $(C_SRCS))

ALL_OBJS := $(C_OBJS) $(ASM_OBJS)

.PHONY: all clean build_submodules
all: $(ALL_OBJS) build_submodules

clean:
	$(RM) $(ALL_OBJS)
	@for submodule in $(SUB_MODULES);\
		do $(MAKE) clean -C $$submodule || exit 1;\
	done

build_submodules:
	@for submodule in $(SUB_MODULES);\
		do $(MAKE) -C $$submodule || exit 1;\
	done

$(OBJ_OUT_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_OUT_DIR)/%.o: %.asm
	$(AS) $(AS_FLAGS) -o $@ $<



-include $(C_DEPS)
