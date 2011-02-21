/*
 * Simple Jitter program
 * Alcatel Alenia Space - 2006
 * GPL Licence
 * Author: Nicolas Hennion
 */
 
#include "jitter.h"

/* Global variables */
extern int socketfd;
extern struct sockaddr_in serveraddr;

/* Signals */
void onsignal(int code) {	
	char *buf;
	socklen_t slen=sizeof(serveraddr);

	if ((code == SIGINT) || (code == SIGTERM) || (code == SIGQUIT)) {				
  		/* Send "end" datagram to the server */
  		buf = (char *) malloc(DEFAULT_BUFFER_SIZE);
		snprintf(buf, DEFAULT_BUFFER_SIZE, "SJITTER-END ");
		if (sendto(socketfd, buf, DEFAULT_BUFFER_SIZE, 0, (const struct sockaddr *) &serveraddr, slen) == -1)
			errorexit("sendto");
		/* Close the socket */
		perror("\nProcess killed on user request...");
		close(socketfd);
		exit(1);
	}
	signal(code,onsignal);
}    

/* Display the error and exit */
void errorexit(char *s) {
	perror(s);
	close(socketfd);
	exit(1);
}

/* get the current time ms */
double getcurrenttimems() {
	struct timeval now;

	gettimeofday(&now, NULL);
	
	return (double) (now.tv_sec+now.tv_usec/1.0E6);
}

/* Displat a (text) progressbar on STDOUT */
void progressbar(int pbcurrent, int pbmax) {
	int i, cursor;
	char bar[BUFFER_CHAR];
	
	cursor = (int) (pbcurrent*80/pbmax);
	for (i=0; i<80; i++) {
		if (i>cursor)
			bar[i] = '_';
		else
			bar[i] = '#';
	}
	bar[i] = '\0';	
	printf("\r%s", bar);
	fflush(stdout);
}
