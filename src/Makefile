UT_DEBUG ?= 0

LIB=lib/log.c lib/mongoose.o lib/sds.o -lunistring
INC=-I./lib -I./lib/klib -I./lib/freebsd

all: prebuild build

prebuild:
	gcc -c lib/mongoose.c -o lib/mongoose.o
	gcc -c lib/sds.c -o lib/sds.o

build:
	gcc mafia.c -o mafia $(LIB) $(INC)

run: build
	./mafia

tests: build
	gcc -c mafia.c -o mafia.o $(INC) -DUNIT_TESTING
	gcc tests.c mafia.o -o tests $(LIB) $(INC) -lpthread -DDEBUG=$(UT_DEBUG)
	./tests
