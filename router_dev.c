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
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_pppox.h>
#include <linux/if_packet.h>
#include <linux/wireless.h>

#include "parser.h"
#include "boa.h"
#include "asp_page.h"
#include "apmib.h"
#include "utility.h"
#include "router_dev.h"

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

static void killSomeDaemon(void)
{
	system("killall -9 sleep 2> /dev/null");
	system("killall -9 routed 2> /dev/null");
	system("killall -9 dnrd 2> /dev/null");
	system("killall -9 ntpclient 2> /dev/null");
	system("killall -9 lld2d 2> /dev/null");
	system("killall -9 reload 2> /dev/null");		
	system("killall -9 iapp 2> /dev/null");	
	system("killall -9 wscd 2> /dev/null");
	system("killall -9 mini_upnpd 2> /dev/null");
	system("killall -9 iwcontrol 2> /dev/null");
	system("killall -9 auth 2> /dev/null");
	system("killall -9 disc_server 2> /dev/null");
	system("killall -9 igmpproxy 2> /dev/null");
	system("echo 1,1 > /proc/br_mCastFastFwd");
	system("killall -9 syslogd 2> /dev/null");
	system("killall -9 klogd 2> /dev/null");
	system("killall -9 ppp_inet 2> /dev/null");
}

static int getPid(char *filename)
{
	struct stat status;
	char buff[100];
	FILE *fp;

	if ( stat(filename, &status) < 0) {
		return -1;
	}

	fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "Read pid file error!\n");
		return -1;
	}
	fgets(buff, 100, fp);
	fclose(fp);

	return (atoi(buff));
}

static void run_init_script(char *arg)
{
	int pid=0;
	int i;
	char tmpBuf[MAX_MSG_BUFFER_SIZE]={0};

	snprintf(tmpBuf, MAX_MSG_BUFFER_SIZE, "%s/%s.pid", _DHCPD_PID_PATH, _DHCPD_PROG_NAME);
	pid = getPid(tmpBuf);
	if ( pid > 0) { 
		kill(pid, SIGUSR1);
	}

	usleep(1000);

	if (pid > 0) {
		system("killall -9 udhcpd 2> /dev/null");
		system("rm -f /var/run/udhcpd.pid 2> /dev/null");
	}
	killSomeDaemon();

	system("killsh.sh");	

	sprintf(tmpBuf, "%s gw %s", _CONFIG_SCRIPT_PROG, arg);
	for(i=3; i<sysconf(_SC_OPEN_MAX); i++)
	{
		close(i);
	}

	sleep(1);
	system(tmpBuf);
}

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

#if 1
static int router_repeater_get(void* context)
{
	char* ptr = (char*)context;
	int length = 0;
	char repeater_enabled1[CONTEXT] = { 0 };
	char repeater_enabled2[CONTEXT] = { 0 };
	char wifi_name1[CONTEXT] = { 0 };
	char wifi_channel1[CONTEXT] = { 0 };
	char wifi_name2[CONTEXT] = { 0 };
	char wifi_channel2[CONTEXT] = { 0 };
	if ((cmd_get("flash get REPEATER_ENABLED1;", (void*)repeater_enabled1) < 0)){
			printf("@@@@@@lash get REPEATER_ENABLED1 error\n");
			return(-1);
	}
	length = strlen(repeater_enabled1);
	repeater_enabled1[length] = '\0';
	printf("@@@@@@ length = %d,repeater_enabled1 = %s\n",length,repeater_enabled1);

	if ((cmd_get("flash get REPEATER_ENABLED2;", (void*)repeater_enabled2) < 0)){
		printf("@@@@@@lash get REPEATER_ENABLED2 error\n");
		return(-1);
	}
	length = strlen(repeater_enabled2);
	repeater_enabled2[length] = '\0';
	printf("@@@@@@ length = %d,repeater_enabled2 = %s\n",length,repeater_enabled2);
	
	if((0 == strcmp(repeater_enabled1,"0")) && (0 == strcmp(repeater_enabled2,"0"))){
		sprintf(ptr, "\{\"repeater\":\"0\"}");
		printf("@@@@@@ ptr = %s\n",ptr);
	}else if(0 == strcmp(repeater_enabled1,"1")){
		if ((cmd_get("flash get REPEATER_SSID1;", (void*)wifi_name1) < 0)){
			printf("@@@@@@ flash get REPEATER_SSID1 error\n");
			return(-1);
		}
		if ((cmd_get("flash get WLAN0_CHANNEL;", (void*)wifi_channel1) < 0)){
			printf("@@@@@@ flash get WLAN0_CHANNEL error\n");
			return(-1);
		}
		sprintf(ptr, "\{\"repeater\":\"1\",\"ssid\":%s,\"channel\":%s}",wifi_name1, wifi_channel1);
		printf("@@@@@@ ptr = %s\n",ptr);
	}else if(0 == strcmp(repeater_enabled2,"1")){
		if ((cmd_get("flash get REPEATER_SSID2;", (void*)wifi_name2) < 0)){
			printf("@@@@@@ flash get REPEATER_SSID2 error\n");
			return(-1);
		}
		if ((cmd_get("flash get WLAN1_CHANNEL;", (void*)wifi_channel2) < 0)){
			printf("@@@@@@ flash get WLAN1_CHANNEL error\n");
			return(-1);
		}
		sprintf(ptr, "\{\"repeater\":\"1\",\"ssid\":%s,\"channel\":%s}",wifi_name2, wifi_channel2);
		printf("@@@@@@ ptr = %s\n",ptr);
	}
	return 0;
}
//iw get ext
char WLAN_IF[20];
static SS_STATUS_Tp pStatus=NULL;
static SS_DEVICE_Tp deviceInfo = NULL;

