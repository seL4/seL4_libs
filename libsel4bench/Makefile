#
# Copyright 2017, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

# Targets
TARGETS := libsel4bench.a

# Source files required to build the target
CFILES := \
	$(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c))) \
	$(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch/$(ARCH)/*.c))) \
	$(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch/$(ARCH)/armv/$(ARMV)/*.c))) \
	$(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch/$(ARCH)/cpu/$(CPU)/*.c)))

# Header files/directories this library provides
HDRFILES := \
	$(sort $(wildcard $(SOURCE_DIR)/include/*)) \
	$(sort $(wildcard $(SOURCE_DIR)/arch_include/$(ARCH)/*)) \
	$(sort $(wildcard $(SOURCE_DIR)/arch_include/$(ARCH)/armv/$(ARMV)/*)) \
	$(sort $(wildcard $(SOURCE_DIR)/arch_include/$(ARCH)/cpu/$(CPU)/*)) \
	$(sort $(wildcard $(SOURCE_DIR)/sel4_arch_include/$(SEL4_ARCH)/*))

include $(SEL4_COMMON)/common.mk
