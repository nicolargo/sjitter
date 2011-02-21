/*
 * Simple Jitter program (server side)
 * Alcatel Alenia Space - 2006
 * GPL Licence
 * Author: Nicolas Hennion
 */
 
#include "jitter.h"

/* Global variables */
int socketfd; 
struct sockaddr_in serveraddr;
struct sockaddr_in6 serveraddr6;

int main(int argc, char **argv) {
	short int tag6 = 0;
	
	struct sockaddr_in clientaddr;
	socklen_t slen=sizeof(clientaddr);
	#ifdef IPV6_SUPPORT 
	socklen_t slen6=sizeof(clientaddr6);
	struct sockaddr_in6 clientaddr6;
	#endif
		
	char *buf, bufstart[MAX_BUFFER_SIZE], bufchar[BUFFER_CHAR];
	extern char *optarg;
	int i, pn, pid;
	int packet_number, buffer_size, port_number=DEFAULT_PORT_NUMBER;
	int errflag=0, iflag=0, checkconf=0, endflag=0;
	double begin=0, end=0, t_send, t_receive, t_transit, t_transitlast, t_transitdelta;
	double jitterlast=0, jittermin=0, jittermax=0, jitteravg=0, jittersum=0, jitterlist[MAX_PACKET_NUMBER];	
	double onewaydelaylast=0, onewaydelaymin=0, onewaydelaymax=0, onewaydelayavg=0, onewaydelaysum=0, onewaydelaylist[MAX_PACKET_NUMBER];	

	/* Manage arguments */
	while ((i = getopt(argc, argv, "6ip:vh")) != EOF) { 
		switch (i) { 
			case '6':
				tag6 = 1;
				break;
			case 'i':
				iflag++;
				break;
			case 'p':
				sscanf(optarg, "%d", &port_number);
				if ((port_number < 1024) || (port_number > MAX_PORT_NUMBER))
					errflag = 1;
				break;
			case 'v':
				fprintf(stderr, "version: sjitter client version %s\n", SJITTERC_VERSION); 
				errflag = 1;
				break;
			case 'h':
				errflag = 1;
				break;
		} 
	} 
	if (errflag) {
		#ifdef IPV6_SUPPORT 
		fprintf(stderr, "usage: sjitters [-6] [-i] [-p PORT]\n"); 
		fprintf(stderr, "\t-6 : Use IPv6 protocol\n");
		#else
		fprintf(stderr, "usage: sjitters [-i] [-p PORT]\n");
		#endif		
		fprintf(stderr, "\t-p PORT: where PORT is the port number (>1024, <%d) [default:%d]\n", MAX_PORT_NUMBER, DEFAULT_PORT_NUMBER);		
		fprintf(stderr, "\t-i : Verbose mode\n");
		exit(2);
	}

	/* Signals management */
	signal(SIGINT,onsignal);
	signal(SIGTERM,onsignal);
	signal(SIGQUIT,onsignal);	

	/* Build the socket */
	if (!tag6) {
		/* IPv4 */
		if ((socketfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
			errorexit("IPv4 socket error");
		memset((char *) &serveraddr, sizeof(serveraddr), 0);
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(port_number);
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(socketfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr))==-1)
			errorexit("IPv4 bind error");
	#ifdef IPV6_SUPPORT
	} else {
		/* IPv6 */
		if ((socketfd=socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP))==-1)
			errorexit("IPv6 socket error");	
		memset((char *) &serveraddr6, sizeof(serveraddr6), 0);
		serveraddr6.sin6_family = AF_INET6;
		serveraddr6.sin6_port = htons(port_number);
		serveraddr6.sin6_addr = in6addr_any;
		if (bind(socketfd, (struct sockaddr *) &serveraddr6, sizeof(serveraddr6))==-1)
			errorexit("IPv6 bind error");
	#endif
	}

	/* Main loop */
	while (1) {
		
		/* Wait START (configuration) datagram */
		while (checkconf==0) {
			if (!tag6) {
				/* IPv4 */			
				if (recvfrom(socketfd, bufstart, sizeof(bufstart), 0, (struct sockaddr *) &clientaddr, &slen)==-1)
					errorexit("IPv4 recvfrom error");
			#ifdef IPV6_SUPPORT
			} else {
				/* IPv6 */			
				if (recvfrom(socketfd, bufstart, sizeof(bufstart), 0, (struct sockaddr *) &clientaddr6, &slen6)==-1)
					errorexit("IPv6 recvfrom error");
			#endif
			}
			if ((sscanf(bufstart, "%s %d %d ", (char *) &bufchar, &packet_number, &buffer_size) == 3)
				&& (strncmp(bufchar, "SJITTER-START", sizeof(bufchar)) == 0) 
				&& (packet_number > 0) && (packet_number <= MAX_PACKET_NUMBER)
				&& (buffer_size >= MIN_BUFFER_SIZE) && (buffer_size <= MAX_BUFFER_SIZE)) {
					checkconf = 1;					
				}
		}
				
		/* Buffer allocation */
		buf = (char *) malloc(buffer_size);
		
		/* 	A START datagram has been received */
		if ((pid = fork()) < 0) {
			errorexit("fork");
		}
		if (pid > 0) {
			/* Father process */
			
			/* Wait the end of the child process */
			waitpid(pid, NULL, 0);
			
			/* Wait for another START datagram */		
			checkconf = 0;
	
		} else { /* pid == 0 */
			/* Child process */
			if (!tag6) {
				/* IPv4 */			
				printf("[Client: %s] Init OK (wait %d datagrams of %d bytes)\n", 
						inet_ntoa(clientaddr.sin_addr), packet_number, buffer_size);
			#ifdef IPV6_SUPPORT
			} else {
				/* IPv6 */
				char buf6[INET6_ADDRSTRLEN];			
				printf("[Client: %s] Init OK (wait %d datagrams of %d bytes)\n", 
						inet_ntop(AF_INET6, &clientaddr6.sin6_addr, buf6, sizeof(buf6)), packet_number, buffer_size);
			#endif
			}
			
			/* Wait data */
			i = 0;
			begin = getcurrenttimems();
			while (!endflag) {
				if (!tag6) {
					/* IPv4 */			
					if (recvfrom(socketfd, buf, buffer_size, 0, (struct sockaddr *) &clientaddr, &slen)==-1)
						errorexit("IPv4 recvfrom error");
				#ifdef IPV6_SUPPORT
				} else {
					/* IPv6 */
					if (recvfrom(socketfd, buf, buffer_size, 0, (struct sockaddr *) &clientaddr6, &slen6)==-1)
						errorexit("IPv6 recvfrom error");					
				#endif
				}
		
				/* Compute Jitter (see in RFC 1889) */
				t_receive = getcurrenttimems();
				if (sscanf (buf, "%s %d %lf ", (char *) &bufchar, &pn, &t_send) > 0) {
					if (strncmp(bufchar, "SJITTER-DATA", sizeof(bufchar)) == 0) {
						/* SJITTER-DATA: the server has received a DATA packet */
						t_transit = t_receive-t_send;
						if (i != 0) {
							t_transitdelta = t_transit - t_transitlast;
							if (t_transitdelta < 0.0)
								t_transitdelta = -t_transitdelta;
							
							/* One way delay */
							onewaydelaylast = t_transit;
							if (onewaydelaylast < onewaydelaymin) 
								onewaydelaymin = onewaydelaylast;
							if (onewaydelaylast > onewaydelaymax) 
								onewaydelaymax = onewaydelaylast;
							onewaydelaysum += onewaydelaylast;	
							onewaydelayavg = onewaydelaysum/i;
							onewaydelaylist[i] = onewaydelaylast;							
														
							/* Jitter */
							jitterlast += (t_transitdelta - jitterlast) / (16.0);
							if (jitterlast < jittermin) 
								jittermin = jitterlast;
							if (jitterlast > jittermax) 
								jittermax = jitterlast;
							jittersum += jitterlast;	
							jitteravg = jittersum/i;
							jitterlist[i] = jitterlast;
							
							if (iflag != 0) {
								/* Only display packets if iglag is set (-i option) */
								if (!tag6) {
									/* IPv4 */			
									printf("[Client: %s] ", inet_ntoa(clientaddr.sin_addr));
								#ifdef IPV6_SUPPORT
								} else {
									/* IPv6 */
									char buf6[INET6_ADDRSTRLEN];			
									printf("[Client: %s] ", inet_ntop(AF_INET6, &clientaddr6.sin6_addr, buf6, sizeof(buf6)));						
								#endif
								}
								printf("One-way delay (ms): Last=%.2lf / Min=%.2lf / Max=%.2lf / Avg=%.2lf\n", onewaydelaylast*1000, onewaydelaymin*1000, onewaydelaymax*1000, onewaydelayavg*1000);
								/*printf("[Client: %s] ", inet_ntoa(clientaddr.sin_addr));*/
								printf("Jitter (ms)       : Last=%.2lf / Min=%.2lf / Max=%.2lf / Avg=%.2lf\n", jitterlast*1000, jittermin*1000, jittermax*1000, jitteravg*1000);
							}
						} else {			
							onewaydelaymin = onewaydelaymax = t_transit;
							jittermin = 65553;						
							if (iflag != 0) {
								if (!tag6) {
									/* IPv4 */			
									printf("[Client: %s] First datagram received\n", inet_ntoa(clientaddr.sin_addr));
								#ifdef IPV6_SUPPORT
								} else {
									/* IPv6 */
									char buf6[INET6_ADDRSTRLEN];			
									printf("[Client: %s] First datagram received\n", inet_ntop(AF_INET6, &clientaddr6.sin6_addr, buf6, sizeof(buf6)));						
								#endif
								}								
							}
						}
						i++;
					} else if (strncmp(bufchar, "SJITTER-END", sizeof(bufchar)) == 0) {
						/* SJITTER-END : the server has received a END packet */
						endflag = 1;
					} else {
						errorexit("Receive an invalid datagram (invalid command)");
					}
				} else {
					errorexit("Receive an invalid datagram (missing parameters))");
				}
				t_transitlast = t_transit;
			}
			end = getcurrenttimems();
							
			/* Display summary */
			if (iflag != 0) {
				if (!tag6) {
					/* IPv4 */			
					printf("[Client: %s] --- Summary\n", inet_ntoa(clientaddr.sin_addr));
				#ifdef IPV6_SUPPORT
				} else {
					/* IPv6 */
					char buf6[INET6_ADDRSTRLEN];			
					printf("[Client: %s] --- Summary\n", inet_ntop(AF_INET6, &clientaddr6.sin6_addr, buf6, sizeof(buf6)));						
				#endif
				}
			}
			if (!tag6) {
				/* IPv4 */			
				printf("[Client: %s] ", inet_ntoa(clientaddr.sin_addr));
			#ifdef IPV6_SUPPORT
			} else {
				/* IPv6 */
				char buf6[INET6_ADDRSTRLEN];			
				printf("[Client: %s] ", inet_ntop(AF_INET6, &clientaddr6.sin6_addr, buf6, sizeof(buf6)));						
			#endif
			}
			printf("Receive %d/%d (%d%%) datagrams in %.2lf seconds (%.0lf Kbps)\n", 
				   i, packet_number, (i*100)/packet_number, end-begin, (packet_number*buffer_size*8)/(end-begin)/1000);
			/*printf("[Client: %s] ", inet_ntoa(clientaddr.sin_addr));*/		   
			printf("One-way delay summary (ms): Min=%.2lf / Max=%.2lf / Avg=%.2lf\n", onewaydelaymin*1000, onewaydelaymax*1000, onewaydelayavg*1000);
			/*printf("[Client: %s] ", inet_ntoa(clientaddr.sin_addr));*/		   
			printf("Jitter summary (ms)       : Min=%.2lf / Max=%.2lf / Avg=%.2lf\n", jittermin*1000, jittermax*1000, jitteravg*1000);
			if (iflag != 0) {
				if (!tag6) {
					/* IPv4 */			
					printf("[Client: %s] --- End\n", inet_ntoa(clientaddr.sin_addr));
				#ifdef IPV6_SUPPORT
				} else {
					/* IPv6 */
					char buf6[INET6_ADDRSTRLEN];			
					printf("[Client: %s] --- End\n", inet_ntop(AF_INET6, &clientaddr6.sin6_addr, buf6, sizeof(buf6)));						
				#endif
				}
			}
													
			exit(0); /* end of the child */
			
			free(buf);
		}

	} /* end of the main loop */
	
	/* Close the socket */
	close(socketfd);
	
	return 0; /* end of the father */

}
