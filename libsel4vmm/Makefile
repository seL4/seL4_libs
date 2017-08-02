#
# Copyright 2017, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the GNU General Public License version 2. Note that NO WARRANTY is provided.
# See "LICENSE_GPLv2.txt" for details.
#
# @TAG(DATA61_GPL)
#

# Targets
TARGETS := libsel4vmm.a

# Source files required to build the target
CFILES := $(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/vmm/*.c)))
CFILES += $(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/platform/*.c)))
CFILES += $(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/driver/*.c)))
CFILES += $(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/manager/*.c)))
CFILES += $(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/processor/*.c)))

# Header files/directories this library provides
HDRFILES := $(sort $(wildcard $(SOURCE_DIR)/include/*))

CFLAGS += -std=gnu99 # -Wall -Werror

include $(SEL4_COMMON)/common.mk