static inline int iw_get_ext(int skfd,           /* Socket to the kernel */
		                     char* ifname,         /* Device name */
				             int request,        /* WE ID */
							 struct iwreq* pwrq)           /* Fixed part of the request */
{
	/* Set device name */
	strncpy(pwrq->ifr_name, ifname, IFNAMSIZ);
	/* Do the request */
    return(ioctl(skfd, request, pwrq));
}

#if 0 /* just for wlan test */
int getWlJoinRequest(char *interface, pBssDscr pBss, unsigned char *res)
{
#ifndef NO_ACTION
	int skfd=0;
	struct iwreq wrq;

	printf("%s %d\n", __FUNCTION__, __LINE__);
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;
	/* Get wireless name */
	printf("%s %d\n", __FUNCTION__, __LINE__);
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
		/* If no wireless name : no wireless extensions */
		return -1;

	printf("%s %d\n", __FUNCTION__, __LINE__);
	wrq.u.data.pointer = (caddr_t)pBss;
	wrq.u.data.length = sizeof(BssDscr);

	printf("%s %d\n", __FUNCTION__, __LINE__);
	if (iw_get_ext(skfd, interface, SIOCGIWRTLJOINREQ, &wrq) < 0)
		return -1;
	printf("%s %d\n", __FUNCTION__, __LINE__);

	close( skfd );

	*res = *(unsigned char *)&wrq.u.data.pointer[0];
#else
	return -1;
#endif

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
int getWlJoinResult(char *interface, unsigned char *res)
{
	int skfd;
	struct iwreq wrq;

	printf("%s %d\n", __FUNCTION__, __LINE__);
	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0){
		/* If no wireless name : no wireless extensions */
		close( skfd );
		return -1;
	}
	printf("%s %d\n", __FUNCTION__, __LINE__);
	wrq.u.data.pointer = (caddr_t)res;
	wrq.u.data.length = 1;

	printf("%s %d\n", __FUNCTION__, __LINE__);
	if (iw_get_ext(skfd, interface, SIOCGIWRTLJOINREQSTATUS, &wrq) < 0){
		close( skfd );
		return -1;
	}
	printf("%s %d\n", __FUNCTION__, __LINE__);
	close( skfd );

	return 0;
}

void Join_ssid(void)
{
	BssDscr dscr;
	unsigned char res = 0;
	int wait_time = 0;

	dscr.bdBssId[0] = 0x1c;
	dscr.bdBssId[1] = 0xfa;
	dscr.bdBssId[2] = 0x68;
	dscr.bdBssId[3] = 0x23;
	dscr.bdBssId[4] = 0x01;
	dscr.bdBssId[5] = 0x4c;

//	dscr.bdType = 16;
	dscr.ChannelNumber = 11;
//	dscr.bdBrates = 15;

	printf("%s %d\n", __FUNCTION__, __LINE__);
	getWlJoinRequest("wlan1", &dscr, &res);
	printf("%s %d\n", __FUNCTION__, __LINE__);

	while(1) {
		if ( res == 1 ) { // wait
			/*prolong join wait time for pocket ap*/
			if (wait_time++ > 10)
			{
				goto ss_err;
			}
			sleep(1);
			continue;
		}
		break;
	}

ss_err: 
	printf("res:%d\n", res);
}
#endif

