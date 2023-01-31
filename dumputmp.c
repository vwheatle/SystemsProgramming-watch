#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <utmp.h>
#include <fcntl.h>

void dumpfile(char *);
void show_utrec(struct utmp *);

char *uttypes[] = {
	"EMPTY", "RUN_LVL", "BOOT_TIME", "OLD_TIME",
	"NEW_TIME", "INIT_PROCESS", "LOGIN_PROCESS",
	"USER_PROCESS", "DEAD_PROCESS", "ACCOUNTING"
};	
char *typename(int typenum) { return uttypes[typenum]; }

int main(int argc, char *argv[]) {
	if (argc == 1)
		dumpfile(UTMP_FILE);
	else
		dumpfile(argv[1]);
	
	return EXIT_SUCCESS;
}

// open file and dump records
void dumpfile(char *fn) {
	int fd;
	if ((fd = open(fn, O_RDONLY)) == -1) {
		perror(fn);
		exit(EXIT_FAILURE);
	}
	
	struct utmp	utrec;
	while(read(fd, &utrec, sizeof(struct utmp)) == sizeof(struct utmp))
		show_utrec(&utrec);
	
	close(fd);
}

void show_utrec(struct utmp *rp) {
	printf("%-8.8s ", rp->ut_user );
	// printf("%-14.14s ", rp->ut_id   ); // used it for hp-ux
	printf("%-12.12s ", rp->ut_line );
	printf("%6d ", rp->ut_pid );
	printf("%4d %-12.12s ", rp->ut_type , typename(rp->ut_type) );
	printf("%12d ", rp->ut_time );
	printf("%-32.32s\n", rp->ut_host ); // have to limit length
}
