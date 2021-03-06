/*************************************************************************
	> File Name: paser.h
	> Author: fwj
	> Mail: fuwenjie2011@126.com 
	> Created Time: 2015年11月21日 星期六 16时14分32秒
 ************************************************************************/
#ifndef _ROUTER_BASE_I_H_
#define _ROUTER_BASE_I_H_

#define DEFAULT_LENGTH 64

typedef enum {
	BOXSET = 0,
	PHONE
} DEVICE_TYPE_t;
//cmd head
typedef struct {
#define CMD_LENGTH 64
	char version[CMD_LENGTH];
	char cmd[CMD_LENGTH];
	char cmd_i[CMD_LENGTH];
} router_cmd_t;

//router_id
typedef struct {
	char id[DEFAULT_LENGTH];
	char data[DEFAULT_LENGTH];
} router_id_t;

//router_info
typedef struct {
#define INFO_LENGTH 64
	char ip[INFO_LENGTH];
	char version[INFO_LENGTH];
	char wifi_name[INFO_LENGTH];
	char lan_mac[INFO_LENGTH];
	char wan_man[INFO_LENGTH];
} router_info_t;

//pppoe config
typedef struct {
	char name[DEFAULT_LENGTH];
	char key[DEFAULT_LENGTH];
} router_wan_pppoe_t;

//dhcp config
typedef struct {
	char cmd[DEFAULT_LENGTH];
} router_wan_dhcp_t;

//dhcp config
typedef struct {
	char ip[DEFAULT_LENGTH];
	char mask[DEFAULT_LENGTH];
	char getway[DEFAULT_LENGTH];
	char dns[DEFAULT_LENGTH];
} router_wan_ip_t;

//router status
typedef struct {
	int status;
} router_status_t;

//router mac address
typedef struct {
	char mac[DEFAULT_LENGTH];
} router_mac_t;

//wifi config
typedef struct {
	char wifiType1[DEFAULT_LENGTH];
	char wifiType2[DEFAULT_LENGTH];
	char name[DEFAULT_LENGTH];
	char key_type[DEFAULT_LENGTH];
	char key[DEFAULT_LENGTH];
} router_wifi_t;

//route reboot
typedef struct {
	int reboot;
} router_reboot_t;

//router reset
typedef struct {
	int reset;
} router_reset_t;

//router reoeater
typedef struct {
	char name[DEFAULT_LENGTH];
	char channel[DEFAULT_LENGTH];
	char key[DEFAULT_LENGTH];
	char key_type[DEFAULT_LENGTH];
} router_repeater_t;

//router check update
typedef struct {
	char version[DEFAULT_LENGTH];
} router_version_t;

//router set update
typedef struct {
	int update;
} router_update_t;

#endif
