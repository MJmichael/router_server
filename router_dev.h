/*************************************************************************
	> File Name: router_dev.h
	> Author: fwj
	> Mail: fuwenjie2011@126.com 
	> Created Time: 2015年11月21日 星期六 16时09分54秒
 ************************************************************************/
#ifndef _ROUTER_DEV_H
#define _ROUTER_DEV_H
#include "router_base_i.h"

#define MAX_BSS_DESC    64
#define _CONFIG_SCRIPT_PROG	"init.sh"
#define _DHCPD_PROG_NAME	"udhcpd"
#define _DHCPD_PID_PATH		"/var/run"

typedef struct _DeviceInfo{
	const char* ssid;  
	const char* bssid; 
	const char* channel;      
	const char* type;
	const char* signal;
} DeviceInfo;

typedef struct _sitesurvey_info {
	int numberRepeater0;
	DeviceInfo dInfoRepeater0[MAX_BSS_DESC];
	int numberRepeater1;
	DeviceInfo dInfoRepeater1[MAX_BSS_DESC];
}SS_DEVICE_T, *SS_DEVICE_Tp;

typedef struct {
	int (*reboot)(DEVICE_TYPE_t, void*);
	int (*reset)(DEVICE_TYPE_t, void*);
	int (*search)(DEVICE_TYPE_t, router_id_t*, void*);
	int (*repeater_get)(DEVICE_TYPE_t, void*);
	int (*wifi_search)(DEVICE_TYPE_t, void*);
	int (*repeater_config)(DEVICE_TYPE_t, router_repeater_t*, void*);
	int (*enrepeater_config)(DEVICE_TYPE_t, void*);
	int (*wan_config_pppoe)(DEVICE_TYPE_t, router_wan_pppoe_t*, router_wifi_t *, void*);
	int (*wan_config_ip)(DEVICE_TYPE_t, router_wan_ip_t*, router_wifi_t *, void*);
	int (*wan_config_mac)(DEVICE_TYPE_t, router_mac_t*, void*);
	int (*wan_config_dhcp)(DEVICE_TYPE_t, router_wan_dhcp_t*, router_wifi_t*, void*);
	int (*wifi_config)(DEVICE_TYPE_t, router_wifi_t*, void*);
	int (*check_update)(DEVICE_TYPE_t, router_version_t*, void*);
	int (*update_firmware)(DEVICE_TYPE_t, void*);
} router_dev_t;

void *router_dev_open(void);
#endif
