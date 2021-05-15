#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
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
