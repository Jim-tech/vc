// sshcmd.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "WS2tcpip.h"
#include "winsock2.h"
#include "windows.h"
#include "time.h"
#include "bru_cmd.h"

#pragma comment(lib, "ws2_32.lib")

#define SSH_PORT_LIST            {22, 27149}
#define PORT_DETECT_MAX_RETRY    60

FILE           *fplog = NULL;

typedef enum
{
	e_ssh_info  		= 0,
	e_ssh_quit_ok       = 1,
	e_ssh_quit_fail     = 2,
}upgrade_msg_e;

#pragma pack(1)
typedef struct
{
	int 			msgtype;
	int             ipaddr;
	char            snstr[32];
	char            macstr[32];
	char            verstr[128];
	char            process[256];
}upgrade_msg_s;
#pragma pack()

int ssh_port_detecting(char *ipstr, unsigned short *pport)
{
	int             rc = 0;
	int             ipaddr;
	unsigned short 	ports[] = SSH_PORT_LIST;
	int             cnt = sizeof(ports) / sizeof(unsigned short);
	SOCKET          hostsock;
	SOCKADDR_IN		servaddr;

	inet_pton(AF_INET, ipstr, &ipaddr);
	
	hostsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
	if (INVALID_SOCKET == hostsock)
	{
		dbgprint("socket failed!");
		return -1;
	}

	servaddr.sin_family =AF_INET;
	servaddr.sin_addr.s_addr = ipaddr;

	time_t time_start = time(NULL);
	
	int retry = 0;
	while (retry++ < PORT_DETECT_MAX_RETRY)
	{
		time_t time_now = time(NULL);

		if (time_now - time_start >= 120 && retry >= 3)
		{
			dbgprint("cost too much time(%d seconds), retry=%d", time_now - time_start, retry);
			break;
		}
		
		for (int i = 0; i < cnt; i++)
		{
			servaddr.sin_port = htons(ports[i]);
			
			rc = connect(hostsock,(LPSOCKADDR)&servaddr, sizeof(servaddr));	
			if (0 == rc)
			{
				closesocket(hostsock);
				*pport = ports[i];

				//wait in case system reboot by cpld
				Sleep(20000);
				
				return 0;
			}
		}

		Sleep(1000);
	}

	closesocket(hostsock);
	return -1;
}

int upgrade_msg_send(char *ipstr, upgrade_msg_e type, char *psn, char *pmac, char *pver, char *pinfo)
{
	int             rc = 0;
	int             ipaddr = -1;
	SOCKET          hostsock;
	SOCKADDR_IN		servaddr;
	upgrade_msg_s   stmsg;

	hostsock = socket(AF_INET, SOCK_DGRAM, 0);	
	if (-1 == hostsock)
	{
		dbgprint("socket failed!");
		return -1;
	}

	inet_pton(AF_INET, "127.0.0.1", &ipaddr);

	memset(&servaddr, 0, sizeof(SOCKADDR_IN)); 
	servaddr.sin_family =AF_INET;
	servaddr.sin_addr.s_addr = ipaddr;
	servaddr.sin_port = htons(65441);

	memset(&stmsg, 0, sizeof(stmsg));
	stmsg.msgtype = type;
	inet_pton(AF_INET, ipstr, &ipaddr);
	stmsg.ipaddr = ntohl(ipaddr);

	if (NULL != psn)
	{
		strncpy_s(stmsg.snstr, psn, sizeof(stmsg.snstr) - 1);
	}
	if (NULL != pmac)
	{
		strncpy_s(stmsg.macstr, pmac, sizeof(stmsg.macstr) - 1);
	}
	if (NULL != pver)
	{
		strncpy_s(stmsg.verstr, pver, sizeof(stmsg.verstr) - 1);
	}
	if (NULL != pinfo)
	{
		strncpy_s(stmsg.process, pinfo, sizeof(stmsg.process) - 1);
	}
	
	rc = sendto(hostsock, (char *)&stmsg, sizeof(upgrade_msg_s), 0, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in));
    if (rc < 0)
    {
		dbgprint("send msg failed!");
		closesocket(hostsock); 
        return -1;
    }

	if (NULL != pinfo)
	{
		dbgprint("%s send msg(%d) info=%s", ipstr, type, pinfo);
	}
	else
	{
		dbgprint("%s send msg(%d)", ipstr, type);
	}
	
	closesocket(hostsock); 
	return 0;
}

