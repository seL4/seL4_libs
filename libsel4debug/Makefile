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
TARGETS := $(notdir ${SOURCE_DIR}).a

# Source files required to build the target
CFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/arch/$(ARCH)/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/plat/$(PLAT)/*.c))

ASMFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/arch/$(ARCH)/*.S))

# Header files/directories this library provides
HDRFILES := \
    $(wildcard ${SOURCE_DIR}/include/*)
HDRFILES += $(wildcard ${SOURCE_DIR}/arch_include/$(ARCH)/*)
HDRFILES += $(wildcard ${SOURCE_DIR}/sel4_arch_include/$(SEL4_ARCH)/*)

include $(SEL4_COMMON)/common.mk
