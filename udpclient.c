/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/time.h>
#include <errno.h>
#include <sys/param.h>

#define MAX_MSG 65535

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    printf("errno=%d\n",errno);
    perror(msg);
    exit(0);
}

double tv_flt(struct timeval *tv) {
    return (1000*tv->tv_sec+tv->tv_usec/1000);
}

int main(int argc, char **argv) {
    fd_set rfds,xfds;
    struct timeval tv;
    int sockfd, portno, n;
    int serverlen;
    int cnt;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char *sbuf = malloc(MAX_MSG);
    char *rbuf = malloc(MAX_MSG);

    /* check command line arguments */
    if (argc < 3) {
       fprintf(stderr,"usage: %s <hostname> <port> [msg size] [repeat-count]\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    int msg_size =  (argc > 3) ? MIN(MAX_MSG,atoi(argv[3])) : 10 ;
    int repeat_count =  (argc > 4) ? atoi(argv[4]) : 1 ;
    memset(sbuf, 'X' , msg_size);
    sbuf[msg_size]= '\0';

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
    serverlen = sizeof(serveraddr);
    n = connect(sockfd, (struct sockaddr *) &serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in connect");

    int rv = setsockopt(sockfd, IPPROTO_IP, IP_RECVERR, NULL , 0);
    if (rv != 0) 
        error("ERROR setting IP_RECVERR on socket");

    struct timeval t0,t1,t2;
    struct timeval acc_a,acc_b;
    timerclear(&acc_a);
    timerclear(&acc_b);
    for (cnt=0; cnt < repeat_count ; cnt++) {
    gettimeofday(&t0,NULL);
    /* send the message to the server */
    n = sendto(sockfd, sbuf, msg_size, 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
    if (n < 0) 
      error("ERROR in sendto");
    gettimeofday(&t1,NULL);
    
/*
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    FD_ZERO(&xfds);
    FD_SET(sockfd, &xfds);
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    n = select(FD_SETSIZE,&rfds,NULL,&xfds,&tv);
    if (n < 0) 
      printf("ERROR in select (%d) %s\n",errno,strerror(errno));
    printf("exit select\n");
    n = recvfrom(sockfd, rbuf, MAX_MSG, MSG_WAITALL || MSG_ERRQUEUE, (struct sockaddr *) &serveraddr, &serverlen);
    n = recv(sockfd, rbuf, MAX_MSG, MSG_ERRQUEUE ); 
    n = recv(sockfd, rbuf, MAX_MSG, 0 );
    n = recv(sockfd, rbuf, MAX_MSG, MSG_WAITALL | MSG_ERRQUEUE );
    n = recv(sockfd, rbuf, MAX_MSG, MSG_ERRQUEUE );
    if (n < 0) 
      printf("ERROR in 1st recv with MSG_ERRQUEUE (%d) %s\n",errno,strerror(errno));
    n = recv(sockfd, rbuf, MAX_MSG, MSG_DONTWAIT );
*/
    n = recv(sockfd, rbuf, MAX_MSG, 0 );
    gettimeofday(&t2,NULL);
    if (n < 0) {
      printf("ERROR in recv (%d) %s\n",errno,strerror(errno));
      n = recv(sockfd, rbuf, MAX_MSG, MSG_ERRQUEUE );
      if (n < 0) 
        printf("ERROR in 2nd recv with MSG_ERRQUEUE (%d) %s\n",errno,strerror(errno));
    } else
      printf("Echo from server: %d bytes (%zd)\n", n, strlen(rbuf));
    struct timeval ta,tb;
    timersub(&t1,&t0,&ta);
    timersub(&t2,&t1,&tb);
    timeradd(&ta,&acc_a,&acc_a);
    timeradd(&tb,&acc_b,&acc_b);
    printf("(%d)time to send: %f\n", cnt, tv_flt(&ta));
    printf("(%d)time to recv: %f\n", cnt, tv_flt(&tb));
    };
    printf("\n(average)time to send: %f(ms)\n", tv_flt(&acc_a)/repeat_count);
    printf("\n(average)time to recv: %f(ms)\n", tv_flt(&acc_b)/repeat_count);

    return 0;
}
