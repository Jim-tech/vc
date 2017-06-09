
// emWriteDlg.cpp : implementation file
//

#include "stdafx.h"
#include "emWrite.h"
#include "emWriteDlg.h"
#include "afxdialogex.h"
#include "ssh.h"

#include "WinSock2.h"
#include <WS2tcpip.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////////////////
unsigned char  g_szAgentCode[0x10000] = {0};
unsigned long  g_ulAgentLen = 0;
unsigned long  g_ulAgentOffset = 0;
unsigned short g_ulBlockNo = 0;
SOCKET		   g_tftpsock = -1;

Cfg_S  g_GlbConfig = {0};

const char g_MonthMap[] = {'-', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C'};

const char g_DateMap[] = {'-', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', //01-10
	                           'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', //11-20
	                           'M', 'N', 'P', 'R', 'S', 'T', 'V', 'W', 'X', 'Y', //21-30
	                           'Z'};                   							 //31

const SSH_ERR_INFO_S g_stSshErrInfo[] =
{
	{ -1, _T("未知错误") },
	{ ERR_WSA_START, _T("创建winsock失败") },
	{ ERR_LIBSSH_INIT, _T("ssh库初始化失败") },
	{ ERR_CONN_FAIL, _T("网络连接失败") },
	{ ERR_SSH_SESSION, _T("ssh连接失败") },
	{ ERR_SSH_SHAKE, _T("ssh协商失败") },
	{ ERR_SSH_KNOWNHOST, _T("ssh已知主机失败") },
	{ ERR_SSH_FINGERPRINT, _T("ssh fingerprint失败") },
	{ ERR_SSH_CHANNEL, _T("ssh创建通道失败") },
	{ ERR_SSH_AUTH, _T("ssh用户认证失败") },
	{ ERR_THREAD_FAIL, _T("创建线程失败") },
	{ ERR_SSH_NOT_READY, _T("ssh服务未就绪") },
};


int TChar2Char(TCHAR *pTChar, char *pChar, int maxlen)
{
	int length;

	length = WideCharToMultiByte(CP_ACP, 0, pTChar, -1, NULL, 0, NULL, NULL);  
	if (length > maxlen)
	{
		return -1;
	}
	
	WideCharToMultiByte(CP_ACP, 0, pTChar, -1, pChar, length, NULL, NULL);
	return 0;
}

int Char2Tchar(char *pChar, TCHAR *pThar, int maxlen)
{
	int length;

	length = MultiByteToWideChar(CP_ACP, 0, pChar, -1, NULL, 0);
	if (length > maxlen)
	{
		return -1;
	}
	
	MultiByteToWideChar(CP_ACP, 0, pChar, -1, pThar, length);
	return 0;
}



int GetPrivateProfileStringToChar(TCHAR *pSession, TCHAR *pKey, TCHAR *pDefaultVal, char *pVal, int maxlen, TCHAR *pPath)
{
	int length;
	TCHAR szBuffer[1024] = {0};
	
	GetPrivateProfileString(pSession, pKey, pDefaultVal, szBuffer, sizeof(szBuffer), pPath);

	length = WideCharToMultiByte(CP_ACP, 0, szBuffer, -1, NULL, 0, NULL, NULL);  
	if (length >= maxlen)
	{
		return -1;
	}
	
	WideCharToMultiByte(CP_ACP, 0, szBuffer, -1, pVal, length, NULL, NULL);
	return 0;
}

int CheckHexInput(TCHAR *pstring)
{
	char szString[64];

	if (0 != TChar2Char(pstring, szString, sizeof(szString)))
	{
		return -1;
	}

	int i = 0;
	for (i = 0; i < sizeof(szString); i++)
	{
		if (0 == szString[i])
		{
			break;
		}

		if ('0' <= szString[i] && '9' >= szString[i])
		{
			continue;
		}
		if ('a' <= szString[i] && 'f' >= szString[i])
		{
			continue;
		}
		if ('A' <= szString[i] && 'F' >= szString[i])
		{
			continue;
		}

		return -1;
	}

	return 0;
}

TCHAR* SshGetErrString(int errCode)
{
	int  max = sizeof(g_stSshErrInfo) / sizeof(SSH_ERR_INFO_S);

	for (int i = 0; i < max; i++)
	{
		if (errCode == g_stSshErrInfo[i].err_code)
		{
			return g_stSshErrInfo[i].pErrInfo;
		}
	}

	return NULL;
}

int g_IpcSerialNo = 0;

/*****************************************************************************
 函 数 名  : Bru_IPCSend_Inner
 功能描述  : 发送消息给BRU，并获取响应
 输入参数  : void *pReqMsg
 			 int ReqLen
 			 int ReplyLen
 输出参数  : void *pReplyMsg    既是输入又是输出
 返 回 值  : int
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2015年11月26日
    作    者   : LinJing
    修改内容   : 新生成函数

*****************************************************************************/
int Bru_IPCSend_Inner( void *pReqMsg, int ReqLen, void *pReplyMsg, int ReplyLen )
{
	int  ret = 0;
	int  retlen;
	int  sockfd;
	int  SerialNo;
	unsigned int  tick_start = 0;
	unsigned int  tick_end = 0;
	unsigned int  tick_cost = 0;
	char szBuffer[RU_IPC_MSG_MAXLEN];
	struct sockaddr_in server_addr;
	int  addrlen = sizeof(struct sockaddr_in);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
	if (sockfd < 0) 
	{ 
		return -1; 
	}

	memset(&server_addr, 0, sizeof(struct sockaddr_in)); 
	server_addr.sin_family = AF_INET; 
	server_addr.sin_port = htons(RU_IPC_UDP_PORT);
	inet_pton(AF_INET, g_GlbConfig.stSshCfg.ipaddr, &server_addr.sin_addr.s_addr);
	//server_addr.sin_addr.s_addr = inet_addr(g_GlbConfig.stSshCfg.ipaddr);

	*(int *)szBuffer = htonl(++g_IpcSerialNo);
	memcpy(szBuffer + sizeof(int), pReqMsg, ReqLen);
	ret = sendto(sockfd, szBuffer, ReqLen + sizeof(int), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    if (ret < 0)
    {
		closesocket(sockfd); 
        return -1;
    }

	unsigned long nonblock = 1;
	ioctlsocket(sockfd, FIONBIO, &nonblock);

	fd_set fds;
	struct timeval tv;
	tv.tv_sec =  15;
	tv.tv_usec = 0;
	memset(szBuffer, 0, sizeof(szBuffer));
	tick_start = GetTickCount();
	while (1)
	{
		FD_ZERO(&fds);
		FD_SET(sockfd, &fds);
		ret = select(sockfd + 1, &fds, NULL, NULL, &tv);
		if (ret <= 0) 
		{
			ret = -1;
			break;
		}

		if (FD_ISSET(sockfd, &fds))
		{
			retlen = recvfrom(sockfd, (char *)szBuffer, sizeof(szBuffer), 0, (struct sockaddr *)&server_addr, &addrlen);
		    if (retlen < 0)
		    {
				continue;
		    }

			SerialNo = *(int *)szBuffer;
			SerialNo = htonl(SerialNo);
			if (SerialNo == g_IpcSerialNo)
			{
				memcpy(pReplyMsg, szBuffer + sizeof(int), ReplyLen);
				ret = 0;
				break;
			}
		}

		//累计时间
		tick_end = GetTickCount();
		if (tick_end > tick_start)
		{
			tick_cost = tick_end - tick_start;
		}
		else
		{
			tick_cost = 0xFFFFFFFF - tick_start + tick_end;
		}

		if (tick_cost > 1000 * 15)
		{
			ret = -1;
			break;
		}
	}

	closesocket(sockfd);
	return ret;
}

/*****************************************************************************
 函 数 名  : Bru_IPCSend
 功能描述  : 发送消息给BRU，并获取响应
 输入参数  : void *pReqMsg
 			 int ReqLen
 			 int ReplyLen
 输出参数  : void *pReplyMsg    既是输入又是输出
 返 回 值  : int
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2015年11月26日
    作    者   : LinJing
    修改内容   : 新生成函数

*****************************************************************************/
int Bru_IPCSend( void *pReqMsg, int ReqLen, void *pReplyMsg, int ReplyLen )
{
	int ret = 0;
	int Cnt = 0;

	while (0 != (ret = Bru_IPCSend_Inner(pReqMsg, ReqLen, pReplyMsg, ReplyLen)))
	{
		Cnt++;
		if (Cnt >= 3)
		{
			return -1;
		}

		Sleep(300);
	}

	return ret;
}

int Read_Agent2Buffer()
{
	int   len = 0;
	FILE *fp = NULL;

	if (0 != fopen_s(&fp, "emWriteAgent", "rb"))
	{
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	rewind(fp);
	
	if (len != fread(g_szAgentCode, 1, len, fp))
	{
		fclose(fp);
		return -1;
	}

	g_ulAgentLen = len;
	fclose(fp);
	return 0;
}

DWORD WINAPI Tftp_ServerThread(LPVOID lpParameter)
{
	SOCKET sock = g_tftpsock;
	SOCKET sockdata;
	unsigned short lastblock = -1;
	
	char szBuffer[1024];

	//创建套接字
	sockdata = socket(AF_INET, SOCK_DGRAM, 0);
	if (INVALID_SOCKET == sockdata)
	{
		closesocket(sock);
		WSACleanup();
		return -1;
	}

	while (1)
	{
		SOCKADDR_IN		clientAddr;
		int             clientlen = sizeof(clientAddr);
		if (recvfrom(sock, szBuffer, sizeof(szBuffer), 0, (struct sockaddr *)&clientAddr, &clientlen) == SOCKET_ERROR)
		{
			break;
		}

		TftpData_S *pPkt = (TftpData_S *)szBuffer;
		switch (ntohs(pPkt->opcode))
		{
			case TFTP_RRQ:
				g_ulAgentOffset = 0;
				g_ulBlockNo = 1;
				lastblock = -1;
				break;
			case TFTP_ACK:
				g_ulBlockNo = ntohs(pPkt->block) + 1;
				g_ulAgentOffset += 512;
				break;
			default:
				break;
		}

		if (lastblock + 1 == g_ulBlockNo && TFTP_ACK == ntohs(pPkt->opcode))
		{
			continue;
		}
		
		//send data
		pPkt->opcode = htons(TFTP_DATA);
		pPkt->block = htons(g_ulBlockNo);

		int len = 0;
		if (g_ulAgentOffset >= g_ulAgentLen) //最后一帧重传
		{
			g_ulAgentOffset -= 512;
			len = g_ulAgentLen - g_ulAgentOffset;
		}
		else if (g_ulAgentOffset + 512 >= g_ulAgentLen)
		{
			len = g_ulAgentLen - g_ulAgentOffset;
			lastblock = g_ulBlockNo;
		}
		else
		{
			len = 512;
		}
	
		memcpy(pPkt+1, &g_szAgentCode[g_ulAgentOffset], len);

	    if (sendto(sock, szBuffer, len + sizeof(TftpData_S), 0, (struct sockaddr *)&clientAddr, sizeof(struct sockaddr_in)) < 0)
	    {
			break;
	    }
	}

	closesocket(sockdata);
	closesocket(sock);
	WSACleanup();
	return 0;
}

int Tftp_ServerInit()
{
	WSADATA			wsd;
	SOCKET			sock;
	SOCKADDR_IN		servAddr;

	//初始化套结字动态库
	if (WSAStartup(MAKEWORD(2,2), &wsd) != 0)
	{
		return -1;
	}
	
	//创建套接字
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (INVALID_SOCKET == sock)
	{
		WSACleanup();
		return -1;
	}

	//服务器地址
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(TFTP_UDP_PORT);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sock, (SOCKADDR *)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return -1;
	}

	g_tftpsock = sock;
	if (false == CreateThread(NULL, 0, Tftp_ServerThread, NULL, 0, NULL))
	{
		closesocket(sock);
		WSACleanup();
		return -1;
	}
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////

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


// CemWriteDlg dialog



CemWriteDlg::CemWriteDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CemWriteDlg::IDD, pParent)
	, m_freqOffset(_T("0"))
	//, m_Mac(_T("68BF74000061"))
	, m_Mac(_T(""))
	, m_GainRx0(_T("0"))
	, m_GainRx1(_T("0"))
	//, m_SN(_T("901156769012158V020"))
	, m_SN(_T(""))
	, m_GainTx0(_T("0"))
	, m_GainTx1(_T("0"))
	, m_ReadInfo(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CemWriteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_BTYPE, m_hwInfoCtrl);
	DDX_Text(pDX, IDC_EDIT_FREQ_OFFSET, m_freqOffset);
	DDV_MaxChars(pDX, m_freqOffset, 8);
	DDX_Text(pDX, IDC_EDIT_MAC, m_Mac);
	DDV_MaxChars(pDX, m_Mac, MAX_MAC_LEN);
	DDX_Text(pDX, IDC_EDIT_RX0, m_GainRx0);
	DDV_MaxChars(pDX, m_GainRx0, 8);
	DDX_Text(pDX, IDC_EDIT_RX1, m_GainRx1);
	DDV_MaxChars(pDX, m_GainRx1, 8);
	DDX_Text(pDX, IDC_EDIT_SN, m_SN);
	DDV_MaxChars(pDX, m_SN, MAX_SN_LEN);
	DDX_Text(pDX, IDC_EDIT_TX0, m_GainTx0);
	DDV_MaxChars(pDX, m_GainTx0, 8);
	DDX_Text(pDX, IDC_EDIT_TX1, m_GainTx1);
	DDV_MaxChars(pDX, m_GainTx1, 8);
	DDX_Text(pDX, IDC_EDIT1, m_ReadInfo);
}

BEGIN_MESSAGE_MAP(CemWriteDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_WRITE, &CemWriteDlg::OnBnClickedButtonWrite)
	ON_BN_CLICKED(IDC_BUTTON_READ, &CemWriteDlg::OnBnClickedButtonRead)
END_MESSAGE_MAP()


// CemWriteDlg message handlers

BOOL CemWriteDlg::OnInitDialog()
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
	ReadConfig();

	m_hwInfoCtrl.SetCurSel(0);

	if (0 != Read_Agent2Buffer())
	{
		MessageBox(_T("请检查当前目录是否存在emWriteAgent文件"), _T("提示"), MB_ICONERROR | MB_OK);
	}

	if (0 != Tftp_ServerInit())
	{
		MessageBox(_T("启动tftp服务出错"), _T("提示"), MB_ICONERROR | MB_OK);
	}
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CemWriteDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CemWriteDlg::OnPaint()
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

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CemWriteDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CemWriteDlg::OnBnClickedButtonWrite()
{
	int ret = 0;

	// TODO: Add your control notification handler code here
	UpdateData(true);

	if (0 != CheckInput())
	{
		return;
	}

	char szReply[1024] = { 0 };
	int ssh_session = -1;
	ret = ssh_sessioninit(g_GlbConfig.stSshCfg.ipaddr, g_GlbConfig.stSshCfg.user, g_GlbConfig.stSshCfg.passwd, &ssh_session);
	if (0 != ret)
	{
		MessageBox(SshGetErrString(ret), _T("提示"), MB_ICONERROR | MB_OK);
		return;
	}
	
	#if  1
	ret |= ssh_executecmd(ssh_session, "killall emWriteAgent", szReply, sizeof(szReply));
	Sleep(100);
	ret |= ssh_executecmd(ssh_session, "rm -rf /tmp/emWriteAgent", szReply, sizeof(szReply));
	Sleep(100);
	char szcmd[512];
	sprintf_s(szcmd, sizeof(szcmd), "tftp -gr emWriteAgent -l /tmp/emWriteAgent %s 8899", g_GlbConfig.stSshCfg.localipaddr);
	ret |= ssh_executecmd(ssh_session, szcmd, szReply, sizeof(szReply));
	Sleep(1000);
	ret |= ssh_executecmd(ssh_session, "chmod +x /tmp/emWriteAgent", szReply, sizeof(szReply));
	Sleep(100);
	ret |= ssh_executecmd(ssh_session, "/tmp/emWriteAgent", szReply, sizeof(szReply));
	Sleep(100);
	if (0 != ret)
	{
		MessageBox(_T("启动代理进程失败"), _T("提示"), MB_ICONERROR | MB_OK);
		goto Quit;
	}
	#endif /* #if 0 */

	RUSaveAllReq_S  stReq;
	RUSetAck_S      stResp;

	/*消息填充一定要进行字节序转换，切记  */
	memset(&stReq, 0xff, sizeof(RUSaveAllReq_S));
	stReq.stAck.tlvHdr.Type = htons(WAGENT_WR_ALL);
	stReq.stAck.tlvHdr.Length = htons(RUIPC_PAYLOAD_LEN(RUSaveAllReq_S));
	stReq.stAck.errCode = 0;
	
	//mac
	unsigned char szMac[12] = {0};
	if (6 != swscanf_s(m_Mac.GetString(), _T("%02x%02x%02x%02x%02x%02x"), &szMac[0], &szMac[1], &szMac[2], &szMac[3], &szMac[4], &szMac[5]))
	{
		MessageBox(_T("输入的MAC非法"), _T("提示"), MB_ICONERROR | MB_OK);
		goto Quit;
	}
	memcpy(stReq.szMac, szMac, sizeof(stReq.szMac));

	//tx gain / freq offset
	if (1 != swscanf_s(m_GainTx0.GetString(), _T("%x"), &stReq.TxGain0))
	{
		MessageBox(_T("输入的通道0发射增益非法"), _T("提示"), MB_ICONERROR | MB_OK);
		goto Quit;
	}
	stReq.TxGain0 = stReq.TxGain0;

	if (1 != swscanf_s(m_GainTx1.GetString(), _T("%x"), &stReq.TxGain1))
	{
		MessageBox(_T("输入的通道1发射增益非法"), _T("提示"), MB_ICONERROR | MB_OK);
		goto Quit;
	}
	stReq.TxGain1 = stReq.TxGain1;

	if (1 != swscanf_s(m_freqOffset.GetString(), _T("%x"), &stReq.freqOffset))
	{
		MessageBox(_T("输入的频偏非法"), _T("提示"), MB_ICONERROR | MB_OK);
		goto Quit;
	}
	stReq.freqOffset = stReq.freqOffset;

	//sn
	char szBuffer[512] = {0};
	ret = TChar2Char((TCHAR *)m_SN.GetString(), szBuffer, sizeof(szBuffer));
	if (0 != ret)
	{
		MessageBox(_T("barcode处理失败"), _T("提示"), MB_ICONERROR | MB_OK);
		goto Quit;
	}
	memcpy(stReq.szSn, szBuffer, sizeof(stReq.szSn));

	//hw ver
	ret = TChar2Char(g_GlbConfig.stHwInfoCfg.szItem[m_hwInfoCtrl.GetCurSel()], szBuffer, sizeof(szBuffer));
	if (0 != ret)
	{
		MessageBox(_T("BType处理失败"), _T("提示"), MB_ICONERROR | MB_OK);
		goto Quit;
	}
	memcpy(stReq.hwVer, szBuffer, sizeof(stReq.hwVer));

	//gps / boot mode / upgrade info
	stReq.gpsinfo = 0xffffffff;
	stReq.bootmode = 0xffffffff;
	stReq.upgradeinfo = 0xffffffff;

	//rx gain
	if (1 != swscanf_s(m_GainRx0.GetString(), _T("%x"), &stReq.RxGain0))
	{
		MessageBox(_T("输入的通道0接收增益非法"), _T("提示"), MB_ICONERROR | MB_OK);
		goto Quit;
	}
	stReq.RxGain0 = stReq.RxGain0;
	
	if (1 != swscanf_s(m_GainRx1.GetString(), _T("%x"), &stReq.RxGain1))
	{
		MessageBox(_T("输入的通道1接收增益非法"), _T("提示"), MB_ICONERROR | MB_OK);
		goto Quit;
	}
	stReq.RxGain1 = stReq.RxGain1;

	ret = Bru_IPCSend(&stReq, sizeof(stReq), &stResp, sizeof(stResp));
	if (0 != ret)
	{
		MessageBox(_T("保存失败"), _T("提示"), MB_ICONERROR | MB_OK);
		goto Quit;
	}
	else
	{
		if (0 != ntohs(stResp.errCode))
		{
			MessageBox(_T("保存失败"), _T("提示"), MB_ICONERROR | MB_OK);
			goto Quit;
		}
	}

	MessageBox(_T("保存成功"), _T("提示"), MB_ICONINFORMATION | MB_OK);

Quit:	
	ssh_sessiondeinit(ssh_session);

	return;
}

int CemWriteDlg::ReadConfig()
{
	TCHAR szCfgDir[512];
	TCHAR szCfgPath[1024] = {0};
	::GetCurrentDirectory(sizeof(szCfgDir), szCfgDir);
	wsprintf(szCfgPath, _T("%s\\emWrite.ini"), szCfgDir);

	FILE *fp = NULL;
	if (0 != _wfopen_s(&fp, szCfgPath, _T("r")))
	{
		MessageBox(_T("配置文件(emWrite.ini)不存在"), _T("提示"), MB_ICONERROR | MB_OK);
		return -1;
	}
	else
	{
		fclose(fp);
	}
	
	//read hwinfo config
	for (int i = 0; i < MAX_CFG_BTYPE_NUM; i++)
	{
		TCHAR szKey[128];
		wsprintf(szKey, _T("Item_%04d"), i);

		if (GetPrivateProfileString(_T("Hardware Info"), szKey, _T(""), g_GlbConfig.stHwInfoCfg.szItem[i], sizeof(g_GlbConfig.stHwInfoCfg.szItem[i]), szCfgPath) <= 0)
		{
			break;
		}

		m_hwInfoCtrl.AddString(g_GlbConfig.stHwInfoCfg.szItem[i]);
	}

	//read ssh config
	int ret  = 0;
	ret |= GetPrivateProfileStringToChar(_T("SSH Config"), _T("bru_ipaddr"), _T(""), g_GlbConfig.stSshCfg.ipaddr, sizeof(g_GlbConfig.stSshCfg.ipaddr), szCfgPath);
	ret |= GetPrivateProfileStringToChar(_T("SSH Config"), _T("username"), _T(""), g_GlbConfig.stSshCfg.user, sizeof(g_GlbConfig.stSshCfg.user), szCfgPath);
	ret |= GetPrivateProfileStringToChar(_T("SSH Config"), _T("password"), _T(""), g_GlbConfig.stSshCfg.passwd, sizeof(g_GlbConfig.stSshCfg.passwd), szCfgPath);
	ret |= GetPrivateProfileStringToChar(_T("SSH Config"), _T("pc_ipaddr"), _T(""), g_GlbConfig.stSshCfg.localipaddr, sizeof(g_GlbConfig.stSshCfg.localipaddr), szCfgPath);
	if (0 != ret)
	{
		MessageBox(_T("ssh配置非法"), _T("提示"), MB_ICONERROR | MB_OK);
		return -1;
	}
	
	return 0;
}

int CemWriteDlg::CheckInput()
{
	//检查MAC
	unsigned char szMac[6] = { 0 };
	unsigned char szTmp[12] = {0};
	if (6 != swscanf_s(m_Mac.GetString(), _T("%02x%02x%02x%02x%02x%02x"), &szTmp[0], &szTmp[1], &szTmp[2], &szTmp[3], &szTmp[4], &szTmp[5]))
	{
		MessageBox(_T("输入的MAC非法"), _T("提示"), MB_ICONERROR | MB_OK);
		return -1;
	}
	memcpy(szMac, szTmp, sizeof(szMac));

	//检查SN，必须是MAX_SN_LEN字节
	if (MAX_SN_LEN != m_SN.GetLength())
	{
		MessageBox(_T("输入的SN非法，SN是19字符的编码"), _T("提示"), MB_ICONERROR | MB_OK);
		return -1;
	}

	if (0 != CheckHexInput((TCHAR *)m_GainTx0.GetString()))
	{
		MessageBox(_T("通道0发射增益非法，只允许16进制数"), _T("提示"), MB_ICONERROR | MB_OK);
		return -1;
	}
	if (0 != CheckHexInput((TCHAR *)m_GainTx1.GetString()))
	{
		MessageBox(_T("通道1发射增益非法，只允许16进制数"), _T("提示"), MB_ICONERROR | MB_OK);
		return -1;
	}
	if (0 != CheckHexInput((TCHAR *)m_GainRx0.GetString()))
	{
		MessageBox(_T("通道0接收增益非法，只允许16进制数"), _T("提示"), MB_ICONERROR | MB_OK);
		return -1;
	}
	if (0 != CheckHexInput((TCHAR *)m_GainRx1.GetString()))
	{
		MessageBox(_T("通道1接收增益非法，只允许16进制数"), _T("提示"), MB_ICONERROR | MB_OK);
		return -1;
	}
	if (0 != CheckHexInput((TCHAR *)m_freqOffset.GetString()))
	{
		MessageBox(_T("频偏非法，只允许16进制数"), _T("提示"), MB_ICONERROR | MB_OK);
		return -1;
	}

	return 0;
}



void CemWriteDlg::OnBnClickedButtonRead()
{
	// TODO: Add your control notification handler code here
	RUTlvHdr_S 		stReq;
	RUSaveAllReq_S  stResp;
	CString strTmp = _T("");

	char szReply[1024] = { 0 };
	int ssh_session = -1;
	int ret = ssh_sessioninit(g_GlbConfig.stSshCfg.ipaddr, g_GlbConfig.stSshCfg.user, g_GlbConfig.stSshCfg.passwd, &ssh_session);
	if (0 != ret)
	{
		MessageBox(SshGetErrString(ret), _T("提示"), MB_ICONERROR | MB_OK);
		return;
	}
	
	#if  1
	ret |= ssh_executecmd(ssh_session, "killall emWriteAgent", szReply, sizeof(szReply));
	Sleep(100);
	ret |= ssh_executecmd(ssh_session, "rm -rf /tmp/emWriteAgent", szReply, sizeof(szReply));
	Sleep(100);
	char szcmd[512];
	sprintf_s(szcmd, sizeof(szcmd), "tftp -gr emWriteAgent -l /tmp/emWriteAgent %s 8899", g_GlbConfig.stSshCfg.localipaddr);
	ret |= ssh_executecmd(ssh_session, szcmd, szReply, sizeof(szReply));
	Sleep(1000);
	ret |= ssh_executecmd(ssh_session, "chmod +x /tmp/emWriteAgent", szReply, sizeof(szReply));
	Sleep(100);
	ret |= ssh_executecmd(ssh_session, "/tmp/emWriteAgent", szReply, sizeof(szReply));
	Sleep(100);
	if (0 != ret)
	{
		MessageBox(_T("启动代理进程失败"), _T("提示"), MB_ICONERROR | MB_OK);
		goto Quit;
	}
	#endif /* #if 0 */
	
	/*消息填充一定要进行字节序转换，切记  */
	memset(&stReq, 0xff, sizeof(RUTlvHdr_S));
	stReq.Type = htons(WAGENT_RD_ALL);
	stReq.Length = 0;

	ret = Bru_IPCSend(&stReq, sizeof(stReq), &stResp, sizeof(stResp));
	if (0 != ret)
	{
		MessageBox(_T("查询失败"), _T("提示"), MB_ICONERROR | MB_OK);
		goto Quit;
	}
	else
	{
		if (0 != ntohs(stResp.stAck.errCode))
		{
			MessageBox(_T("查询失败"), _T("提示"), MB_ICONERROR | MB_OK);
			goto Quit;
		}
	}

	m_ReadInfo = _T("");

	strTmp.Format(_T("Mac=%02x%02x-%02x%02x-%02x%02x\r\n"), stResp.szMac[0],
														stResp.szMac[1],
														stResp.szMac[2],
														stResp.szMac[3],
														stResp.szMac[4],
														stResp.szMac[5]);
	m_ReadInfo += strTmp;

	TCHAR szBuffer[1024];
	Char2Tchar((char *)stResp.szSn, szBuffer, sizeof(szBuffer));
	strTmp.Format(_T("SN=%s\r\n"), szBuffer);
	m_ReadInfo += strTmp;

	strTmp.Format(_T("freqOffset=0x%x\r\n"), stResp.freqOffset);
	m_ReadInfo += strTmp;
	strTmp.Format(_T("TxGain0=0x%x\r\n"), stResp.TxGain0);
	m_ReadInfo += strTmp;
	strTmp.Format(_T("RxGain0=0x%x\r\n"), stResp.RxGain0);
	m_ReadInfo += strTmp;
	strTmp.Format(_T("TxGain1=0x%x\r\n"), stResp.TxGain1);	
	m_ReadInfo += strTmp;
	strTmp.Format(_T("RxGain1=0x%x\r\n"), stResp.RxGain1);
	m_ReadInfo += strTmp;

	Char2Tchar((char *)stResp.hwVer, szBuffer, sizeof(szBuffer));
	strTmp.Format(_T("hwVer=%s\r\n"), szBuffer);
	m_ReadInfo += strTmp;

	UpdateData(false);

Quit:	
	ssh_sessiondeinit(ssh_session);
	
}