static int getWlSiteSurveyRequestForRepeater(char *interface, int *pStatus)
{
#ifndef NO_ACTION
    int skfd=0;
    struct iwreq wrq;
    unsigned char result;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0){
      /* If no wireless name : no wireless extensions */
      close( skfd );
        return -1;
	}
    wrq.u.data.pointer = (caddr_t)&result;
    wrq.u.data.length = sizeof(result);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLSCANREQ, &wrq) < 0){
    	//close( skfd );
		//return -1;
	}
    close( skfd );

    if ( result == 0xff )
    	*pStatus = -1;
    else
	*pStatus = (int) result;
#else
	*pStatus = -1;
#endif
#ifdef CONFIG_RTK_MESH 
	// ==== modified by GANTOE for site survey 2008/12/26 ==== 
	return (int)*(char*)wrq.u.data.pointer; 
#else
	return 0;
#endif
}

static int getWlSiteSurveyResultForRepeater(char *interface, SS_STATUS_Tp pStatus )
{
#ifndef NO_ACTION
    int skfd=0;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1){
		printf("@@@@@@@@ error 1th\n");
		return -1;
	}
    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0){
      /* If no wireless name : no wireless extensions */
	  printf("@@@@@@@@ error 2th\n");
      close( skfd );
        return -1;
	}
    wrq.u.data.pointer = (caddr_t)pStatus;

    if ( pStatus->number == 0 )
    	wrq.u.data.length = sizeof(SS_STATUS_T);
    else
        wrq.u.data.length = sizeof(pStatus->number);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLGETBSSDB, &wrq) < 0){
    	close( skfd );
	printf("@@@@@@@@ error 3th\n");
	return -1;
	}
    close( skfd );
#else
	printf("@@@@@@@@ error 4th\n");
	return -1 ;
#endif

    return 0;
}

static int getWlBssInfoForRepeater(char *interface, bss_info *pInfo)
{
#ifndef NO_ACTION
    int skfd=0;
    struct iwreq wrq;



    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;
    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
      /* If no wireless name : no wireless extensions */
      {
      	 close( skfd );
        return -1;
      }

    wrq.u.data.pointer = (caddr_t)pInfo;
    wrq.u.data.length = sizeof(bss_info);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLGETBSSINFO, &wrq) < 0){
    	 close( skfd );
	return -1;
	}
    close( skfd );
#else
    memset(pInfo, 0, sizeof(bss_info)); 
#endif

    return 0;
}

static void whatWillSend(void* context){
	char tempBuffer[4096];
	char tempConBuffer[1024];
	char* ptr = (char*)context;
	int i = 0;
	int tempNumber = deviceInfo->numberRepeater0;
	strlcpy(tempBuffer,"{",sizeof(tempBuffer));
	for(i= 0;i<tempNumber;i++){
		sprintf(tempConBuffer, "\"ssid\":\"%s\",\"bssid\":%s,\"channel\":%s,\"signal\":%s,",	
			deviceInfo->dInfoRepeater0[i].ssid,deviceInfo->dInfoRepeater0[i].bssid,deviceInfo->dInfoRepeater0[i].channel,deviceInfo->dInfoRepeater0[i].signal);
		strlcat(tempBuffer,tempConBuffer,sizeof(tempBuffer));
	}
	
	tempNumber = deviceInfo->numberRepeater1;
	for(i= 0;i<tempNumber;i++){
		if(i<(tempNumber -1)){
			sprintf(tempConBuffer, "\"ssid\":\"%s\",\"bssid\":%s,\"channel\":%s,\"signal\":%s,",	
				deviceInfo->dInfoRepeater1[i].ssid,deviceInfo->dInfoRepeater1[i].bssid,deviceInfo->dInfoRepeater1[i].channel,deviceInfo->dInfoRepeater1[i].signal);
			strlcat(tempBuffer,tempConBuffer,sizeof(tempBuffer));
		}else{
			sprintf(tempConBuffer, "\"ssid\":%s,\"bssid\":%s,\"channel\":%s,\"signal\":%s",
				deviceInfo->dInfoRepeater1[i].ssid,deviceInfo->dInfoRepeater1[i].bssid,deviceInfo->dInfoRepeater1[i].channel,deviceInfo->dInfoRepeater1[i].signal);
			strlcat(tempBuffer,tempConBuffer,sizeof(tempBuffer));
		}
	}

	strlcat(tempBuffer,"}",sizeof(tempBuffer));
	sprintf(ptr,"%s",tempBuffer);
	printf("@@@@@@ check 8th ptr = %s\n",ptr);
}


