
// BatchUpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "BatchUp.h"
#include "ColorListCtrl.h"
#include "BatchUpDlg.h"
#include "afxdialogex.h"
#include <IPHlpApi.h>
#include "pcap.h"

#pragma comment(lib,"IPHlpApi.lib")

#pragma warning (disable: 4146)
#import "c://Program Files (x86)//Common Files//system//ado//msadox.dll"
//#import "c://Program Files (x86)//Common Files//system//ado//msado15.dll" no_namespace rename ("EOF", "adoEOF")    
#import "c://Program Files (x86)//Common Files//system//ado//msado15.dll" no_namespace rename ("EOF", "adoEOF")
#pragma warning (default: 4146)

#if  0
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#endif /* #if 0 */


typedef enum
{
	HD_ID = 0,
	HD_IP = 1,
	HD_SN = 2,
	HD_MAC = 3,
	HD_IMG = 4,
	HD_STATE = 5,
	HD_PROGRESS = 6
}List_Hd_E;

typedef enum
{
	E_Init,
	E_1stDone,
	E_2ndDone,
	E_Done
}Upgrade_Fsm_E;

#pragma pack(1)
typedef struct dhcphdr
{
	unsigned char       op;
	unsigned char       hwtype;
	unsigned char       hwlen;
	unsigned char       hops;
	unsigned int        xid;
	unsigned short      secs;
	unsigned short      flags;
	unsigned int        ciaddr;
	unsigned int        yiaddr;
	unsigned int        siaddr;
	unsigned int        giaddr;
	unsigned char       chaddr[16];
	unsigned char       sname[64];
	unsigned char       file[128];
}DhcpHdr_S;
#pragma pack()

#define     IPLIST_MAX_ITEM      10240
#define     WAIT_SECONDS         15

typedef struct iplist
{
	DWORD               ipaddr;
    unsigned char       macaddr[6];
	char                szsn[32];
	char                szmac[32];
    char                szversion[128];
    int                 state;
    int                 idletime;
    int                 starttime;
	bool                runnning;
	bool                used;
	HANDLE              hProcess;
}IPList_S;

int             g_ctrl_index_next = 0;

bool 			g_thread_quiting = FALSE;
HANDLE       	g_hIPListMutex = NULL;
IPList_S	 	g_astIPList[IPLIST_MAX_ITEM];
CBatchUpDlg    *g_pstDlgPtr = NULL;
pcap_t*      	g_pd = NULL;
char         	g_ifname[512] = {0};
int          	g_iplocal = -1;
char           	g_ifmac[6] = {0};
FILE           *fplog = NULL;

#define      UPDATE_UI_TIMER  1

#define dbgprint(...) \
    do\
    {\
		SYSTEMTIME __sys;\
		GetLocalTime(&__sys);\
		if (NULL != fplog) \
		{\
            fprintf(fplog, "[%04d-%02d-%02d %02d:%02d:%02d][%s][%d][0x%08x]", __sys.wYear, __sys.wMonth, __sys.wDay, \
            					__sys.wHour, __sys.wMinute, __sys.wSecond, __FUNCTION__, __LINE__, GetCurrentThreadId());\
    		fprintf(fplog, __VA_ARGS__);\
    		fprintf(fplog, "\n");\
    		fflush(fplog);\
		}\
		printf("[%04d-%02d-%02d %02d:%02d:%02d][%s][%d][0x%08x]", __sys.wYear, __sys.wMonth, __sys.wDay, \
        					__sys.wHour, __sys.wMinute, __sys.wSecond, __FUNCTION__, __LINE__, GetCurrentThreadId());\
        printf(__VA_ARGS__);\
        printf("\n");\
        fflush(stdout);\
        fflush(stderr);\
     }while(0)

typedef enum
{
	e_ssh_info  		= 0,
	e_ssh_quit_ok       = 1,
	e_ssh_quit_fail     = 2,
}upgrade_msg_e;

typedef enum
{
	plat_t2k  		= 0,
	plat_t3k  		= 1,
}platform_e;

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

typedef struct
{
	unsigned char  dmac[6];
	unsigned char  smac[6];
	unsigned short ethtype;
}ether_hdr_s;

typedef struct
{
	ether_hdr_s    ethhdr;
	unsigned short hwtype;
	unsigned short proto;
	unsigned char  hwsize;
	unsigned char  protosize;
	unsigned short opcode;
	unsigned char  sendmac[6];
	unsigned int   sendip;
	unsigned char  targetmac[6];
	unsigned int   targetip;
}arp_pkt_s;
#pragma pack()

_ConnectionPtr g_pConnection = NULL;

int DB_Init()
{
	//read database config from config.ini
	TCHAR  sztype[64] = {0};
	TCHAR  szsqlserv[64] = {0};
	TCHAR  szdbname[64] = {0};
	TCHAR  szuser[64] = {0};
	TCHAR  szpasswd[64] = {0};
	CString strdbcmd;

	//初始化配置文件路径
	TCHAR szCfgDir[512];
	TCHAR szCfgIniPath[1024];
	::GetCurrentDirectory(sizeof(szCfgDir), szCfgDir);
	wsprintf(szCfgIniPath, _T("%s\\config.ini"), szCfgDir);
	
	GetPrivateProfileString(_T("database"), _T("type"), _T("local"), sztype, sizeof(sztype), szCfgIniPath);
	if (0 == _tcscmp(_T("local"), sztype))
	{
		GetPrivateProfileString(_T("database"), _T("localdb"), _T(""), szdbname, sizeof(szdbname), szCfgIniPath);
		strdbcmd.Format(_T("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%s;"), szdbname);
	}
	else if (0 == _tcscmp(_T("remote"), sztype))
	{
		GetPrivateProfileString(_T("database"), _T("sqlserver"), _T(""), szsqlserv, sizeof(szsqlserv), szCfgIniPath);
		GetPrivateProfileString(_T("database"), _T("remotedb"), _T(""), szdbname, sizeof(szdbname), szCfgIniPath);
		GetPrivateProfileString(_T("database"), _T("username"), _T(""), szuser, sizeof(szuser), szCfgIniPath);
		GetPrivateProfileString(_T("database"), _T("passwd"), _T(""), szpasswd, sizeof(szpasswd), szCfgIniPath);
		strdbcmd.Format(_T("Provider=SQLOLEDB;Server=%s;Database=%s;Uid=%s;Pwd=%s;"), szsqlserv, szdbname, szuser, szpasswd);
	}
	else
	{
		AfxMessageBox(_T("config.ini中database的类型只能是local或者remote"));
		return -1;
	}

	wprintf(strdbcmd);
	
	/*connect  */
	try
	{
		g_pConnection.CreateInstance(__uuidof(Connection));
		g_pConnection->Open(strdbcmd.GetString(), "", "", adConnectUnspecified);

		return 0;
	}
	catch (_com_error &e)
	{
		CString strlog;
		strlog = (LPCSTR)e.Description();
		wprintf(strlog);
		return -1;
	}

	printf("connect database ok\n");
	return 0;
}

void DB_DeInit()
{
	try
	{
		if (g_pConnection->State)
		{
			g_pConnection->Close();
			g_pConnection.Release();
		}
	}
	catch (_com_error &e)
	{
		CString strlog;
		strlog = (LPCSTR)e.Description();
		wprintf(strlog);
	}

	return;
}

