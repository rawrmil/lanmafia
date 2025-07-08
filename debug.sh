#!/bin/sh

ls *.c | entr -r make && ./mafia