static void wlSiteSurveyTblLoad(){
	int i;
	int wifiNumber= 0;
	FILE* sstabRepeater0;
	FILE* sstabRepeater1;
	char buffer[1024];
	
	if (deviceInfo ==NULL) {
		deviceInfo = calloc(1, sizeof(SS_DEVICE_T));
		if ( deviceInfo== NULL ) {
			printf("@@@@@@@ Allocate SS_DEVICE_Info buffer failed!\n");
			return ;
		}
	}
#if 1
	sstabRepeater0 = fopen("/proc/wlan0/SS_Result", "r");
	if (sstabRepeater0 == NULL) {
		free(deviceInfo);  
		deviceInfo = NULL;
		printf("@@@@@@ failed to open /proc/wlan0/SS_Result (%s)\n", strerror(errno));
		return;
	}else{
		deviceInfo->numberRepeater0 = 0;
		while (fgets(buffer, sizeof(buffer)-1, sstabRepeater0)) {
			for (i = 0; buffer[i] && isspace(buffer[i]); ++i);
			if (buffer[i] == '\0' || buffer[i] == '=') continue;
		
		   	char* original = strdup(buffer);
		   	char* ssType = strtok(buffer+i, " :");
		 	 if(0 == strcmp(ssType ,"HwAddr")){
		  		char* bssid = strtok(NULL, " \t\n");
				deviceInfo->dInfoRepeater0[wifiNumber].bssid = strdup(bssid);
			}else if(0 == strcmp(ssType ,"Channel")){
				char* channel= strtok(NULL, " \t\n");
				deviceInfo->dInfoRepeater0[wifiNumber].channel = strdup(channel);
			}else if(0 == strcmp(ssType ,"SSID")){
				char* ssid= strtok(NULL, " \t\n");
				deviceInfo->dInfoRepeater0[wifiNumber].ssid = strdup(ssid);
			}else if(0 == strcmp(ssType ,"Type")){
				char* type= strtok(NULL, " \t\n");
				deviceInfo->dInfoRepeater0[wifiNumber].type = strdup(type);
			}else if(0 == strcmp(ssType ,"Signal")){
				char* signal= strtok(NULL, " \t\n");
				deviceInfo->dInfoRepeater0[wifiNumber].signal = strdup(signal);
				wifiNumber++;
			}
			free(original);
		}
		deviceInfo->numberRepeater0 = wifiNumber;
		printf("@@@@@@ deviceInfo->numberRepeater0=%d\n",deviceInfo->numberRepeater0);
	}
#endif
	wifiNumber = 0;
	sstabRepeater1 = fopen("/proc/wlan1/SS_Result", "r");
	if (sstabRepeater1 == NULL) {
		free(deviceInfo);  
		deviceInfo = NULL;
		printf("@@@@@@ failed to open /proc/wlan1/SS_Result (%s)\n", strerror(errno));
		return;
	}else{
		deviceInfo->numberRepeater1= 0;
		while (fgets(buffer, sizeof(buffer)-1, sstabRepeater1)) {
			for (i = 0; buffer[i] && isspace(buffer[i]); ++i);
			if (buffer[i] == '\0' || buffer[i] == '=') continue;
		
		   	char* original = strdup(buffer);
		   	char* ssType = strtok(buffer+i, " :");
		 	 if(0 == strcmp(ssType ,"HwAddr")){
		  		char* bssid = strtok(NULL, " \t\n");
				deviceInfo->dInfoRepeater1[wifiNumber].bssid = strdup(bssid);
			}else if(0 == strcmp(ssType ,"Channel")){
				char* channel= strtok(NULL, " \t\n");
				deviceInfo->dInfoRepeater1[wifiNumber].channel = strdup(channel);
			}else if(0 == strcmp(ssType ,"SSID")){
				char* ssid= strtok(NULL, " \t\n");
				deviceInfo->dInfoRepeater1[wifiNumber].ssid = strdup(ssid);
			}else if(0 == strcmp(ssType ,"Type")){
				char* type= strtok(NULL, " \t\n");
				deviceInfo->dInfoRepeater1[wifiNumber].type = strdup(type);
			}else if(0 == strcmp(ssType ,"Signal")){
				char* signal= strtok(NULL, " \t\n");
				deviceInfo->dInfoRepeater1[wifiNumber].signal = strdup(signal);
				wifiNumber++;
			}
			free(original);
		}
		deviceInfo->numberRepeater1 = wifiNumber;
		printf("@@@@@@ deviceInfo->numberRepeater1=%d\n",deviceInfo->numberRepeater1);
	}
#if 0
	for (i=0; i<deviceInfo->numberRepeater0; i++) { 
		printf("@@@@@@ ssid=%s\n",deviceInfo->dInfoRepeater0[i].ssid);
		printf("@@@@@@ bssid=%s\n",deviceInfo->dInfoRepeater0[i].bssid);
		printf("@@@@@@ channel=%s\n",deviceInfo->dInfoRepeater0[i].channel);
		printf("@@@@@@ type=%s\n",deviceInfo->dInfoRepeater0[i].type);
		printf("@@@@@@ signal=%s\n",deviceInfo->dInfoRepeater0[i].signal);
		printf("---------------------5G Over---------------------\n");
	}

	for (i=0; i<deviceInfo->numberRepeater1; i++) { 
		printf("@@@@@@ ssid=%s\n",deviceInfo->dInfoRepeater1[i].ssid);
		printf("@@@@@@ bssid=%s\n",deviceInfo->dInfoRepeater1[i].bssid);
		printf("@@@@@@ channel=%s\n",deviceInfo->dInfoRepeater1[i].channel);
		printf("@@@@@@ type=%s\n",deviceInfo->dInfoRepeater1[i].type);
		printf("@@@@@@ signal=%s\n",deviceInfo->dInfoRepeater1[i].signal);
		printf("---------------------2.4G Over---------------------\n");
	}
#endif
	fclose(sstabRepeater0);
	fclose(sstabRepeater1);
}

