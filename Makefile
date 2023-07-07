CONFIG ?= Debug
LIB ?= 0

ifeq ($(CONFIG),Debug)
BUILD_DIR     := build/debug
FUNC_OPTFLAGS := -Og -g -fno-strict-aliasing
OPTFLAGS      := -Og -g -fno-strict-aliasing
# Static C runtime linking
LIBS          := -Wl,/nodefaultlib:libcmt -Wl,/nodefaultlib:ucrt -Wl,/nodefaultlib:libucrt -llibcmtd -llibvcruntimed -llibucrtd
# Dynamic
# LIBS          := -Wl,/nodefaultlib:libcmt -Wl,/nodefaultlib:ucrt -Wl,/nodefaultlib:libucrt -lmsvcrtd -lvcruntimed -lucrtd
else ifeq ($(CONFIG),Release)
BUILD_DIR     := build/release
FUNC_OPTFLAGS := -O2 -g -fno-strict-aliasing
OPTFLAGS      := -O2 -g -fno-strict-aliasing
# Static C runtime linking
LIBS          := -Wl,/nodefaultlib:libcmt -Wl,/nodefaultlib:ucrt -Wl,/nodefaultlib:libucrt -llibcmt -llibvcruntime -llibucrt
# Dynamic
# LIBS          := -Wl,/nodefaultlib:libcmt -Wl,/nodefaultlib:ucrt -Wl,/nodefaultlib:libucrt -lmsvcrt -lvcruntime -lucrt
else
$(error "Invalid build configuration: $(CONFIG)")
endif

SRC_DIRS := portultra src rsp

FUNCS_DIR := RecompiledFuncs
FUNCS_LIB := $(BUILD_DIR)/RecompiledFuncs.lib
FUNCS_C_SRCS   := $(wildcard $(FUNCS_DIR)/*.c)
FUNCS_CXX_SRCS := $(wildcard $(FUNCS_DIR)/*.cpp)

FUNC_BUILD_DIR := $(BUILD_DIR)/RecompiledFuncs
FUNCS_C_OBJS   := $(addprefix $(BUILD_DIR)/,$(FUNCS_C_SRCS:.c=.o))
FUNCS_CXX_OBJS := $(addprefix $(BUILD_DIR)/,$(FUNCS_CXX_SRCS:.cpp=.o))
ALL_FUNC_OBJS  := $(FUNCS_C_OBJS) $(FUNCS_CXX_OBJS)

BUILD_SRC_DIRS := $(addprefix $(BUILD_DIR)/,$(SRC_DIRS))
C_SRCS   := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
CXX_SRCS := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))

C_OBJS   := $(addprefix $(BUILD_DIR)/,$(C_SRCS:.c=.o))
CXX_OBJS := $(addprefix $(BUILD_DIR)/,$(CXX_SRCS:.cpp=.o))
ALL_OBJS := $(C_OBJS) $(CXX_OBJS)


CC  := clang
CXX := clang++
LIB := clang++
LD  := clang++

FUNC_CFLAGS   := $(FUNC_OPTFLAGS) -c -Wno-unused-but-set-variable
FUNC_CXXFLAGS := $(FUNC_OPTFLAGS) -std=c++20 -c
FUNC_CPPFLAGS := -Iinclude
LIBFLAGS      := $(OPTFLAGS) -fuse-ld=llvm-lib
LIB_DIR       ?= C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.29.30133\lib\x64
UCRT_DIR      ?= C:\Program Files (x86)\Windows Kits\10\lib\10.0.22000.0\ucrt\x64;
SDK_DIR       ?= C:\Program Files (x86)\Windows Kits\10\lib\10.0.22000.0\um\x64

WARNFLAGS := -Wall -Wextra -Wpedantic -Wno-gnu-anonymous-struct
CFLAGS    := -ffunction-sections -fdata-sections -march=nehalem $(OPTFLAGS) $(WARNFLAGS) -c
CXXFLAGS  := -ffunction-sections -fdata-sections -march=nehalem $(OPTFLAGS) $(WARNFLAGS) -std=c++20 -c
CPPFLAGS  := -Iinclude -Ithirdparty
LDFLAGS   := -v -Wl,/OPT:REF $(OPTFLAGS) $(LIBS) -L"$(LIB_DIR:;=)" -L"$(UCRT_DIR:;=)" -L"$(SDK_DIR:;=)" lib/RT64/$(CONFIG)/RT64.lib

ifeq ($(LIB),1)
TARGET := $(BUILD_DIR)/MMRecomp.dll
LDFLAGS += -shared
else
TARGET := $(BUILD_DIR)/MMRecomp.exe
endif

default: $(TARGET)

clean: 
	rmdir /S /Q $(subst /,\\,$(BUILD_DIR))

cleanfuncs:


$(FUNCS_CXX_OBJS) : $(BUILD_DIR)/%.o : %.cpp | $(FUNC_BUILD_DIR)
	@$(CXX) $(FUNC_CXXFLAGS) $(FUNC_CPPFLAGS) $^ -o $@
	
$(FUNCS_C_OBJS) : $(BUILD_DIR)/%.o : %.c | $(FUNC_BUILD_DIR)
	@$(CC) $(FUNC_CFLAGS) $(FUNC_CPPFLAGS) $^ -o $@

$(FUNCS_LIB): $(ALL_FUNC_OBJS) | $(BUILD_DIR)
	$(LIB) $(LIBFLAGS) $(FUNC_BUILD_DIR)/*.o -o $@



$(CXX_OBJS) : $(BUILD_DIR)/%.o : %.cpp | $(BUILD_SRC_DIRS)
	$(CXX) -MMD -MF $(@:.o=.d) $(CXXFLAGS) $(CPPFLAGS) $< -o $@
	
$(C_OBJS) : $(BUILD_DIR)/%.o : %.c | $(BUILD_SRC_DIRS)
	$(CC) -MMD -MF $(@:.o=.d) $(CFLAGS) $(CPPFLAGS) $< -o $@

$(TARGET): $(FUNCS_LIB) $(ALL_OBJS) | $(BUILD_SRC_DIRS)
	$(LD) $(LDFLAGS) -o $@ $^

$(BUILD_SRC_DIRS) $(FUNC_BUILD_DIR) $(BUILD_DIR):
	mkdir $(subst /,\\,$@)

-include $(ALL_OBJS:.o=.d)

MAKEFLAGS += --no-builtin-rules
.SUFFIXES:
.PHONY: default clean cleanfuncs

print-% : ; $(info $* is a $(flavor $*) variable set to [$($*)]) @true
