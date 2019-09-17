STD = -std=c99

OPTIONS = -Wall -Wpedantic -Wno-unused-result -O0 -g

run: datatypes.h pingpong.h pingpong.c pingpong-tasks2.c
	gcc $(STD) $(OPTIONS) datatypes.h pingpong.h pingpong.c pingpong-tasks2.c -o run
