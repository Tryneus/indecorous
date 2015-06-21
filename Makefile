SRC_DIR = ./src
TEST_DIR = ./test
OBJ_DIR = ./bin/obj
BIN_DIR = ./bin
EXT_DIR = ./external

UDNS_PATH = $(EXT_DIR)/udns-0.4
UDNS_CONFIGURE = $(UDNS_PATH)/configure
UDNS_MAKEFILE = $(UDNS_PATH)/Makefile
UDNS_LIB = $(UDNS_PATH)/libudns.a

CXX ?= g++

ifeq ($(CXX),gcc)
  override CXX = g++
else
  ifeq ($(CXX),clang)
    override CXX = clang++
  endif
endif

CXX_FLAGS = -std=c++14 -I$(SRC_DIR) -I$(TEST_DIR) -Wall -Wextra -Werror
CXX_FLAGS += $(addprefix -I, $(UDNS_PATH))
CXX_FLAGS += -Wnon-virtual-dtor -Wno-deprecated-declarations
CXX_FLAGS += -Wformat=2 -Wswitch-enum
CXX_FLAGS += -Wundef -Wvla -Wshadow -Wmissing-noreturn

# We use c++14 extensions, so force us to use a compiler version with support
# TODO: make this more portable
ifeq ($(CXX),g++)
  override CXX = g++-4.9
else
  ifeq ($(CXX),clang++)
    override CXX = clang++-3.5
    CXX_FLAGS += -Wconditional-uninitialized -Wused-but-marked-unused
  endif
endif

ifeq ($(DEBUG),1)
else
  CXX_FLAGS += -O3
  #CXX_FLAGS += -fno-strict-aliasing -DNDEBUG
endif

# Hack because newton's version of gdb is older
#CXX_FLAGS += -g
CXX_FLAGS += -gdwarf-3 -fdata-sections -ffunction-sections
CXX_FLAGS += -D__STDC_FORMAT_MACROS

LD_FLAGS = -lstdc++ -Wl,--gc-sections -lpthread -lrt $(EXTERNAL_INCLUDES)
BIN_NAME = coro_test

ALL_SOURCES := $(shell find $(SRC_DIR) -name '*.cc' -not -name '\.*')
ALL_TESTS := $(shell find $(TEST_DIR) -name '*.cc' -not -name '\.*')
SRC_OBJS := $(patsubst $(SRC_DIR)/%.cc,$(OBJ_DIR)/%.o,$(ALL_SOURCES))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cc,$(OBJ_DIR)/%.o,$(ALL_TESTS))
EXTERNAL_OBJS := $(UDNS_LIB)
ALL_OBJS := $(SRC_OBJS) $(TEST_OBJS) $(EXTERNAL_OBJS)

.PHONY: all test val_test clean
all: $(BIN_DIR)/$(BIN_NAME)

test: $(BIN_DIR)/$(BIN_NAME)
	$(BIN_DIR)/$(BIN_NAME)

val_test: $(BIN_DIR)/$(BIN_NAME) .valgrind.supp
	valgrind --suppressions=.valgrind.supp --leak-check=full --track-origins=yes $(BIN_DIR)/$(BIN_NAME)

$(BIN_DIR)/$(BIN_NAME): $(ALL_OBJS) Makefile
	mkdir -p $(dir $@)
	$(CXX) $(ALL_OBJS) $(LD_FLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc $(ALL_SOURCES) Makefile
	mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.cc $(ALL_TESTS) Makefile
	mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

$(UDNS_LIB): $(UDNS_MAKEFILE)
	$(MAKE) -C $(UDNS_PATH)

$(UDNS_MAKEFILE): $(UDNS_CONFIGURE)
	$(UDNS_CONFIGURE)

clean:
	rm -rf $(BIN_DIR)
