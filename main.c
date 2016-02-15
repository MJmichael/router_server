#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 

#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "router_dev.h"
#include "parser.h"

#ifndef _DEBUG_MAIN_
#define _DEBUG_MAIN_
#endif

#ifdef _DEBUG_MAIN_
#define DEBUG_WARN(m, ...) printf(m, __VA_ARGS__)
#else
#define DEBUG_WARN(m, ...)
#endif

#ifndef DEBUG_ERR
#define DEBUG_ERR(m) printf(m)
#endif

#define ERR_EXIT(m) \
    do { \
        perror(m); \
		exit(EXIT_FAILURE); \
	} while(0); 

#if 0
static int get_broadcast_addr(char* addr)
{
	int inet_sock;
	struct ifreq ifr;

	bzero(&ifr, sizeof(ifr));
	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
	/**
	  strcpy(ifr.ifr_name, "eth0");
	  if (ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0)
	  {
	  perror("ioctl");
	  return -1;
	  }
	  printf("host:%s\n", inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
	**/
/**
	bzero(&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name, "br0");
	if (ioctl(inet_sock, SIOCGIFBRDADDR, &ifr) < 0)
	{
		perror("ioctl");
		return -1;
	}
**/

	bzero(&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name, "br0");
	if (ioctl(inet_sock, SIOCGIFFLAGS, &ifr) < 0)
	{
		perror("ioctl");
		return -1;
	}

	return(ifr.ifr_flags & IFF_UP ? 1 : 0);
//	get_broadcast_addr(addr);
//	sprintf(addr, "%s", inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
//	printf("broadcast:%s\n", inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
//  return 0;
}
#else
static int get_host_status(const char* addr)
{
	int inet_sock;
	struct ifreq ifr;

	bzero(&ifr, sizeof(ifr));
	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);

	strcpy(ifr.ifr_name, addr);
	if (ioctl(inet_sock, SIOCGIFFLAGS, &ifr) < 0)
	{
		perror("ioctl");
		return -1;
	}

	return(ifr.ifr_flags & IFF_UP ? 1 : 0);
}
#endif

/**
** wait network online;
**/
void wait_online(void)
{
	do {
#ifdef _DEBUG_MAIN_
			DEBUG_ERR("Wait for eth0 up\n");
#endif
			usleep(1000);
	} while(get_host_status("eth0") == 0);
	
	system("route add -host 255.255.255.255 dev br0");
}

/**
** server start, send udp broadcast: init;
**/
int server_init(int sock)
{
	struct sockaddr_in servaddr; 
	//int sock; 
	char str[] = "UBoxV002:response:get_router_reboot;\{\"ServerInit\":\"success\"}";
	char addr[1024];

	memset(&servaddr,  0,  sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(5188); 
//	servaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 

/**
	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) <  0) 
	{
		DEBUG_ERR("socket");
		return(-1);
	}
**/

	static int opt = 1;
	int nb = 0;  
  	nb = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));  
   	if(nb == -1)  
    {  
        DEBUG_ERR("error\n");
        return(-1);  
    } 

	struct timeval tv_out;
    tv_out.tv_sec = 5;//µÈ´ý10Ãë
    tv_out.tv_usec = 0;
	nb = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv_out, sizeof(tv_out));
	if(nb == -1)
    {  
        DEBUG_ERR("error\n");
        return(-1);  
    }

	char recvbuf[1024] = {0}, sendbuf[4096]; 
	int n;
	socklen_t recvlen; 

	recvlen = sizeof(servaddr);
	do {
		sendto(sock, str, strlen(str), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)); 
		usleep(1000);

		n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&servaddr, &recvlen);
		if(n == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			break;
		}
		else if (n >  0)
		{
			if(start_with(recvbuf, "UNetConnectSuccess")) 
			{
				printf("Over net check\n");
				break;
			}
			else
			{
				continue;
			}			
		}		
	}while (1);
	
	//close(sock);

	return 0;
}

/**
 **  udp loop
 **/
void loop(int sock) 
{ 
	int router_init = 1;

/**
	struct sockaddr_in servaddr; 
	memset(&servaddr,  0,  sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(5188); 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
**/

	//for recv udp broadcast  
	struct sockaddr_in recvaddr;  

	bzero(&recvaddr, sizeof(struct sockaddr_in));
	recvaddr.sin_family = AF_INET;  
	recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	recvaddr.sin_port = htons(5188);

	char recvbuf[1024] = {0}, sendbuf[4096]; 
	int n;
	socklen_t recvlen = sizeof(recvaddr);

	//create server sock
	//bind
	if (bind(sock, (struct sockaddr *)&recvaddr,  sizeof(recvaddr)) <  0) 
	{
		ERR_EXIT( "Bind Error!");
	}

	do {
		memset(recvbuf,  0,  sizeof(recvbuf));
		n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&recvaddr, &recvlen);
#ifdef _DEBUG_MAIN_
		DEBUG_WARN("Recv from %s:%s\n", inet_ntoa(recvaddr.sin_addr), recvbuf);
#endif

		if(n == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			break;
		}
		else if (n >  0)
		{
			if (start_with(recvbuf, "UBoxV002"))
			{
				if (parser_cmd(recvbuf, BOXSET, &router_init, sendbuf) == 0) 
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
			} else if(start_with(recvbuf, "UPhoneV002")) {
				if (parser_cmd(recvbuf, PHONE, &router_init, sendbuf) == 0) 
				{
					//response to client, send	
#ifdef _DEBUG_MAIN_
					DEBUG_WARN("sendbuf:%s\n", sendbuf);
#endif
					if (router_init == 0) 
					{
#ifdef _DEBUG_MAIN_
						DEBUG_ERR("no need init\n");
#endif
						sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&recvaddr, recvlen);
					} else if(router_init == 1) {
#ifdef _DEBUG_MAIN_
						DEBUG_ERR("need init\n");
#endif
						sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&recvaddr, recvlen);
						router_init_script("all");
					}
				} else {
					sendto(sock, "Error Cmd", strlen("Error Cmd"),  0, (struct sockaddr *)&recvaddr, recvlen);
				}
				continue;
			}
		}
	} while(1);
}

//Update module thread
void* check_update_thread(void *args)
{
	//char tmp[1024];
	do {
		check_version(0, NULL);
		sleep(5*60);
	//	router_update_firmware(PHONE, (void*)tmp);
	} while(1);
}

int main(int argc, char* argv[]) 
{ 
	pid_t pid;    
	pthread_t p_thread;

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
//1. parser init
    parser_init();

//2. wait online
	wait_online();

//2. Update thread
    pthread_create(&p_thread, NULL, check_update_thread, NULL);

//3. create server sock and loop
	int sock; 

	do
	{
		//creat sock and online;
		if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		{
			ERR_EXIT( "socket error\n"); 
		}
		
		//server init
		if(server_init(sock) < 0)
		{
			ERR_EXIT("ServerInit error\n");
		}

		//loop
		loop(sock);   

		close(sock);
	} while(1);

// fix me;
	return 0; 
}
