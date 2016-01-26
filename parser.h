/*************************************************************************
	> File Name: paser.h
	> Author: fwj
	> Mail: fuwenjie2011@126.com 
	> Created Time: 2015年11月21日 星期六 16时14分32秒
 ************************************************************************/
#ifndef _PASER_H_
#define _PASER_H_

#include "router_base_i.h"

int parser_cmd(char buf[], DEVICE_TYPE_t type, int* router_init, void* context);
int parser_init(void);
#endif
