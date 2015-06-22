CXX ?= g++

ifeq ($(CXX),gcc)
  override CXX = g++
else
  ifeq ($(CXX),clang)
    override CXX = clang++
  endif
endif

# We use c++14 extensions, so force us to use a compiler version with support
# TODO: make this more portable
ifeq ($(CXX),g++)
  override CXX = g++-4.9
  CXX_FLAGS =
  COMPILER_SUFFIX = gcc4.9
else
  ifeq ($(CXX),clang++)
    override CXX = clang++-3.5
    CXX_FLAGS = -Wconditional-uninitialized -Wused-but-marked-unused
    COMPILER_SUFFIX = clang3.5
  endif
endif

ifeq ($(DEBUG),1)
else
  CXX_FLAGS += -O3 -DNDEBUG
endif

SRC_DIR = src
TEST_DIR = test
OBJ_DIR = bin/obj_$(COMPILER_SUFFIX)
TEST_OBJ_DIR = bin/test_obj_$(COMPILER_SUFFIX)
BIN_DIR = bin
EXT_DIR = external

UDNS_PATH = $(EXT_DIR)/udns-0.4
UDNS_CONFIGURE = $(UDNS_PATH)/configure
UDNS_MAKEFILE = $(UDNS_PATH)/Makefile
UDNS_LIB_FILENAME = libudns.a
UDNS_LIB = $(UDNS_PATH)/$(UDNS_LIB_FILENAME)

CXX_FLAGS += -std=c++14 -I$(SRC_DIR) -I$(TEST_DIR) -Wall -Wextra -Werror
CXX_FLAGS += $(addprefix -I,$(UDNS_PATH))
CXX_FLAGS += -Wnon-virtual-dtor -Wno-deprecated-declarations
CXX_FLAGS += -Wformat=2 -Wswitch-enum
CXX_FLAGS += -Wundef -Wvla -Wshadow -Wmissing-noreturn
CXX_FLAGS += -gdwarf-3 -fdata-sections -ffunction-sections
CXX_FLAGS += -D__STDC_FORMAT_MACROS -DINDECOROUS_STRICT

LD_FLAGS = -lstdc++ -Wl,--gc-sections -lpthread -lrt
BIN_NAME = coro_test_$(COMPILER_SUFFIX)

ALL_SOURCES := $(shell find $(SRC_DIR) -name '*.cc' -not -name '\.*')
ALL_TESTS := $(shell find $(TEST_DIR) -name '*.cc' -not -name '\.*')
SRC_OBJS := $(patsubst $(SRC_DIR)/%.cc,$(OBJ_DIR)/%.o,$(ALL_SOURCES))
SRC_DEPS := $(patsubst $(SRC_DIR)/%.cc,$(OBJ_DIR)/%.d,$(ALL_SOURCES))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cc,$(TEST_OBJ_DIR)/%.o,$(ALL_TESTS))
TEST_DEPS := $(patsubst $(TEST_DIR)/%.cc,$(TEST_OBJ_DIR)/%.d,$(ALL_TESTS))
EXTERNAL_OBJS := $(UDNS_LIB)
ALL_OBJS := $(SRC_OBJS) $(TEST_OBJS) $(EXTERNAL_OBJS)

.PHONY: all test val_test clean
all: $(BIN_DIR)/$(BIN_NAME)

test: $(BIN_DIR)/$(BIN_NAME)
	$(BIN_DIR)/$(BIN_NAME)

val_test: $(BIN_DIR)/$(BIN_NAME) .valgrind.supp
	valgrind --suppressions=.valgrind.supp --leak-check=full --track-origins=yes $(BIN_DIR)/$(BIN_NAME)

clean:
	rm -rf $(BIN_DIR)
	@$(MAKE) -C $(UDNS_PATH) clean

-include $(SRC_DEPS)
-include $(TEST_DEPS)

$(TEST_OBJ_DIR)/%.d: $(TEST_DIR)/%.cc
	@mkdir -p $(dir $@)
	@$(CXX) $(CXX_FLAGS) -MM -MT '$(patsubst $(TEST_DIR)/%.cc,$(TEST_OBJ_DIR)/%.o,$<)' $< -MF $@

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.cc
	@mkdir -p $(dir $@)
	@$(CXX) $(CXX_FLAGS) -MM -MT '$(patsubst $(SRC_DIR)/%.cc,$(OBJ_DIR)/%.o,$<)' $< -MF $@

$(BIN_DIR)/$(BIN_NAME): $(ALL_OBJS) Makefile
	@echo "  $(CXX) $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(ALL_OBJS) $(LD_FLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc $(OBJ_DIR)/%.d Makefile
	@echo "  $(CXX) $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXX_FLAGS) -c -o $@ $<

$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.cc $(TEST_OBJ_DIR)/%.d Makefile
	@echo "  $(CXX) $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXX_FLAGS) -c -o $@ $<

$(UDNS_LIB): $(UDNS_MAKEFILE)
	@echo "  $(MAKE) $@"
	@$(MAKE) -C $(UDNS_PATH) $(UDNS_LIB_FILENAME)

$(UDNS_MAKEFILE): $(UDNS_CONFIGURE)
	$(UDNS_CONFIGURE)
