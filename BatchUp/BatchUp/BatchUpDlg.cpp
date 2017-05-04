
// BatchUpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "BatchUp.h"
#include "ColorListCtrl.h"
#include "BatchUpDlg.h"
#include "afxdialogex.h"
#include <IPHlpApi.h>
#include "bru_cmd.h"
#include "pcap.h"

#pragma comment(lib,"IPHlpApi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
	E_Upgrade,
	E_Check,
	E_Done
}Upgrade_Fsm_E;

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

#define     MAX_RETRY            10
#define     IPLIST_MAX_ITEM      256

#define     WAIT_SECONDS         20

bool 		 g_scanthread_quiting = FALSE;
IPList_S	 g_astIPList[IPLIST_MAX_ITEM];
CBatchUpDlg *g_pstDlgPtr = NULL;
HANDLE       g_hListCtrlMutex = NULL;
HANDLE       g_hIPListMutex = NULL;
pcap_t*      g_pd = NULL;

#define      UPDATE_UI_TIMER  1


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

void Add2List(IPList_S *pstInfo)
{
	WaitForSingleObject(g_hListCtrlMutex,INFINITE);
	if (NULL == g_pstDlgPtr)
	{
		ReleaseMutex(g_hListCtrlMutex);
		return;
	}

	bool is_new = TRUE;
	int  item_index = g_pstDlgPtr->m_listCtrl.GetItemCount();

	/*�����Ƿ��Ѿ�����  */
	for (int i = 0; i < g_pstDlgPtr->m_listCtrl.GetItemCount(); i++)
	{
		if (pstInfo->ipaddr == g_pstDlgPtr->m_listCtrl.GetItemData(i))
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
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("׼������"));
			break;
		case E_Upgrade:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("����..."));
			break;						
		#if  0
		case E_1stDone:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("��������"));
			break;
		case E_2ndDone:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("�������"));
			break;
		#endif /* #if 0 */
		case E_Check:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("�������"));
			break;
		case E_Done:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("�������"));
			break;			
		default:
			break;
	}

	//progress
	g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_PROGRESS, _T("׼������"));
	g_pstDlgPtr->m_listCtrl.SetItemColor(item_index, RGB(0, 0, 0), RGB(0xff, 0xff, 0xff));
	
	ReleaseMutex(g_hListCtrlMutex);
}

void UpdateProgress(DWORD ipaddr, CString strinfo)
{
	WaitForSingleObject(g_hListCtrlMutex,INFINITE);

	if (NULL == g_pstDlgPtr)
	{
		ReleaseMutex(g_hListCtrlMutex);
		return;
	}

	int  item_index = -1;
	int  node_index = -1;

	/*�����Ƿ��Ѿ�����  */
	for (int i = 0; i < g_pstDlgPtr->m_listCtrl.GetItemCount(); i++)
	{
		if (ipaddr == g_pstDlgPtr->m_listCtrl.GetItemData(i))
		{
			item_index = i;
			break;
		}
	}

	WaitForSingleObject(g_hIPListMutex,INFINITE);
	for (int i = 0; i < IPLIST_MAX_ITEM; i++)
	{
		if (TRUE != g_astIPList[i].used)
		{
			continue;
		}
		
		if (ipaddr == g_astIPList[i].ipaddr)
		{
			node_index = i;
			break;
		}
	}
	ReleaseMutex(g_hIPListMutex);

	if (item_index < 0 || node_index < 0)
	{
		ReleaseMutex(g_hListCtrlMutex);
		return;
	}

	if (TRUE != g_astIPList[node_index].connectted)
	{
		ReleaseMutex(g_hListCtrlMutex);
		return;
	}
	
	//state
	switch(g_astIPList[node_index].state)
	{
		case E_Init:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("׼������"));
			break;
		case E_Upgrade:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("����..."));
			break;						
		#if  0
		case E_1stDone:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("��������"));
			break;
		case E_2ndDone:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("�������"));
			break;
		#endif /* #if 0 */
		case E_Check:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("�������"));
			break;
		case E_Done:
			g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_STATE, _T("�������"));
			break;						
		default:
			break;
	}
	
	//progress
	g_pstDlgPtr->m_listCtrl.SetItemText(item_index, HD_PROGRESS, strinfo);
	g_pstDlgPtr->m_listCtrl.SetItemColor(item_index, RGB(0, 0, 0), RGB(0xff, 0xff, 0xff));

	ReleaseMutex(g_hListCtrlMutex);
}