static void wlSiteSurveyTblRequest( ){
	 int status;
	 int wait_time;
	 unsigned char res;
	while (1) {
		printf("----------WLAN_IF = %s\n",WLAN_IF);
		switch(getWlSiteSurveyRequestForRepeater(WLAN_IF, &status)) { 
			case -2: 
				printf("----------Auto scan running!!\n"); 
				break; 
			case -1: 
				printf("----------Site-survey request failed!\n"); 
				break; 
			default: 
				break; 
		} 
		if (status != 0) {
			if (wait_time++ > 15) {
				free(pStatus);  
				pStatus = NULL;
				printf("----------scan request timeout\n");
				goto REQUESTERROR;
			}
			sleep(1);
		}else{
			break;
		}
	}
	wait_time = 0;
	while (1) {
		res = 1;	
		if ( getWlSiteSurveyResultForRepeater(WLAN_IF, (SS_STATUS_Tp)&res) < 0 ) {
			goto REQUESTERROR;
		}
		if (res == 0xff) {
			if (wait_time++ > 20) {
				goto REQUESTERROR;
			}
			sleep(1);
		}else{
			break;
		}
	}
	
REQUESTERROR:
				free(pStatus);  
				pStatus = NULL;
				return;
		
}

static int  wlSiteSurveyTblGetRepeater0(){
	BssDscr *pBss;
	bss_info bss;
	
	if (pStatus==NULL) {
		pStatus = calloc(1, sizeof(SS_STATUS_T));
		if ( pStatus == NULL ) {
			printf("@@@@@@@ Allocate buffer failed!\n");
			return -1;
		}
	}
	
	pStatus->number = 0;  
	sprintf(WLAN_IF,"%s","wlan0");
	printf("@@@@@@ WLAN_IF = %s\n",WLAN_IF);
	if ( getWlSiteSurveyResultForRepeater(WLAN_IF, pStatus) < 0 ) { 
		printf("@@@@@@Read site-survey status failed!\n");
		free(pStatus);  
		pStatus = NULL;
		return -1;
	}
	if ( getWlBssInfoForRepeater(WLAN_IF, &bss) < 0) {
		free(pStatus);  
		pStatus = NULL;
		printf("@@@@@@Get bssinfo failed!");
		return -1;
	}
	
	if(pStatus->number == 0){
		printf("@@@@@@ check 1-0th\n");
		return  -2;
	}else if(pStatus->number == 0xff){
		printf("@@@@@@ check 2-0th\n");
		return -2;
	}
	free(pStatus);  
	pStatus = NULL;
	return 0;
}

