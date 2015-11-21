#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <string.h> 

#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    }  while ( 0); 

void echo_ser( int sock) 
{ 
    char recvbuf[ 1024] = { 0}; 
    struct sockaddr_in recvaddr; 
    socklen_t recvlen; 
    int n; 
    
    bzero(&recvaddr, sizeof(struct sockaddr_in));
    recvaddr.sin_family = AF_INET;  
    recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    recvaddr.sin_port = htons(5188); 

	char temp[] = "172.18.8.21";

    while ( 1) 
    { 
        recvlen =  sizeof(recvaddr); 
        memset(recvbuf,  0,  sizeof(recvbuf)); 
        n = recvfrom(sock, recvbuf,  sizeof(recvbuf),  0, (struct sockaddr *)&recvaddr, &recvlen); 
        if (n == - 1) 
        { 
            if (errno == EINTR) 
            {
                continue;
            } 

            ERR_EXIT( "recvfrom error"); 
        } 
        else  if(n >  0) 
        { 
            printf("%s, addr:%s\n", recvbuf, inet_ntoa(recvaddr.sin_addr));
         //   printf("%s\n", recvbuf); 
            sendto(sock, temp, n,  0, ( struct sockaddr *)&recvaddr, strlen(temp)); 
        } 
    } 
    close(sock); 
} 

int main( void) 
{ 
    int sock; 
    if ((sock = socket(PF_INET, SOCK_DGRAM,  0)) <  0)
    {
        ERR_EXIT( "socket error"); 
    }

    const int opt = 1;
    int nb = 0;
      
    nb = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));
    if(nb == -1)    
    {
        printf("setsockopt error\n");
        exit(0);
    } 

    struct sockaddr_in servaddr; 
    memset(&servaddr,  0,  sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(5188); 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 

    if (bind(sock, ( struct sockaddr *)&servaddr,  sizeof(servaddr)) <  0) 
    {
        ERR_EXIT( "bind error"); 
    }

    echo_ser(sock); 

    return  0; 
}
