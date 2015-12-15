/*************************************************************************
  > File Name: reouter_dev.c
  > Author: fwj
  > Mail: fuwenjie2011@126.com 
  > Created Time: 2015年11月21日 星期六 15时48分18秒
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

static int omit_char(const char src[], char c, char* dst)
{
	char *p_pos = src;

	do
	{
		if (*p_pos == c)
		{
			*p_pos++;
		}
		else
		{
			*dst++ = *p_pos++;
		}
	} while(*p_pos !='\0');

	*dst = '\0';

	return 0;
}

static int cmd_get(char cmd[], void* context)
{
#define MAXLINE 1024
	char result_buf[MAXLINE], result_head[MAXLINE], result[MAXLINE], dst[MAXLINE];
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
		sscanf(result_buf, "%[^=]%*c%100[^/;]", result_head, result);
#ifdef _DEBUG_ROUTER_DEV_
		printf("command %s, result_buf %s, result %s\r\n", cmd, result_buf, result);
#endif
		char c ='\\';
		omit_char(result, c, dst);

		memcpy(context, dst, strlen(dst));
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

#ifdef _DEBUG_ROUTER_DEV_
	DEBUG_WARN("cmd_set:%s\n", cmd);
#endif

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
	//run_init_script("all");

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

//all init
	run_init_script("all");

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

	//wifi name
	char wifi_name[CONTEXT] = { 0 };
	char wifi_key[CONTEXT] = { 0 };

	//ppp name
	char ppp_name[CONTEXT] = { 0 };
	char ppp_key[CONTEXT] = { 0 };

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

	if ((cmd_get("flash get WLAN1_SSID;", (void*)wifi_name) < 0))
	{
		DEBUG_ERR("flash get WLAN1_SSID error\n");
		return(-1);
	}

	if ((cmd_get("flash get WLAN1_WPA_PSK;", (void*)wifi_key) < 0))
	{
		DEBUG_ERR("flash get WLAN1_WPA_PSK error\n");
		return(-1);
	}

	if ((cmd_get("flash get PPP_USER_NAME;", (void*)ppp_name) < 0))
	{
		DEBUG_ERR("flash get PPP_USER_NAME error\n");
		return(-1);
	}

	if ((cmd_get("flash get PPP_PASSWORD;", (void*)ppp_key) < 0))
	{
		DEBUG_ERR("flash get PPP_PASSWORD error\n");
		return(-1);
	}
	//combinate response info
	sprintf(ptr, "\{\"IP\":\"%s\",\"version\":\"3.4.6.7\",\"lan_mac\":\"%s\",\"wan_mac\":\"%s\",\"wifi_name\":%s,\"wifi_key\":%s,\"ppp_name\":%s,\"ppp_key\":%s}",
			def_ip_addr, hw_nic0_addr, hw_nic1_addr, wifi_name, wifi_key, ppp_name, ppp_key);
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

// init all
	run_init_script("all");

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

// init all
	run_init_script("all");

	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", result);
	return 0;
}

#if 0
int calc_mac(char* mac, void *context)                                                                                                                                       
{
	if ((mac == NULL) || (context == NULL))
	{
		return -1;
	}

	mac[3] += 0x10;
	strcpy((char*)context, mac);

	return 0;
}
#else
int calc_mac(const unsigned char* mac, ...)
{
	va_list list;
	int sum = 0;
	char *context;

	unsigned char *addr = mac;

	if (mac == NULL)
	{
		return -1;
	}

	va_start(list, mac);

	do {
		addr[3] += 0x10;
		context = va_arg(list, char*);

		if(context)
		{
			strcpy(context, mac);
		}
	} while(context);

	va_end(list);

	return 0;
}
#endif

//mac addr config
static int set_mac_addr(router_mac_t *config, void* context)
{
	char result[64], cmd[128];
	unsigned char mac0[6], mac1[6], mac2[6], mac3[6], mac4[6];
	char mac_0[2], mac_1[2], mac_2[2], mac_3[2], mac_4[2], mac_5[2];
	int ret;

	if ((context == NULL) || (config == NULL))
	{
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("set wan config error\n");
#endif
		return -1;
	}

#ifdef _DEBUG_ROUTER_DEV_
	DEBUG_WARN("%s\n", config->mac);
#endif

	ret = sscanf(config->mac, "%02s%02s%02s%02s%02s%02s", mac_0, mac_1, mac_2, mac_3, mac_4, mac_5);

	mac0[0] = strtol(mac_0, NULL, 16);
	mac0[1] = strtol(mac_1, NULL, 16);
	mac0[2] = strtol(mac_2, NULL, 16);
	mac0[3] = strtol(mac_3, NULL, 16);
	mac0[4] = strtol(mac_4, NULL, 16);
	mac0[5] = strtol(mac_5, NULL, 16);

	calc_mac(mac0, mac1, mac2, mac3, mac4, NULL);

#ifdef _DEBUG_ROUTER_DEV_
	DEBUG_WARN("%02x,%02x,%02x,%02x,%02x,%02x\n", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);
	DEBUG_WARN("%02x,%02x,%02x,%02x,%02x,%02x\n", mac1[0], mac1[1], mac1[2], mac1[3], mac1[4], mac1[5]);
	DEBUG_WARN("%02x,%02x,%02x,%02x,%02x,%02x\n", mac2[0], mac2[1], mac2[2], mac2[3], mac2[4], mac2[5]);
	DEBUG_WARN("%02x,%02x,%02x,%02x,%02x,%02x\n", mac3[0], mac3[1], mac3[2], mac3[3], mac3[4], mac3[5]);
	DEBUG_WARN("%02x,%02x,%02x,%02x,%02x,%02x\n", mac4[0], mac4[1], mac4[2], mac4[3], mac4[4], mac4[5]);
#endif

// eth0	
	sprintf(cmd, "flash set HW_NIC0_ADDR %02x%02x%02x%02x%02x%02x;", mac1[0], mac1[1], mac1[2], mac1[3], mac1[4], mac1[5]);
	if (cmd_set(cmd, result) < 0)
	{
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("flash default error\n");
#endif
		return(-1);
	}

// eth1
	sprintf(cmd, "flash set HW_NIC1_ADDR %02x%02x%02x%02x%02x%02x;", mac2[0], mac2[1], mac2[2], mac2[3], mac2[4], mac2[5]);
	if (cmd_set(cmd, result) < 0)
	{
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("flash default error\n");
#endif
		return(-1);
	}

// wlan0
	sprintf(cmd, "flash set HW_WLAN0_WLAN_ADDR %02x%02x%02x%02x%02x%02x;", mac3[0], mac3[1], mac3[2], mac3[3], mac3[4], mac3[5]);
	if (cmd_set(cmd, result) < 0)
	{
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("flash default error\n");
#endif
		return(-1);
	}

// wlan1
	sprintf(cmd, "flash set HW_WLAN0_WLAN_ADDR %02x%02x%02x%02x%02x%02x;", mac4[0], mac4[1], mac4[2], mac4[3], mac4[4], mac4[5]);
	if (cmd_set(cmd, result) < 0)
	{
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("flash default error\n");
#endif
		return(-1);
	}

// init all
	run_init_script("all");

	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", result);
	return 0;
}

//wan config static IP WAN_DHCP:0
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

	sprintf(cmd, "flash set WAN_DHCP 0; flash set WAN_IP_ADDR %s; flash set WAN_SUBNET_MASK %s; flash set WAN_DEFAULT_GATEWAY %s;flash set DNS1 %s;",
			config->ip,  config->mask, config->getway, config->dns);

	if (cmd_set(cmd, result) < 0)
	{
		DEBUG_ERR("flash default error\n");
		return(-1);
	}

// init all
	run_init_script("all");

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

// init all
	run_init_script("all");

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
	router->wan_config_mac = set_mac_addr;
	router->wifi_config = set_wifi_config;

	return (void*)router;
}
