/*************************************************************************
	> File Name: parser.c
	> Author: fwj
	> Mail: fuwenjie2011@126.com 
	> Created Time: 2015年11月21日 星期六 11时24分03秒
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "router_dev.h"

static router_dev_t* g_router;

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

void* handle_cmd(char cmd[], char data[])
{
	printf("cmd:%s, data:%s\n", cmd, data);

	if (start_with(cmd, "search"))			
	{
		router_id_t id;

		sscanf(data, "%[^:]%*c%s", id.id, id.data);
		g_router->search(&id);
	}else if(start_with(cmd, "set_wan_config"))
	{
		router_wan_t wan_config;
		
		sscanf(data, "%[^:]%*c%s", wan_config.key, wan_config.name);
		g_router->wan_config(&wan_config);
	}else if(start_with(cmd, "set_wifi_config"))
	{
		router_wifi_t wifi_config;
		
		sscanf(data, "%[^:]%*c%[^:]%*c%s", wifi_config.name, wifi_config.key_type, wifi_config.key);
		g_router->wifi_config(&wifi_config);
	}else if(start_with(cmd, "set_router_reboot"))
	{
		g_router->reboot();
	}else if(start_with(cmd, "set_router_reset"))
	{
		g_router->reset();
	}
}

void parser_buf(char buf[])
{
	char *ptr, *tmp;
	router_cmd_t cmd;

	ptr = strtok_r(buf, ";", &tmp);
	sscanf(ptr,"%[^:]%*c%[^:]%*c%s", cmd.version, cmd.cmd, cmd.cmd_i);
	printf("version:%s, cmd:%s, cmd_i:%s\n", cmd.version, cmd.cmd, cmd.cmd_i);

	handle_cmd(cmd.cmd_i, tmp);
}

//main loop
int main(int argc, char * argv[])  
{  
	char str[]="UBoxV002:search:search_box;id:172.18.8.21";  
	char *ptr, *tmp;  

	router_cmd_t cmd;

	printf("before strtok:  str=%s\n",str);  
	g_router = (router_dev_t*)router_dev_open(); 

	if(g_router == NULL)
	{
		printf("router device open failed\n");
		return -1;
	}

	parser_buf(str);
	/*
	ptr = strtok_r(str, ";", &tmp);  

	printf("ptr:%s\n", ptr);
	printf("str:%s\n", str);
	printf("str:%s\n", tmp);

	if (start_with(ptr, "UBoxV002"))
	{
		sscanf(ptr,"%[^:]%*c%[^:]%*c%s", cmd.version, cmd.cmd, cmd.cmd_i);
		printf("version:%s, cmd:%s, cmd_i:%s\n", cmd.version, cmd.cmd, cmd.cmd_i);
	}

	while (ptr != NULL){  
		printf("ptr=%s\n",ptr);  
		ptr = strtok_r(NULL, ";", &tmp);  
	}
	*/

	return 0;  
}
