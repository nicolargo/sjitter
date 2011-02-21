/*
 * Simple Jitter program
 * Alcatel Alenia Space - 2006
 * GPL Licence
 * Author: Nicolas Hennion
 */
 
#ifndef JITTER_H_
#define JITTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
 
/* Constants */
#define SJITTER_NAME			"SJitter"
#define SJITTER_VERSION			"0.18.1"
#define SJITTERC_NAME			"SJitter client"
#define SJITTERC_VERSION		"0.18.1"
#define SJITTERS_NAME			"SJitter server"
#define SJITTERS_VERSION		"0.18.1"
#define BUFFER_CHAR				1024
#define BUFFER_INET4_ADDR		15

/* Network constants */
#define DEFAULT_BUFFER_SIZE 	1400	/* bytes */
#define MIN_BUFFER_SIZE 		128
#define MAX_BUFFER_SIZE 		8000
#define DEFAULT_PORT_NUMBER 	9930
#define MAX_PORT_NUMBER 		65535
#define DEFAULT_PACKET_NUMBER 	100
#define MAX_PACKET_NUMBER 		65535
#define DEFAULT_TIME_NUMBER 	10
#define DEFAULT_BITRATE			100		/* Kbits per second */
#define DEFAULT_TOS				0x00

/* Headers */
void onsignal(int code);
void errorexit(char *s);
double getcurrenttimems();
void progressbar(int pbcurrent, int pbmax);

#endif /*JITTER_H_*/