void UpdateResult(DWORD ipaddr, bool isok)
{
	WaitForSingleObject(g_hListCtrlMutex,INFINITE);

	if (NULL == g_pstDlgPtr)
	{
		ReleaseMutex(g_hListCtrlMutex);
		return;
	}

	int  item_index = -1;

	/*�����Ƿ��Ѿ�����  */
	for (int i = 0; i < g_pstDlgPtr->m_listCtrl.GetItemCount(); i++)
	{
		if (ipaddr == g_pstDlgPtr->m_listCtrl.GetItemData(i))
		{
			item_index = i;
			break;
		}
	}

	if (item_index < 0)
	{
		ReleaseMutex(g_hListCtrlMutex);
		return;
	}

	if (isok)
	{
		g_pstDlgPtr->m_listCtrl.SetItemColor(item_index, RGB(0, 0, 0), RGB(0, 0xff, 0));
	}
	else
	{
		g_pstDlgPtr->m_listCtrl.SetItemColor(item_index, RGB(0, 0, 0), RGB(0xff, 0, 0));
	}

	ReleaseMutex(g_hListCtrlMutex);	
}

/* 
 * ����������
 */
UINT Thread_Check(LPVOID pParam)
{
	int       retry = 0;
	IPList_S *pstIPInfo = (IPList_S *)pParam;

	DWORD     ipaddr = pstIPInfo->ipaddr;

	dbgprint("enter");

	WaitForSingleObject(g_hIPListMutex,INFINITE);
	if (pstIPInfo->state < E_Upgrade)
	{
		dbgprint("!! unexpect state(%d) ip=0x%08x !!", pstIPInfo->state, ipaddr);

		pstIPInfo->used = FALSE;
		pstIPInfo->runnning = FALSE;
		ReleaseMutex(g_hIPListMutex);
		
		return -1;
	}
	pstIPInfo->state = E_Check;
	ReleaseMutex(g_hIPListMutex);

	while (1)
	{
		WaitForSingleObject(g_hIPListMutex,INFINITE);
		int waittime = pstIPInfo->waittime;
		ReleaseMutex(g_hIPListMutex);
		
		if (waittime > WAIT_SECONDS)
		{
			break;
		}
		
		Sleep(1000);
	}

	int 	ret = 0;
	int  	session = -1;
	bool    retry_enable = TRUE;

	while (retry++ < MAX_RETRY && TRUE == retry_enable)
	{
		CString strlog;
		strlog.Format(_T("��������%d/%d"), retry, MAX_RETRY);
		UpdateProgress(ipaddr, strlog);
		Sleep(5000);

		dbgprint("login");
		ret = bru_ssh_login(ipaddr, &session);
		if (0 != ret)
		{
			dbgprint("ssh login fail, ip=0x%08x, ret=%d", ipaddr, ret);
			//bru_ssh_logout(session);
			continue;
		}

		WaitForSingleObject(g_hIPListMutex,INFINITE);
		pstIPInfo->connectted = TRUE;
		ReleaseMutex(g_hIPListMutex);

		dbgprint("get bootm");
		
		int  bootm = 0xFFFFFFFF;
		ret |= bru_ssh_get_bootm(session, &bootm);
		if (0 != ret)
		{
			dbgprint("get info fail, ip=0x%08x, ret=%d", ipaddr, ret);
			bru_ssh_logout(session);
			continue;
		}		

		dbgprint("get current version");

		//�ϴ���ƹ���
		bru_ssh_uploadfile(session, "ledctrl", "/tmp/ledctrl");
		
		UpdateProgress(ipaddr, _T("��鵱ǰ��"));

		char szversion[128] = {0};
		ret |= bru_ssh_get_curr_version(session, 0, szversion, sizeof(szversion));
		if (0 != ret)
		{
			dbgprint("get software version fail, ip=0x%08x, ret=%d", ipaddr, ret);
			bru_ssh_logout(session);
			continue;
		}

		WaitForSingleObject(g_hIPListMutex,INFINITE);
		memcpy(pstIPInfo->szversion, szversion, sizeof(pstIPInfo->szversion)-1);
		Add2List(pstIPInfo);
		ReleaseMutex(g_hIPListMutex);

		dbgprint("check current version");

		//��鵱ǰ���汾
		char sz_dstver[256] = {0};
		utils_TChar2Char((TCHAR *)g_pstDlgPtr->m_ver.GetString(), sz_dstver, sizeof(sz_dstver));
		if (NULL == strstr(szversion, sz_dstver))
		{
			UpdateProgress(ipaddr, _T("��ǰ���汾��Ŀ��汾��һ��"));
			dbgprint("upgrade check fail, ip=0x%08x, ret=%d", ipaddr, ret);
			ret = -1;
			break;
		}

		dbgprint("check backup version");
		
		UpdateProgress(ipaddr, _T("��鱸��"));
		ret = bru_ssh_checkback(session, bootm, sz_dstver);
		if (0 != ret)
		{
			dbgprint("upgrade check fail, ip=0x%08x, ret=%d", ipaddr, ret);
			UpdateProgress(ipaddr, _T("�����汾��Ŀ��汾��һ��"));
			ret = -1;
			break;
		}

		dbgprint("check version complete");
		//UpdateProgress(ipaddr, _T("�����ɹ�"));
		break;
	}

	//bru_ssh_reboot(session);

	bru_ssh_logout(session);

	WaitForSingleObject(g_hIPListMutex,INFINITE);
	pstIPInfo->state = E_Done;
	pstIPInfo->runnning = FALSE;
	ReleaseMutex(g_hIPListMutex);
	UpdateProgress(ipaddr, _T("�����ɹ�"));
	
	if (0 == ret)
	{
		UpdateResult(ipaddr, TRUE);
	}
	else
	{
		UpdateResult(ipaddr, FALSE);
	}

	WaitForSingleObject(g_hIPListMutex,INFINITE);
	pstIPInfo->used = FALSE;
	ReleaseMutex(g_hIPListMutex);
	
	dbgprint("ip=0x%08x update finished, ret=%d", ipaddr, ret);
	dbgprint("check version quit");
	return ret;
}