int ssh_upgrade(char *ipstr, char *macstr, char *fwfile, unsigned short port, int doublearea)
{
	int  			rc = -1;
	int  			session = -1;
	
	//开始升级
	bool retry_enable = TRUE;
	for (int i = 0; i < 20 && TRUE == retry_enable; i++)
	{
		char  szinfo[256] = {0};
		sprintf_s(szinfo, sizeof(szinfo), "尝试login %d/%d", i, 20);
		upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, szinfo);
		
		//login
		rc = bru_ssh_login(ipstr, port, &session);
		if (0 != rc)
		{
			dbgprint("ssh login fail, ip=%s, ret=%d", ipstr, rc);
			bru_ssh_logout(session);
			Sleep(1000);
			continue;
		}
		upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, "login成功");

		/*获取软件信息*/
		char szsn[64] = {0};
		char szmac[64] = {0};
		int  bootm = 0xFFFFFFFF;
		rc |= bru_ssh_get_sn(session, szsn, sizeof(szsn));
		rc |= bru_ssh_get_mac(session, szmac, sizeof(szmac));
		rc |= bru_ssh_get_bootm(session, &bootm);
		if (0 != rc)
		{
			dbgprint("get info fail, ip=%s, ret=%d", ipstr, rc);
			bru_ssh_logout(session);
			Sleep(1000);
			continue;
		}

		upgrade_msg_send(ipstr, e_ssh_info, szsn, szmac, NULL, "获取设备信息OK");

		/*配置mac  */
		char *pblankmac = "FFFFFFFFFFFF";
		dbgprint("szmac=[%s]", szmac);
		if (0 == strcmp(szmac, pblankmac))
		{
			rc = bru_ssh_set_macaddr(session, macstr);
			if (0 != rc)
			{
				dbgprint("set macaddr fail, ip=%s, ret=%d", ipstr, rc);
				bru_ssh_logout(session);
				Sleep(1000);
				continue;
			}
		}

		//获取版本
		char szversion[128] = {0};
		rc |= bru_ssh_get_curr_version(session, 0, szversion, sizeof(szversion));
		if (0 != rc)
		{
			dbgprint("get software version fail, ip=%s, ret=%d", ipstr, rc);
			bru_ssh_logout(session);
			continue;
		}

		upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, szversion, "获取版本成功");
		
		upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, "上传软件");

		/*上传软件  */
		rc = bru_ssh_uploadfile(session, fwfile, "/tmp/firmware.img");
		if (0 != rc)
		{
			dbgprint("upload img fail, ip=%s, ret=%d", ipstr, rc);
			bru_ssh_logout(session);
			upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, "上传失败");
			continue;
		}
		
		//上传点灯工具
		bru_ssh_uploadfile(session, "ledctrl", "/tmp/ledctrl");

		retry_enable = FALSE;

		bru_ssh_ledfastflash(session);

		upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, "升级中");
		
		rc = bru_ssh_upgrade(session);

		char  logfilename[256] = {0};
		sprintf_s(logfilename, ".\\log\\%s_update.log", ipstr);
		bru_ssh_downloadfile(session, "/tmp/updatelog", logfilename);
		
		if (0 != rc)
		{
			bru_ssh_reboot(session);
			dbgprint("upgrade fail, ip=%s, ret=%d", ipstr, rc);
			bru_ssh_logout(session);
			rc = -1;
			upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, "升级失败");
			break;
		}

		rc = 0;
		upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, "进入下一阶段");
		break;
	}

	if (0 == rc)
	{
		bru_ssh_reboot(session);
	}
	
	bru_ssh_logout(session);

	return rc;
}

