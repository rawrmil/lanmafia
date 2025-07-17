#!/bin/sh

ls *.c Makefile | entr -r make build && ./mafia
