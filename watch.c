// CS4-53203: Systems Programming
// Name: V Wheatley
// Date: 2023-01-30
// Assignment2.txt

// C stuff
#include <stdlib.h> // -> EXIT_*
#include <stdio.h> // buffered I/O

#include <string.h> // -> str*, mem*
#include <ctype.h> // -> is*
#include <stdbool.h> // -> bool

#include <limits.h> // -> ULONG_MAX
#include <time.h>

#include <errno.h>

// Linux stuff
#include <unistd.h> // syscalls
#include <fcntl.h> // file flags

struct WatchedUser {
	char *name; // stolen from argv, probably fine
	bool present;
};

int main(int argc, char *argv[]) {
	unsigned int pollTime = 300; // in seconds
	
	if (argc <= 1) {
		char *name = (argc > 0) ? argv[0] : "watch";
		fprintf(stderr,
			"Usage: %s [pollTime (%d)] lognames...\n"
			"- poll time is given in seconds\n"
			"- lognames are those seen in who and utmp\n",
			name, pollTime
		);
		exit(EXIT_FAILURE);
	}
	
	size_t namesStart = 2; // if argv[1] is pollTime (2) or not (1)
	
	char *firstBad = NULL;
	unsigned long pollTimeL = strtoul(argv[1], &firstBad, 0);
	if (*firstBad != '\0') {
		fprintf(stderr, "bleep bleep well it's not a poll time\n");
		namesStart = 1; // this is not a poll time
	} else {
		fprintf(stderr, "bleep bleep got a poll time %ld\n", pollTimeL);
		pollTime = (unsigned int)pollTimeL; // atoi is a crime against humanity
	}
	
	if (namesStart >= (size_t)argc) {
		fprintf(stderr, "jeez, at least give me a single logname..\n");
		exit(EXIT_FAILURE);
	}
	
	size_t usersLen = argc - namesStart;
	struct WatchedUser *users = malloc(sizeof(struct WatchedUser) * usersLen);
	
	for (size_t i = 0; i < usersLen; i++) {
		users[i].name = argv[i + namesStart];
		users[i].present = false;
	}
	
	// TODO: initial "well this user is already here" note
	
	// TODO: delay - read - check loop..
	
	free(users);
	
	return EXIT_SUCCESS;
}
