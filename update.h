#ifndef GET_CONNECTION_H
#define GET_CONNECTION_H

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define DEBUG_WARN 1

static inline int get_connection(const char *hostname, const int port, const char **err)
{                                           
	struct	sockaddr_in addr;
	struct	hostent *host;
	int s;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

#ifdef DEBUG_WARN	
    printf("In get_connection hostname is:%s\n",hostname);
#endif

	if(strcmp(hostname, "os.taixin.cn") == 0)
    {	 
		  addr.sin_addr.s_addr = inet_addr("172.18.8.202"); 
	} else {
		if((host = gethostbyname(hostname)) == NULL) {
			*err = "gethostbyname() failed 1th";
			return -1;
		}else{
			addr.sin_addr	= *(struct in_addr*)host->h_addr;
		}
	}

	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == -1) {
		*err = "socket() failed";
		return -1;
	}

	if(connect(s, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		*err = "connect() failed";
		return -1;
	}

	return s;
}
//check update version
int check_version(int argc, char *argv[]);

#endif /* GET_CONNECTION_H */
