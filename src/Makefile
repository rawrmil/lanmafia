all: prebuild build

prebuild:
	gcc -c lib/mongoose.c -o lib/mongoose.o
	gcc -c lib/sds.c -o lib/sds.o

build:
	gcc mafia.c lib/mongoose.o lib/sds.o -o mafia -lunistring -I./lib
