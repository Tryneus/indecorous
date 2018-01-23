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
  override CXX = g++-5
  CXX_FLAGS =
  LD_FLAGS =
  COMPILER_SUFFIX = gcc5
else
  ifeq ($(CXX),clang++)
    override CXX = clang++-5.0
    CXX_FLAGS = -Wconditional-uninitialized -Wused-but-marked-unused
    LD_FLAGS =
    COMPILER_SUFFIX = clang
  endif
endif

ifeq ($(DEBUG),1)
  BUILD_TYPE = $(join $(COMPILER_SUFFIX),_debug)
else
  BUILD_TYPE = $(join $(COMPILER_SUFFIX),_release)
  CXX_FLAGS += -O3 -DNDEBUG
#  LD_FLAGS += -flto
endif

SRC_DIR = src
TEST_DIR = test
BENCH_DIR = bench
OBJ_DIR = bin/obj
CORE_OBJ_DIR = $(OBJ_DIR)/core_$(BUILD_TYPE)
TEST_OBJ_DIR = $(OBJ_DIR)/test_$(BUILD_TYPE)
BENCH_OBJ_DIR = $(OBJ_DIR)/bench_$(BUILD_TYPE)
BIN_DIR = bin
EXT_DIR = external

UDNS_PATH = $(EXT_DIR)/udns-0.4
UDNS_MAKEFILE = $(UDNS_PATH)/Makefile
UDNS_LIB_FILE = libudns.a
UDNS_LIB = $(UDNS_PATH)/$(UDNS_LIB_FILE)

CATCH_PATH = $(EXT_DIR)/catch

CXX_FLAGS += -std=c++14 -I$(SRC_DIR) -I$(TEST_DIR) -I$(BENCH_DIR)
CXX_FLAGS += $(addprefix -I,$(UDNS_PATH) $(CATCH_PATH))
CXX_FLAGS += -Wall -Wextra -Werror -Weffc++
CXX_FLAGS += -Wnon-virtual-dtor -Wno-deprecated-declarations
CXX_FLAGS += -Wformat=2 -Wformat-nonliteral -Wformat-security
CXX_FLAGS += -Wswitch-enum -Wswitch-default -Wundef -Wvla -Wshadow
CXX_FLAGS += -Wuninitialized -Wmissing-include-dirs -Wmissing-noreturn
CXX_FLAGS += -Wunused-parameter -Wstrict-aliasing
#CXX_FLAGS += -Wpedantic
CXX_FLAGS += -gdwarf-3 -fdata-sections -ffunction-sections -fno-rtti
CXX_FLAGS += -D__STDC_FORMAT_MACROS

LD_FLAGS += -lstdc++ -Wl,--gc-sections -lpthread -lrt

TEST_BIN = coro_test_$(BUILD_TYPE)
BENCH_BIN = coro_bench_$(BUILD_TYPE)

ALL_SOURCES := $(shell find $(SRC_DIR) -name '*.cc' -not -name '\.*')
SRC_OBJS := $(patsubst $(SRC_DIR)/%.cc,$(CORE_OBJ_DIR)/%.o,$(ALL_SOURCES))
SRC_DEPS := $(patsubst $(SRC_DIR)/%.cc,$(CORE_OBJ_DIR)/%.d,$(ALL_SOURCES))

ALL_TESTS := $(shell find $(TEST_DIR) -name '*.cc' -not -name '\.*')
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cc,$(TEST_OBJ_DIR)/%.o,$(ALL_TESTS))
TEST_DEPS := $(patsubst $(TEST_DIR)/%.cc,$(TEST_OBJ_DIR)/%.d,$(ALL_TESTS))

ALL_BENCHES := $(shell find $(BENCH_DIR) -name '*.cc' -not -name '\.*')
BENCH_OBJS := $(patsubst $(BENCH_DIR)/%.cc,$(BENCH_OBJ_DIR)/%.o,$(ALL_BENCHES))
BENCH_DEPS := $(patsubst $(BENCH_DIR)/%.cc,$(BENCH_OBJ_DIR)/%.d,$(ALL_BENCHES))

EXTERNAL_OBJS := $(UDNS_LIB)
ALL_TEST_OBJS := $(SRC_OBJS) $(TEST_OBJS) $(EXTERNAL_OBJS)
ALL_BENCH_OBJS := $(SRC_OBJS) $(BENCH_OBJS) $(EXTERNAL_OBJS)

.PHONY: all test val_test clean
all: $(BIN_DIR)/$(TEST_BIN) $(BIN_DIR)/$(BENCH_BIN)

test: $(BIN_DIR)/$(TEST_BIN)
	$<

bench: $(BIN_DIR)/$(BENCH_BIN)
	$<

val_test: $(BIN_DIR)/$(TEST_BIN) .valgrind.supp
	valgrind --suppressions=.valgrind.supp --leak-check=full --track-origins=yes $<

val_bench: $(BIN_DIR)/$(BENCH_BIN) .valgrind.supp
	valgrind --suppressions=.valgrind.supp --leak-check=full --track-origins=yes $<

clean:
	rm -rf $(BIN_DIR)
	@$(MAKE) -C $(UDNS_PATH) clean

-include $(SRC_DEPS)
-include $(TEST_DEPS)

$(BIN_DIR)/$(TEST_BIN): $(ALL_TEST_OBJS) Makefile
	@echo "  $(CXX) $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(ALL_TEST_OBJS) $(LD_FLAGS) -o $@

$(BIN_DIR)/$(BENCH_BIN): $(ALL_BENCH_OBJS) Makefile
	@echo "  $(CXX) $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(ALL_BENCH_OBJS) $(LD_FLAGS) -o $@

$(CORE_OBJ_DIR)/%.d: $(SRC_DIR)/%.cc
	@mkdir -p $(dir $@)
	@$(CXX) $(CXX_FLAGS) -MM -MT '$(patsubst $(SRC_DIR)/%.cc,$(CORE_OBJ_DIR)/%.o,$<)' $< -MF $@

$(TEST_OBJ_DIR)/%.d: $(TEST_DIR)/%.cc
	@mkdir -p $(dir $@)
	@$(CXX) $(CXX_FLAGS) -MM -MT '$(patsubst $(TEST_DIR)/%.cc,$(TEST_OBJ_DIR)/%.o,$<)' $< -MF $@

$(BENCH_OBJ_DIR)/%.d: $(BENCH_DIR)/%.cc
	@mkdir -p $(dir $@)
	@$(CXX) $(CXX_FLAGS) -MM -MT '$(patsubst $(BENCH_DIR)/%.cc,$(BENCH_OBJ_DIR)/%.o,$<)' $< -MF $@

$(CORE_OBJ_DIR)/%.o: $(SRC_DIR)/%.cc $(CORE_OBJ_DIR)/%.d Makefile
	@echo "  $(CXX) $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXX_FLAGS) -c -o $@ $<

$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.cc $(TEST_OBJ_DIR)/%.d Makefile
	@echo "  $(CXX) $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXX_FLAGS) -c -o $@ $<

$(BENCH_OBJ_DIR)/%.o: $(BENCH_DIR)/%.cc $(BENCH_OBJ_DIR)/%.d Makefile
	@echo "  $(CXX) $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXX_FLAGS) -c -o $@ $<

$(UDNS_LIB): $(UDNS_MAKEFILE)
	@echo "  $(MAKE) $@"
	@$(MAKE) -C $(UDNS_PATH) $(UDNS_LIB_FILE)

$(UDNS_MAKEFILE): $(UDNS_PATH)/configure
	@echo "  configure $@"
	cd $(UDNS_PATH); ./configure
