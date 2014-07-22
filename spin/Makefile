#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

all: safety liveness

pan.c: sem.promela
	spin -a $<

pan-safety: pan.c
	gcc $< -DREACH -DSAFETY -o $@

pan-liveness: pan.c
	gcc $< -DNP -DNOREDUCE -o $@

safety: pan-safety
	./pan-safety | tee /dev/stderr | grep -q 'errors: 0'
	touch safety

liveness: pan-liveness
	./pan-liveness -l -m100000 | tee /dev/stderr | grep -q 'errors: 0'
	touch liveness

clean:
	rm -f pan-safety
	rm -f pan-liveness
	rm -f safety
	rm -f liveness
	rm -f *.trail
	rm -f pan.*

