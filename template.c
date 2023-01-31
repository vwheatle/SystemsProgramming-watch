// CS4-53203: Systems Programming
// Name: V Wheatley
// Date: 2023-01-30
// Assignment2.txt

// C stuff
#include <stdlib.h> // -> EXIT_*
#include <stdio.h> // buffered I/O

#include <string.h> // -> str*, mem*
#include <stdbool.h> // -> bool

// Linux stuff
#include <unistd.h> // syscalls
#include <fcntl.h> // file flags

int main(int argc, char *argv[]) {
	if (argc == 1) {
		fprintf(stderr, "Usage: %s some thing\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	return EXIT_SUCCESS;
}
