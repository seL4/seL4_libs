#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

# Targets
TARGETS := libsel4bench.a

# Source files required to build the target
CFILES := \
	$(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c)) \
	$(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/$(ARCH)/*.c)) \
	$(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/$(ARCH)/armv/$(ARMV)/*.c)) \
	$(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/$(ARCH)/cpu/$(CPU)/*.c))

# Header files/directories this library provides
# Note: sel4_client.h may not have been built at the time this is evaluated.
HDRFILES := \
	$(wildcard $(SOURCE_DIR)/include/*) \
	$(wildcard $(SOURCE_DIR)/arch_include/$(ARCH)/*) \
	$(wildcard $(SOURCE_DIR)/arch_include/$(ARCH)/armv/$(ARMV)/*) \
	$(wildcard $(SOURCE_DIR)/arch_include/$(ARCH)/cpu/$(CPU)/*) \

# Extra include directory for magpie to find type.h and sel4arch.idl4
INCLUDE_DIRS := include arch_include/$(ARCH)

include $(SEL4_COMMON)/common.mk

