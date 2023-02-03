V Wheatley  
Systems Programming

# `watch`

## `Makefile`

I wrote a makefile for things! It's alright. Didn't realize that recipe names actually mattered for a bit.

It compiles with most warnings and the UB sanitizer enabled, and it runs with valgrind. (This is admittedly a bit overkill, but I'm now hyper-aware of all errors!! Even the "still-reachable" "memory leaks" that happen when I kill a program!)

I'm also a bit conservative with my C version. I don't want to accidentally use features nobody else's compiler supports!

```make
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
	valgrind $(VALGRIND_FLAGS) ./mywatch 5 `whoami` reboot
```

## `utmplib.h`

This has been reconfigured and renamed from the included version. I removed the `UTNULL` definition because the built-in `NULL` definition works just fine and is valid C. I also changed the type of the buffer and just casted it to a `void*` when I needed to `read`.

```c
/* utmplib.c  - functions to buffer reads from utmp file
 *
 * functions are
 * utmp_open(filename) - open file
 *   returns -1 on error
 * utmp_next() - return pointer to next struct
 *   returns NULL on eof / error
 * utmp_close() - close file
 *
 * reads NRECS per read and then doles them out from the buffer
 */

#include <unistd.h> // -> read, close
#include <fcntl.h> // -> open
#include <utmp.h> // -> struct utmp, UTMP_FILE

#define NRECS 16

static struct utmp utmpbuf[NRECS];
static size_t num_recs; // length of buffer currently used
static size_t cur_rec; // next record to return from utmp_next
static int fd_utmp = -1;

int utmp_open(char *);
struct utmp *utmp_next();
int utmp_reload();
void utmp_close();

int utmp_open(char *filename) {
	// set default filename if none was given
	if (filename == NULL) filename = UTMP_FILE;
	
	fd_utmp = open(filename, O_RDONLY); // open it
	cur_rec = num_recs = 0; // no recs yet
	
	return fd_utmp; // return file descriptor
	// (caller can handle error conditions)
}

struct utmp *utmp_next() {
	// if we don't even have a file descriptor,
	// return a null pointer.
	if (fd_utmp == -1) return NULL;
	
	// if we ran out of records to give out,
	// and we failed to fetch more records,
	// return a null pointer.
	if (cur_rec == num_recs && utmp_reload() == 0)
		return NULL;
	
	// get address of next record
	struct utmp *recp = &utmpbuf[cur_rec];
	cur_rec++;
	return recp;
}

// read next batch of records into buffer
int utmp_reload() {
	// reset pointer to first record in buffer
	cur_rec = 0;
	
	// read records from file
	ssize_t amt_read = read(fd_utmp, (void *)utmpbuf, sizeof(utmpbuf));
	
	// if i/o error returned, we didn't get any new records.
	if (amt_read < 0) return 0;
	
	// how many records did we load?
	num_recs = amt_read / sizeof(utmpbuf[0]);
	return num_recs;
}

void utmp_close() {
	// don't close if not open
	if (fd_utmp != -1) {
		close(fd_utmp);
		fd_utmp = -1;
	}
}
```

## `watch.c`

Here's the main program. I kept a struct named `WatchedUser` to hold information about each of the users you've specified, and if they're present or not. I made a small function that reads the `utmp` file and updates this list of users, and I save the last value of `nowPresent` into `lastPresent` so I can later manually build a list of users that just logged off/on.

I also find the current user's name and put that in the list of users too, but I don't print out its changes -- instead, I just check if the current user logs off, and quit if so.

```c
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
```

And it's a bit difficult to test this because I don't develop on a multi-user system. However, I connected to `wasp`, ran `who`, arbitrarily selected several users, and ran my program with them as the arguments. Their names have been anonymized, but you can definitely trust me that this works.

I also added a `fake_person` as the last username, to ensure that my program doesn't outright lie.

```text
$ ./mywatch 30 user1 user2 user3 `whoami` user4 fake_person
user1 user2 user3 vwheatle user4 are currently logged in
[exactly 5 minutes pass]
[because i had a debug "awake" message in the screenshot i'm transcribing]
[and it printed 10 times, and this was set for 30 seconds]
user2 logged out
[one minute passes]
^C
$
```

Thrilling!