int DB_Insert(_variant_t szVals[], int ValCnt)
{
	int  ret;

	try
	{
		_RecordsetPtr m_pRecordset;
		m_pRecordset.CreateInstance(__uuidof(Recordset));
		
		TCHAR szSqlCmd[1024];
		wsprintf(szSqlCmd, _T("SELECT * FROM BatchUpgrade"));

		m_pRecordset->Open(szSqlCmd, g_pConnection.GetInterfacePtr(), adOpenStatic, adLockOptimistic, adCmdText);

		if (ValCnt != m_pRecordset->GetFields()->Count)
		{
			//dbgprint("ValCnt=%d m_pRecordset->GetFields()->Count=%d\n", ValCnt, m_pRecordset->GetFields()->Count);
			CString strlog;
			strlog.Format(_T("表字段数不正确，表字段[%d], 插入的字段[%d]"), m_pRecordset->GetFields()->Count, ValCnt);
			wprintf(strlog);
			
			ret = -1;
		}
		else
		{
			ret = 0;
			m_pRecordset->AddNew();

			for (int i = 0; i < ValCnt; i++)
			{
				//dbgprint("i=%d \n", i);
				m_pRecordset->PutCollect(_variant_t((long)i), szVals[i]);
			}

			m_pRecordset->Update();
		}		
		Sleep(50 * ValCnt);
		m_pRecordset->Close();
		m_pRecordset.Release();
	}
	catch (_com_error &e)
	{
		CString strlog;
		strlog = (LPCSTR)e.Description();
		wprintf(strlog);
		return -1;
	}

	return ret;
}

int DB_UpgradeResult(IPList_S *pstInfo)
{
	char curtime[256] = {0};
	CTime curr = CTime::GetCurrentTime();
	
	sprintf_s(curtime, sizeof(curtime), "%04d-%02d-%02d %02d:%02d:%02d",
		curr.GetYear(), curr.GetMonth(), curr.GetDay(), 
		curr.GetHour(), curr.GetMinute(), curr.GetSecond());

	_variant_t szVals[] =
	{
		(_variant_t)pstInfo->szsn,
		(_variant_t)pstInfo->szmac,
		(_variant_t)pstInfo->szversion,
		(_variant_t)curtime,
		(_variant_t)((plat_t3k == g_pstDlgPtr->m_platform_cmb.GetCurSel()) ? "T3K" : "T2K"),
		(_variant_t)((1 == g_pstDlgPtr->m_doublearea) ? "True" : "False")
	};

	DB_Insert(szVals, sizeof(szVals) / sizeof(_variant_t));

	return 0;
}

void utils_TChar2Char(TCHAR *pIn, char *pOut, int maxlen)
{
	int len = WideCharToMultiByte(CP_ACP, 0, pIn, -1, NULL, 0, NULL, NULL);  
	if (len >= maxlen)
	{
		len = maxlen;
	}
	
	WideCharToMultiByte(CP_ACP, 0, pIn, -1, pOut, len, NULL, NULL);
}

void utils_Char2Tchar(char *pIn, TCHAR *pOut, int maxlen)
{
	int len = MultiByteToWideChar(CP_ACP, 0, pIn, strlen(pIn)+1, NULL, 0);
	if (len >= maxlen)
	{
		len = maxlen;
	}
	
	MultiByteToWideChar(CP_ACP, 0, pIn, strlen(pIn)+1, pOut, len);
}

void UpdateProgress(upgrade_msg_s *pmsg)
{
	if (NULL == g_pstDlgPtr)
	{
		return;
	}
	
	int  ipaddr = pmsg->ipaddr;

	IPList_S *pstInfo = NULL;
	bool      done = FALSE;
	int       result = 0;

	char szinfo[256] = {0};

	dbgprint("recv msg from dut, ip=%d.%d.%d.%d msgtype=%d", (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, ipaddr & 0xFF, pmsg->msgtype);
	
	WaitForSingleObject(g_hIPListMutex,INFINITE);
	for (int i = 0; i < IPLIST_MAX_ITEM; i++)
	{
		if (TRUE != g_astIPList[i].used)
		{
			continue;
		}

		if (ipaddr == g_astIPList[i].ipaddr)
		{
			g_astIPList[i].idletime = 0;
			
			if (0 != strlen(pmsg->snstr))
			{
				strncpy_s(g_astIPList[i].szsn, pmsg->snstr, sizeof(g_astIPList[i].szsn)-1);
			}
			if (0 != strlen(pmsg->macstr))
			{
				strncpy_s(g_astIPList[i].szmac, pmsg->macstr, sizeof(g_astIPList[i].szmac)-1);
			}
			if (0 != strlen(pmsg->verstr))
			{
				strncpy_s(g_astIPList[i].szversion, pmsg->verstr, sizeof(g_astIPList[i].szversion)-1);
			}

			if (0 != strlen(pmsg->process))
			{
				strncpy_s(szinfo, pmsg->process, sizeof(szinfo)-1);
			}

			pstInfo = &(g_astIPList[i]);
			
			break;
		}
		
	}
	ReleaseMutex(g_hIPListMutex);

	if (NULL == pstInfo)
	{
		dbgprint("!!!recv msg from dut, ip=%d.%d.%d.%d msgtype=%d", (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, ipaddr & 0xFF, pmsg->msgtype);
		return;
	}

	bool is_new = TRUE;
	int  item_index = g_pstDlgPtr->m_listCtrl.GetItemCount();

	/*查找是否已经存在  */
	for (int i = 0; i < g_pstDlgPtr->m_listCtrl.GetItemCount(); i++)
	{
		if (ipaddr == g_pstDlgPtr->m_listCtrl.GetItemData(i))
		{
			is_new = FALSE;
			item_index = i;
			break;
		}
	}
	
	CString strval;
	TCHAR   szval[512] = {0};

	//id
	if (TRUE == is_new)
	{
		strval.Format(_T("%d"), item_index + 1);
		g_pstDlgPtr->m_listCtrl.InsertItem(item_index, strval);
	}

	//ipaddr
	strval.Format(_T("%d.%d.%d.%d"),  (pstInfo->ipaddr >> 24) & 0xFF,
		                              (pstInfo->ipaddr >> 16) & 0xFF,
		                              (pstInfo->ipaddr >> 8) & 0xFF,
		                              (pstInfo->ipaddr >> 0) & 0xFF);
	g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_IP, strval);
	g_pstDlgPtr->m_listCtrl.SetItemData(item_index, pstInfo->ipaddr);

	//sn
	utils_Char2Tchar(pstInfo->szsn, szval, sizeof(szval));
	g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_SN, szval);

	//mac
	utils_Char2Tchar(pstInfo->szmac, szval, sizeof(szval));
	g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_MAC, szval);

	//version
	utils_Char2Tchar(pstInfo->szversion, szval, sizeof(szval));
	g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_IMG, szval);
	
	//state
	switch(pstInfo->state)
	{
		case E_Init:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("升级当前区"));
			break;
		case E_1stDone:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("升级对区"));
			break;
		case E_2ndDone:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("升级检查"));
			break;
		case E_Done:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("升级完成"));
			break;			
		default:
			break;
	}

	//更新状态机
	WaitForSingleObject(g_hIPListMutex,INFINITE);
	switch (pmsg->msgtype)
	{
	case e_ssh_quit_ok:
		pstInfo->runnning = FALSE;
		pstInfo->hProcess = NULL;
		
		if (TRUE == g_pstDlgPtr->m_doublearea)
		{
			if (pstInfo->state < E_1stDone)
			{
				pstInfo->state = E_1stDone;
			}
			else if (pstInfo->state < E_2ndDone)
			{
				pstInfo->state = E_2ndDone;
			}
			else if (pstInfo->state < E_Done)
			{
				pstInfo->state = E_Done;
				pstInfo->used = FALSE;
				done = TRUE;
				DB_UpgradeResult(pstInfo);
			}
		}
		else
		{
			if (pstInfo->state < E_2ndDone)
			{
				pstInfo->state = E_2ndDone;
			}
			else if (pstInfo->state < E_Done)
			{
				pstInfo->state = E_Done;
				pstInfo->used = FALSE;
				done = TRUE;
				DB_UpgradeResult(pstInfo);
			}
		}
		break;
		
	case e_ssh_quit_fail:
		done = TRUE;
		result = -1;
		pstInfo->used = FALSE;
		pstInfo->state = E_Init;
		pstInfo->runnning = FALSE;
		pstInfo->hProcess = NULL;
		break;
		
	default:
		break;
	}	
	ReleaseMutex(g_hIPListMutex);
	
	if (0 != strlen(szinfo))
	{
		utils_Char2Tchar(szinfo, szval, sizeof(szval));
		g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_PROGRESS, szval);
	}

	if (done)
	{
		g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("升级结束"));
		
		if (0 == result)
		{
			g_pstDlgPtr->m_listCtrl.SetItemColor(item_index, RGB(0, 0, 0), RGB(0, 0xff, 0));
		}
		else
		{
			g_pstDlgPtr->m_listCtrl.SetItemColor(item_index, RGB(0, 0, 0), RGB(0xff, 0, 0));
		}
	}
	else
	{
		g_pstDlgPtr->m_listCtrl.SetItemColor(item_index, RGB(0, 0, 0), RGB(0xff, 0xff, 0xff));	
	}

	g_pstDlgPtr->m_listCtrl.EnsureVisible(item_index, true);
}

