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

#ifdef _DEBUG_ROUTER_DEV_
#define DEBUG_WARN(m, ...) printf(m, __VA_ARGS__)
#else
#define DEBUG_WARN(m, ...)
#endif

#ifdef CONTEXT
#undef CONTEXT
#define CONTEXT 1024
#else
#define CONTEXT 1024
#endif

#define DEBUG_ERR(m) \
	do { \
		perror(m); \
	} while(0); 

static int cmd_get(char cmd[], void* context)
{
#define MAXLINE 1024
	char result_buf[MAXLINE], result_head[MAXLINE], result[MAXLINE];
	FILE *fp;

	fp = popen(cmd, "r");

	if (NULL == fp)
	{
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("popen error\n");
#endif
		return(-1);
	}

	while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
	{
		//remove '\n'
		if('\n' == result_buf[strlen(result_buf)-1])
		{
			result_buf[strlen(result_buf)-1] = '\0';
		}
#ifdef _DEBUG_ROUTER_DEV_
		printf("command %s, result_buf %s\r\n", cmd, result_buf);
#endif
		sscanf(result_buf, "%[^=]%*c%s", result_head, result);
		memcpy(context, result, strlen(result));
	}

	//close fp
	pclose(fp);

	return 0;
}

static int cmd_set(char cmd[], void* context)
{
	if((cmd == NULL) || (context == NULL))
	{
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("cmd error\n");
#endif
		return(-1);
	}
	if(system(cmd) < 0)
	{
		sprintf(context, "%s", "fail");
	}
	else
	{
		sprintf(context, "%s", "success");
	}

	return(0);
}

//相关接口封装
//router reboot;1:success, 0:failed
static int router_reboot(void* context)
{
	char result[64];

	if (context == NULL)
	{
		return(-1);
	}
#ifdef _DEBUG_ROUTER_DEV_
	DEBUG_WARN("%s\n", __FUNCTION__);
#endif

	if (cmd_set("reboot", result) < 0)
	{
		DEBUG_ERR("router reboot error\n");
		return(-1);
	}

	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", result);

	return 0;
}

//router reset
static int router_reset(void* context)
{
	char result[64];

	if (context == NULL)
	{
		return(-1);
	}
#ifdef _DEBUG_ROUTER_DEV_
	DEBUG_WARN("%s\n", __FUNCTION__);
#endif

	if (cmd_set("flash default", result) < 0)
	{
		DEBUG_ERR("flash default error\n");
		return(-1);
	}

	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", result);

	return(0);
}

//router search 
static int router_search(router_id_t *id, void* context)
{
	char* ptr = (char*)context;
	//default ip mask getway
	char def_ip_addr[CONTEXT] = { 0 };
	char def_subnet_mask[CONTEXT] = { 0 };
	char def_default_getway[CONTEXT] = { 0 };

	//wan lan mac
	char hw_nic0_addr[CONTEXT] = { 0 };
	char hw_nic1_addr[CONTEXT] = { 0 };

	//wlan mac
	char hw_wlan0_addr[CONTEXT] = { 0 };
	char hw_wlan1_addr[CONTEXT] = { 0 };

	//repeat status
	char repeater_enabled0[CONTEXT] = { 0 };
	char repeater_enabled1[CONTEXT] = { 0 };

	if ((context == NULL) || (context == NULL))
	{
		return -1;
	}

#ifdef _DEBUG_ROUTER_DEV_
	DEBUG_WARN("%s\n", __FUNCTION__);
#endif

	if ((cmd_get("flash get IP_ADDR;", (void*)def_ip_addr) < 0))
	{
		DEBUG_ERR("flash get IP_ADDR error\n");
		return(-1);
	}

	if ((cmd_get("flash get SUBNET_MASK;", (void*)def_subnet_mask) < 0))
	{
		DEBUG_ERR("flash get SUBNET_MASK error\n");
		return(-1);
	}

	if ((cmd_get("flash get DEFAULT_GATEWAY;", (void*)def_default_getway) < 0))
	{
		DEBUG_ERR("flash get DEFAULT_GATEWAY error\n");
		return(-1);
	}
	//wan lan mac
	if ((cmd_get("flash get HW_NIC0_ADDR;", (void*)hw_nic0_addr) < 0))
	{
		DEBUG_ERR("flash get HW_NIC0_ADDR error\n");
		return(-1);
	}

	if ((cmd_get("flash get HW_NIC1_ADDR;", (void*)hw_nic1_addr) < 0))
	{
		DEBUG_ERR("flash get HW_NIC1_ADDR error\n");
		return(-1);
	}
	//wifi mac
	if ((cmd_get("flash get wlan HW_WLAN0_WLAN_ADDR;", (void*)hw_wlan0_addr) < 0))
	{
		DEBUG_ERR("flash get HW_WLAN0_WLAN_ADDR error\n");
		return(-1);
	}

	if ((cmd_get("flash get wlan1 HW_WLAN1_WLAN_ADDR;", (void*)hw_wlan1_addr) < 0))
	{
		DEBUG_ERR("flash get HW_WLAN1_WLAN_ADDR error\n");
		return(-1);
	}
	//repeat status
	if ((cmd_get("flash get REPEATER_ENABLED1;", (void*)repeater_enabled0) < 0))
	{
		DEBUG_ERR("flash get REPEATER_ENABLED1 error\n");
		return(-1);
	}

	if ((cmd_get("flash get REPEATER_ENABLED2;", (void*)repeater_enabled1) < 0))
	{
		DEBUG_ERR("flash get REPEATER_ENABLED2 error\n");
		return(-1);
	}

	//combinate return string;
	sprintf(ptr, "\{\"IP\":\"%s\",\"version\":\"3.4.6.7\",\"lan_mac\":\"%s\",\"wan_mac\":\"%s\"}",
			def_ip_addr, hw_nic0_addr, hw_nic1_addr);
#ifdef _DEBUG_ROUTER_DEV_
	DEBUG_ERR(ptr);
#endif

return 0;
}

