
/*************************************************************************
 *> File Name: echocli_udp.c
 *> Author: Simba
 *> Mail: dameng34@163.com
 *> Created Time: Sun 03 Mar 2013 06:13:55 PM CST
 *************************************************************************/ 

#include <unistd.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 

#define ERR_EXIT(m) {do {perror(m); exit(EXIT_FAILURE);} while(0);} 

int start_with(char s1[], char s2[])
{
#define UpperChar(c) (((c)>='a' && (c)<='z')?(c)+'A'-'a':(c))
	int i;

	for (i=0; s1[i]!='\0'&&s2[i]!='\0'; i++)
	{
		/*any different?*/
		if (UpperChar(s1[i]) != UpperChar(s2[i])) 
		{
			return 0;
		}
	}
	/*s2 is longer than s1*/
	return (s2[i] != '\0') ? 0 : 1;
}

void echo_cli(int sock) 
{ 
	struct sockaddr_in servaddr; 
	memset(&servaddr,  0,  sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(5188); 
	servaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST); /*default 172.18.8.21*/
	//servaddr.sin_addr.s_addr = inet_addr("172.18.8.21"); /*default 172.18.8.21*/

	int ret; 
	char sendbuf[1024] = {0}; 
	char recvbuf[1024] = {0}; 
	char tmpbuf[1024] = {0};
//	char str[]="UBoxV002:set:set_router_search;id:172.18.8.21";
	char str[]="UBoxV002:set:set_router_reboot;reboot";


	while (fgets(tmpbuf, sizeof(tmpbuf), stdin) !=  NULL) 
	{ 
		tmpbuf[strlen(tmpbuf)-1] = '\0';
		if(start_with(tmpbuf, "reset") || start_with(tmpbuf, "reboot"))
		{
			sprintf(sendbuf, "UBoxV002:set:set_router_%s;%s", tmpbuf, tmpbuf);
		}
		else if(start_with(tmpbuf, "wan") || start_with(tmpbuf, "search"))
		{
			sprintf(sendbuf, "UBoxV002:set:set_router_%s;id:172.18.8.21", tmpbuf);
		}
		else
		{
			sprintf(sendbuf, "UBoxV002:set:set_router_%s;id:wpa:172.18.8.21", tmpbuf);
		}

		sendto(sock, sendbuf, strlen(sendbuf),  0, (struct sockaddr *)&servaddr, sizeof(servaddr)); 

		ret = recvfrom(sock, recvbuf,  sizeof(recvbuf),  0,  NULL,  NULL); 
		if (ret == - 1) 
		{ 
			if (errno == EINTR) 
				continue; 
			ERR_EXIT( "recvfrom"); 
		} 

		printf("recv from server:%s\n", recvbuf);
		memset(sendbuf,  0,  sizeof(sendbuf)); 
		memset(recvbuf,  0,  sizeof(recvbuf)); 
		memset(tmpbuf,  0,  sizeof(tmpbuf)); 
	} 

	close(sock); 


} /*kajdf*/

int main(int argc, char* argv[]) 
{ 
	int sock; 
	if ((sock = socket(PF_INET, SOCK_DGRAM,  0)) <  0) 
	{
		ERR_EXIT("socket");
	}
	
	static int opt = 1;
	int nb = 0;  
  	nb = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));  
   	if(nb == -1)  
    {  
        printf("error\n");
        return 0;  
    }  
	
	echo_cli(sock); 

	return  0; 
}
