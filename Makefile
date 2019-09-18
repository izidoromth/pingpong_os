STD = -std=c99

OPTIONS = -Wall -DDEBUG -Wpedantic -Wno-unused-result -O0 -g

run: datatypes.h pingpong.h pingpong.c pingpong-dispatcher.c queue.h queue.c
	gcc $(STD) $(OPTIONS) datatypes.h pingpong.h pingpong.c pingpong-dispatcher.c queue.h queue.c -o run
