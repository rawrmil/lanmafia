#!/bin/sh

make build
./mafia -D &
make tests
pkill ./mafia
