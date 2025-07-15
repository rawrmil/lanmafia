all:
	gcc mafia.c mongoose.c sds.c -o mafia -lunistring
	./mafia
