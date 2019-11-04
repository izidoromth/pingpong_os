STD = -std=c90

OPTIONS = -Wall -Wpedantic -Wno-unused-result -O0 -g

run: datatypes.h pingpong.h pingpong.c pingpong-barrier.c pingpong-barrier.c queue.h queue.c
	gcc $(OPTIONS) datatypes.h pingpong.h pingpong.c pingpong-barrier.c queue.h queue.c -o run