HANDLE create_upgrade_process(int ipaddr, unsigned char mac[6])
{
	char  ipstr[32] = {0};
	char  macstr[32] = {0};
	char  filepath[1024] = {0};
	char  cmdline[2048] = {0};
	TCHAR tcmdline[4096] = {0};

	sprintf_s(ipstr, sizeof(ipstr), "%d.%d.%d.%d", (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, ipaddr & 0xFF);
	sprintf_s(macstr, sizeof(macstr), "%02X%02X%02X%02X%02X%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	utils_TChar2Char((TCHAR *)g_pstDlgPtr->m_imgPath.GetString(), filepath, sizeof(filepath));

	sprintf_s(cmdline, sizeof(cmdline), "sshcmd.exe upgrade %s %s %s %d %d", ipstr, macstr, filepath, g_pstDlgPtr->m_doublearea, g_pstDlgPtr->m_platform_cmb.GetCurSel());
	utils_Char2Tchar(cmdline, tcmdline, sizeof(tcmdline));
	
	STARTUPINFO si;
	PROCESS_INFORMATION pi;	

	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.wShowWindow=SW_HIDE;
	
	if (CreateProcess(NULL, tcmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		dbgprint("create upgrade process for %s ok", ipstr);
		return pi.hProcess;
	}
	else
	{
		dbgprint("create upgrade process for %s fail", ipstr);
		return NULL;
	}
}

HANDLE create_check_process(int ipaddr, unsigned char mac[6])
{
	char  ipstr[32] = {0};
	char  macstr[32] = {0};
	char  filever[128] = {0};
	char  cmdline[2048] = {0};
	TCHAR tcmdline[4096] = {0};
	
	sprintf_s(ipstr, sizeof(ipstr), "%d.%d.%d.%d", (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, ipaddr & 0xFF);
	sprintf_s(macstr, sizeof(macstr), "%02X%02X%02X%02X%02X%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	utils_TChar2Char((TCHAR *)g_pstDlgPtr->m_ver.GetString(), filever, sizeof(filever));

	sprintf_s(cmdline, sizeof(cmdline), "sshcmd.exe check %s %s %s %d %d", ipstr, macstr, filever, g_pstDlgPtr->m_doublearea, g_pstDlgPtr->m_platform_cmb.GetCurSel());
	utils_Char2Tchar(cmdline, tcmdline, sizeof(tcmdline));
	
	STARTUPINFO si;
	PROCESS_INFORMATION pi;	

	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.wShowWindow = SW_HIDE;
	
	if (CreateProcess(NULL, tcmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		dbgprint("create check process for %s ok", ipstr);
		return pi.hProcess;
	}
	else
	{
		dbgprint("create check process for %s fail", ipstr);
		return NULL;
	}
}

void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	DWORD         ipaddr = 0xFFFFFFFF;
	unsigned char szmac[6] = {0xFF};
	bool          dhcp = FALSE;

	if ((0x08 == pkt_data[12]) && (0x06 == pkt_data[13]))
	{
		arp_pkt_s *pkt = (arp_pkt_s *)pkt_data;

		//只处理reply
		if ((2 != ntohs(pkt->opcode)) || (1 != ntohs(pkt->hwtype)) || (0x0800 != ntohs(pkt->proto)) || (6 != pkt->hwsize) || (4 != pkt->protosize))
		{
			//dbgprint("!!! arp drop !!!");
			return;
		}

		ipaddr = ntohl(pkt->sendip);
		memcpy(szmac, pkt->sendmac, 6);
		
		//查看是否是重复的IP
		WaitForSingleObject(g_hIPListMutex,INFINITE);
		for (int i = 0; i < IPLIST_MAX_ITEM; i++)
		{
			if (TRUE == g_astIPList[i].used)
			{
				continue;
			}

			if (ipaddr == g_astIPList[i].ipaddr)
			{
				if (g_astIPList[i].state >= E_Done || g_astIPList[i].idletime < 30)
				{
					ReleaseMutex(g_hIPListMutex);
					return;
				}
			}
		}
		ReleaseMutex(g_hIPListMutex);
		
		//dbgprint("!!!arp received ip=%d.%d.%d.%d!!!", (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, ipaddr & 0xFF);
	}
	else if ((0x08 == pkt_data[12]) && (0x00 == pkt_data[13]))
	{
		unsigned char *szpkt = (unsigned char *)(pkt_data + 14 + 20 + 8);
		int            pktlen = header->len - 14 - 20 - 8;

		DhcpHdr_S *pdhcphdr = (DhcpHdr_S *)szpkt;
		if (1 != pdhcphdr->op)  /*request  */
		{
			return;
		}

		if (pktlen < sizeof(DhcpHdr_S) + sizeof(unsigned int))
		{
			return;
		}
		
		//mgic cookie
		unsigned int magiccookie = *(unsigned int *)(szpkt + sizeof(DhcpHdr_S));
		magiccookie = htonl(magiccookie);
		if (0x63825363 != magiccookie)
		{
			dbgprint("invliad magic cookie");
			return;
		}

		unsigned char dhcp_msg_type = 255;
		bool          vendor_matched = FALSE;
		unsigned char defszmac[6] = {0x00, 0x15, 0x17, 0x14, 0xC6, 0x04};
		
		int offset = sizeof(DhcpHdr_S) + sizeof(unsigned int);
		while ((szpkt[offset] != 255) && (offset + 2 < pktlen))  /* 2表示tlv字段的type和len  */
		{
			unsigned char v_type = szpkt[offset];
			unsigned char v_len = szpkt[offset + 1];

			if (offset + 2 + v_len > pktlen)
			{
				dbgprint("invliad dhcp msg");
				break;
			}
			
			if (53 == v_type) /* dhcp msg type  */
			{
				if (1 == v_len)
				{
					dhcp_msg_type = szpkt[offset + 2];
				}
			}

			if (61 == v_type) /* client identifier  */
			{
				if (7 == v_len && 1 == szpkt[offset + 2])
				{
					memcpy(szmac, &szpkt[offset + 3], sizeof(szmac));

					#if  1
					if (0 == memcmp(defszmac, szmac, sizeof(szmac)))
					{
						return;
					}
					#endif /* #if 0 */
				}
			}			

			if (50 == v_type) /* request ip  */
			{
				if (4 == v_len)
				{
					ipaddr = *(DWORD *)&szpkt[offset + 2];
					ipaddr = ntohl(ipaddr);
					dhcp = TRUE;
				}
			}			

			if (60 == v_type) /* vendor class identifier  */
			{
				if (v_len == strlen("udhcp 1.18.5"))
				{
					if (0 == memcmp("udhcp 1.18.5", &szpkt[offset + 2], v_len))
					{
						vendor_matched = TRUE;
					}
				}
			}

			
			offset += 2;
			offset += v_len;
			if (szpkt[offset] == 255)
			{
				dbgprint("dhcp option end");
				break;
			}
		}

		if (TRUE != vendor_matched || 3 != dhcp_msg_type) /*dhcp request  */
		{
			return;
		}
	}

	#if  0
	char ddddmac1[] = {0x00, 0x10, 0x28, 0x73, 0x05, 0x06};
	char ddddmac2[] = {0x48,0xFF,0x3B,0x3A,0x1A,0xFE};
	if ((0 != memcmp(ddddmac1, szmac, 6)) && (0 != memcmp(ddddmac2, szmac, 6)))
	{
		return;
	}
	#endif /* #if 0 */

	#if  0
	if (0xAc10018B != ipaddr)
	{
		dbgprint("!!not allowed ipaddr!!");
		return;
	}
	#endif /* #if 0 */

	if (ntohl(g_iplocal) == ipaddr)
	{
		return;
	}
	else
	{
		//dbgprint("local=0x%x ip=0x%x", ntohl(g_iplocal), ipaddr);

		int net = ipaddr & 0xFFFFFF00;
		int host = ipaddr & 0xFF;
		if (host == 0xFF || host == 0 || net != (ntohl(g_iplocal) & 0xFFFFFF00))
		{
			return;
		}
	}

	dbgprint("ip=%d.%d.%d.%d mac=[%02X:%02X:%02X:%02X:%02X:%02X]",   (ipaddr >> 24) & 0xFF,
																	  (ipaddr >> 16) & 0xFF,
																	  (ipaddr >> 8) & 0xFF,
																	  ipaddr & 0xFF,
																	  szmac[0], szmac[1], szmac[2],
																	  szmac[3], szmac[4], szmac[5]);

	WaitForSingleObject(g_hIPListMutex,INFINITE);

	IN_ADDR sk_addr;

	sk_addr.s_addr = htonl(ipaddr);
	char ipstring[20];
	inet_ntop(AF_INET, &sk_addr, ipstring, sizeof(ipstring));

	int i = 0;
	
	//查看是否是重复的IP
	for (i = 0; i < IPLIST_MAX_ITEM; i++)
	{
		if (TRUE != g_astIPList[i].used)
		{
			continue;
		}

		if (ipaddr == g_astIPList[i].ipaddr)
		{
			//修复升级过程中重启的问题
			if (dhcp)
			{
				if (g_astIPList[i].starttime >= 30)
				{
					dbgprint("ip=%d.%d.%d.%d recv dhcp 30 seconds after startup, though it rebooted", (ipaddr >> 24) & 0xFF,
																	  (ipaddr >> 16) & 0xFF,
																	  (ipaddr >> 8) & 0xFF,
																	  ipaddr & 0xFF);
					
					if (NULL != g_astIPList[i].hProcess)
					{
						TerminateProcess(g_astIPList[i].hProcess, 0);
					}
					g_astIPList[i].runnning = FALSE;
					g_astIPList[i].idletime = 0;
					g_astIPList[i].hProcess = NULL;
				}
				
				g_astIPList[i].starttime = 0;
			}
			
			break;
		}
	}

	if (i < IPLIST_MAX_ITEM)
	{
		if (E_Init <= g_astIPList[i].state && g_astIPList[i].state <= E_2ndDone)
		{
			if (g_astIPList[i].runnning == TRUE)
			{
				dbgprint("dut busy, ip=%s, state=%d ", ipstring, g_astIPList[i].state);
				ReleaseMutex(g_hIPListMutex);
				return;
			}
			else
			{
				g_astIPList[i].runnning = TRUE;
				g_astIPList[i].idletime = 0;
				dbgprint("dut start, ip=%s, state=%d ", ipstring, g_astIPList[i].state);
			}
		}
		else
		{
			dbgprint("invalid state, ip=%s, state=%d ", ipstring, g_astIPList[i].state);
			memset(&(g_astIPList[i]), 0, sizeof(IPList_S));
			g_astIPList[i].state = E_Init;
			g_astIPList[i].runnning = FALSE;
			
			ReleaseMutex(g_hIPListMutex);
			return;
		}
	}
	else //new
	{
		/*将搜索到的IP添加到队列  */
		for (int j = 0; j < IPLIST_MAX_ITEM;  j++)
		{
			i = (g_ctrl_index_next + j) % IPLIST_MAX_ITEM;
			
			if (TRUE != g_astIPList[i].used)
			{
				g_astIPList[i].ipaddr = ipaddr;
				memcpy(g_astIPList[i].macaddr, szmac, 6);
				memset(g_astIPList[i].szsn, 0, sizeof(g_astIPList[i].szsn));
				memset(g_astIPList[i].szmac, 0, sizeof(g_astIPList[i].szmac));
				memset(g_astIPList[i].szversion, 0, sizeof(g_astIPList[i].szversion));
				g_astIPList[i].state = E_Init;
				g_astIPList[i].idletime = 0;
				g_astIPList[i].starttime = 0;
				g_astIPList[i].runnning = TRUE;
				g_astIPList[i].used = TRUE;
				break;
			}
		}

		g_ctrl_index_next++;
		dbgprint("new dut start, ip=%s, state=%d ", ipstring, g_astIPList[i].state);
	}


	if (i < IPLIST_MAX_ITEM)
	{
		if (g_astIPList[i].state < E_2ndDone)
		{
			if (NULL == (g_astIPList[i].hProcess = create_upgrade_process(ipaddr, szmac)))
			{
				dbgprint("update fail");
				memset(&(g_astIPList[i]), 0, sizeof(IPList_S));
				g_astIPList[i].state = E_Init;
				g_astIPList[i].runnning = FALSE;
			}
		}
		else
		{
			if (NULL == (g_astIPList[i].hProcess = create_check_process(ipaddr, szmac)))
			{
				dbgprint("check fail");
				memset(&(g_astIPList[i]), 0, sizeof(IPList_S));
				g_astIPList[i].state = E_Init;
				g_astIPList[i].runnning = FALSE;
			}
		}
	}

	ReleaseMutex(g_hIPListMutex);
	return;
}

DWORD WINAPI thread_capture(LPVOID lpParameter)
{
	{
		if (0 == strlen(g_ifname))
		{
			g_pstDlgPtr->MessageBox(_T("没有找到对应的网卡"));
			return -1;
		}
		
		char ebuf[PCAP_ERRBUF_SIZE];
		g_pd = pcap_open_live(g_ifname, 65535, 1, 1000, ebuf);
		if (!g_pd) 
		{
			dbgprint("pcap open fail, %s", ebuf);
			return -1;
		}

		pcap_setbuff(g_pd, 16*1024*1024);
		pcap_set_buffer_size(g_pd, 16*1024*1024);

		char filter_app[] = "udp dst port 67 or arp"; /* 过滤表达式 */
		struct bpf_program filter; /* 已经编译好的过滤器 */
		if (pcap_compile(g_pd, &filter, filter_app, 1, 0xFFFFFFFF) < 0)
		{
			dbgprint("compile filter fail");
			pcap_close(g_pd);
			g_pd = NULL;
			return -1;			
		}
		if (pcap_setfilter(g_pd, &filter) < 0)
		{
			dbgprint("set filter fail");
			pcap_close(g_pd);
			g_pd = NULL;
			return -1;			
		}

		dbgprint("pcap start listenning");
		
		/* start the capture */
		while (1)
		{
			if (g_thread_quiting)
			{
				break;
			}

			if (NULL == g_pd)
			{
				break;
			}

			pcap_loop(g_pd, 1, packet_handler, NULL);		
		}
		
		pcap_close(g_pd);
		g_pd = NULL;
	}

	dbgprint("capture thread quitting");
	return 0;
}

DWORD WINAPI thread_arp(LPVOID lpParameter)
{
	if (0 == strlen(g_ifname))
	{
		g_pstDlgPtr->MessageBox(_T("没有找到对应的网卡"));
		return -1;
	}

	int ip_local = ntohl(g_iplocal);
	dbgprint("local ip: %d.%d.%d.%d", (ip_local >> 24) & 0xFF, (ip_local >> 16) & 0xFF, (ip_local >> 8) & 0xFF, ip_local & 0xFF);
	
	int ipstart = (ip_local & 0xFFFFFF00);
	
    pcap_t *fp;
	char errbuf[PCAP_ERRBUF_SIZE];
    if ( (fp = pcap_open_live(g_ifname,            		// name of the device
	                          2000,    					// portion of the packet to capture (only the first 100 bytes)
	                          1,						// promiscuous mode
	                          1000,               		// read timeout
	                          errbuf              		// error buffer
	                        ) ) == NULL)
    {
		g_pstDlgPtr->MessageBox(_T("打开网卡失败"));
		return -1;
    }

	while (1)
	{
		if (g_thread_quiting)
		{
			break;
		}
		
		arp_pkt_s arppkt;

		for (int i = 0; i < 255; i++)
		{
			if (g_thread_quiting)
			{
				break;
			}
			
			if (i % 8 == 0)
			{
				Sleep(1000);
			}
			
			int ipaddr = ipstart + i;
			
			if (ip_local == ipaddr || ipstart == ipaddr || (ipstart + 255) == ipaddr)
			{
				continue;
			}
			
			memset(&arppkt, 0, sizeof(arppkt));
			memset(arppkt.ethhdr.dmac, 0xFF, sizeof(arppkt.ethhdr.dmac));
			memcpy(arppkt.ethhdr.smac, g_ifmac, sizeof(arppkt.ethhdr.smac));
			arppkt.ethhdr.ethtype = htons(0x0806);

			arppkt.hwtype = htons(0x01);
			arppkt.proto = htons(0x0800);
			arppkt.hwsize = 6;
			arppkt.protosize = 4;
			arppkt.opcode = htons(0x01);
			memcpy(arppkt.sendmac, g_ifmac, sizeof(arppkt.sendmac));
			arppkt.sendip = htonl(ip_local);
			memset(arppkt.targetmac, 0xFF, sizeof(arppkt.targetmac));
			arppkt.targetip = htonl(ipaddr);

			if (pcap_sendpacket(fp, (const unsigned char *)&arppkt, sizeof(arppkt)) != 0)
			{
				dbgprint("send arp to ip(0x%x) fail", ipaddr);
			}
		}
	}
	pcap_close(fp);
	
	dbgprint("detect thread quitting");
	return 0;
}

DWORD WINAPI thread_process(LPVOID lpParameter)
{
	int             ret = 0;
	int             ipaddr;
	SOCKET			sock;
	SOCKADDR_IN		servaddr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
	{
		dbgprint("socket() failed; %d\n", WSAGetLastError());
		return 1;
	}

	unsigned long nonblock = 1;
	ioctlsocket(sock, FIONBIO, &nonblock);

	inet_pton(AF_INET, "127.0.0.1", &ipaddr);

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(65441);
	servaddr.sin_addr.s_addr = ipaddr;

	if (bind(sock, (SOCKADDR *)&servaddr, sizeof(servaddr)) == SOCKET_ERROR)
	{
		dbgprint("bind() failed: %d\n", WSAGetLastError());
		closesocket(sock);
		return 1;
	}

	fd_set fds;
	struct timeval tv;
	tv.tv_sec =  0;
	tv.tv_usec = 500000;
	while (1)
	{
		if (g_thread_quiting)
		{
			break;
		}
		
		FD_ZERO(&fds);
		FD_SET(sock, &fds);
		ret = select(sock + 1, &fds, NULL, NULL, &tv);
		if (ret < 0) 
		{
			ret = -1;
			dbgprint("select() failed: %d\n", WSAGetLastError());
			break;
		}

		if (ret == 0) 
		{
			continue;
		}

		if (FD_ISSET(sock, &fds))
		{
			upgrade_msg_s  stmsg;
			
			SOCKADDR_IN clientaddr;
			int 		nclientlen = sizeof(clientaddr);
			int retlen = recvfrom(sock, (char *)&stmsg, sizeof(stmsg), 0, (SOCKADDR *)&clientaddr, &nclientlen);
		    if (retlen < 0)
		    {
				continue;
		    }

			UpdateProgress(&stmsg);
		}
	}

	closesocket(sock);
	dbgprint("process thread quit");
	return 0;
}


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()



// CBatchUpDlg dialog

CBatchUpDlg::CBatchUpDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CBatchUpDlg::IDD, pParent)
	, m_doublearea(TRUE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_pCapThread = NULL;
	m_pArpThread = NULL;
	m_pProcessThread = NULL;

	fopen_s(&fplog, "batchup.log", "a+");

	g_pstDlgPtr = this;

	if (0 != DB_Init())
	{
		MessageBox(_T("初始化数据库失败"));
	}
}

CBatchUpDlg::~CBatchUpDlg()
{
	if (NULL != fplog)
	{
		fclose(fplog);
		fplog = NULL;
	}
	
	g_pstDlgPtr = NULL;
	
	DB_DeInit();
}

void CBatchUpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST_UP, m_listCtrl);
	DDX_Text(pDX, IDC_EDIT_SW, m_imgPath);
	DDX_Text(pDX, IDC_EDIT_VER, m_ver);
	DDX_Control(pDX, IDC_COMBO_NET, m_netcardCtrl);
	DDX_Check(pDX, IDC_CHECK_TYPE, m_doublearea);
	DDX_Control(pDX, IDC_COMBO_PLATFORM, m_platform_cmb);
}

BEGIN_MESSAGE_MAP(CBatchUpDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_FILE, &CBatchUpDlg::OnBnClickedButtonFile)
	ON_BN_CLICKED(IDC_BUTTON_START, &CBatchUpDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CBatchUpDlg::OnBnClickedButtonStop)

	ON_WM_PAINT()
	ON_WM_SYSCOMMAND()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK_TYPE, &CBatchUpDlg::OnClickedTypeCheckBox)
	ON_WM_SIZING()
	ON_CBN_SELCHANGE(IDC_COMBO_NET, &CBatchUpDlg::OnSelchangeComboNet)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_UP, &CBatchUpDlg::OnLvnColumnclickListUp)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_UP, &CBatchUpDlg::OnNMRClickListUp)
	ON_COMMAND(ID_COPY_OUT, &CBatchUpDlg::OnCopy2ClipBoard)
