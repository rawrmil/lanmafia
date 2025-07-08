all:
	gcc mafia.c mongoose.c -o mafia -ljansson
	./mafia
