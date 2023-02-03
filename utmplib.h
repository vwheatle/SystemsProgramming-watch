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
