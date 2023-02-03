CC = gcc
CC_FLAGS = -std=c99 -Wall -Wpedantic -Wextra -fsanitize=undefined
VALGRIND_FLAGS = --quiet --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=3 --error-exitcode=1

all: dumputmp mywatch

clean:
	rm -f dumputmp mywatch

dumputmp: dumputmp.c
	$(CC) $(CC_FLAGS) -o dumputmp dumputmp.c

mywatch: watch.c utmplib.h
	$(CC) $(CC_FLAGS) -o mywatch watch.c

run:
	@echo 'valgrind ./mywatch `whoami` reboot'
	@valgrind $(VALGRIND_FLAGS) ./mywatch 5 `whoami` reboot
