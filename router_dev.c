/*************************************************************************
	> File Name: reouter_dev.c
	> Author: fwj
	> Mail: fuwenjie2011@126.com 
	> Created Time: 2015年11月21日 星期六 15时48分18秒
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "router_dev.h"
#include "parser.h"

//相关接口封装
//router reboot;1:success, 0:failed
static char* router_reboot(void)
{
	printf("%s\n", __FUNCTION__);
	return NULL;
}

//router reset
static char* router_reset(void)
{
	printf("%s\n", __FUNCTION__);
	return NULL;
}

//router search 
static char* router_search(router_id_t *id)
{
	printf("%s\n", __FUNCTION__);
	return NULL;
}

//wan config
static char* set_wan_config(router_wan_t *config)
{
	printf("%s\n", __FUNCTION__);
	return NULL;
}

//wifi config
static char* set_wifi_config(router_wifi_t *config)
{
	printf("%s\n", __FUNCTION__);
	return NULL;
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
