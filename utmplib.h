/* utmplib.c  - functions to buffer reads from utmp file
 *
 * functions are
 * utmp_open(filename) - open file
 * returns -1 on error
 * utmp_next() - return pointer to next struct
 * returns NULL on eof
 * utmp_close() - close file
 *
 * reads NRECS per read and then doles them out from the buffer
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <utmp.h>

#include <unistd.h>

#define NRECS 16
#define NULLUT ((struct utmp *)NULL)
#define UTSIZE (sizeof(struct utmp))

static char utmpbuf[NRECS * UTSIZE]; // buffer to hold
static int num_recs; // number of records stored
static int cur_rec; // next record
static int fd_utmp = -1; // file to read from

int utmp_open(char *);
struct utmp *utmp_next();
int utmp_reload();
void utmp_close();

int utmp_open(char *filename) {
	if (filename == NULL) filename = UTMP_FILE;
	
	fd_utmp = open(filename, O_RDONLY); // open it
	cur_rec = num_recs = 0; // no recs yet
	
	return fd_utmp; // report
}

struct utmp *utmp_next() {
	struct utmp *recp;
	
	// if we don't even have a file descriptor,
	// return a null pointer.
	if (fd_utmp == -1)
		return NULLUT;
	
	// if we ran out of records to give out,
	// return a null pointer.
	if (cur_rec == num_recs && utmp_reload() == 0)
		return NULLUT;
	
	// get address of next record
	recp = (struct utmp *) &utmpbuf[cur_rec * UTSIZE];
	cur_rec++;
	return recp;
}

// read next batch of records into buffer
int utmp_reload() {
	// read them in
	int amt_read = read(fd_utmp, utmpbuf, NRECS * UTSIZE);
	
	// how many did we get?
	num_recs = amt_read / UTSIZE;
	// reset pointer
	cur_rec = 0;
	return num_recs;
}

void utmp_close() {
	if (fd_utmp != -1) { // don't close if not open
		close(fd_utmp);
		fd_utmp = -1; // actually freaking update file descriptor
	}
}
