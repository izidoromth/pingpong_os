STD = -std=c90

OPTIONS = -Wall -Wpedantic -Wno-unused-result -O0 -g

run: datatypes.h pingpong.h pingpong.c pingpong-semaphore.c pingpong-racecond.c queue.h queue.c
	gcc $(OPTIONS) datatypes.h pingpong.h pingpong.c pingpong-semaphore.c queue.h queue.c -o run_sem
	gcc $(OPTIONS) datatypes.h pingpong.h pingpong.c pingpong-racecond.c queue.h queue.c -o run_race
