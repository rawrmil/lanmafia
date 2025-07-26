#!/bin/sh

[ $1 = 'run' ] && ls *.c Makefile | entr -r make run
[ $1 = 'tests' ] && ls *.c Makefile | entr -r make tests UT_VERBOSE=1