/* 
 * ��ȡģ�����Ϣ������ʼ����
 */
UINT Thread_Update(LPVOID pParam)
{
	int       retry = 0;
	IPList_S *pstIPInfo = (IPList_S *)pParam;

	DWORD     ipaddr = pstIPInfo->ipaddr;

	dbgprint("enter");

	WaitForSingleObject(g_hIPListMutex,INFINITE);
	if (pstIPInfo->state >= E_Upgrade)
	{
		dbgprint("!! unexpect state(%d) ip=0x%08x !!", pstIPInfo->state, ipaddr);

		pstIPInfo->used = FALSE;
		pstIPInfo->runnning = FALSE;
		ReleaseMutex(g_hIPListMutex);
		
		return -1;
	}
	pstIPInfo->state = E_Upgrade;
	dbgprint("mac=%02X%02X%02X%02X%02X%02X", pstIPInfo->macaddr[0], pstIPInfo->macaddr[1], pstIPInfo->macaddr[2], 
		                                     pstIPInfo->macaddr[3], pstIPInfo->macaddr[4], pstIPInfo->macaddr[5]);
	
	ReleaseMutex(g_hIPListMutex);

	while (1)
	{
		WaitForSingleObject(g_hIPListMutex,INFINITE);
		int waittime = pstIPInfo->waittime;
		ReleaseMutex(g_hIPListMutex);
		
		if (waittime > WAIT_SECONDS)
		{
			break;
		}
		
		Sleep(1000);
	}
	
	int 	ret = 0;
	int  	session = -1;
	bool    retry_enable = TRUE;

	while (retry++ < MAX_RETRY && TRUE == retry_enable)
	{
		CString strlog;
		strlog.Format(_T("��������%d/%d"), retry, MAX_RETRY);
		UpdateProgress(ipaddr, strlog);
		Sleep(5000);

		dbgprint("login");
		ret = bru_ssh_login(ipaddr, &session);
		if (0 != ret)
		{
			dbgprint("ssh login fail, ip=0x%08x, ret=%d", ipaddr, ret);
			//bru_ssh_logout(session);
			continue;
		}

		WaitForSingleObject(g_hIPListMutex,INFINITE);
		pstIPInfo->connectted = TRUE;
		ReleaseMutex(g_hIPListMutex);

		dbgprint("get dut info");
		
		/*��ȡ�����Ϣ*/
		char szsn[64] = {0};
		char szmac[64] = {0};
		int  bootm = 0xFFFFFFFF;
		ret |= bru_ssh_get_sn(session, szsn, sizeof(szsn));
		ret |= bru_ssh_get_mac(session, szmac, sizeof(szmac));
		ret |= bru_ssh_get_bootm(session, &bootm);
		if (0 != ret)
		{
			dbgprint("get info fail, ip=0x%08x, ret=%d", ipaddr, ret);
			bru_ssh_logout(session);
			continue;
		}

		char *pblankmac = "FFFFFFFFFFFF";
		dbgprint("szmac=[%s]", szmac);
		if (0 == strcmp(szmac, pblankmac))
		{
			dbgprint("!!set mac address!!");
			ret = bru_ssh_set_macaddr(session, pstIPInfo->macaddr);
			if (0 != ret)
			{
				dbgprint("set macaddr fail, ret=%d", ret);
			}
		}
		
		dbgprint("get dut version");
		
		char szversion[128] = {0};
		ret |= bru_ssh_get_curr_version(session, 0, szversion, sizeof(szversion));
		if (0 != ret)
		{
			dbgprint("get software version fail, ip=0x%08x, ret=%d", ipaddr, ret);
			bru_ssh_logout(session);
			continue;
		}

		WaitForSingleObject(g_hIPListMutex,INFINITE);
		memcpy(pstIPInfo->szsn, szsn, sizeof(pstIPInfo->szsn)-1);
		memcpy(pstIPInfo->szmac, szmac, sizeof(pstIPInfo->szmac)-1);
		memcpy(pstIPInfo->szversion, szversion, sizeof(pstIPInfo->szversion)-1);
		Add2List(pstIPInfo);
		ReleaseMutex(g_hIPListMutex);
		
		UpdateProgress(ipaddr, _T("�ϴ����"));

		retry_enable = FALSE;

		Sleep(2000);

		dbgprint("upload software");
			
		/*�ϴ����  */
		char szimgfile[512] = {0};
		utils_TChar2Char((TCHAR *)(g_pstDlgPtr->m_imgPath.GetString()), szimgfile, sizeof(szimgfile));
		ret = bru_ssh_uploadfile(session, szimgfile, "/tmp/firmware.img");
		if (0 != ret)
		{
			dbgprint("upload img fail, ip=0x%08x, ret=%d", ipaddr, ret);
			UpdateProgress(ipaddr, _T("����ʧ��"));
			bru_ssh_logout(session);
			continue;
		}

		dbgprint("upgrade easyupgrade");

		/*�ϴ���������  */
		ret = bru_ssh_uploadfile(session, "easyupgrade", "/tmp/easyupgrade");
		if (0 != ret)
		{
			dbgprint("upload easyupgrade fail, ip=0x%08x, ret=%d", ipaddr, ret);
			UpdateProgress(ipaddr, _T("����ʧ��"));
			bru_ssh_logout(session);
			continue;
		}

		//�ϴ���ƹ���
		bru_ssh_uploadfile(session, "ledctrl", "/tmp/ledctrl");

		/*����  */
		UpdateProgress(ipaddr, _T("������"));
		ret = bru_ssh_upgrade(session);
		if (0 != ret)
		{
			dbgprint("upgrade fail, ip=0x%08x, ret=%d", ipaddr, ret);
			UpdateProgress(ipaddr, _T("����ʧ��"));
			bru_ssh_logout(session);
			ret = -1;
			break;
		}
		
		UpdateProgress(ipaddr, _T("������һ�׶�"));
		dbgprint("upgrade complete");
		
		break;
	}

	
	if (0 == ret)
	{
		WaitForSingleObject(g_hIPListMutex,INFINITE);
		#if  0
		if (pstIPInfo->state < E_1stDone)
		{
			pstIPInfo->state = E_1stDone;
		}
		else if (pstIPInfo->state < E_2ndDone)
		{
			pstIPInfo->state = E_2ndDone;
		}
		#endif /* #if 0 */
		ReleaseMutex(g_hIPListMutex);

		bru_ssh_reboot(session);
	}
	else
	{
		WaitForSingleObject(g_hIPListMutex,INFINITE);
		bool connect = pstIPInfo->connectted;
		ReleaseMutex(g_hIPListMutex);

		if (TRUE == connect)
		{
			UpdateResult(ipaddr, FALSE);
		}
		
		WaitForSingleObject(g_hIPListMutex,INFINITE);
		pstIPInfo->used = FALSE;
		ReleaseMutex(g_hIPListMutex);
	}

	bru_ssh_logout(session);

	WaitForSingleObject(g_hIPListMutex,INFINITE);
	pstIPInfo->runnning = FALSE;
	ReleaseMutex(g_hIPListMutex);

	dbgprint("ip=0x%08x update finished, ret=%d", ipaddr, ret);
	dbgprint("upgrade quit");
	return ret;
}


