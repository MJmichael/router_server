/*
 *      Web server handler routines for wlan stuffs
 *
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *
 *      $Id: fmwlan.c,v 1.69 2009/09/04 07:06:23 keith_huang Exp $
 *
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef WIFI_SIMPLE_CONFIG
#include <sys/time.h>
#endif

#include "boa.h"
#include "asp_page.h"
#include "apmib.h"
#include "apform.h"
#include "utility.h"

#ifdef WLAN_EASY_CONFIG
#include "../md5.h"
#endif

// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT 
#include "web_voip.h"
#endif

//#define SDEBUG(fmt, args...) printf("[%s %d]"fmt,__FUNCTION__,__LINE__,## args)
#define SDEBUG(fmt, args...) {}
//#define P2P_DEBUG(fmt, args...) printf("[%s %d]"fmt,__FUNCTION__,__LINE__,## args)
#define P2P_DEBUG(fmt, args...) {}

#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
extern void Stop_Domain_Query_Process(void);
extern void Reset_Domain_Query_Setting(void);
extern int Start_Domain_Query_Process;
#endif

#ifdef  CONFIG_RTL_P2P_SUPPORT
/*it indicate which interface we need to query P2P state 
  or no interface need to query(no any interface under P2P mode)
  when wifi mode change then refill it to 255*/
static int p2p_query_which_interface=255;
#endif

#ifdef WLAN_EASY_CONFIG
#define DO_CONFIG_WAIT_TIME	60
#define CONFIG_SUCCESS		0
#define AUTOCONF_PID_FILENAME	("/var/run/autoconf.pid")

static int wait_config = CONFIG_SUCCESS;
#endif

static SS_STATUS_Tp pStatus=NULL;

#ifdef CONFIG_RTK_MESH
#define _FILE_MESH_ASSOC "mesh_assoc_mpinfo"
#define _FILE_MESH_ROUTE "mesh_pathsel_routetable"
#define _FILE_MESH_ROOT  "mesh_root_info"
#define _FILE_MESH_PROXY "mesh_proxy_table"
#define _FILE_MESH_PORTAL "mesh_portal_table"		
#define _FILE_MESHSTATS  "mesh_stats"
#endif // CONFIG_RTK_MESH

#ifdef WIFI_SIMPLE_CONFIG
enum {	CALLED_FROM_WLANHANDLER=1, CALLED_FROM_WEPHANDLER=2, CALLED_FROM_WPAHANDLER=3, CALLED_FROM_ADVANCEHANDLER=4};
struct wps_config_info_struct {
	int caller_id;
	int wlan_mode;
	int auth;
	int shared_type;
	int wep_enc;
	int wpa_enc;
	int wpa2_enc;
	unsigned char ssid[MAX_SSID_LEN];
	int KeyId;
	unsigned char wep64Key1[WEP64_KEY_LEN];
	unsigned char wep64Key2[WEP64_KEY_LEN];
	unsigned char wep64Key3[WEP64_KEY_LEN];
	unsigned char wep64Key4[WEP64_KEY_LEN];
	unsigned char wep128Key1[WEP128_KEY_LEN];
	unsigned char wep128Key2[WEP128_KEY_LEN];
	unsigned char wep128Key3[WEP128_KEY_LEN];
	unsigned char wep128Key4[WEP128_KEY_LEN];
	unsigned char wpaPSK[MAX_PSK_LEN+1];
};
static struct wps_config_info_struct wps_config_info;
static void update_wps_configured(int reset_flag);
#endif

#if defined(CONFIG_RTL_92D_SUPPORT)
static int _isBandModeBoth()
{
	int val;
	apmib_get(MIB_WLAN_BAND2G5G_SELECT, (void *)&val);
	if(val == BANDMODEBOTH)
		return 1;
	else
		return 0;
}
#endif

static void _Start_Wlan_Applications(void)
{

#if defined (CONFIG_RTL_92D_SUPPORT)
	if(_isBandModeBoth())
		system("sysconf wlanapp start wlan0 wlan1 br0");
	else
		system("sysconf wlanapp start wlan0 br0");
#else
	system("sysconf wlanapp start wlan0 br0");
#endif
	sleep(1);
	/*sysconf upnpd 1(isgateway) 1(opmode is bridge)*/
	system("sysconf upnpd 1 1");
	sleep(1);
}

