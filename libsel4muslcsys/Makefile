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
TARGETS := libsel4muslcsys.a

# Source files required to build the target
CFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c) $(wildcard $(SOURCE_DIR)/src/arch/${ARCH}/*.c))

# Header files/directories this library provides
HDRFILES := $(wildcard $(SOURCE_DIR)/include/*)

NK_CFLAGS += -D_XOPEN_SOURCE=700

include $(SEL4_COMMON)/common.mk
