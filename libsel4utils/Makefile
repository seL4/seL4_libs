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
TARGETS := libsel4utils.a

# Source files required to build the target
CFILES := $(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c)))
CFILES += $(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/vspace/*.c)))
CFILES += $(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/irq_server/*.c)))
CFILES += $(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/serial_server/*.c)))
CFILES += $(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/time_server/*.c)))
CFILES += $(sort $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/sel4_arch/${SEL4_ARCH}/*.c)))

# Header files/directories this library provides
HDRFILES := $(sort $(wildcard $(SOURCE_DIR)/include/*))
HDRFILES += $(sort $(wildcard $(SOURCE_DIR)/arch_include/$(ARCH)/*))
HDRFILES += $(sort $(wildcard $(SOURCE_DIR)/sel4_arch_include/$(SEL4_ARCH)/*))

include $(SEL4_COMMON)/common.mk