END_MESSAGE_MAP()


// CBatchUpDlg message handlers

BOOL CBatchUpDlg::OnInitDialog()
{
	CreateDirectory(_T("Log"), 0);
	CreateDirectory(_T("Data"), 0);
	
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	//设置测试项列表控件
	LONG lStyle;
	lStyle = GetWindowLong(m_listCtrl.m_hWnd, GWL_STYLE);	//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK;							    //清除显示方式位
	lStyle |= LVS_REPORT;									//设置style
	SetWindowLong(m_listCtrl.m_hWnd, GWL_STYLE, lStyle);	//设置style

	DWORD dwStyle = m_listCtrl.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;						//选中某行使整行高亮（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_GRIDLINES;							//网格线（只适用与report风格的listctrl）
	//dwStyle |= LVS_EX_CHECKBOXES;							//item前生成checkbox控件
	m_listCtrl.SetExtendedStyle(dwStyle);					//设置扩展风格

	m_listCtrl.InsertColumn(HD_ID, _T("序号"), LVCFMT_LEFT, 50);//插入列
	m_listCtrl.InsertColumn(HD_IP, _T("IP"), LVCFMT_LEFT, 90);	//插入列
	m_listCtrl.InsertColumn(HD_SN, _T("SN"), LVCFMT_LEFT, 160);	//插入列
	m_listCtrl.InsertColumn(HD_MAC, _T("MAC"), LVCFMT_LEFT, 120);	//插入列
	m_listCtrl.InsertColumn(HD_IMG, _T("Version"), LVCFMT_LEFT, 300);	//插入列
	m_listCtrl.InsertColumn(HD_STATE, _T("阶段"), LVCFMT_LEFT, 100);	//插入列
	m_listCtrl.InsertColumn(HD_PROGRESS, _T("状态"), LVCFMT_LEFT, 300);	//插入列

	memset(g_astIPList, 0, sizeof(g_astIPList));
	for (int i = 0; i < IPLIST_MAX_ITEM; i++)
	{
		g_astIPList[i].state = E_Init;
		g_astIPList[i].runnning = FALSE;
		g_astIPList[i].used = FALSE;
	}

	memset(g_ifname, 0, sizeof(g_ifname));
	{
		//detect ethernet interface
		pcap_if_t *alldevs;
		pcap_if_t *d;
		char errbuf[PCAP_ERRBUF_SIZE+1];

		/* Retrieve the device list */
		if(pcap_findalldevs(&alldevs, errbuf) == -1)
		{
			MessageBox(_T("没有可用的网卡"));
			return FALSE;
		}
		
		/* Scan the list printing every entry */
		int index = 0;
		for (d = alldevs; d; d = d->next)
		{
			if (0 == strlen(g_ifname))
			{
				sprintf_s(g_ifname, "%s", d->name);
			}
			
			TCHAR  szDesc[128];
			TCHAR  szDev[256];

			struct pcap_addr *lastaddr = d->addresses;
			
			while (lastaddr)
			{
				if (!lastaddr->next)
				{
					break;
				}
				lastaddr = lastaddr->next;
			}
			
			DWORD  ipaddr;
			ipaddr = ((struct sockaddr_in *)(lastaddr->addr))->sin_addr.s_addr;
			utils_Char2Tchar(d->description, szDesc, sizeof(szDesc));
			wsprintf(szDev, _T("%d.%d.%d.%d@%s"), ipaddr & 0xFF, (ipaddr >> 8) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 24) & 0xFF, szDesc);

			if (-1 == g_iplocal)
			{
				g_iplocal = ipaddr;
			}
			
			m_netcardCtrl.InsertString(index, szDev);
			index++;
		}

		/* Free the device list */
		pcap_freealldevs(alldevs);

		m_netcardCtrl.SetCurSel(0);
	}
	
	#if  1
	/*获取网卡  */
	{
	    PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO;
	    unsigned long    ulSize = sizeof(IP_ADAPTER_INFO);

	    //获取适配器信息
	    int nRet = GetAdaptersInfo(pIpAdapterInfo,&ulSize);
	    if (ERROR_BUFFER_OVERFLOW == nRet)
	    {
	        //空间不足，删除之前分配的空间
	        delete []pIpAdapterInfo;

	        //重新分配大小
	        pIpAdapterInfo = (PIP_ADAPTER_INFO) new BYTE[ulSize];

	        //获取适配器信息
	        nRet = GetAdaptersInfo(pIpAdapterInfo,&ulSize);

	        //获取失败
	        if (ERROR_SUCCESS != nRet)
	        {
	            if (pIpAdapterInfo != NULL)
	            {
	                delete []pIpAdapterInfo;
	            }

				MessageBox(_T("获取网卡列表失败"));
	            return TRUE;
	        }
	    }
		
	    //赋值指针
	    PIP_ADAPTER_INFO pIterater = pIpAdapterInfo;
	    while (pIterater)
	    {
			//dbgprint("g_ifname=%s", g_ifname);
			if (NULL != strstr(g_ifname, pIterater->AdapterName))
			{
				dbgprint("AdapterName=%s, mac=%02x:%02x:%02x:%02x:%02x:%02x", pIterater->AdapterName, 
																			  pIterater->Address[0],
																			  pIterater->Address[1],
																			  pIterater->Address[2],
																			  pIterater->Address[3],
																			  pIterater->Address[4],
																			  pIterater->Address[5]);
				
				memcpy(g_ifmac, pIterater->Address, sizeof(g_ifmac));
				break;
			}
			
	        pIterater = pIterater->Next;
	    }

	    //清理
	    if (pIpAdapterInfo)
	    {
	        delete []pIpAdapterInfo;
	    }	
	}
	#endif /* #if 0 */

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);

	m_doublearea = TRUE;
	m_sortorder_inc = FALSE;

	m_platform_cmb.InsertString(plat_t2k, _T("T2K"));
	m_platform_cmb.InsertString(plat_t3k, _T("T3K"));
	m_platform_cmb.SetCurSel(0);

	CreateDirectory(_T("log"), 0);
	
	SetTimer(UPDATE_UI_TIMER, 1000, 0);


	#if  0
	g_astIPList[0].used = 1;
	g_astIPList[0].ipaddr = 0;
	g_astIPList[0].state = E_Done;
	upgrade_msg_s stmsg;
	stmsg.ipaddr = 0;
	sprintf_s(stmsg.snstr, sizeof(stmsg.snstr), "1202000050173HP0010");
	sprintf_s(stmsg.macstr, sizeof(stmsg.macstr), "001520200101");
	sprintf_s(stmsg.verstr, sizeof(stmsg.verstr), "BRS_V100R001C00B004_ATE_4239@170712");
	sprintf_s(stmsg.process, sizeof(stmsg.process), "ok");
	stmsg.msgtype = 1;
	UpdateProgress(&stmsg);

	g_astIPList[1].used = 1;
	g_astIPList[1].ipaddr = 1;
	g_astIPList[1].state = E_Done;
	stmsg.ipaddr = 1;
	sprintf_s(stmsg.snstr, sizeof(stmsg.snstr), "1202000050173HP0011");
	sprintf_s(stmsg.macstr, sizeof(stmsg.macstr), "001520200102");
	sprintf_s(stmsg.verstr, sizeof(stmsg.verstr), "BRS_V100R001C00B004_ATE_4239@170713");
	sprintf_s(stmsg.process, sizeof(stmsg.process), "ok");
	stmsg.msgtype = 1;
	UpdateProgress(&stmsg);
	
	#endif /* #if 1 */
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CBatchUpDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CBatchUpDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void CBatchUpDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if (nType != SIZE_RESTORED && nType != SIZE_MAXIMIZED)
	{
		return;
	}

	float fsp[2];
	POINT Newp; //获取现在对话框的大小
	CRect recta;
	GetClientRect(&recta);     //取客户区大小  
	Newp.x = recta.right - recta.left;
	Newp.y = recta.bottom - recta.top;
	fsp[0] = (float)Newp.x / old.x;
	fsp[1] = (float)Newp.y / old.y;
	CRect Rect;
	int woc;
	CPoint OldTLPoint, TLPoint; //左上角
	CPoint OldBRPoint, BRPoint; //右下角
	HWND  hwndChild = ::GetWindow(m_hWnd, GW_CHILD);  //列出所有控件  
	while (hwndChild)
	{
		woc = ::GetDlgCtrlID(hwndChild);//取得ID
		GetDlgItem(woc)->GetWindowRect(Rect);
		ScreenToClient(Rect);
		OldTLPoint = Rect.TopLeft();
		TLPoint.x = long(OldTLPoint.x*fsp[0]);
		TLPoint.y = long(OldTLPoint.y*fsp[1]);
		OldBRPoint = Rect.BottomRight();
		BRPoint.x = long(OldBRPoint.x *fsp[0]);
		BRPoint.y = long(OldBRPoint.y *fsp[1]);
		Rect.SetRect(TLPoint, BRPoint);
		GetDlgItem(woc)->MoveWindow(Rect, TRUE);
		hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT);
	}

	old = Newp;

	static bool init_first = TRUE;
	if (init_first)
	{
		init_first = FALSE;
		return;
	}
	
	for (int i = 0; i < m_listCtrl.GetHeaderCtrl()->GetItemCount(); i++)
	{
		int width = m_listCtrl.GetColumnWidth(i);
		width = long(width *fsp[0]);
		m_listCtrl.SetColumnWidth(i, width);
	}	
}


// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CBatchUpDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CBatchUpDlg::OnBnClickedButtonFile()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	
	CFileDialog  dlg(true, _T(".IMG"), NULL, OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, _T("software image(*.IMG)|*.IMG|All Files(*.*)|*.*||"));
	if (dlg.DoModal() == IDOK)
	{
		m_imgPath = dlg.GetPathName();
		m_ver = dlg.GetFileTitle();
		UpdateData(FALSE);
	}
}

void CBatchUpDlg::OnBnClickedButtonStart()
{
	// TODO: Add your control notification handler code here
	g_thread_quiting = FALSE;

	memset(g_astIPList, 0, sizeof(g_astIPList));
	for (int i = 0; i < IPLIST_MAX_ITEM; i++)
	{
		g_astIPList[i].runnning = FALSE;
		g_astIPList[i].state = E_Init;
		g_astIPList[i].used = FALSE;
	}
	UpdateData(TRUE);
	
	if (m_imgPath.GetLength() == 0)
	{
		MessageBox(_T("请选择软件路径"));
		goto Quit_On_Fail;
	}

	m_pCapThread = CreateThread(NULL, 0, thread_capture, NULL, 0, NULL);
	if (NULL == m_pCapThread)
	{
		MessageBox(_T("启动IP监听线程出错"));
		goto Quit_On_Fail;
	}

	m_pArpThread = CreateThread(NULL, 0, thread_arp, NULL, 0, NULL);
	if (NULL == m_pArpThread)
	{
		MessageBox(_T("启动arp探测线程出错"));
		goto Quit_On_Fail;
	}

	m_pProcessThread = CreateThread(NULL, 0, thread_process, NULL, 0, NULL);
	if (NULL == m_pProcessThread)
	{
		MessageBox(_T("启动进度监控线程出错"));
		goto Quit_On_Fail;
	}

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_COMBO_NET)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT_VER)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_FILE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK_TYPE)->EnableWindow(FALSE);
	GetDlgItem(IDC_COMBO_PLATFORM)->EnableWindow(FALSE);
	
	goto Quit_On_OK;

