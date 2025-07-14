all:
	gcc mafia.c mongoose.c -o mafia -lunistring
	./mafia
