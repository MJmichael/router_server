/*
** router update moudle
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "update.h"
#include "router_dev.h"

#define PORT 9999
#ifndef ret_msg
#define ret_msg printf
#endif

#ifndef print_debug
#define print_debug printf
#endif

#ifndef RELEASE_VERSION
#define RELEASE_VERSION 1
#endif

#define BUFLEN			4096
#define BUFFREE(name)	(BUFLEN - strlen(name))

#define VERSION		"V1.0"
#define EMAIL		"fuwenjie@taixin.cn"
#define PNAME		"update"
#define AUTHOR		"Michael"
#define HOMEPAGE	"http://www.taixin.cn"

#ifdef RELEASE_VERSION
#define HOST_NAME "os.taixin.cn"
#else
#define HOST_NAME "172.18.8.200:9999"
#endif 

#define VERSION_FILE "/tmp/version"
#define FILENAME "/tmp/fw.bin"


enum Status {
	RET_OK,
	RET_WARNING,
	RET_ERROR,
	RET_WRONG_USAGE
};

static struct dyndns_return_codes {
	const int code;
	const char *message;
	const int  error;
} return_codes[] = {
	{ 0,	"no update needed",				0 },
	{ 1,	"successfully updated",				0 },
	{ 2,	"bad hostname",					1 },
	{ 3,	"bad password",					1 },
	{ 4,	"bad user",					1 },
	{ 6,	"account has been banned",			1 },
	{ 7,	"invalid ip",					1 },
	{ 8,	"host has been disabled",			1 },
	{ 9,	"invalid host (web redirect)",			1 },
	{ 10,	"bad group",					1 },
	{ 11,	"group has been updated",			0 },
	{ 12,	"no update needed",				0 },
	{ 13,	"this client software has been disabled", 1 },
	{ 0,	NULL,						0 }
};

struct arguments {
	char *versionCode;
	char *chipType;
	char *macAddr;
	char *model;
};

static int check_server_msg(int s, char *hostname);

static int request(const int s, struct arguments *args)
{
	char message[BUFLEN];
        
    (void)snprintf(message, BUFLEN,
                       "GET /routerAction.do?method=queryRouterNewVersion&versionCode=%s&chipType=%s&mac=%s&model=%s",
                       args->versionCode, args->chipType, args->macAddr, args->model);      
	{
		char buffer[1024];
                
		(void)snprintf(buffer, 1024,
                               " HTTP/1.1\r\n"
                               "Host: %s\r\n"
                               "Authorization: Basic %s\r\n"
                               "User-Agent: %s - %s\r\n"
                               "Connection: close\r\n"
                               "Pragma: no-cache\r\n\r\n",
                               HOST_NAME, PNAME, VERSION, HOMEPAGE);
		(void)strncat(message, buffer, (BUFLEN - 1 - strlen(message)));
	}
	print_debug("\n\nMessage:"
		    "\n--------------------------------------\n"
		    "%s\n--------------------------------------\n", message);
	
	if(write(s, message, strlen(message)) == -1) 
	{
		ret_msg("write() failed");
		return RET_ERROR;
	}
        
	return RET_OK;        
}

//chipType: RealTek 1; mtk 2;
int check_version(int argc, char *argv[])
{
    struct arguments args;
	int s, ret;
	const char *ptr;
	char context_mac[64], context_version[64];

    (void)memset(&args, 0, sizeof(struct arguments));

	//versionCode
	router_get_version(context_version);
	args.versionCode = strdup(context_version);
	
	//mac
	router_get_mac(context_mac);
	args.macAddr = strdup(context_mac);

	//RealTek 1; mtk 2;
	args.chipType = strdup("1"); //RealTek 1; mtk 2;
	args.model = strdup("1");
     
	s = get_connection("os.taixin.cn", PORT, &ptr);
	if (s == -1) 
	{
		ret_msg("%s: %s", ptr, "os.taixin.cn");
		ret = RET_ERROR;
	} else {
		ret = request(s, &args);
		if(ret == RET_OK) 
		{
			ret = check_server_msg(s, "os.taixin.cn");
		}
		(void)close(s);
	}
 
	return ret;
}

static int check_server_msg(int s, char *hostname)
{
#ifndef BUFSIZE
#define BUFSIZE 4096
#endif

	int n, ret_code;
	char server_msg[BUFSIZE], *ptr;
	char download[512], message[512];
	char cmd[512], version[64];

	if(access(FILENAME, F_OK) == 0)
	{
		ret_msg("File fw.bin update\n");
		close(s);
		return RET_OK;
	}
	(void)memset(server_msg, 0, sizeof(server_msg));

// no block
	fd_set rfds;
    struct timeval tv;
    int retval, maxfd;

	FD_ZERO(&rfds);
	FD_SET(s, &rfds);

	tv.tv_sec = 10;
	tv.tv_usec = 0;	

	retval = select(s+1, &rfds, NULL, NULL, &tv);
	if (retval <= 0) 
	{
		ret_msg("Timeout exit!\n");
		close(s);
		return RET_ERROR;
	}else if(FD_ISSET(s, &rfds)) {
		if(read(s, server_msg, sizeof(server_msg) - 1) < 0) 
		{
			ret_msg("Read() failed!\n");
			close(s);
			return RET_ERROR;
		}
	}

	print_debug("\n\nServer message:"
			"\n--------------------------------------\n"
			"%s\n--------------------------------------\n",
			server_msg);

	if(strstr(server_msg, "HTTP/1.1 200 OK") ||
			strstr(server_msg, "HTTP/1.0 200 OK") ) 
	{

		ptr = strstr(server_msg, "\"versionCode\"");
		ret_msg("%s\n", ptr);
//version message
		sscanf(ptr + 14, "%[^,]%*c%s", version, message);
		ret_msg("versionCode:%s\n", version);

//atoi version
		ret_code = atoi((ptr+14));
		ret_msg("ret_code:%d\n", ret_code);

		if (ret_code != -1) 
		{
			ptr = strstr(server_msg, "\"url\":");
			ret_msg("%s\n", ptr);
/*Skip url*/
			sscanf(ptr + 7, "%[^\"]%*c%s", download, message);
			printf("download:%s, message:%s\n", download, message);

			sprintf(cmd, "wget -t 3 -O %s \"%s\"\n", FILENAME, download);
			printf("cmd:%s\n", cmd);
			system(cmd);

			FILE *fd;
			fd = fopen(VERSION_FILE, "wb+");
			fwrite((version+3), (strlen(version)-3), 1, fd);
			fclose(fd);
		} else {
			ret_msg("os.taixin.cn: No New Version\n");
			close(s);
			return RET_ERROR;
		}
	} else {
		ret_msg("os.taixin.cn: Internal Server Error\n");
		close(s);
		return RET_ERROR;
	}

	close(s);

	return RET_OK;
}
