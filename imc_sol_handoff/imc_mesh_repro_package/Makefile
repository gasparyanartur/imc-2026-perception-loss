CXX ?= g++
CXXFLAGS ?= -std=c++17 -O3 -DNDEBUG -march=native
ROOT := $(abspath .)
BUILD := $(ROOT)/build

CURRENT := $(BUILD)/v33_t7
V23 := $(BUILD)/v23
V33 := $(BUILD)/v33
PINEAPPLE := $(BUILD)/pineapple_v072
EVAL := $(BUILD)/imc_proxy_eval

.PHONY: all clean fast full tier-small tier-huge manifest

all: $(CURRENT) $(V23) $(V33) $(PINEAPPLE) $(EVAL)

$(BUILD):
	mkdir -p $@

$(CURRENT): solvers/current/nebula_atomic_region_v33_t7.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

$(V23): solvers/baselines/nebula_ceiling_mix_v23.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

$(V33): solvers/baselines/nebula_atomic_region_v33.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

$(PINEAPPLE): solvers/baselines/gasparyanartur-pineapple-v072-20260711004207-001.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

$(EVAL): tools/imc_proxy_eval.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

fast: all
	bash scripts/run_fast_suite.sh

full: all
	bash scripts/run_full_suite.sh

tier-small: all
	bash scripts/run_tier_regression.sh

tier-huge: all
	bash scripts/run_tier_regression.sh --include-huge

manifest:
	bash scripts/make_manifest.sh

clean:
	rm -rf $(BUILD) generated/run_* generated/tier_* results/reproduced_*
