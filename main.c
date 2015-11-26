#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 

#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>

#include "router_dev.h"
#include "parser.h"

#ifndef _DEBUG_MAIN_
#define _DEBUG_MAIN_
#endif

#ifdef _DEBUG_MAIN_
#define DEBUG_WARN(format, ...) printf(format, __VA_ARGS__)
#else
#define DEBUG_WARN(format, ...)
#endif

#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while(0); 

/**
**  udp loop
**/
void loop(int sock) 
{ 
    struct sockaddr_in servaddr; 
    memset(&servaddr,  0,  sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(5188); 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
//bind
    if (bind(sock, (struct sockaddr *)&servaddr,  sizeof(servaddr)) <  0) 
    {
        ERR_EXIT( "Bind Error!");
    }   

//for recv udp broadcast  
    struct sockaddr_in recvaddr;  
    
    bzero(&recvaddr, sizeof(struct sockaddr_in));
    recvaddr.sin_family = AF_INET;  
    recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    recvaddr.sin_port = htons(5188);
     
    char recvbuf[1024] = {0}, sendbuf[1024]; 
    int n;
    socklen_t recvlen; 
     
	do {
		recvlen = sizeof(recvaddr);
		memset(recvbuf,  0,  sizeof(recvbuf));
		n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&recvaddr, &recvlen);
#ifdef _DEBUG_MAIN_
		DEBUG_WARN("recv:%s\n", recvbuf);
#endif

		if(n == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
		}
		else if (n >  0)
		{
			if (start_with(recvbuf, "UBoxV002"))
			{
				if (parser_cmd(recvbuf, sendbuf) == 0) 
				{
//response to client, send	
#ifdef _DEBUG_MAIN_
					DEBUG_WARN("sendbuf:%s\n", sendbuf);
#endif
					sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&recvaddr, recvlen);
				}
				else
				{
					sendto(sock, "Error Cmd", strlen("Error Cmd"),  0, (struct sockaddr *)&recvaddr, recvlen);
				}
				continue;
			}
		}
	} while(1);
	close(sock);
}


int main(int argc, char* argv[]) 
{ 
	pid_t pid;

//throw ourself to background;
	pid = fork();

	switch(pid) {
		case -1:
			perror("fork/getpid");
			exit(1);
		case 0:
			break;
		default:
			exit(0);
	}
//1.init
    parser_init();

//2.create sock
    int sock; 
    
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        ERR_EXIT( "socket error"); 
    }

//3.loop
    loop(sock);   

    return 0; 
}