Quit_On_Fail:
	g_thread_quiting = TRUE;
	
	if (NULL != m_pCapThread)
	{
		WaitForMultipleObjects(1, &m_pCapThread, TRUE, INFINITE);
		CloseHandle(m_pCapThread);
		m_pCapThread = NULL;
	}
	if (NULL != m_pArpThread)
	{
		WaitForMultipleObjects(1, &m_pArpThread, TRUE, INFINITE);
		CloseHandle(m_pArpThread);
		m_pArpThread = NULL;
	}	
	if (NULL != m_pProcessThread)
	{
		WaitForMultipleObjects(1, &m_pProcessThread, TRUE, INFINITE);
		CloseHandle(m_pProcessThread);
		m_pProcessThread = NULL;
	}
	return;

Quit_On_OK:
	return;
}

void CBatchUpDlg::OnBnClickedButtonStop()
{
	// TODO: Add your control notification handler code here
	g_thread_quiting = TRUE;
	if (NULL != m_pCapThread)
	{
		WaitForMultipleObjects(1, &m_pCapThread, TRUE, INFINITE);
		CloseHandle(m_pCapThread);
		m_pCapThread = NULL;
	}
	if (NULL != m_pArpThread)
	{
		WaitForMultipleObjects(1, &m_pArpThread, TRUE, INFINITE);
		CloseHandle(m_pArpThread);
		m_pArpThread = NULL;
	}	
	if (NULL != m_pProcessThread)
	{
		WaitForMultipleObjects(1, &m_pProcessThread, TRUE, INFINITE);
		CloseHandle(m_pProcessThread);
		m_pProcessThread = NULL;
	}

	if (NULL != g_pd)
	{
		Sleep(1000);
		pcap_close(g_pd);
		g_pd = NULL;
	}

	WaitForSingleObject(g_hIPListMutex,INFINITE);
	for (int i = 0; i < IPLIST_MAX_ITEM; i++)
	{
		if (TRUE != g_astIPList[i].used)
		{
			continue;
		}
		
		if (NULL != g_astIPList[i].hProcess)
		{
			TerminateProcess(g_astIPList[i].hProcess, 0);
		}
	}
	ReleaseMutex(g_hIPListMutex);
			
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_COMBO_NET)->EnableWindow(TRUE);
	GetDlgItem(IDC_EDIT_VER)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_FILE)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK_TYPE)->EnableWindow(TRUE);
	GetDlgItem(IDC_COMBO_PLATFORM)->EnableWindow(TRUE);
	
	dbgprint("user force stop....quit");
}
void CBatchUpDlg::OnClickedTypeCheckBox()
{
	// TODO: Add your control notification handler code here

	UpdateData(TRUE);
}

