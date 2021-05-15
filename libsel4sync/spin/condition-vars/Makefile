#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

all: safety liveness

pan.c: cv.pml
	spin -a $<

pan-safety: pan.c
	gcc $< -DREACH -DSAFETY -o $@

pan-ltl: pan.c
	gcc $< -DREACH -o $@

pan-liveness: pan.c
	gcc $< -DNP -DNOREDUCE -o $@

safety: pan-safety pan-ltl
	./pan-ltl -m1000 -a -N broadcast0 | tee /dev/stderr | grep -q 'errors: 0'
	./pan-ltl -m1000 -a -N broadcast1 | tee /dev/stderr | grep -q 'errors: 0'
	./pan-ltl -m1000 -a -N broadcast2 | tee /dev/stderr | grep -q 'errors: 0'
	./pan-ltl -m1000 -a -N broadcast3 | tee /dev/stderr | grep -q 'errors: 0'
	./pan-safety -m10000 | tee /dev/stderr | grep -q 'errors: 0'
	touch safety

liveness: pan-liveness
	./pan-liveness -l -m100000 | tee /dev/stderr | grep -q 'errors: 0'
	touch liveness

clean:
	rm -f pan-safety
	rm -f pan-liveness
	rm -f pan-ltl
	rm -f safety
	rm -f liveness
	rm -f *.trail
	rm -f pan.*