int ssh_check(char *ipstr, char *macstr, char *fwver, unsigned short port, int doublearea)
{
	int  			rc = -1;
	int  			session = -1;
	
	//开始升级
	bool retry_enable = TRUE;
	for (int i = 0; i < 20 && TRUE == retry_enable; i++)
	{
		char  szinfo[256] = {0};
		sprintf_s(szinfo, sizeof(szinfo), "尝试login %d/%d", i, 20);
		upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, szinfo);
		
		//login
		rc = bru_ssh_login(ipstr, port, &session);
		if (0 != rc)
		{
			dbgprint("ssh login fail, ip=%s, ret=%d", ipstr, rc);
			bru_ssh_logout(session);
			Sleep(1000);
			continue;
		}

		upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, "login成功");

		/*获取软件信息*/
		char szsn[64] = {0};
		char szmac[64] = {0};
		int  bootm = 0xFFFFFFFF;
		rc |= bru_ssh_get_sn(session, szsn, sizeof(szsn));
		rc |= bru_ssh_get_mac(session, szmac, sizeof(szmac));
		rc |= bru_ssh_get_bootm(session, &bootm);
		if (0 != rc)
		{
			dbgprint("get info fail, ip=%s, ret=%d", ipstr, rc);
			bru_ssh_logout(session);
			Sleep(1000);
			continue;
		}

		upgrade_msg_send(ipstr, e_ssh_info, szsn, szmac, NULL, "获取设备信息OK");
		
		//获取版本
		char szversion[128] = {0};
		rc |= bru_ssh_get_curr_version(session, 0, szversion, sizeof(szversion));
		if (0 != rc)
		{
			dbgprint("get software version fail, ip=%s, ret=%d", ipstr, rc);
			bru_ssh_logout(session);
			continue;
		}

		upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, szversion, "获取版本成功");
		
		//上传点灯工具
		bru_ssh_uploadfile(session, "ledctrl", "/tmp/ledctrl");

		retry_enable = FALSE;

		upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, "检查主区");
		if (NULL == strstr(szversion, fwver))
		{
			dbgprint("upgrade check fail, ip=%s, ret=%d", ipstr, rc);
			dbgprint("szversion=[%s]", szversion);
			dbgprint("sz_dstver=[%s]", fwver);
			rc = -1;
			upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, "检查主区");
			break;
		}

		if (doublearea)
		{
			upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, "检查备区");
			rc = bru_ssh_checkback(session, bootm, fwver);
			if (0 != rc)
			{
				dbgprint("upgrade check fail, ip=%s, ret=%d", ipstr, rc);
				rc = -1;
				upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, "检查备区");
				break;
			}
		}

		rc = 0;
		bru_ssh_complete(session);
		upgrade_msg_send(ipstr, e_ssh_info, NULL, NULL, NULL, "升级成功");
		break;
	}
	
	bru_ssh_logout(session);
	
	return rc;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int 			rc = 0;
	char       	   *ipstr = NULL;
	char       	   *macstr = NULL;
	char       	   *fwfile = NULL;
	char       	   *fwver = NULL;
	int             doublecheck = 0;
	unsigned short 	port = 22;
	WSADATA 		wsadata;

	if (argc < 6)
	{
		dbgprint("usage: sshupgrade upgrade ipaddr macaddr fw doublearea");
		dbgprint("usage: sshupgrade check   ipaddr macaddr fwver doublearea");
		return -1;
	}

	if (0 == strcmp("upgrade", argv[1]))
	{
		ipstr = argv[2];
		macstr = argv[3];
		fwfile = argv[4];
		doublecheck = atoi(argv[5]);
	}
	else if (0 == strcmp("check", argv[1]))
	{
		ipstr = argv[2];
		macstr = argv[3];
		fwver= argv[4];
		doublecheck = atoi(argv[5]);
	}
	else
	{
		dbgprint("usage: sshupgrade upgrade ipaddr macaddr fw doublearea");
		dbgprint("usage: sshupgrade check   ipaddr macaddr fwver doublearea");
		return -1;
	}

	char szlogfile[512];
	sprintf_s(szlogfile, sizeof(szlogfile), ".\\log\\%s.log", ipstr);
	fopen_s(&fplog, szlogfile, "a+");

    rc = WSAStartup(MAKEWORD(2,0), &wsadata);
    if (0 != rc) 
	{
        dbgprint("WSAStartup failed with error: %d", rc);
		if (fplog) {fclose(fplog);fplog=NULL;}
        return -1;
    }

	rc = ssh_port_detecting(ipstr, &port);
    if (0 != rc) 
	{
        dbgprint("detecting ssh port of %s fail: %d", ipstr, rc);
		upgrade_msg_send(ipstr, e_ssh_quit_fail, NULL, NULL, NULL, "非法设备");
		WSACleanup();
		if (fplog) {fclose(fplog);fplog=NULL;}
        return -2;
    }
	dbgprint("detecting ssh port of %s is: %d", ipstr, port);

	if (0 == strcmp("upgrade", argv[1]))
	{
		rc = ssh_upgrade(ipstr, macstr, fwfile, port, doublecheck);
	}
	else if (0 == strcmp("check", argv[1]))
	{
		rc = ssh_check(ipstr, macstr, fwver, port, doublecheck);
	}
	else
	{
		dbgprint("usage: sshupgrade upgrade ipaddr macaddr fw doublecheck");
		dbgprint("usage: sshupgrade check   ipaddr macaddr fwver doublecheck");
		WSACleanup();
		if (fplog) {fclose(fplog);fplog=NULL;}
		return -1;
	}

	if (0 == rc)
	{
		upgrade_msg_send(ipstr, e_ssh_quit_ok, NULL, NULL, NULL, NULL);
	}
	else
	{
		upgrade_msg_send(ipstr, e_ssh_quit_fail, NULL, NULL, NULL, NULL);
	}

	WSACleanup();
	if (fplog) {fclose(fplog);fplog=NULL;}
	return rc;
}

