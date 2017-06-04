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
TARGETS := libsel4muslcsys.a

# Source files required to build the target
CFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c) $(wildcard $(SOURCE_DIR)/src/arch/${ARCH}/*.c))

# Header files/directories this library provides
HDRFILES := $(wildcard $(SOURCE_DIR)/include/*)

MUSLC_HIGHEST_SYSCALL := $(shell cat $(STAGE_DIR)/include/bits/syscall.h | sed 's/^.*[^0-9]\([0-9]*\)$$/\1/' | sort -nr | head -1)

NK_CFLAGS += -D_XOPEN_SOURCE=700 -DMUSLC_HIGHEST_SYSCALL=$(MUSLC_HIGHEST_SYSCALL)

include $(SEL4_COMMON)/common.mk