static int wlSiteSurveyTblGetRepeater1(){
	BssDscr *pBss;
	bss_info bss;
	
	if (pStatus==NULL) {
		pStatus = calloc(1, sizeof(SS_STATUS_T));
		if ( pStatus == NULL ) {
			printf("@@@@@@@ Allocate buffer failed!\n");
			return -1;
		}
	}
	
	pStatus->number = 0;  
	sprintf(WLAN_IF,"%s","wlan1");
	printf("@@@@@@ WLAN_IF = %s\n",WLAN_IF);
	if ( getWlSiteSurveyResultForRepeater(WLAN_IF, pStatus) < 0 ) { 
		printf("@@@@@@Read site-survey status failed!\n");
		free(pStatus);  
		pStatus = NULL;
		return -1;
	}
	if ( getWlBssInfoForRepeater(WLAN_IF, &bss) < 0) {
		free(pStatus);  
		pStatus = NULL;
		printf("@@@@@@Get bssinfo failed!");
		return -1;
	}
	
	if(pStatus->number == 0){
		printf("@@@@@@ check 1-1th\n");
		return -2;
	}else if(pStatus->number == 0xff){
		printf("@@@@@@ check 2-1th\n");
		return -2;
	}
	free(pStatus);  
	pStatus = NULL;
	return 0;
}

