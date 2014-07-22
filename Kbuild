#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#



ifdef CONFIG_KERNEL_STABLE
libsel4platsupport-y: libsel4simple-stable
else
libsel4platsupport-y: libsel4simple-default
endif
libsel4platsupport: libutils libsel4utils libsel4 libsel4vspace libsel4simple libplatsupport libsel4platsupport-y $(libc) common
libs-$(CONFIG_LIB_SEL4_PLAT_SUPPORT) += libsel4platsupport
