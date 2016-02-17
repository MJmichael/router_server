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
#include <poll.h>

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

static int pipe_fw[2];

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

int write_all(int fd, const char *buf, size_t count)
{
	int ret;
	int len  = 0;
	int save = count;

	while(count > 0)
	{
		ret = write(fd, buf + len, count);
		if (ret <= 0) {
			if (errno == EINTR)
				continue;
			else
				break;
		}
		len   += ret;
		count -= ret;
	}

	return len == save ? 0 : -1;
}

int read_all(int fd, char *buf, size_t count)
{
	int ret;
	int len  = 0;
	int save = count;

	//while(count > 0)
	//{
		ret = read(fd, buf + len, count);
#if 0
		if (ret <= 0) {
			if (errno == EINTR)
				continue;
			else
				break;
		}
#endif
	//	len   += ret;
	//	count -= ret;
	//}

	//return len == save ? 0 : -1;
		return ret;
}

//Server online module thread
void* server_online_thread(void *args)
{
	struct sockaddr_in servaddr; 
	//int sock; 
	char str[] = "UBoxV002:response:get_router_reboot;\{\"ServerInit\":\"success\"}";
	char addr[1024];
	int sockfd;
	
	memset(&servaddr,  0,  sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(5188); 
	servaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	static int opt = 1;
	int nb = 0;  

	sockfd = socket(AF_INET,SOCK_DGRAM,0);
		
	nb = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));  
	if(nb == -1)  
	{  
		DEBUG_ERR("error\n");
		return(-1);  
	} 

	nb = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));  
   	if(nb == -1)  
    {  
        DEBUG_ERR("error\n");
		return(-1);  
	}

	struct  pollfd pfd[2];
	int          ret;
	unsigned int timeout = 3000;//8 seconds
	int          pollcnt = 1;
	char         recv[1024];

	do {
		pfd[0].fd      = pipe_fw[0];
		pfd[0].events  = POLLIN;
		pfd[0].revents = 0;
		
		ret = poll(pfd, pollcnt, timeout);
		if(ret == 0)
		{
			printf("poll timeout\n");
			sendto(sockfd, str, strlen(str), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)); 
		}

		if (ret < 0) {
			if ((errno == EAGAIN) || (errno == EINTR)) {
				continue;
			}
			printf("poll failed: %d(%s)\n", errno, strerror(errno));
		}
		
		if (pfd[0].revents & POLLIN) 
		{
			printf("Poll Event POLLIN\n");
			ret = read_all(pipe_fw[0], recv, sizeof(recv));
			if (ret < 0) {
				printf("Recv info from pipe failed: %d(%s)\n", errno, strerror(errno));
			}
			printf("Recv info from pipe: (%s)\n", recv);

			sendto(sockfd, recv, strlen(recv), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)); 
		}
	} while(1);

	close(sockfd);
}

/**
** server start, send udp broadcast: init;
**/
//int server_init(int sock)
int server_init(void)
{
	struct sockaddr_in recvaddr; 
	char str[] = "UBoxV002:response:get_router_reboot;\{\"ServerInit\":\"success\"}";
	char addr[1024];
	int router_init = 1;

	memset(&recvaddr,  0,  sizeof(recvaddr)); 
	recvaddr.sin_family = AF_INET; 
	recvaddr.sin_port = htons(5188); 
	recvaddr.sin_addr.s_addr = htonl(INADDR_ANY); 

	static int opt = 1;
	int nb = 0;  

	char recvbuf[1024] = {0}, sendbuf[4096]; 
	socklen_t recvlen = sizeof(recvaddr);

	//select
	fd_set rfds;
	struct timeval tv;
	int retval, maxfd;
	int sock;

	do {

		if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		{
			return(-1);
		}

		nb = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));  
		if (nb == -1)  
		{  
			DEBUG_ERR("error\n");
			close(sock);
			continue;
		}

		//create server sock and bind
		if (bind(sock, (struct sockaddr *)&recvaddr,  sizeof(recvaddr)) <  0)
		{
			printf( "Bind Error!");
			close(sock);
			continue;
		}
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);

		tv.tv_sec = 5;
		tv.tv_usec = 0;	
		
		memset(sendbuf, 0, sizeof(sendbuf));
		memset(recvbuf, 0, sizeof(recvbuf));

		retval = select(sock+1, &rfds, NULL, NULL, &tv);
		if (retval <= 0) 
		{
			printf("Timeout exit!\n");
		}
		else if(FD_ISSET(sock, &rfds)) 
		{
			if(read(sock, recvbuf, sizeof(recvbuf) - 1) < 0) 
			{
				close(sock);
				continue;
			}
			printf("%d %s: %s\n", __LINE__, __FUNCTION__, recvbuf);

			if(start_with(recvbuf, "UNetConnectSuccess")) 
			{
				printf("Sever is Online!\n");
			} 
			else if (start_with(recvbuf, "UBoxV002:set"))
			{
				if (parser_cmd(recvbuf, BOXSET, &router_init, sendbuf) == 0) 
				{
					//response to Box client, send	
#ifdef _DEBUG_MAIN_
					DEBUG_WARN("sendbuf:%s\n", sendbuf);
#endif
					//sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&recvaddr, recvlen);
					printf("sendbuf:%s\n", sendbuf);
					write_all(pipe_fw[1], sendbuf, strlen(sendbuf));
				}
				close(sock);
				continue;
			} 
#if 0
			else if(start_with(recvbuf, "UPhoneV002")) 
			{
				if (parser_cmd(recvbuf, PHONE, &router_init, sendbuf) == 0) 
				{
					//response to Phone client, send	
#ifdef _DEBUG_MAIN_
					DEBUG_WARN("sendbuf:%s\n", sendbuf);
#endif
					if (router_init == 0) 
					{
#ifdef _DEBUG_MAIN_
						DEBUG_ERR("no need init\n");
#endif
						//	sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&recvaddr, recvlen);
						write_all(pipe_fw[1], sendbuf, strlen(sendbuf));
					} else if(router_init == 1) {
#ifdef _DEBUG_MAIN_
						DEBUG_ERR("need init\n");
#endif
						//		sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&recvaddr, recvlen);
						write_all(pipe_fw[1], sendbuf, strlen(sendbuf));
						router_init_script("all");
					}
#endif
				}
				close(sock);
				continue;
			}while (1);

			close(sock);

			return 0;
		}

#if 0
/**
 **  udp loop
 **/
void loop(int sock) 
{ 
	int router_init = 1;

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
					//response to Box client, send	
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
					//response to Phone client, send	
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
#endif

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
	pthread_t pthread_online;
	int ret;

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

	ret = pipe(pipe_fw);
	if (ret < 0) {
		printf("pipe error: %d(%s)\n", errno, strerror(errno));
		return -1;
	}
//1. parser init
    parser_init();

//2. wait online
	wait_online();

//3. Update thread
    pthread_create(&p_thread, NULL, check_update_thread, NULL);

//4. online_thread;
	pthread_create(&pthread_online, NULL, server_online_thread, NULL);

//5. create server sock and loop

	do
	{
		//creat sock and online;
		//int sock; 
		/**
		if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		{
			ERR_EXIT( "socket error\n"); 
		}
		**/
		
		//server init
		if(server_init() < 0)
		{
			ERR_EXIT("ServerInit error\n");
		}

		//loop
//		loop(sock);   
//		close(sock);
	} while(1);

// fix me;
	return 0; 
}