static int router_scanf(void* context)
{
	switch(wlSiteSurveyTblGetRepeater0()){
		case -2: 
			printf("----------need request then get!\n"); 
			wlSiteSurveyTblRequest( );
			wlSiteSurveyTblGetRepeater0();
			break; 
		case -1: 
			printf("----------get failed\n"); 
			return -1; 
		default: 
			break; 
	}

	switch(wlSiteSurveyTblGetRepeater1()){
		case -2: 
			printf("----------need request then get!\n"); 
			wlSiteSurveyTblRequest( );
			wlSiteSurveyTblGetRepeater1();
			break; 
		case -1: 
			printf("----------get failed\n"); 
			return -1; 
		default: 
			break; 
	}
	wlSiteSurveyTblLoad();
	whatWillSend(context);	
	return 0;
}
static int router_repeater_config(router_repeater_t *config, void* context)
{
	char repeaterWifi[64],result[64], cmd[256];
	int i = 0;

	if ((context == NULL) || (config == NULL)){
#ifdef _DEBUG_ROUTER_DEV_
		DEBUG_ERR("@@@@@@ set repeater config error\n");
#endif
		return -1;
	}
	sprintf(repeaterWifi, config->name);
	int length = strlen(repeaterWifi);
	repeaterWifi[length] = '\0';
	printf("@@@@@@ length = %d,repeaterWifi = %s\n",length,repeaterWifi);

	printf("@@@@@@ repeater check 1th\n");
	sprintf(cmd, "flash set DHCP 0");
	if (cmd_set(cmd, result) < 0){
		DEBUG_ERR("@@@@@@ flash default error 1th\n");
		return(-1);
	}

	for (i=0; i<deviceInfo->numberRepeater0; i++) { 
		if(0 == strcmp(repeaterWifi,deviceInfo->dInfoRepeater0[i].ssid)){
#if 1
			printf("@@@@@@ repeater check 2-0th\n");
			sprintf(cmd, "flash set REPEATER_ENABLED2 0;flash set REPEATER_ENABLED1 1; flash set REPEATER_SSID1 %s; flash set WLAN0_CHANNEL %s",
					config->name,  config->channel);
			if (cmd_set(cmd, result) < 0){
				DEBUG_ERR("@@@@@@ flash default error 2th\n");
				return(-1);
			}
			
			printf("@@@@@@ repeater check 3-0th\n");
			if (strcmp(config->key_type, "WPA-Mixed") == 0){
				printf("@@@@@@ repeater check 4-0th\n");
				sprintf(cmd, "flash set WLAN0_VAP4_WLAN_DISABLED 0");
				if (cmd_set(cmd, result) < 0){
					DEBUG_ERR("@@@@@@ flash default error 3th\n");
					return(-1);
				}
			
				printf("@@@@@@ repeater check 5-0th\n");
				sprintf(cmd, "flash set WLAN0_VAP4_ENCRYPT 6; WLAN0_VAP4_WPA_CIPHER_SUITE 3");
				if (cmd_set(cmd, result) < 0){
					DEBUG_ERR("@@@@@@ flash default error 4th\n");
					return(-1);
				}
			
				printf("@@@@@@ repeater check 6-0th\n");
				sprintf(cmd, "flash set WLAN0_VAP4_WPA2_CIPHER_SUITE 3");
				if (cmd_set(cmd, result) < 0){
					DEBUG_ERR("@@@@@@ flash default error 5th\n");
					return(-1);
				}
				
				printf("@@@@@@ repeater check 7-0th\n");
				sprintf(cmd, "flash set WLAN0_VAP4_SSID %s; flash set WLAN0_VAP4_WPA_PSK %s",
					config->name,  config->key);
				if (cmd_set(cmd, result) < 0){
					DEBUG_ERR("@@@@@@ flash default error 6th\n");
					return(-1);
				}
			
				printf("@@@@@@ repeater check 8-0th\n");
				sprintf(cmd, "flash set WLAN1_VAP4_WSC_CONFIGURED 1; flash set WLAN1_VAP4_WSC_SSID %s",
					config->name);
				if (cmd_set(cmd, result) < 0){
					DEBUG_ERR("@@@@@@ flash default error 7th\n");
					return(-1);
				}
			}
#endif
		}
	}

	for (i=0; i<deviceInfo->numberRepeater1; i++) { 
		if(0 == strcmp(repeaterWifi,deviceInfo->dInfoRepeater1[i].ssid)){
			printf("@@@@@@ repeater check 2-1th\n");
			sprintf(cmd, "flash set REPEATER_ENABLED1 0; flash set REPEATER_ENABLED2 1; flash set REPEATER_SSID2 %s; flash set WLAN1_CHANNEL %s",
					config->name,  config->channel);
			if (cmd_set(cmd, result) < 0){
				DEBUG_ERR("@@@@@@ flash default error 2th\n");
				return(-1);
			}
	
			printf("@@@@@@ repeater check 3-1th\n");
			if (strcmp(config->key_type, "WPA-Mixed") == 0){
				printf("@@@@@@ repeater check 4-1th\n");
				sprintf(cmd, "flash set WLAN1_VAP4_WLAN_DISABLED 0");
				if (cmd_set(cmd, result) < 0){
					DEBUG_ERR("@@@@@@ flash default error 3th\n");
					return(-1);
				}

				printf("@@@@@@ repeater check 5-1th\n");
				sprintf(cmd, "flash set WLAN1_VAP4_ENCRYPT 6; WLAN1_VAP4_WPA_CIPHER_SUITE 3");
				if (cmd_set(cmd, result) < 0){
					DEBUG_ERR("@@@@@@ flash default error 4th\n");
					return(-1);
				}

				printf("@@@@@@ repeater check 6-1th\n");
				sprintf(cmd, "flash set WLAN1_VAP4_WPA2_CIPHER_SUITE 3");
				if (cmd_set(cmd, result) < 0){
					DEBUG_ERR("@@@@@@ flash default error 5th\n");
					return(-1);
				}
		
				printf("@@@@@@ repeater check 7-1th\n");
				sprintf(cmd, "flash set WLAN1_VAP4_SSID %s; flash set WLAN1_VAP4_WPA_PSK %s",
					config->name,  config->key);
				if (cmd_set(cmd, result) < 0){
					DEBUG_ERR("@@@@@@ flash default error 6th\n");
					return(-1);
				}
	
				printf("@@@@@@ repeater check 8-1th\n");
				sprintf(cmd, "flash set WLAN1_VAP4_WSC_CONFIGURED 1; flash set WLAN1_VAP4_WSC_SSID %s",
					config->name);
				if (cmd_set(cmd, result) < 0){
					DEBUG_ERR("@@@@@@ flash default error 7th\n");
					return(-1);
				}
			}
		}
	}
	
	run_init_script("all");
	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", result);
	
	free(deviceInfo);	
	deviceInfo = NULL;

	return 0;
}

