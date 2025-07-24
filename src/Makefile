UT_VERBOSE ?= 0

all: prebuild build

prebuild:
	gcc -c lib/mongoose.c -o lib/mongoose.o
	gcc -c lib/sds.c -o lib/sds.o

build:
	gcc mafia.c lib/log.c lib/mongoose.o lib/sds.o -o mafia -lunistring -I./lib

run: build
	./mafia

tests: build
	gcc -c mafia.c -o mafia.o -I./lib -DUNIT_TESTING
	gcc tests.c mafia.o lib/mongoose.o lib/sds.o -o tests -lunistring -lpthread -I./lib -DVERBOSE=$(UT_VERBOSE)
	./tests
