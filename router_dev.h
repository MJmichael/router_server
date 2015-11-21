/*************************************************************************
	> File Name: router_dev.h
	> Author: fwj
	> Mail: fuwenjie2011@126.com 
	> Created Time: 2015年11月21日 星期六 16时09分54秒
 ************************************************************************/
#ifndef ROUTER_DEV_H
#define ROUTER_DEV_H
#include "parser.h"

typedef struct {
//	void *handle;

	int (*reboot)(void);
	int (*reset)(void);
	int (*search)(void);
	int (*wan_config)(router_wan_t*);
	int (*wifi_config)(router_wifi_t*);
} router_dev_t;


void *router_dev_open(void);
#endif
