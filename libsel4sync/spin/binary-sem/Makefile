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

all: safety

PROMELA=binary-sem.pml

pan.c: ${PROMELA}
	spin -a $<

pan: pan.c
	gcc -O2 $< -DREACH -o $@

.PHONY: safety
safety: pan
	./pan -N cansend -a -m500000 | tee /dev/stderr | grep -q 'errors: 0'
	./pan -N mutex -a -m500000 | tee /dev/stderr | grep -q 'errors: 0'
	./pan -N liveness -a -m500000 | tee /dev/stderr | grep -q 'errors: 0'

clean:
	rm -f pan
	rm -f ${PROMELA}.trail
	rm -f pan.*
	rm -f _spin_nvr.tmp
