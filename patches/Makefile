TARGET = patches.elf

CC      := clang
LD      := ld.lld
OBJCOPY := llvm-objcopy

CFLAGS   := -target mips -mips2 -mabi=32 -O2 -mno-odd-spreg -fomit-frame-pointer -G0 -Wall -Wextra -Wno-incompatible-library-redeclaration -Wno-unused-parameter -Wno-unknown-pragmas -Wno-unused-variable
CPPFLAGS := -nostdinc -D_LANGUAGE_C -DMIPS -I ../../mm/include -I ../../mm/src -I ../../mm/build -I ../../mm/assets
LDFLAGS  := -nostdlib -T patches.ld -T syms.ld
BINFLAGS := -O binary

C_SRCS := $(wildcard *.c)
C_OBJS := $(C_SRCS:.c=.o)
C_DEPS := $(C_SRCS:.c=.d)

DATABIN := $(TARGET:.elf=.bin)

$(DATABIN): $(TARGET)
	$(OBJCOPY) $(BINFLAGS) $(TARGET) $@

$(TARGET): $(C_OBJS) patches.ld syms.ld
	$(LD) $(LDFLAGS) $(C_OBJS) -o $@

$(C_OBJS): %.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -MMD -MF $(@:.o=.d) -c -o $@

clean:
	rm -rf $(C_OBJS) $(TARGET) $(DATABIN)

-include $(C_DEPS)

.PHONY: clean
