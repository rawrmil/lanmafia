#!/bin/sh

[ $1 = 'run' ] && ls *.c Makefile | entr -r make run
[ $1 = 'tests' ] && ls tests/*.c Makefile | entr -r make tests