static int router_enrepeater_config(void* context)
{
	char result[64], cmd[256];

	if (context == NULL){
		DEBUG_ERR("@@@@@@ set repeater config error\n");
		return -1;
	}
	printf("@@@@@@ repeater check 1th\n");
	sprintf(cmd, "flash set DHCP 2");
	if (cmd_set(cmd, result) < 0){
		DEBUG_ERR("@@@@@@ flash default error 1th\n");
		return(-1);
	}
	
	printf("@@@@@@ repeater check 2th\n");
	sprintf(cmd, "flash set REPEATER_ENABLED1 0");
	if (cmd_set(cmd, result) < 0){
		DEBUG_ERR("@@@@@@ flash default error 2th\n");
		return(-1);
	}

	printf("@@@@@@ repeater check 3th\n");
	sprintf(cmd, "flash set REPEATER_ENABLED2 0");
	if (cmd_set(cmd, result) < 0){
		DEBUG_ERR("@@@@@@ flash default error 3th\n");
		return(-1);
	}
	
	run_init_script("all");
	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", result);

	return 0;
}
#endif
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
#if 1
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
	sprintf(cmd, "flash set HW_WLAN1_WLAN_ADDR %02x%02x%02x%02x%02x%02x;", mac4[0], mac4[1], mac4[2], mac4[3], mac4[4], mac4[5]);
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
#endif
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
	if (strcmp(config->wifiType1, "5GHz") == 0){

		if (strcmp(config->key_type, "None") == 0){
			sprintf(cmd0, "flash set WLAN0_SSID %s-5G; flash set WLAN0_ENCRYPT 0; flash set WLAN0_WPA_PSK %s",
					config->name, config->key);
		}else{
			sprintf(cmd0, "flash set WLAN0_SSID %s-5G; flash set WLAN0_ENCRYPT 2; flash set WLAN0_WPA_PSK %s",
					config->name, config->key);
		}

		if ((cmd_set(cmd0, result) < 0) ){
#ifdef _DEBUG_ROUTER_DEV_
			DEBUG_ERR("flash default error\n");
#endif
			return(-1);
		}
	}
	
	if (strcmp(config->wifiType2, "2.4GHz") == 0){
	
			if (strcmp(config->key_type, "None") == 0){
				sprintf(cmd1, "flash set WLAN1_SSID %s-2.4G; flash set WLAN1_ENCRYPT 0; flash set WLAN1_WPA_PSK %s",
						config->name, config->key);
			}else{
	
				sprintf(cmd1, "flash set WLAN1_SSID %s-2.4G; flash set WLAN1_ENCRYPT 2; flash set WLAN1_WPA_PSK %s",
						config->name, config->key);
			}
	
			if ((cmd_set(cmd1, result) < 0) ){
#ifdef _DEBUG_ROUTER_DEV_
				DEBUG_ERR("flash default error\n");
#endif
				return(-1);
			}
		}

// init all
	run_init_script("all");

	sprintf((char*)context, "\{\"STATUS\":\"%s\"}", result);
	
	return 0;
}

#define FIRMWARE_NAME "fw.bin"
int firmware_len=0;
char *firmware_data;

static int route_update_firmware(void)
{
	int fileLen = 0;
	char *buff = NULL;
	char tmpBuf[200];
	char *submitUrl;
	char lan_ip[30];	
	char lan_ip_buf[30];
	FILE *fd;
	
	struct stat fileStat = {0};
	int readLen=0,i=0;
	 if(!isFileExist(FIRMWARE_NAME))
	 {
		strcpy(tmpBuf, ("Error!form ware is not exist in usb storage!\n"));
		goto ret_err;
	 }
	 stat(FIRMWARE_NAME,&fileStat);
	 fileLen=fileStat.st_size;
		 
	fd = open(FIRMWARE_NAME, O_RDONLY);
	if (!fd){
		strcpy(tmpBuf, ("Open image file failed!\n"));
		goto ret_err;
	}
	lseek(fd, 0L, SEEK_SET);
	printf("<read image from mem device>\n");
	
	buff = malloc(fileLen + 17);
	if(buff == NULL)
	{
		sprintf(tmpBuf, ("malloc %d failed !\n"),fileLen+17);
		goto ret_err;
	}
	bzero(buff, fileLen+17);
		
	strcpy(buff,WINIE6_STR);
	buff[13] = 0x0d;
	buff[14] = 0x0a;
	buff[15] = 0x0d;
	buff[16] = 0x0a;
		
	readLen = read(fd, buff+17, fileLen);
	if(readLen != fileLen)
	{
		sprintf(tmpBuf, ("read %d but file len is %d, read fail!\n"), readLen, fileLen);
		goto ret_err;
	}

	firmware_data=buff;
	firmware_len=fileLen+17;	

	doFirmwareUpgrade(firmware_data, firmware_len, 0, tmpBuf);
	return;
ret_err:
	ERR_MSG(tmpBuf);
	return;	
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
	router->repeater_get = router_repeater_get;
	router->wifi_search=router_scanf;
	router->repeater_config = router_repeater_config;
	router->enrepeater_config = router_enrepeater_config;
	router->wan_config_pppoe = set_wan_pppoe;
	router->wan_config_ip = set_wan_ip;
	router->wan_config_dhcp = set_wan_dhcp;
	router->wan_config_mac = set_mac_addr;
	router->wifi_config = set_wifi_config;

	return (void*)router;
}