void CBatchUpDlg::OnSelchangeComboNet()
{
	// TODO: Add your control notification handler code here
	int sel_index = m_netcardCtrl.GetCurSel();	
	int index = 0;

	pcap_if_t *alldevs;
	pcap_if_t *d;
	char errbuf[PCAP_ERRBUF_SIZE+1];
	
	/* Retrieve the device list */
	if(pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		MessageBox(_T("没有可用的网卡"));
		return;
	}

	for (d = alldevs; d; d = d->next)
	{
		if (sel_index == index)
		{
			sprintf_s(g_ifname, "%s", d->name);
			break;
		}

		index++;
	}

	/* Free the device list */
	pcap_freealldevs(alldevs);

	if (0 == strlen(g_ifname))
	{
		MessageBox(_T("没有找到对应的网卡"));
		return;
	}

	CString str;
	m_netcardCtrl.GetLBText(sel_index, str);

	int num[4];
	swscanf_s(str.GetString(), _T("%d.%d.%d.%d@"), &num[0], &num[1], &num[2], &num[3]);
	dbgprint("local ip: %d.%d.%d.%d", num[0], num[1], num[2], num[3]);

	int ip_local = num[3] | (num[2] << 8) | (num[1] << 16) | (num[0] << 24);
	g_iplocal = htonl(ip_local);

	/*获取网卡  */
	{
	    PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO;
	    unsigned long    ulSize = sizeof(IP_ADAPTER_INFO);

	    //获取适配器信息
	    int nRet = GetAdaptersInfo(pIpAdapterInfo,&ulSize);
	    if (ERROR_BUFFER_OVERFLOW == nRet)
	    {
	        //空间不足，删除之前分配的空间
	        delete []pIpAdapterInfo;

	        //重新分配大小
	        pIpAdapterInfo = (PIP_ADAPTER_INFO) new BYTE[ulSize];

	        //获取适配器信息
	        nRet = GetAdaptersInfo(pIpAdapterInfo,&ulSize);

	        //获取失败
	        if (ERROR_SUCCESS != nRet)
	        {
	            if (pIpAdapterInfo != NULL)
	            {
	                delete []pIpAdapterInfo;
	            }

				MessageBox(_T("获取网卡列表失败"));
	            return ;
	        }
	    }
		
	    //赋值指针
	    PIP_ADAPTER_INFO pIterater = pIpAdapterInfo;
	    while (pIterater)
	    {
			if (NULL != strstr(g_ifname, pIterater->AdapterName))
			{
				dbgprint("AdapterName=%s, mac=%02x:%02x:%02x:%02x:%02x:%02x", pIterater->AdapterName, 
																			  pIterater->Address[0],
																			  pIterater->Address[1],
																			  pIterater->Address[2],
																			  pIterater->Address[3],
																			  pIterater->Address[4],
																			  pIterater->Address[5]);
				
				memcpy(g_ifmac, pIterater->Address, sizeof(g_ifmac));
				break;
			}
			
	        pIterater = pIterater->Next;
	    }

	    //清理
	    if (pIpAdapterInfo)
	    {
	        delete []pIpAdapterInfo;
	    }	
	}	
}

