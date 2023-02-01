CC = gcc
CC_FLAGS = -Wall -Wpedantic -Wextra
VALGRIND_FLAGS = --quiet --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=3 --error-exitcode=1

all: dumputmp watch

clean:
	rm -f dumputmp myWatch

dumputmp: dumputmp.c
	$(CC) $(CC_FLAGS) -o dumputmp dumputmp.c

watch: watch.c utmplib.h
	$(CC) $(CC_FLAGS) -o myWatch watch.c

run:
	@echo 'valgrind ./myWatch `whoami` reboot'
	@valgrind $(VALGRIND_FLAGS) ./myWatch `whoami` reboot
