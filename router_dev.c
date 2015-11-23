/*************************************************************************
	> File Name: reouter_dev.c
	> Author: fwj
	> Mail: fuwenjie2011@126.com 
	> Created Time: 2015年11月21日 星期六 15时48分18秒
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "router_dev.h"
#include "parser.h"

#ifndef _DEBUG_ROUTER_DEV_
#define _DEBUG_ROUTER_DEV_
#endif

//相关接口封装
//router reboot;1:success, 0:failed
static int router_reboot(void* context)
{
#ifdef _DEBUG_ROUTER_DEV_
	printf("%s\n", __FUNCTION__);
#endif
	if (context == NULL)
	{
		return -1;
	}

	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", "1");

	return 0;
}

//router reset
static int router_reset(void* context)
{
	if (context == NULL)
	{
		return -1;
	}
#ifdef _DEBUG_ROUTER_DEV_
	printf("%s\n", __FUNCTION__);
#endif
	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", "1");

	return 0;
}

//router search 
static int router_search(router_id_t *id, void* context)
{
#ifdef _DEBUG_ROUTER_DEV_
	printf("%s\n", __FUNCTION__);
#endif

	char* ptr = (char*)context;
	char str[] = "\{\"IP\":\"172.18.8.1\",\"version\":\"3.4.6.6\",\"wifi_name\":\"DTVOS\",\"lan_mac\":\"112233445566\",\"wan_mac\":\"112233445566\"}\n";

	if (context == NULL)
	{
		return -1;
	}

	if (ptr)
	{
		memcpy(ptr, str, strlen(str));
	}

	return 0;
}

//wan config
static int set_wan_config(router_wan_t *config, void* context)
{
#ifdef _DEBUG_ROUTER_DEV_
	printf("%s\n", __FUNCTION__);
#endif
	if(context == NULL)
	{
		return -1;
	}

	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", "1");

	return 0;
}

//wifi config
static int set_wifi_config(router_wifi_t *config, void* context)
{
#ifdef _DEBUG_ROUTER_DEV_
	printf("%s\n", __FUNCTION__);
#endif
	if (context == NULL)
	{
		return -1;
	}

	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", "1");

	return 0;
}

//init router handle
void *router_dev_open(void)
{
	router_dev_t *router = (router_dev_t*)malloc(sizeof(router_dev_t));
	if(router == NULL)
	{
		printf("alloc router_dev_t failed\n");
		return NULL;
	}

	router->reboot =  router_reboot;
	router->reset = router_reset;
	router->search = router_search;
	router->wan_config = set_wan_config;
	router->wifi_config = set_wifi_config;

	return (void*)router;
}
