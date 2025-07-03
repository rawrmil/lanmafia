#!/bin/sh

ls -d * | entr sh -c 'make && ./mafia'
