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
#include "router_base_i.h"

#ifndef _DEBUG_PARSER_
#define _DEBUG_PARSER_  
#endif 

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

static int handle_cmd(char cmd[], char data[], void* context)
{
	if (context == NULL)
	{
		return -1;
	}
#ifdef _DEBUG_PARSER_
	printf("cmd:%s, data:%s\n", cmd, data);
#endif

	if (start_with(cmd, "set_router_search"))			
	{
		router_id_t id;

		sscanf(data, "%[^:]%*c%s", id.id, id.data);
		return g_router->search(&id, context);
	}
	else if(start_with(cmd, "set_router_wanPPPOE"))
	{
		router_wan_pppoe_t wan_configPPPOE;
		
		sscanf(data, "%[^:]%*c%s", wan_configPPPOE.key, wan_configPPPOE.name);
		return g_router->wan_config_pppoe(&wan_configPPPOE, context);
	}
	else if(start_with(cmd, "set_router_wanIP"))
	{
		router_wan_ip_t wan_configIP;
		
		sscanf(data, "%[^:]%*c%[^:]%*c%[^:]%*c%s", wan_configIP.ip, wan_configIP.mask, 
			wan_configIP.getway, wan_configIP.dns);
		return g_router->wan_config_ip(&wan_configIP, context);
	}
	else if(start_with(cmd, "set_router_wanDHCP"))
	{
		return g_router->wan_config_dhcp((router_wan_dhcp_t*)data, context);
	}
	else if(start_with(cmd, "set_router_wifi"))
	{
		router_wifi_t wifi_config;
		
		sscanf(data, "%[^:]%*c%[^:]%*c%s", wifi_config.name, wifi_config.key_type, wifi_config.key);
		return g_router->wifi_config(&wifi_config, context);
	}
	else if(start_with(cmd, "set_router_reboot"))
	{
		return g_router->reboot(context);
	}
	else if(start_with(cmd, "set_router_reset"))
	{
		return g_router->reset(context);
	}
	else
	{
		return -1;
	}
}

int parser_cmd(char buf[], void* context)
{
	char *ptr, *tmp, tmpbuf[1024];
	router_cmd_t cmd;
	int ret;

	ptr = strtok_r(buf, ";", &tmp);
	sscanf(ptr,"%[^:]%*c%[^:]%*c%s", cmd.version, cmd.cmd, cmd.cmd_i);

#ifdef _DEBUG_PARSER_
	printf("version:%s, cmd:%s, cmd_i:%s\n", cmd.version, cmd.cmd, cmd.cmd_i);
#endif

	ret = handle_cmd(cmd.cmd_i, tmp, tmpbuf);
	sprintf((char*)context, "%s;%s", ptr, tmpbuf);

	return ret;
}

int parser_init(void)
{
	g_router = (router_dev_t*)router_dev_open(); 

	if(g_router == NULL)
	{
		printf("router device open failed\n");
		return -1;
	}

#ifdef _DEBUG_PARSER_
	printf("router device open success\n");
#endif
	return 0;
}

//main loop
/**
int main(int argc, char *argv[])  
{  
	char str[]="UBoxV002:set:set_router_search;id:172.18.8.21";  
	char *ptr, *tmp;  
	char context[1024];

	router_cmd_t cmd;

	printf("before strtok:  str=%s\n",str);  
	g_router = (router_dev_t*)router_dev_open(); 

	if(g_router == NULL)
	{
		printf("router device open failed\n");
		return -1;
	}

	parser_buf(str, (void*)context);

	if(strlen(context))
	{
		printf("context:%s\n", context);
	}

	return 0;  
}
**/
