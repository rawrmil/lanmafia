#!/bin/sh

make build
./mafia -D > ./mafia.log 2>&1 &
make tests
pkill mafia
