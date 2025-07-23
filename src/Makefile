all: prebuild build

prebuild:
	gcc -c lib/mongoose.c -o lib/mongoose.o
	gcc -c lib/sds.c -o lib/sds.o

build:
	gcc mafia.c lib/mongoose.o lib/sds.o -o mafia -lunistring -I./lib

run: build
	./mafia

tests: build
	gcc ut_backend.c lib/mongoose.o lib/sds.o -o ut_backend -lpthread -I./lib
	#./mafia > /dev/null 2>&1 &
	./ut_backend
