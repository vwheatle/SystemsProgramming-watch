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
#include <pwd.h> // getpwuid

#include "utmplib.h"

struct WatchedUser {
	char *name; // stolen from argv, probably fine
	bool lastPresent, nowPresent;
};

// Open, read, and close the utmp file to update the array of WatchedUsers.
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
	
	if (usersStart >= (size_t)argc) {
		fprintf(stderr, "jeez, at least give me a single logname..\n");
		exit(EXIT_FAILURE);
	}
	
	// GET YOUR USER NAME
	
	uid_t you = geteuid(); // geteuid
	struct passwd *yourPasswd;
	if ((yourPasswd = getpwuid(you)) == NULL) {
		perror("you're not anything??");
		exit(EXIT_FAILURE);
	}
	char *yourName = yourPasswd->pw_name;
	
	// note: getpwuid doesn't allocate anything, it just keeps a static area
	//  that it keeps forever. calling this again would clobber the yourPasswd
	//  struct and the yourName string. thankfully, we don't do that.
	
	// INITIALIZE LIST OF USERS
	
	// the list of watched users should also include yourself, because
	// the program needs to know when you log off so it can terminate.
	
	size_t usersLen = argc - usersStart;
	size_t allUsersLen = usersLen + 1;
	struct WatchedUser *users = malloc(sizeof(struct WatchedUser) * allUsersLen);
	
	fprintf(stderr, "bleep bleep specified %ld users..\n", usersLen);
	
	for (size_t i = 0; i < allUsersLen; i++) {
		if (i != allUsersLen - 1)
			users[i].name = argv[i + usersStart];
		
		users[i].lastPresent = users[i].nowPresent = false;
	}
	
	// also keep a handle directly to yourself, for convenience
	struct WatchedUser *yourUser = &users[allUsersLen - 1];
	yourUser->name = yourName;
	
	// FIRST LOOK AT UTMP FILE
	
	updateWatchedUsers(users, allUsersLen);
	
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
	
	// while the program doesn't get interrupted from sleep...
	// and while you're actually logged on...
	while (sleep(pollTime) == 0 && yourUser->nowPresent) {
		printf("checking\n");
		updateWatchedUsers(users, allUsersLen);
		
		// all users that just logged out
		nonzeroUsers = false;
		for (size_t i = 0; i < usersLen; i++) {
			if (users[i].lastPresent && !users[i].nowPresent) {
				printf("%s ", users[i].name);
				nonzeroUsers = true;
			}
		}
		if (nonzeroUsers) puts("logged out\n");
		
		// all users that just logged in
		nonzeroUsers = false;
		for (size_t i = 0; i < usersLen; i++) {
			if (!users[i].lastPresent && users[i].nowPresent) {
				printf("%s ", users[i].name);
				nonzeroUsers = true;
			}
		}
		if (nonzeroUsers) puts("logged in\n");
	}
	
	free(users);
	
	return EXIT_SUCCESS;
}
