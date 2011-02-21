/*
 * Simple Jitter program (client side)
 * Alcatel Alenia Space - 2006
 * GPL Licence
 * Author: Nicolas Hennion
 */
 
#include "jitter.h"

/* Global variables */
int socketfd; 
struct sockaddr_in serveraddr;
#ifdef IPV6_SUPPORT
struct sockaddr_in6 serveraddr6;
#endif

int main(int argc, char **argv) {
	short int tag6 = 0;
	
	socklen_t slen=sizeof(serveraddr);
#ifdef IPV6_SUPPORT
	socklen_t slen6=sizeof(serveraddr6);	
#endif
	
	struct hostent *hptr;
	char *buf, server_name[BUFFER_CHAR], server_ip[BUFFER_CHAR];
	extern char *optarg;
	int i, errflag=1, n_flag=0, t_flag=0;
	int packet_number=DEFAULT_PACKET_NUMBER, time_number=DEFAULT_TIME_NUMBER;
	int port_number=DEFAULT_PORT_NUMBER, buffer_size=DEFAULT_BUFFER_SIZE, bitrate=DEFAULT_BITRATE, tos = DEFAULT_TOS;
	double begin=0, end=0, sendbegin=0, sendend=0, lastprogressbar=0;
	unsigned long packet_delay, packet_size;
	
	/* Manage arguments */
	#ifdef IPV6_SUPPORT
	while ((i = getopt(argc, argv, "6c:n:t:p:w:b:s:v")) != EOF) {
	#else
	while ((i = getopt(argc, argv, "c:n:t:p:w:b:s:v")) != EOF) {
	#endif	 
		switch (i) { 
			#ifdef IPV6_SUPPORT
			case '6':
				tag6 = 1;
				break;
			#endif	 			
			case 'c': 				
				strncpy(server_name, optarg, BUFFER_CHAR);
				if (!tag6) {
					/* IPv4 */		
					hptr = gethostbyname(server_name);
				#ifdef IPV6_SUPPORT
				} else {
					/* IPv6 */
					hptr = gethostbyname2(server_name,AF_INET6);
				#endif
				}
				if (hptr == NULL) {
					fprintf(stderr, "Invalid server IP address or name...\n"); 					
					errflag = 1;
				} else {					
					inet_ntop(hptr->h_addrtype, *(hptr->h_addr_list), server_ip, sizeof(server_ip));
					errflag = 0;
				}
				break;
			case 's': 		
                /* TOS option specified */
				sscanf(optarg, "%x", &tos);
				if ((tos < 0x00) || (tos > 0xFF))
					errflag = 1;					
				break;
			case 'n':
				sscanf(optarg, "%d", &packet_number);
				n_flag = 1;
				if ((packet_number < 2) || (packet_number > MAX_PACKET_NUMBER) || t_flag)
					errflag = 1;					
				break;				
			case 't':
				sscanf(optarg, "%d", &time_number);
				t_flag = 1;				
				if ((time_number < 2) || n_flag)
					errflag = 1;
				break;				
			case 'p':
				sscanf(optarg, "%d", &port_number);
				if ((port_number < 1024) || (port_number > MAX_PORT_NUMBER))
					errflag = 1;
				break;
			case 'w':
				sscanf(optarg, "%d", &buffer_size);
				if ((buffer_size < MIN_BUFFER_SIZE) || (buffer_size > MAX_BUFFER_SIZE))
					errflag = 1;
				break;
			case 'b':
				sscanf(optarg, "%d", &bitrate);
				if (bitrate < 11 )
					errflag = 1;
				break;
			case 'v':
				fprintf(stderr, "version: sjitter client version %s\n", SJITTERC_VERSION); 
				errflag = 1;
				break;
		} 
	} 
	if (errflag) { 
		#ifdef IPV6_SUPPORT
		fprintf(stderr, "usage: sjitterc [-6] -c SERVER [[-n NBPCKT] | [-t SECOND]] [-p PORTNB] [-w SIZE] [-b BITRAT] [-s TOS]\n");
		fprintf(stderr, "\t-6 : Use the IPv6 protocol\n");
		#else
		fprintf(stderr, "usage: sjitterc -c SERVER [[-n NBPCKT] | [-t SECOND]] [-p PORTNB] [-w SIZE] [-b BITRAT] [-s TOS]\n");		
		#endif 
		fprintf(stderr, "\t-c SERVER: where SERVER is the server IP address or name\n");
		fprintf(stderr, "\t-n NBPCKT: where NCPCKT is the number of datagram (>1 , <%d) [default:%d]\n", MAX_PACKET_NUMBER, DEFAULT_PACKET_NUMBER);
		fprintf(stderr, "\t-t SECOND: where SECOND is the number of second (>1) [default:%d]\n", DEFAULT_TIME_NUMBER);		
		fprintf(stderr, "\t-p PORTNB: where PORTNB is the port number (>1024, <%d) [default:%d]\n", MAX_PORT_NUMBER, DEFAULT_PORT_NUMBER);
		fprintf(stderr, "\t-w SIZE: where SIZE is the application buffer size (bytes) (>%d, <%d) [default:%d]\n", MIN_BUFFER_SIZE, MAX_BUFFER_SIZE, DEFAULT_BUFFER_SIZE);
		fprintf(stderr, "\t-b BITRATE: where BITRATE is the bitrate (IP level) in Kbps  (>10) [default:%d]\n", DEFAULT_BITRATE);	
		fprintf(stderr, "\t-s TOS: where TOS is the hexadecimal value for IP header TOS field (>=0x00, <=0xFF) [default:%x]\n", DEFAULT_TOS);
		if (n_flag && t_flag) {
			fprintf(stderr, "It is not possible to use -n and -t in the same command line\n");
		}
		exit(2);
	}

	/* Buffer allocation */
	buf = (char *) malloc(buffer_size);

	/* Signals management */
	signal(SIGINT, onsignal);
	signal(SIGTERM, onsignal);
	signal(SIGQUIT, onsignal);

	/* Build the socket */
	if (!tag6) {
		/* IPv4 */	
		if ((socketfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
			errorexit("IPv4 socket error");
		/* Set TOS field */
		if (setsockopt(socketfd, IPPROTO_IP, IP_TOS, (char*) &tos, sizeof(tos)) == -1)
           errorexit("IPv4 setsockopt error");
		memset((char *) &serveraddr, sizeof(serveraddr), 0);
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(port_number);
		if (inet_aton(server_ip, &serveraddr.sin_addr)==0)
			perror("IPv4 inet_aton error");
	#ifdef IPV6_SUPPORT
	} else {
		/* IPV6 */
		if ((socketfd=socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP))==-1)
			errorexit("IPv6 socket error");
		memset((char *) &serveraddr6, sizeof(serveraddr6), 0);
		serveraddr6.sin6_family = AF_INET6;
		serveraddr6.sin6_port = htons(port_number);
		if (inet_pton(AF_INET6, server_ip, &serveraddr6.sin6_addr)==0)
			perror("IPV6 inet_aton error");
	#endif
	}
	
	if (t_flag) {
		/* -t option set, compute the packet number */
		packet_number = (time_number*(bitrate*1000.0))/((buffer_size+28)*8);
	} else {
		/* Compute the estimate time */
		time_number = (packet_number*((buffer_size+28)*8))/(bitrate*1000.0);
	}
	if (!tag6) {
		/* IPv4 */		
		printf("Send data (%d datagrams of %d bytes / %d Kbps) to the server:port %s:%d\n", 
				packet_number, buffer_size, bitrate, inet_ntoa(serveraddr.sin_addr), port_number);
	#ifdef IPV6_SUPPORT
	} else {
		/* IPv6 */
		char buf6[INET6_ADDRSTRLEN];			
		printf("Send data (%d datagrams of %d bytes / %d Kbps) to the [server]:port [%s]:%d\n", 
				packet_number, buffer_size, bitrate, inet_ntop(AF_INET6, &serveraddr6.sin6_addr, buf6, sizeof(buf6)), port_number);
	#endif
	}
	
	/* Send configuration datagram to the server */
	snprintf(buf, buffer_size, "SJITTER-START %d %d ", packet_number, buffer_size);
	if (!tag6) {
		/* IPv4 */		
		if (sendto(socketfd, buf, buffer_size, 0, (const struct sockaddr *) &serveraddr, slen)==-1)
			errorexit("IPv4 sendto error");
	#ifdef IPV6_SUPPORT
	} else {
		/* IPv6 */
		if (sendto(socketfd, buf, buffer_size, 0, (const struct sockaddr *) &serveraddr6, slen6)==-1)
			errorexit("IPv6 sendto error");		
	#endif
	}

	/* Compute delay for bitrate (goal on the IP level) */
	packet_size = (unsigned long) ((buffer_size+28)*8);
	packet_delay = (unsigned long) ((packet_size/(bitrate*1000.0))*1000000);
	
	/* Send data to the server */
	/* Init the packet number tag */
	i=0;
	/* Star the main loop */
	printf("Sending data (estimate time: %d seconds)...\n", time_number);
	begin = lastprogressbar = getcurrenttimems();
	while (i<packet_number) {
		
		/* Manage bitrate */
		usleep(packet_delay-((sendend-sendbegin)*1000000));
		
		/* Send datagram on the network interface */
		sendbegin = getcurrenttimems();	/* Send begin... */				
		snprintf(buf, buffer_size, "SJITTER-DATA %d %lf ", i, sendbegin);
		if (!tag6) {
			/* IPv4 */				
			if (sendto(socketfd, buf, buffer_size, 0, (const struct sockaddr *) &serveraddr, slen)==-1)
				errorexit("IPv4 sendto error");
		#ifdef IPV6_SUPPORT
		} else {
			/* IPv6 */
			if (sendto(socketfd, buf, buffer_size, 0, (const struct sockaddr *) &serveraddr6, slen6)==-1)
				errorexit("IPv6 sendto error");			
		#endif
		}				
		sendend = getcurrenttimems(); /* ... send end */
		
		/* Display progress bar (every 1 second)*/
		if ((sendend-lastprogressbar) > 1) {
			progressbar(i, packet_number);
			lastprogressbar = sendend;
		}
		
		/* Next packet */
		i++;
	}
	end = getcurrenttimems();

	/* Send "SJITTER-END" datagram to the server */
	snprintf(buf, buffer_size, "SJITTER-END ");
	if (!tag6) {
		/* IPv4 */				
		if (sendto(socketfd, buf, buffer_size, 0, (const struct sockaddr *) &serveraddr, slen)==-1)
			errorexit("IPv4 sendto error");
	#ifdef IPV6_SUPPORT
	} else {
		/* IPv6 */
		if (sendto(socketfd, buf, buffer_size, 0, (const struct sockaddr *) &serveraddr6, slen6)==-1)
			errorexit("IPv6 sendto error");			
	#endif
	}				
		
	/* Summary */
	printf("\r                                                                                \r");
	printf("Summary: %d datagrams sent in %.2lf seconds (%.0lf Kbps)\n", 
		   packet_number, end-begin, (packet_number*buffer_size*8)/(end-begin)/1000);
	
	close(socketfd);
	free(buf);
	
	return 0;
}
