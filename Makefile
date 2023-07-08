# Copyright (C) 2023 Advanced Micro Devices, Inc
#
# SPDX-License-Identifier: MIT

PLATFORM := xilinx_vck5000_gen4x8_qdma_2_202220_1
TARGET := hw
#TARGET := x86sim
FREQ := 250

XPFM = $(shell platforminfo -p $(PLATFORM) --json="file")
XSA = $(strip $(patsubst %.xpfm, % , $(shell basename $(PLATFORM))))
#DSPLIB_ROOT = $(shell readlink -f ../../Vitis_Libraries/dsp)

# OUTPUT PRODUCTS 
BUILD_DIR = build.$(TARGET)
WORK_DIR = Work
SRC_DIR = $(shell readlink -f src/)
DATA_DIR = $(shell readlink -f data/)
#CONSTRAINTS_DIR = $(shell readlink -f constraints/)

# DEPENDENCIES for make aie
GRAPH_CPP := $(SRC_DIR)/graph.cpp
DEPS := $(GRAPH_CPP)
DEPS += $(SRC_DIR)/graph.h
DEPS += $(SRC_DIR)/kernels.h
DEPS += $(SRC_DIR)/xf_filter2d_aie.hpp
DEPS += $(SRC_DIR)/xf_filter2d.cc
# Add your own dependencies

AIE_FLAGS = --platform=$(XPFM)
#AIE_FLAGS += -include=$(DSPLIB_ROOT)/L1/include/aie
#AIE_FLAGS += -include=$(DSPLIB_ROOT)/L1/src/aie
#AIE_FLAGS += -include=$(DSPLIB_ROOT)/L1/tests/aie/inc
#AIE_FLAGS += -include=$(DSPLIB_ROOT)/L1/tests/aie/src
#AIE_FLAGS += -include=$(DSPLIB_ROOT)/L2/include/aie
#AIE_FLAGS += -include=$(DSPLIB_ROOT)/L2/tests/aie/common/inc


#AIE_FLAGS += --pl-freq=$(FREQ)
#AIE_FLAGS += --dataflow
AIE_FLAGS += --Xchess="main:darts.xargs=-nb"

all: $(BUILD_DIR)/libadf.a

$(BUILD_DIR)/libadf.a: $(DEPS)
	@mkdir -p $(BUILD_DIR);
	cd $(BUILD_DIR); \
	aiecompiler -v --target=$(TARGET) \
		--stacksize=2000 \
		-include="$(XILINX_VITIS)/aietools/include" \
		-include="$(SRC_DIR)"  \
		-include="$(DATA_DIR)" \
		$(AIE_FLAGS) \
		$(GRAPH_CPP) \
		-workdir=$(WORK_DIR) 2>&1 | tee aiecompiler.log

clean:
	rm -rf $(BUILD_DIR) *.log *.jou

aieemu:
	cd $(BUILD_DIR); \
	aiesimulator --pkg-dir=$(WORK_DIR) --i=.. --profile #--dump-vcd=foo

x86sim:
	cd $(BUILD_DIR); \
	x86simulator --pkg-dir=$(WORK_DIR) --i=..
