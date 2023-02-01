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

#include "utmplib.h"

struct WatchedUser {
	char *name; // stolen from argv, probably fine
	bool lastPresent, nowPresent;
};

void updateWatchedUsers(struct WatchedUser *users, size_t usersLen) {
	if (utmp_open(NULL) == -1) {
		perror("initial utmp");
		exit(EXIT_FAILURE);
	}
	
	for (size_t i = 0; i < usersLen; i++) {
		users[i].lastPresent = users[i].nowPresent;
		users[i].nowPresent = false;
	}
	
	struct utmp *record;
	while ((record = utmp_next()) != NULLUT)
		for (size_t i = 0; i < usersLen; i++)
			if (strncmp(record->ut_user, users[i].name, UT_NAMESIZE) == 0)
				users[i].nowPresent = true;
	
	// clean up after yourself
	utmp_close();
}

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
	
	// DETERMINE IF POLL TIME WAS SPECIFIED
	
	size_t usersStart = 2; // if argv[1] is pollTime (2) or not (1)
	
	char *firstBad = NULL;
	unsigned long pollTimeL = strtoul(argv[1], &firstBad, 0);
	if (*firstBad != '\0') {
		fprintf(stderr, "bleep bleep well it's not a poll time\n");
		usersStart = 1; // this is not a poll time
	} else {
		fprintf(stderr, "bleep bleep got a poll time %ld\n", pollTimeL);
		pollTime = (unsigned int)pollTimeL; // atoi is a crime against humanity
	}
	
	// INITIALIZE LIST OF USERS
	
	if (usersStart >= (size_t)argc) {
		fprintf(stderr, "jeez, at least give me a single logname..\n");
		exit(EXIT_FAILURE);
	}
	
	size_t usersLen = argc - usersStart;
	struct WatchedUser *users = malloc(sizeof(struct WatchedUser) * usersLen);
	
	fprintf(stderr, "bleep bleep specified %ld users..\n", usersLen);
	
	for (size_t i = 0; i < usersLen; i++) {
		users[i].name = argv[i + usersStart];
		users[i].lastPresent = users[i].nowPresent = false;
	}
	
	// FIRST LOOK AT UTMP FILE
	
	updateWatchedUsers(users, usersLen);
	
	bool nonzeroUsers = false;
	for (size_t i = 0; i < usersLen; i++) {
		if (users[i].nowPresent) {
			printf("%s ", users[i].name);
			nonzeroUsers = true;
		}
	}
	
	if (nonzeroUsers)
		printf("are currently logged in\n");
	else
		printf("none of the specified users are currently logged in\n");
	
	// LOOP OF UPDATEING
	
	while (sleep(pollTime) == 0) {
		printf("checking\n");
		updateWatchedUsers(users, usersLen);
		
		// all users that just logged out
		nonzeroUsers = false;
		for (size_t i = 0; i < usersLen; i++) {
			if (users[i].lastPresent && !users[i].nowPresent) {
				printf("%s ", users[i].name);
				nonzeroUsers = true;
			}
		}
		if (nonzeroUsers) printf(" logged out\n");
		
		// all users that just logged in
		nonzeroUsers = false;
		for (size_t i = 0; i < usersLen; i++) {
			if (!users[i].lastPresent && users[i].nowPresent) {
				printf("%s ", users[i].name);
				nonzeroUsers = true;
			}
		}
		if (nonzeroUsers) printf(" logged in\n");
	}
	
	free(users);
	
	return EXIT_SUCCESS;
}


