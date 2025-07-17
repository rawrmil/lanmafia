all: prebuild build

prebuild:
	gcc -c mongoose.c -o mongoose.o
	gcc -c sds.c -o sds.o

build:
	gcc mafia.c mongoose.o sds.o -o mafia -lunistring
	./mafia
