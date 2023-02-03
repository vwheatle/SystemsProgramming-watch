// CS4-53203: Systems Programming
// Name: V Wheatley
// Date: 2023-01-30
// Assignment2.txt

// C stuff
#include <stdlib.h> // -> EXIT_*
#include <stdio.h> // -> printf, fprintf, puts

#include <string.h> // -> strncmp
#include <stdbool.h> // -> bool
#include <limits.h> // -> UINT_MAX
#include <errno.h> // -> perror

// Linux stuff
#include <unistd.h> // syscalls
#include <pwd.h> // -> getpwuid

#include "utmplib.h" // -> utmp_*

// struct to gather a bunch of information about a user into one place.
struct WatchedUser {
	char *name; // pointer into argv, probably fine to do
	bool lastPresent, nowPresent;
};

// Open, read, and close the utmp file to update the array of WatchedUsers.
void updateWatchedUsers(struct WatchedUser *users, size_t usersLen) {
	if (utmp_open(NULL) == -1) {
		perror("tried to open utmp file");
		exit(EXIT_FAILURE);
	}
	
	for (size_t i = 0; i < usersLen; i++) {
		users[i].lastPresent = users[i].nowPresent;
		users[i].nowPresent = false;
	}
	
	struct utmp *record;
	while ((record = utmp_next()) != NULL)
		for (size_t i = 0; i < usersLen; i++)
			if (strncmp(record->ut_user, users[i].name, UT_NAMESIZE) == 0)
				users[i].nowPresent = true;
	
	// clean up after yourself
	utmp_close();
}

int main(int argc, char *argv[]) {
	unsigned int pollTime = 300; // in seconds
	
	// SHOW USAGE
	
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
	unsigned long pollTimeL = strtoul(argv[1], &firstBad, 10);
	if (*firstBad != '\0') {
		usersStart = 1; // 1st argument isn't a poll time, probably a logname
	} else {
		// doing this silly thing because i greatly dislike atoi
		if (pollTimeL > (unsigned long)UINT_MAX) {
			pollTime = UINT_MAX;
		} else {
			pollTime = (unsigned int)pollTimeL;
		}
	}
	
	// DETERMINE IF ANY LOGNAMES WERE GIVEN
	
	if (usersStart >= (size_t)argc) {
		fprintf(stderr, "please specify one or more lognames.\n");
		exit(EXIT_FAILURE);
	}
	
	// GET YOUR USER NAME
	
	uid_t you = geteuid(); // geteuid
	struct passwd *yourPasswd;
	if ((yourPasswd = getpwuid(you)) == NULL) {
		perror("tried to get username from passwd file");
		exit(EXIT_FAILURE);
	}
	char *yourName = yourPasswd->pw_name;
	
	// note: getpwuid doesn't allocate anything, it just keeps a static area
	//  that it keeps forever. calling this again would clobber the yourPasswd
	//  struct and the yourName string. thankfully, we don't do that.
	
	// INITIALIZE LIST OF USERS
	
	// the list of watched users should also include yourself, because
	// the program needs to know when you log off so it can terminate.
	
	size_t argUsersLen = argc - usersStart;
	size_t allUsersLen = argUsersLen + 1;
	struct WatchedUser *users = malloc(sizeof(struct WatchedUser) * allUsersLen);
	
	fprintf(stderr, "bleep bleep specified %ld users..\n", argUsersLen);
	
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
	
	// only compare status of the names given as arguments.
	bool nonzeroUsers = false;
	for (size_t i = 0; i < argUsersLen; i++) {
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
		updateWatchedUsers(users, allUsersLen);
		
		// all users that just logged out
		nonzeroUsers = false;
		for (size_t i = 0; i < argUsersLen; i++) {
			if (users[i].lastPresent && !users[i].nowPresent) {
				printf("%s ", users[i].name);
				nonzeroUsers = true;
			}
		}
		if (nonzeroUsers) puts("logged out");
		
		// all users that just logged in
		nonzeroUsers = false;
		for (size_t i = 0; i < argUsersLen; i++) {
			if (!users[i].lastPresent && users[i].nowPresent) {
				printf("%s ", users[i].name);
				nonzeroUsers = true;
			}
		}
		if (nonzeroUsers) puts("logged in");
	}
	
	puts("you're no longer online! quitting.");
	
	free(users);
	
	return EXIT_SUCCESS;
}
