#
# @file    tools/sdk/c/Makefile
# @author  Luke Tokheim, luke@motionnode.com
# @version 2.0
#
CC=gcc

sdk_main: main.o
	$(CC) -o $@ $<

main.o: main.c
	$(CC) -O3 -Wall -pedantic -ansi -c $< -o $@ -I.

clean:
	rm -f main.o sdk_main