void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
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
	DWORD         ipaddr = 0xFFFFFFFF;
	unsigned char szmac[6] = {0xFF};
	unsigned char defszmac[6] = {0x00, 0x15, 0x17, 0x14, 0xC6, 0x04};
	
	int offset = sizeof(DhcpHdr_S) + sizeof(unsigned int);
	while ((szpkt[offset] != 255) && (offset + 2 < pktlen))  /* 2��ʾtlv�ֶε�type��len  */
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

				if (0 == memcmp(defszmac, szmac, sizeof(szmac)))
				{
					return;
				}
			}
		}			

		if (50 == v_type) /* request ip  */
		{
			if (4 == v_len)
			{
				ipaddr = *(DWORD *)&szpkt[offset + 2];
				ipaddr = htonl(ipaddr);
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

	#if  0
	if (0xAc10018B != ipaddr)
	{
		dbgprint("!!not allowed ipaddr!!");
		return;
	}
	#endif /* #if 0 */

	dbgprint("ip=%d.%d.%d.%d mac=[%02X:%02X:%02X:%02X:%02X:%02X]",   (ipaddr >> 24) & 0xFF,
																	  (ipaddr >> 16) & 0xFF,
																	  (ipaddr >> 8) & 0xFF,
																	  ipaddr & 0xFF,
																	  szmac[0], szmac[1], szmac[2],
																	  szmac[3], szmac[4], szmac[5]);

	WaitForSingleObject(g_hIPListMutex,INFINITE);

	int i = 0;
	
	//�鿴�Ƿ����ظ���IP
	for (i = 0; i < IPLIST_MAX_ITEM; i++)
	{
		if (TRUE != g_astIPList[i].used)
		{
			continue;
		}

		//���mac��ͻ�����µĸ��Ǿ͵�
		if (0 == memcmp(g_astIPList[i].macaddr, szmac, 6))
		{
			g_astIPList[i].ipaddr = ipaddr;
			g_astIPList[i].waittime = 0;
			g_astIPList[i].connectted = FALSE;
			break;
		}
		
		if (ipaddr == g_astIPList[i].ipaddr)
		{
			g_astIPList[i].waittime = 0;
			g_astIPList[i].connectted = FALSE;
			break;
		}
	}

	if (i < IPLIST_MAX_ITEM)
	{
		if (E_Init <= g_astIPList[i].state && g_astIPList[i].state <= E_Check)
		{
			if (g_astIPList[i].runnning == TRUE)
			{
				dbgprint("dut busy, ip=0x%08x, state=%d ", ipaddr, g_astIPList[i].state);
				ReleaseMutex(g_hIPListMutex);
				return;
			}
			else
			{
				g_astIPList[i].runnning = TRUE;
				dbgprint("dut start, ip=0x%08x, state=%d ", ipaddr, g_astIPList[i].state);
			}
		}
		else
		{
			dbgprint("invalid state, ip=0x%08x, state=%d ", ipaddr, g_astIPList[i].state);
			memset(&(g_astIPList[i]), 0, sizeof(IPList_S));
			g_astIPList[i].state = E_Init;
			g_astIPList[i].runnning = FALSE;
			
			ReleaseMutex(g_hIPListMutex);
			return;
		}
	}
	else //new
	{
		/*����������IP��ӵ�����  */
		for (i = 0; i < IPLIST_MAX_ITEM; i++)
		{
			if (TRUE != g_astIPList[i].used)
			{
				g_astIPList[i].ipaddr = ipaddr;
				memcpy(g_astIPList[i].macaddr, szmac, 6);
				g_astIPList[i].state = E_Init;
				g_astIPList[i].waittime = 0;
				g_astIPList[i].runnning = TRUE;
				g_astIPList[i].used = TRUE;
				g_astIPList[i].connectted = FALSE;
				break;
			}
		}

		dbgprint("new dut start, ip=0x%08x, state=%d ", ipaddr, g_astIPList[i].state);
	}


	if (i < IPLIST_MAX_ITEM)
	{
		if (g_astIPList[i].state < E_Upgrade)
		{
			if (NULL == AfxBeginThread(Thread_Update, &g_astIPList[i]))
			{
				dbgprint("update fail");
				memset(&(g_astIPList[i]), 0, sizeof(IPList_S));
				g_astIPList[i].state = E_Init;
				g_astIPList[i].runnning = FALSE;
			}
		}
		else
		{
			if (NULL == AfxBeginThread(Thread_Check, &g_astIPList[i]))
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

/* 
 * ����dhcp request���ģ�������ȡIP��ַ
 */
UINT Thread_Snooping(LPVOID pParam)
{
	{
		int sel_index = g_pstDlgPtr->m_netcardCtrl.GetCurSel();	
		int index = 0;

		pcap_if_t *alldevs;
		pcap_if_t *d;
		char errbuf[PCAP_ERRBUF_SIZE+1];
		
		/* Retrieve the device list */
		if(pcap_findalldevs(&alldevs, errbuf) == -1)
		{
			g_pstDlgPtr->MessageBox(_T("û�п��õ�����"));
			g_scanthread_quiting = 0;
			return -1;
		}

		char ifname[256] = {0};
		for (d = alldevs; d; d = d->next)
		{
			if (sel_index == index)
			{
				memcpy(ifname, d->name, strlen(d->name));
				break;
			}

			index++;
		}

		/* Free the device list */
		pcap_freealldevs(alldevs);

		if (0 == strlen(ifname))
		{
			g_pstDlgPtr->MessageBox(_T("û���ҵ���Ӧ������"));
			g_scanthread_quiting = 0;
			return -1;
		}

		
		char ebuf[PCAP_ERRBUF_SIZE];
		g_pd = pcap_open_live(ifname, 65535, 1, 1000, ebuf);
		if (!g_pd) 
		{
			dbgprint("pcap open fail, %s", ebuf);
			g_scanthread_quiting = 0;
			return -1;
		}

		char filter_app[] = "udp dst port 67"; /* ���˱��ʽ */
		struct bpf_program filter; /* �Ѿ�����õĹ����� */
		if (pcap_compile(g_pd, &filter, filter_app, 1, 0xFFFFFFFF) < 0)
		{
			dbgprint("compile filter fail");
			pcap_close(g_pd);
			g_pd = NULL;
			g_scanthread_quiting = 0;
			return -1;			
		}
		if (pcap_setfilter(g_pd, &filter) < 0)
		{
			dbgprint("set filter fail");
			pcap_close(g_pd);
			g_pd = NULL;
			g_scanthread_quiting = 0;
			return -1;			
		}

		dbgprint("pcap start listenning");
		
		/* start the capture */
		while (1)
		{
			if (g_scanthread_quiting)
			{
				break;
			}
			
			pcap_loop(g_pd, 1, packet_handler, NULL);		
		}
		
		pcap_close(g_pd);
		g_pd = NULL;
	}

	dbgprint("thread quitting");
	g_scanthread_quiting = FALSE;
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
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_pScanThread = NULL;

#ifdef _DEBUG  
	//AllocConsole();
#endif	

	g_pstDlgPtr = this;
}

CBatchUpDlg::~CBatchUpDlg()
{
#ifdef _DEBUG  
	//FreeConsole();
#endif	

	g_pstDlgPtr = NULL;
}

void CBatchUpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST_UP, m_listCtrl);
	DDX_Text(pDX, IDC_EDIT_SW, m_imgPath);
	DDX_Text(pDX, IDC_EDIT_VER, m_ver);
	DDX_Control(pDX, IDC_COMBO_NET, m_netcardCtrl);
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
END_MESSAGE_MAP()


// CBatchUpDlg message handlers

BOOL CBatchUpDlg::OnInitDialog()
{
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
	//���ò������б�ؼ�
	LONG lStyle;
	lStyle = GetWindowLong(m_listCtrl.m_hWnd, GWL_STYLE);	//��ȡ��ǰ����style
	lStyle &= ~LVS_TYPEMASK;							    //�����ʾ��ʽλ
	lStyle |= LVS_REPORT;									//����style
	SetWindowLong(m_listCtrl.m_hWnd, GWL_STYLE, lStyle);	//����style

	DWORD dwStyle = m_listCtrl.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;						//ѡ��ĳ��ʹ���и�����ֻ������report����listctrl��
	dwStyle |= LVS_EX_GRIDLINES;							//�����ߣ�ֻ������report����listctrl��
	//dwStyle |= LVS_EX_CHECKBOXES;							//itemǰ����checkbox�ؼ�
	m_listCtrl.SetExtendedStyle(dwStyle);					//������չ���

	m_listCtrl.InsertColumn(HD_ID, _T("���"), LVCFMT_LEFT, 50);//������
	m_listCtrl.InsertColumn(HD_IP, _T("IP"), LVCFMT_LEFT, 90);	//������
	m_listCtrl.InsertColumn(HD_SN, _T("SN"), LVCFMT_LEFT, 160);	//������
	m_listCtrl.InsertColumn(HD_MAC, _T("MAC"), LVCFMT_LEFT, 120);	//������
	m_listCtrl.InsertColumn(HD_IMG, _T("Version"), LVCFMT_LEFT, 300);	//������
	m_listCtrl.InsertColumn(HD_STATE, _T("�׶�"), LVCFMT_LEFT, 100);	//������
	m_listCtrl.InsertColumn(HD_PROGRESS, _T("״̬"), LVCFMT_LEFT, 300);	//������

	memset(g_astIPList, 0, sizeof(g_astIPList));
	for (int i = 0; i < IPLIST_MAX_ITEM; i++)
	{
		g_astIPList[i].runnning = FALSE;
		g_astIPList[i].used = FALSE;
	}

	{
		//detect ethernet interface
		pcap_if_t *alldevs;
		pcap_if_t *d;
		char errbuf[PCAP_ERRBUF_SIZE+1];

		/* Retrieve the device list */
		if(pcap_findalldevs(&alldevs, errbuf) == -1)
		{
			MessageBox(_T("û�п��õ�����"));
			return FALSE;
		}
		
		/* Scan the list printing every entry */
		int index = 0;
		for (d = alldevs; d; d = d->next)
		{
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
			m_netcardCtrl.InsertString(index, szDev);
			index++;
		}

		/* Free the device list */
		pcap_freealldevs(alldevs);

		m_netcardCtrl.SetCurSel(0);
	}
	
	#if  0
	/*��ȡ����  */
	{
	    PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO;
	    unsigned long    ulSize = sizeof(IP_ADAPTER_INFO);

	    //��ȡ��������Ϣ
	    int nRet = GetAdaptersInfo(pIpAdapterInfo,&ulSize);
	    if (ERROR_BUFFER_OVERFLOW == nRet)
	    {
	        //�ռ䲻�㣬ɾ��֮ǰ����Ŀռ�
	        delete []pIpAdapterInfo;

	        //���·����С
	        pIpAdapterInfo = (PIP_ADAPTER_INFO) new BYTE[ulSize];

	        //��ȡ��������Ϣ
	        nRet = GetAdaptersInfo(pIpAdapterInfo,&ulSize);

	        //��ȡʧ��
	        if (ERROR_SUCCESS != nRet)
	        {
	            if (pIpAdapterInfo != NULL)
	            {
	                delete []pIpAdapterInfo;
	            }

				MessageBox(_T("��ȡ�����б�ʧ��"));
	            return TRUE;
	        }
	    }
		
	    //��ֵָ��
	    PIP_ADAPTER_INFO pIterater = pIpAdapterInfo;
	    while (pIterater)
	    {
	        //ָ��IP��ַ�б�
	        PIP_ADDR_STRING pIpAddr = &pIterater->IpAddressList;
	        //while (pIpAddr)
	        {
				CString strNetInfo;
				TCHAR   szAdpterIP[256] = {0};
				TCHAR   szAdpterMask[256] = { 0 };
				TCHAR   szAdpterDesc[256] = {0};

				utils_Char2Tchar(pIpAddr->IpAddress.String, szAdpterIP, sizeof(szAdpterIP));
				utils_Char2Tchar(pIpAddr->IpMask.String, szAdpterMask, sizeof(szAdpterMask));
				utils_Char2Tchar(pIterater->Description, szAdpterDesc, sizeof(szAdpterDesc));
				
				strNetInfo.Format(_T("%s@%s@%s"), szAdpterIP, szAdpterMask, szAdpterDesc);
				m_netcardCtrl.AddString(strNetInfo);
	            //pIpAddr = pIpAddr->Next;
	        }

	        pIterater = pIterater->Next;
	    }

	    //����
	    if (pIpAdapterInfo)
	    {
	        delete []pIpAdapterInfo;
	    }	

		m_netcardCtrl.SetCurSel(0);
	}
	#endif /* #if 0 */

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);

	g_hListCtrlMutex = CreateMutex(NULL, FALSE, _T("listctrl"));
	if (NULL == g_hListCtrlMutex)
	{
		MessageBox(_T("����g_hListCtrlMutex�����ź���ʧ��"));
	}

	g_hIPListMutex = CreateMutex(NULL, FALSE, _T("IPlist"));
	if (NULL == g_hIPListMutex)
	{
		MessageBox(_T("����g_hIPListMutex�����ź���ʧ��"));
	}

	SetTimer(UPDATE_UI_TIMER, 1000, 0);
	
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
	POINT Newp; //��ȡ���ڶԻ���Ĵ�С
	CRect recta;
	GetClientRect(&recta);     //ȡ�ͻ�����С  
	Newp.x = recta.right - recta.left;
	Newp.y = recta.bottom - recta.top;
	fsp[0] = (float)Newp.x / old.x;
	fsp[1] = (float)Newp.y / old.y;
	CRect Rect;
	int woc;
	CPoint OldTLPoint, TLPoint; //���Ͻ�
	CPoint OldBRPoint, BRPoint; //���½�
	HWND  hwndChild = ::GetWindow(m_hWnd, GW_CHILD);  //�г����пؼ�  
	while (hwndChild)
	{
		woc = ::GetDlgCtrlID(hwndChild);//ȡ��ID
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
	UpdateData(TRUE);
	
	if (m_imgPath.GetLength() == 0)
	{
		MessageBox(_T("��ѡ�����·��"));
		goto Quit_On_Fail;
	}

	m_pScanThread = AfxBeginThread(Thread_Snooping, NULL);
	if (NULL == m_pScanThread)
	{
		MessageBox(_T("����IP�������̳���"));
		goto Quit_On_Fail;
	}

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(TRUE);

	goto Quit_On_OK;

Quit_On_Fail:
	if (NULL != m_pScanThread)
	{
		g_scanthread_quiting = TRUE;
		while (g_scanthread_quiting)
		{
			Sleep(10);
		}
		Sleep(10);
		m_pScanThread = NULL;
	}

Quit_On_OK:
	return;
}

void CBatchUpDlg::OnBnClickedButtonStop()
{
	// TODO: Add your control notification handler code here

	if (NULL != g_pd)
	{
		pcap_breakloop(g_pd);
	}
	
	g_scanthread_quiting = TRUE;
	while (g_scanthread_quiting)
	{
		Sleep(10);
	}
	Sleep(10);
	m_pScanThread = NULL;

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);
}

void CBatchUpDlg::OnDestroy()
{
	// TODO: Add your message handler code here

	if (NULL != m_pScanThread)
	{
		g_scanthread_quiting = TRUE;
		while (g_scanthread_quiting)
		{
			Sleep(10);
		}

		m_pScanThread = NULL;
	}	

	if (NULL != g_hListCtrlMutex)
	{
		CloseHandle(g_hListCtrlMutex);
	}

	if (NULL != g_hIPListMutex)
	{
		CloseHandle(g_hIPListMutex);
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
				
				g_astIPList[i].waittime++;
			}
			ReleaseMutex(g_hIPListMutex);
			
			UpdateData(FALSE);
			break;

		default:
			break;
	}
}