//wan config pppoe WAN_DHCP:3
static int set_wan_pppoe(router_wan_pppoe_t *config, void* context)
{
	char result[64], cmd[128];

	if ((context == NULL) || (config == NULL))
	{
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("set wan config error\n");
#endif
		return -1;
	}

	sprintf(cmd, "flash set WAN_DHCP 3; flash set PPP_USER_NAME %s; flash set PPP_PASSWORD %s;", 
			config->name,  config->key);

	if (cmd_set(cmd, result) < 0)
	{
		DEBUG_ERR("flash default error\n");
		return(-1);
	}

	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", result);

	return 0;
}

//wan config WAN_DHCP:1
static int set_wan_dhcp(router_wan_dhcp_t *config, void* context)
{
	char result[64], cmd[128];

	if ((context == NULL) || (config == NULL))
	{
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("set wan config error\n");
#endif
		return -1;
	}

	if (cmd_set("flash set WAN_DHCP 1;", result) < 0)
	{
		DEBUG_ERR("flash default error\n");
		return(-1);
	}

	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", result);
	return 0;
}

//wan config static IP WAN_DHCP:0\2
static int set_wan_ip(router_wan_ip_t *config, void* context)
{
	char result[64], cmd[256];

	if ((context == NULL) || (config == NULL))
	{
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("set wan config error\n");
#endif
		return -1;
	}

	sprintf(cmd, "flash set WAN_DHCP 2; flash set WAN_IP_ADDR %s; flash set WAN_SUBNET_MASK %s; flash set WAN_DEFAULT_GATEWAY %s;flash set DNS1 %s;",
			config->ip,  config->mask, config->getway, config->dns);

	if (cmd_set(cmd, result) < 0)
	{
		DEBUG_ERR("flash default error\n");
		return(-1);
	}

	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", result);
	return 0;
}

//wifi config
static int set_wifi_config(router_wifi_t *config, void* context)
{
	char result[64], cmd0[128], cmd1[128];

	if ((context == NULL) || (config == NULL))
	{
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("set wifi config error\n");
#endif
		return(-1);
	}

#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_WARN("config->type:%s\n", config->key_type);
#endif
// force key type 2;
	if (strcmp(config->key_type, "None") == 0)
	{
		sprintf(cmd0, "flash set WLAN0_SSID %s-5G; flash set WLAN0_ENCRYPT 0; flash set WLAN0_WPA_PSK %s",
				config->name, config->key);

		sprintf(cmd1, "flash set WLAN1_SSID %s-2.4G; flash set WLAN1_ENCRYPT 0; flash set WLAN1_WPA_PSK %s",
				config->name, config->key);
	}
	else
	{
		sprintf(cmd0, "flash set WLAN0_SSID %s-5G; flash set WLAN0_ENCRYPT 2; flash set WLAN0_WPA_PSK %s",
				config->name, config->key);

		sprintf(cmd1, "flash set WLAN1_SSID %s-2.4G; flash set WLAN1_ENCRYPT 2; flash set WLAN1_WPA_PSK %s",
				config->name, config->key);
	}

	if ((cmd_set(cmd0, result) < 0) || cmd_set(cmd1, result))
	{
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("flash default error\n");
#endif
		return(-1);
	}

	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", result);

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
	router->wan_config_pppoe = set_wan_pppoe;
	router->wan_config_ip = set_wan_ip;
	router->wan_config_dhcp = set_wan_dhcp;
	router->wifi_config = set_wifi_config;

	return (void*)router;
}
