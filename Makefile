SRC_DIR = ./src
OBJ_DIR = ./bin/obj
BIN_DIR = ./bin

# We use c++14 extensions, so we need the latest versions of the compilers
ifndef $(CXX)
  CXX = g++-4.9
endif

ifeq ($(CXX),gcc)
  CXX = g++-4.9
endif
ifeq ($(CXX),g++)
  CXX = g++-4.9
endif
ifeq ($(CXX),clang)
  CXX = clang++-3.5
endif
ifeq ($(CXX),clang++)
  CXX = clang++-3.5
endif

CXX_FLAGS = -std=c++14 -I$(SRC_DIR) -g -Wall -Wextra -Werror
CXX_FLAGS += -Wnon-virtual-dtor -Wno-deprecated-declarations
CXX_FLAGS += -Wformat=2 -Wswitch-enum -Wswitch-default
CXX_FLAGS += -Wundef -Wvla -Wshadow -Wmissing-noreturn
#CXX_FLAGS += -Wconditional-uninitialized -Wused-but-marked-unused
ifneq ($(DEBUG),1)
  RT_CXXFLAGS += -O3 -DNDEBUG
  #RT_CXXFLAGS += -fno-strict-aliasing
endif

LD_FLAGS = -lstdc++ -static-libgcc
BIN_NAME = rpc_test

ALL_SOURCES := $(shell find $(SRC_DIR) -name '*.cc' -not -name '\.*')
ALL_OBJS := $(patsubst $(SRC_DIR)/%.cc,$(OBJ_DIR)/%.o,$(ALL_SOURCES))

all: $(BIN_DIR)/$(BIN_NAME)

$(BIN_DIR)/$(BIN_NAME): $(ALL_OBJS) Makefile
	mkdir -p $(dir $@)
	$(CXX) $(ALL_OBJS) $(LD_FLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc $(ALL_SOURCES) Makefile
	mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

clean:
	rm -rf $(BIN_DIR)
