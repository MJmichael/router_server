/*************************************************************************
	> File Name: router_dev.h
	> Author: fwj
	> Mail: fuwenjie2011@126.com 
	> Created Time: 2015年11月21日 星期六 16时09分54秒
 ************************************************************************/
#ifndef _ROUTER_DEV_H
#define _ROUTER_DEV_H
#include "router_base_i.h"

typedef struct {
	int (*reboot)(void*);
	int (*reset)(void*);
	int (*search)(router_id_t*, void*);
	int (*wan_config_pppoe)(router_wan_pppoe_t*, void*);
	int (*wan_config_ip)(router_wan_ip_t*, void*);
	int (*wan_config_mac)(router_mac_t*, void*);
	int (*wan_config_dhcp)(router_wan_dhcp_t*, void*);
	int (*wifi_config)(router_wifi_t*, void*);
} router_dev_t;


void *router_dev_open(void);
#endif