/////////////////////////////////////////////////////////////////////////////
#ifndef NO_ACTION
//Patch: kill some daemons to free some RAM in order to call "init.sh gw al"l more quickly
//which need more tests
void killSomeDaemon(void)
{
	system("killall -9 sleep 2> /dev/null");
	system("killall -9 routed 2> /dev/null");
	//	system("killall -9 pppoe 2> /dev/null");
	//	system("killall -9 pppd 2> /dev/null");
	//	system("killall -9 pptp 2> /dev/null");
	system("killall -9 dnrd 2> /dev/null");
	system("killall -9 ntpclient 2> /dev/null");
	//	system("killall -9 miniigd 2> /dev/null");	//comment for miniigd iptables rule recovery
	system("killall -9 lld2d 2> /dev/null");
	//	system("killall -9 l2tpd 2> /dev/null");	
	//	system("killall -9 udhcpc 2> /dev/null");	
	//	system("killall -9 udhcpd 2> /dev/null");	
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
#ifdef WLAN_HS2_CONFIG	
	system("killall -9 hs2 2> /dev/null");	
#endif
#ifdef CONFIG_IPV6
	system("killall -9 dhcp6c 2> /dev/null");
	system("killall -9 dhcp6s 2> /dev/null");
	system("killall -9 radvd 2> /dev/null");
	system("killall -9 ecmh 2> /dev/null");
	//kill mldproxy
	system("killall -9 mldproxy 2> /dev/null");
#endif
#ifdef CONFIG_SNMP
	system("killall -9 snmpd 2> /dev/null");
	system("rm -f /var/run/snmpd.pid");
#endif
}

int getPid(char *filename)
{
	struct stat status;
	char buff[100];
	FILE *fp;

	if ( stat(filename, &status) < 0)
		return -1;
	fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "Read pid file error!\n");
		return -1;
	}
	fgets(buff, 100, fp);
	fclose(fp);

	return (atoi(buff));
}

int run_init_script_flag = 1;
int needReboot = 1;

void run_init_script(char *arg)
{
	int pid=0;
	int i;
	char tmpBuf[MAX_MSG_BUFFER_SIZE]={0};

#ifdef REBOOT_CHECK
	if(run_init_script_flag == 1){
#endif

#ifdef RTK_USB3G
		system("killall -9 mnet 2> /dev/null");
		system("killall -9 hub-ctrl 2> /dev/null");
		system("killall -9 usb_modeswitch 2> /dev/null");
		system("killall -9 ppp_inet 2> /dev/null");
		system("killall -9 pppd 2> /dev/null");
		system("rm /etc/ppp/connectfile >/dev/null 2>&1");
#endif /* #ifdef RTK_USB3G */

#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
		Stop_Domain_Query_Process();
		Reset_Domain_Query_Setting();
#endif

#if defined(CONFIG_RTL_ULINKER)
		{
			extern int kill_ppp(void);
			int wan_mode, op_mode;

			apmib_get(MIB_OP_MODE,(void *)&op_mode);
			apmib_get(MIB_WAN_DHCP,(void *)&wan_mode);
			if(wan_mode == PPPOE && op_mode == GATEWAY_MODE)
			{
				kill_ppp();
			}

			stop_dhcpc();
			stop_dhcpd();
			clean_auto_dhcp_flag();
			disable_bridge_dhcp_filter();
		}
#endif

		snprintf(tmpBuf, MAX_MSG_BUFFER_SIZE, "%s/%s.pid", _DHCPD_PID_PATH, _DHCPD_PROG_NAME);
		pid = getPid(tmpBuf);
		if ( pid > 0)
			kill(pid, SIGUSR1);

		usleep(1000);

		if (pid > 0){
			system("killall -9 udhcpd 2> /dev/null");
			system("rm -f /var/run/udhcpd.pid 2> /dev/null");
		}

		//Patch: kill some daemons to free some RAM in order to call "init.sh gw all" more quickly
		//which need more tests especially for 8196c 2m/16m
		killSomeDaemon();

		system("killsh.sh");	// kill all running script	

#if 0
		run_init_script_flag = 0;
		needReboot = 0;
#endif
		// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
		web_restart_solar();
#endif

		sprintf(tmpBuf, "%s gw %s", _CONFIG_SCRIPT_PROG, arg);
		for(i=3; i<sysconf(_SC_OPEN_MAX); i++)
		{
			close(i);
		}

		sleep(1);
		system(tmpBuf);
		//set route host
		system("route add -host 255.255.255.255 dev br0");
	}
}

#endif //#ifndef NO_ACTION
