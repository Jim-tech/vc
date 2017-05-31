// sshupgrade.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "WS2tcpip.h"
#include "winsock2.h"
#include "windows.h"

#pragma comment(lib, "ws2_32.lib")

#if  0
#define dbgprint(...)
#else
#define dbgprint(...) \
    do\
    {\
        printf("[%s][%d]", __FUNCTION__, __LINE__);\
        printf(__VA_ARGS__);\
        printf("\n");\
     }while(0)
#endif /* #if 0 */

#define SSH_PORT_LIST            {22, 27149}

int ssh_port_detecting(int ipaddr, unsigned short *pport)
{
	int             rc = 0;
	unsigned short 	ports[] = SSH_PORT_LIST;
	int             cnt = sizeof(ports) / sizeof(unsigned short);
	SOCKET          hostsock;
	SOCKADDR_IN		servaddr;
	
	hostsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
	if (INVALID_SOCKET == hostsock)
	{
		dbgprint("socket failed!");
		return -1;
	}

	servaddr.sin_family =AF_INET;
	servaddr.sin_addr.s_addr = ipaddr;
	
	int timetotal = 60000;
	int timepass = 0;
	while (1)
	{
		if (timepass > timetotal)
		{
			break;
		}
		
		for (int i = 0; i < cnt; i++)
		{
			servaddr.sin_port = htons(ports[i]);
			
			rc = connect(hostsock,(LPSOCKADDR)&servaddr, sizeof(servaddr));	
			if(0 == rc)
			{
				closesocket(hostsock);
				*pport = ports[i];
				return 0;
			}
		}

		Sleep(1000);
		timepass += 1000;
	}

	closesocket(hostsock);
	return -1;
}

int ssh_start_session(char *ipstr, unsigned short port)
{
	int     rc = 0;
	char    szcmd[1024];

	if (22 == port)
	{
		sprintf_s(szcmd, sizeof(szcmd), "plink.exe -batch -l root -pw root123 %s", ipstr);
	}
	else
	{
		sprintf_s(szcmd, sizeof(szcmd), "plink.exe -batch -l admin -pw admin %s", ipstr);
	}

	memset(&g_sshcmd_ctrl, 0, sizeof(childp_s));
	rc = createprocess_withpipe(szcmd, &g_sshcmd_ctrl);
	if (0 != rc)
	{
		dbgprint("[%d] error return", __LINE__);
		destoryprocess_withpipe(&g_sshcmd_ctrl);
		return rc;
	}

	Sleep(5000);

	#if  1
	char  szBuffer[0x400] = {0};
	DWORD dwlen;

	sprintf_s(szBuffer, sizeof(szBuffer), "ls >/tmp/jim.log\r");
	if(!WriteFile(g_sshcmd_ctrl.hstdin[0], szBuffer, sizeof(szBuffer), &dwlen, NULL))
	{
		dbgprint("[%d] error return", __LINE__);
		exit(0);
	}

	Sleep(100);

	memset(szBuffer, 0, sizeof(szBuffer));
	if(!ReadFile(g_sshcmd_ctrl.hstdout[0], szBuffer, 0x400, &dwlen, NULL))
	{
		dbgprint("[%d] error return", __LINE__);
		exit(0);
	}

	if(!dwlen)  
	{
		dbgprint("[%d] error return", __LINE__);
		exit(0);
	}

	printf(szBuffer);  

	//TerminateProcess(g_sshcmd_ctrl.hprocess, 0);  
	//destoryprocess_withpipe(&g_sshcmd_ctrl);  

	dbgprint("[%d] error return", __LINE__);
	#endif /* #if 0 */
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int 			rc = 0;
	char       	   *ipstr = NULL;
	char       	   *fwfile = NULL;
	int         	ipaddr = -1;
	unsigned short 	port = 22;
	WSADATA 		wsadata;

	if (argc < 3)
	{
		dbgprint("usage: sshupgrade ipaddr fw");
		return -1;
	}
	ipstr = argv[1];
	fwfile = argv[2];

    rc = WSAStartup(MAKEWORD(2,0), &wsadata);
    if (0 != rc) 
	{
        dbgprint("WSAStartup failed with error: %d", rc);
        return -1;
    }

	inet_pton(AF_INET, ipstr, &ipaddr);
	dbgprint("sshupgrade %s", ipstr);

	rc = ssh_port_detecting(ipaddr, &port);
    if (0 != rc) 
	{
        dbgprint("detecting ssh port of %s fail: %d", ipstr, rc);
        return -2;
    }
	dbgprint("detecting ssh port of %s is: %d", ipstr, port);

	ssh_start_session(ipstr, port);

	

	destoryprocess_withpipe(&g_sshcmd_ctrl);  
	WSACleanup();
	
	return 0;
}