void CBatchUpDlg::OnDestroy()
{
	// TODO: Add your message handler code here
	g_thread_quiting = TRUE;
	
	if (NULL != m_pCapThread)
	{
		WaitForMultipleObjects(1, &m_pCapThread, TRUE, INFINITE);
		CloseHandle(m_pCapThread);
		m_pCapThread = NULL;
	}
	if (NULL != m_pArpThread)
	{
		WaitForMultipleObjects(1, &m_pArpThread, TRUE, INFINITE);
		CloseHandle(m_pArpThread);
		m_pArpThread = NULL;
	}	
	if (NULL != m_pProcessThread)
	{
		WaitForMultipleObjects(1, &m_pProcessThread, TRUE, INFINITE);
		CloseHandle(m_pProcessThread);
		m_pProcessThread = NULL;
	}


	KillTimer(UPDATE_UI_TIMER);
	
	CDialogEx::OnDestroy();
}

void CBatchUpDlg::OnTimer(UINT nIDEvent)
{
	switch(nIDEvent)
	{
		case UPDATE_UI_TIMER:
			WaitForSingleObject(g_hIPListMutex,INFINITE);
			for (int i = 0; i < IPLIST_MAX_ITEM; i++)
			{
				if (TRUE != g_astIPList[i].used)
				{
					continue;
				}
				
				g_astIPList[i].starttime++;
				g_astIPList[i].idletime++;

				if (g_astIPList[i].idletime > 300)
				{
					if (NULL != g_astIPList[i].hProcess)
					{
						TerminateProcess(g_astIPList[i].hProcess, 0);
					}
					g_astIPList[i].used = FALSE;
					g_astIPList[i].state = E_Init;
					g_astIPList[i].runnning = FALSE;
					g_astIPList[i].idletime = 0;
					g_astIPList[i].starttime = 0;
					g_astIPList[i].hProcess = NULL;
					g_astIPList[i].ipaddr = 0;
				}
			}
			ReleaseMutex(g_hIPListMutex);

			UpdateData(FALSE);
			break;

		default:
			break;
	}
}






void CBatchUpDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialogEx::OnSizing(fwSide, pRect);

	// TODO: Add your message handler code here
}

static int CALLBACK MyCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int row1 = (int) lParam1;
	int row2 = (int) lParam2;

	CColorListCtrl* lc = (CColorListCtrl*)lParamSort;
	CString lp1 = lc->GetItemText(row1, g_pstDlgPtr->m_list_clickedCol);
	CString lp2 = lc->GetItemText(row2, g_pstDlgPtr->m_list_clickedCol);

	if (g_pstDlgPtr->m_sortorder_inc)
		return lp1.CompareNoCase(lp2);
	else
		return lp2.CompareNoCase(lp1);
}

void CBatchUpDlg::OnLvnColumnclickListUp(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here

	m_list_clickedCol = pNMLV->iSubItem;//点击的列

	m_listCtrl.SortItems(MyCompareProc, (DWORD_PTR)&m_listCtrl);

	if (m_sortorder_inc)
		m_sortorder_inc = false;
	else
		m_sortorder_inc = true;
	
	*pResult = 0;	
}


void CBatchUpDlg::OnNMRClickListUp(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;

	if (m_listCtrl.GetSelectedCount() <= 0)
	{
		return;
	}

	CMenu menu, *pPopup;
	menu.LoadMenu(IDR_MENU_COPY);
	pPopup = menu.GetSubMenu(0);
	CPoint myPoint;
	ClientToScreen(&myPoint);
	GetCursorPos(&myPoint); //鼠标位置  
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, myPoint.x, myPoint.y, this);

}


void CBatchUpDlg::OnCopy2ClipBoard()
{
	// TODO: Add your command handler code here
	CString str;

	str = _T("");
	POSITION pos = m_listCtrl.GetFirstSelectedItemPosition(); //pos选中的首行位置
	while (pos) //如果选择多行
	{
		int nIdx = -1;

		nIdx = m_listCtrl.GetNextSelectedItem(pos);

		if (nIdx >= 0 && nIdx < m_listCtrl.GetItemCount())
		{
			for (int col = HD_ID; col <= HD_PROGRESS; col++)
			{
				str += m_listCtrl.GetItemText(nIdx, col);
				str += _T("\t");
			}
			str += _T("\r\n");
		}
		else
		{
			break;
		}
	}

	if (OpenClipboard())
	{
		EmptyClipboard();
		HGLOBAL hClip = GlobalAlloc(GMEM_MOVEABLE, (str.GetLength() + 1)*sizeof(TCHAR));
		if (hClip)
		{
			LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hClip);
			memcpy(lptstrCopy, str.GetString(), str.GetLength() * sizeof(TCHAR));
			lptstrCopy[str.GetLength()] = (TCHAR)0;
			GlobalUnlock(hClip);
		}

		SetClipboardData(CF_UNICODETEXT, hClip);
		CloseClipboard();
	}
}
