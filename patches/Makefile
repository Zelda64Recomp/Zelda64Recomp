TARGET = patches.elf

CC      := clang
LD      := ld.lld
OBJCOPY := llvm-objcopy

CFLAGS   := -target mips -mips2 -mabi=32 -O2 -mno-abicalls -mno-odd-spreg -fomit-frame-pointer -ffast-math -fno-unsafe-math-optimizations -mno-check-zero-division -G0 -Wall -Wextra -Wno-incompatible-library-redeclaration -Wno-unused-parameter -Wno-unknown-pragmas -Wno-unused-variable -Wno-missing-braces -Wno-unsupported-floating-point-opt
CPPFLAGS := -nostdinc -D_LANGUAGE_C -DMIPS -I. -I ../lib/mm-decomp/include -I ../lib/mm-decomp/src -I ../lib/mm-decomp/assets -I../lib/RT64-HLE/include
LDFLAGS  := -nostdlib -T patches.ld -T syms.ld --just-symbols=../mm.us.rev1.elf --allow-multiple-definition
BINFLAGS := -O binary

C_SRCS := $(wildcard *.c)
C_OBJS := $(C_SRCS:.c=.o)
C_DEPS := $(C_SRCS:.c=.d)

DATABIN := $(TARGET:.elf=.bin)

$(DATABIN): $(TARGET)
	$(OBJCOPY) $(BINFLAGS) $(TARGET) $@

$(TARGET): $(C_OBJS) patches.ld syms.ld
	$(LD) $(C_OBJS) $(LDFLAGS) -o $@

$(C_OBJS): %.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -MMD -MF $(@:.o=.d) -c -o $@

clean:
	rm -rf $(C_OBJS) $(TARGET) $(DATABIN)

-include $(C_DEPS)

.PHONY: clean
