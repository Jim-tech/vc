
// BatchItDlg.cpp : implementation file
//

#include "stdafx.h"
#include "BatchIt.h"
#include "BatchItDlg.h"
#include "afxdialogex.h"
#include "Ws2tcpip.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma warning (disable: 4146)
#import "c://Program Files (x86)//Common Files//system//ado//msadox.dll"
#import "c://Program Files (x86)//Common Files//system//ado//msado15.dll" no_namespace rename ("EOF", "adoEOF")
#pragma warning (default: 4146)

#define dbgprint(...) \
    do\
	    {\
		SYSTEMTIME __sys;\
		GetLocalTime(&__sys);\
		printf("[%04d-%02d-%02d %02d:%02d:%02d][%s][%d][0x%08x]", __sys.wYear, __sys.wMonth, __sys.wDay, \
        					__sys.wHour, __sys.wMinute, __sys.wSecond, __FUNCTION__, __LINE__, GetCurrentThreadId());\
        printf(__VA_ARGS__);\
        printf("\n");\
        fflush(stdout);\
        fflush(stderr);\
	     }while(0)


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


// CBatchItDlg dialog

typedef enum
{
	E_ITEM_NO,
	E_ITEM_IP,
	E_ITEM_SN,
	E_ITEM_GPSVER,
	E_ITEM_STATUS,
	E_ITEM_TIMECOSTS,
};

typedef enum
{
	E_STATE_NULL,
	E_STATE_OK,
	E_STATE_FAIL,
};

typedef enum
{
	e_ssh_info = 0,
	e_ssh_quit_ok = 1,
	e_ssh_quit_fail = 2,
}upgrade_msg_e;

#define      UPDATE_UI_TIMER  1
#define      MAX_SECONDS      600

#pragma pack(1)

typedef struct
{
	int 			msgtype;
	int             ipaddr;
	char            snstr[32];
	char            verstr[128];
	char            process[256];
}upgrade_msg_s;

#pragma pack()

typedef struct iplist
{
	DWORD               ipaddr;
	char                szuser[128];
	char                szpasswd[128];
	char                szsn[32];
	char                szversion[128];
	int                 used;
	int                 state;
	int                 running;
	int                 run_seconds;
	HANDLE              hProcess;
}IPList_S;

IPList_S		g_devlist[2048];
CBatchItDlg    *g_pstDlgPtr = NULL;

CBatchItDlg::CBatchItDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CBatchItDlg::IDD, pParent)
	, m_listFile(_T(""))
	, m_gpsFirmware(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CBatchItDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DEV, m_devList);
	DDX_Text(pDX, IDC_EDIT_LISTFILE, m_listFile);
	DDX_Text(pDX, IDC_EDIT_GPSFW, m_gpsFirmware);
}

BEGIN_MESSAGE_MAP(CBatchItDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(ID_MENU_ABOUT, &CBatchItDlg::OnMenuAbout)
	ON_COMMAND(ID_MENU_EXIT, &CBatchItDlg::OnMenuExit)
	ON_WM_SIZE()
	ON_COMMAND(ID_MENU_IMPDEV, &CBatchItDlg::OnMenuImpdev)
	ON_COMMAND(ID_MENU_SELALL, &CBatchItDlg::OnMenuSelall)
	ON_COMMAND(ID_OPERATION_DESELECTALL, &CBatchItDlg::OnOperationDeselectall)
	ON_COMMAND(ID_OPERATION_SELECTFAILED, &CBatchItDlg::OnOperationSelectfailed)
	ON_BN_CLICKED(IDC_BUTTON_START, &CBatchItDlg::OnBnClickedButtonStart)
	ON_WM_TIMER()
	ON_COMMAND(ID_OPERATION_SELECTFIRMWARE, &CBatchItDlg::OnOperationSelectfirmware)
	ON_COMMAND(ID_CONTROL_STOP, &CBatchItDlg::OnControlStop)
	ON_COMMAND(ID_CONTROL_START, &CBatchItDlg::OnControlStart)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_DEV, &CBatchItDlg::OnRclickListDev)
END_MESSAGE_MAP()

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
	int len = MultiByteToWideChar(CP_ACP, 0, pIn, strlen(pIn) + 1, NULL, 0);
	if (len >= maxlen)
	{
		len = maxlen;
	}

	MultiByteToWideChar(CP_ACP, 0, pIn, strlen(pIn) + 1, pOut, len);
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
	tv.tv_sec = 0;
	tv.tv_usec = 300000;
	while (1)
	{
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

			for (int i = 0; i < sizeof(g_devlist) / sizeof(IPList_S); i++)
			{
				if (!g_devlist[i].running)
				{
					continue;
				}

				if (stmsg.ipaddr == g_devlist[i].ipaddr)
				{
					sprintf_s(g_devlist[i].szsn, sizeof(g_devlist[i].szsn), "%s", stmsg.snstr);
					sprintf_s(g_devlist[i].szversion, sizeof(g_devlist[i].szversion), "%s", stmsg.verstr);

					TCHAR  tsz_buffer[512];
					utils_Char2Tchar(g_devlist[i].szsn, tsz_buffer, sizeof(tsz_buffer));
					g_pstDlgPtr->m_devList.SetItemText(i, E_ITEM_SN, tsz_buffer);

					utils_Char2Tchar(g_devlist[i].szversion, tsz_buffer, sizeof(tsz_buffer));
					g_pstDlgPtr->m_devList.SetItemText(i, E_ITEM_GPSVER, tsz_buffer);

					utils_Char2Tchar(stmsg.process, tsz_buffer, sizeof(tsz_buffer));
					g_pstDlgPtr->m_devList.SetItemText(i, E_ITEM_STATUS, tsz_buffer);

					wsprintf(tsz_buffer, _T("%d"), g_devlist[i].run_seconds);
					g_pstDlgPtr->m_devList.SetItemText(i, E_ITEM_TIMECOSTS, tsz_buffer);

					switch (stmsg.msgtype)
					{
					case e_ssh_quit_ok:
						g_devlist[i].run_seconds = 0;
						g_devlist[i].state = E_STATE_OK;
						g_devlist[i].running = false;
						g_devlist[i].hProcess = NULL;
						g_pstDlgPtr->m_devList.SetItemColor(i, RGB(0, 0, 0), RGB(0, 0xff, 0));
						break;
					case e_ssh_quit_fail:
						g_devlist[i].run_seconds = 0;
						g_devlist[i].state = E_STATE_FAIL;
						g_devlist[i].running = false;
						g_devlist[i].hProcess = NULL;
						g_pstDlgPtr->m_devList.SetItemColor(i, RGB(0, 0, 0), RGB(0xFF, 0x0, 0));
						break;
					default:
						g_pstDlgPtr->m_devList.SetItemColor(i, RGB(0, 0, 0), RGB(0xFF, 0xFF, 0xFF));
						break;
					}
				}
			}
		}
	}

	closesocket(sock);
	dbgprint("process thread quit");
	return 0;
}

// CBatchItDlg message handlers

BOOL CBatchItDlg::OnInitDialog()
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
	//设置测试项列表控件
	LONG lStyle;
	lStyle = GetWindowLong(m_devList.m_hWnd, GWL_STYLE);	//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK;							    //清除显示方式位
	lStyle |= LVS_REPORT;									//设置style
	SetWindowLong(m_devList.m_hWnd, GWL_STYLE, lStyle);		//设置style

	DWORD dwStyle = m_devList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;						//选中某行使整行高亮（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_GRIDLINES;							//网格线（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_CHECKBOXES;							//item前生成checkbox控件
	m_devList.SetExtendedStyle(dwStyle);					//设置扩展风格

	m_devList.InsertColumn(E_ITEM_NO, _T("ID"), LVCFMT_LEFT, 50);//插入列
	m_devList.InsertColumn(E_ITEM_IP, _T("IP"), LVCFMT_LEFT, 90);
	m_devList.InsertColumn(E_ITEM_SN, _T("SN"), LVCFMT_LEFT, 160);
	m_devList.InsertColumn(E_ITEM_GPSVER, _T("GPS Firmware"), LVCFMT_LEFT, 100);
	m_devList.InsertColumn(E_ITEM_STATUS, _T("Status"), LVCFMT_LEFT, 400);
	m_devList.InsertColumn(E_ITEM_TIMECOSTS, _T("Seconds"), LVCFMT_LEFT, 100);

	memset(g_devlist, 0, sizeof(g_devlist));

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);

	CMenu *pdlgMenu = GetMenu();
	pdlgMenu->EnableMenuItem(ID_MENU_SELALL, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	pdlgMenu->EnableMenuItem(ID_OPERATION_SELECTFAILED, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	pdlgMenu->EnableMenuItem(ID_OPERATION_DESELECTALL, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

	m_pProcessThread = CreateThread(NULL, 0, thread_process, NULL, 0, NULL);
	if (NULL == m_pProcessThread)
	{
		MessageBox(_T("start monitor thread fail"));
	}

	SetTimer(UPDATE_UI_TIMER, 1000, 0);

	g_pstDlgPtr = this;

	CreateDirectory(_T("Log"), 0);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CBatchItDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CBatchItDlg::OnPaint()
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
HCURSOR CBatchItDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CBatchItDlg::OnMenuAbout()
{
	// TODO: Add your command handler code here
	CAboutDlg dlg;
	dlg.DoModal();
}


void CBatchItDlg::OnMenuExit()
{
	// TODO: Add your command handler code here
	CDialogEx::OnOK();
}


void CBatchItDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
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

	for (int i = 0; i < m_devList.GetHeaderCtrl()->GetItemCount(); i++)
	{
		int width = m_devList.GetColumnWidth(i);
		width = long(width *fsp[0]);
		m_devList.SetColumnWidth(i, width);
	}
}


void CBatchItDlg::OnMenuImpdev()
{
	// TODO: Add your command handler code here
	CFileDialog  dlg(true, _T(".xlsx"), NULL, OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, _T("device list(*.xlsx)|*.xlsx|All Files(*.*)|*.*||"));
	if (dlg.DoModal() == IDOK)
	{
		CString listFilePath = dlg.GetPathName();

		_ConnectionPtr pCon = NULL;
		HRESULT hr = pCon.CreateInstance(__uuidof(Connection));
		if (FAILED(hr))
		{
			MessageBox(_T("打开设备列表文件失败"));
			return;
		}
		
		TCHAR szcmd[2048];
		wsprintf(szcmd, _T("Provider=Microsoft.ACE.OLEDB.12.0;Data Source=%s;Extended Properties=Excel 12.0 Xml"), listFilePath.GetString());
		hr = pCon->Open(szcmd, "", "", adConnectUnspecified);
		if (FAILED(hr))
		{
			MessageBox(_T("打开设备列表文件失败"));

			pCon.Release();
			return;
		}

		m_devList.DeleteAllItems();

		_RecordsetPtr pRecordset;
		pRecordset.CreateInstance(__uuidof(Recordset));

		pRecordset->Open("SELECT * FROM [DevList$]", pCon.GetInterfacePtr(), adOpenStatic, adLockOptimistic, adCmdText);
		while (!pRecordset->adoEOF)
		{
			CString val;

			_variant_t ip = pRecordset->GetCollect(_variant_t((long)0));
			_variant_t user = pRecordset->GetCollect(_variant_t((long)1));
			_variant_t passwd = pRecordset->GetCollect(_variant_t((long)2));

			val = ip.bstrVal;
			if (0 == val.GetLength())
			{
				break;
			}

			int index = m_devList.GetItemCount();
			
			val.Format(_T("%d"), index + 1);
			m_devList.InsertItem(index, val);
			m_devList.SetItemText(index, E_ITEM_IP, ip.bstrVal);
			m_devList.SetItemColor(index, RGB(0, 0, 0), RGB(0xFF, 0xFF, 0xFF));
			m_devList.SetCheck(index, true);

			for (int i = 0; i < sizeof(g_devlist) / sizeof(IPList_S); i++)
			{
				if (false == g_devlist[i].used)
				{
					struct in_addr sip;
					char ipaddrstr[32];
					utils_TChar2Char(ip.bstrVal, ipaddrstr, sizeof(ipaddrstr));
					inet_pton(AF_INET, ipaddrstr, (void *)&sip);
					g_devlist[i].ipaddr = ntohl(sip.s_addr);

					utils_TChar2Char(user.bstrVal, g_devlist[i].szuser, sizeof(g_devlist[i].szuser));
					utils_TChar2Char(passwd.bstrVal, g_devlist[i].szpasswd, sizeof(g_devlist[i].szpasswd));

					g_devlist[i].state = E_STATE_NULL;
					g_devlist[i].running = false;
					g_devlist[i].used = true;

					m_devList.SetItemData(index, g_devlist[i].ipaddr);
					break;
				}
			}
			pRecordset->MoveNext();
		}

		pRecordset->Close();
		pRecordset.Release();

		pCon->Close();
		pCon.Release();

		m_listFile = listFilePath;
		UpdateData(false);

		GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);

		CMenu *pdlgMenu = GetMenu();
		pdlgMenu->EnableMenuItem(ID_MENU_SELALL, MF_BYCOMMAND | MF_ENABLED);
		pdlgMenu->EnableMenuItem(ID_OPERATION_SELECTFAILED, MF_BYCOMMAND | MF_ENABLED);
		pdlgMenu->EnableMenuItem(ID_OPERATION_DESELECTALL, MF_BYCOMMAND | MF_ENABLED);
	}
}


void CBatchItDlg::OnMenuSelall()
{
	// TODO: Add your command handler code here
	for (int i = 0; i < m_devList.GetItemCount(); i++)
	{
		m_devList.SetCheck(i, true);
	}
}


void CBatchItDlg::OnOperationDeselectall()
{
	// TODO: Add your command handler code here
	for (int i = 0; i < m_devList.GetItemCount(); i++)
	{
		m_devList.SetCheck(i, false);
	}
}


void CBatchItDlg::OnOperationSelectfailed()
{
	// TODO: Add your command handler code here
	for (int i = 0; i < m_devList.GetItemCount(); i++)
	{
		if (E_STATE_FAIL == g_devlist[i].state)
		{
			m_devList.SetCheck(i, true);
		}
		else
		{
			m_devList.SetCheck(i, false);
		}
	}
}

void CBatchItDlg::OnBnClickedButtonStart()
{
	if (0 == m_gpsFirmware.GetLength())
	{
		MessageBox(_T("Please select the gps firmware"));
		return;
	}

	int devcnt = 0;
	for (int i = 0; i < m_devList.GetItemCount(); i++)
	{
		if (m_devList.GetCheck(i))
		{
			devcnt++;
		}
	}

	if (devcnt == 0)
	{
		MessageBox(_T("No device need to upgrade"));
		return;
	}

	// TODO: Add your control notification handler code here
	for (int i = 0; i < m_devList.GetItemCount(); i++)
	{
		if (!m_devList.GetCheck(i))
		{
			continue;
		}

		m_devList.SetItemColor(i, RGB(0, 0, 0), RGB(0xFF, 0xFF, 0xFF));

		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&pi, sizeof(pi));
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.wShowWindow = SW_HIDE;

		TCHAR tcmdline[4096] = { 0 };
		TCHAR ipstr[32] = { 0 };
		TCHAR username[256] = { 0 };
		TCHAR passwd[256] = { 0 };

		int ipaddr = g_devlist[i].ipaddr;
		wsprintf(ipstr, _T("%d.%d.%d.%d"), (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, ipaddr & 0xFF);

		utils_Char2Tchar(g_devlist[i].szuser, username, sizeof(username));
		utils_Char2Tchar(g_devlist[i].szpasswd, passwd, sizeof(passwd));
		wsprintf(tcmdline, _T("gpsup.exe %s %s %s %s"), ipstr, username, passwd, m_gpsFirmware.GetString());

		if (CreateProcess(NULL, tcmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		{
			g_devlist[i].running = true;
			g_devlist[i].run_seconds = 0;
			g_devlist[i].hProcess = pi.hProcess;

			m_devList.SetItemText(i, E_ITEM_STATUS, _T("login..."));
		}
		else
		{
			g_devlist[i].state = E_STATE_FAIL;
			g_devlist[i].running = false;
			g_devlist[i].run_seconds = 0;
			g_devlist[i].hProcess = NULL;

			m_devList.SetItemText(i, E_ITEM_STATUS, _T("connect device fail"));
			m_devList.SetItemColor(i, RGB(0, 0, 0), RGB(0xff, 0, 0));
		}
	}

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);

	CMenu *pdlgMenu = GetMenu();
	pdlgMenu->EnableMenuItem(ID_MENU_SELALL, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	pdlgMenu->EnableMenuItem(ID_OPERATION_SELECTFAILED, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	pdlgMenu->EnableMenuItem(ID_OPERATION_DESELECTALL, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	pdlgMenu->EnableMenuItem(ID_MENU_IMPDEV, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	pdlgMenu->EnableMenuItem(ID_OPERATION_SELECTFIRMWARE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

}


void CBatchItDlg::OnTimer(UINT_PTR nIDEvent)
{
	int runningcnt = 0;

	// TODO: Add your message handler code here and/or call default
	switch (nIDEvent)
	{
	case UPDATE_UI_TIMER:

		for (int i = 0; i < m_devList.GetItemCount(); i++)
		{
			if (!g_devlist[i].running)
			{
				continue;
			}

			runningcnt++;

			g_devlist[i].run_seconds++;
			if (MAX_SECONDS <= g_devlist[i].run_seconds)
			{
				g_devlist[i].state = E_STATE_FAIL;
				if (NULL != g_devlist[i].hProcess)
				{
					TerminateProcess(g_devlist[i].hProcess, 0);
					g_devlist[i].hProcess = NULL;
				}

				g_devlist[i].running = false;
				m_devList.SetItemText(i, E_ITEM_STATUS, _T("timeout"));
				m_devList.SetItemColor(i, RGB(0, 0, 0), RGB(0xff, 0, 0));
			}

			TCHAR tsz_buffer[128];
			wsprintf(tsz_buffer, _T("%d"), g_devlist[i].run_seconds);
			m_devList.SetItemText(i, E_ITEM_TIMECOSTS, tsz_buffer);
			m_devList.EnsureVisible(i, true);
		}

		if (0 == runningcnt)
		{
			GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);

			CMenu *pdlgMenu = GetMenu();
			pdlgMenu->EnableMenuItem(ID_MENU_SELALL, MF_BYCOMMAND | MF_ENABLED);
			pdlgMenu->EnableMenuItem(ID_OPERATION_SELECTFAILED, MF_BYCOMMAND | MF_ENABLED);
			pdlgMenu->EnableMenuItem(ID_OPERATION_DESELECTALL, MF_BYCOMMAND | MF_ENABLED);
			pdlgMenu->EnableMenuItem(ID_MENU_IMPDEV, MF_BYCOMMAND | MF_ENABLED);
			pdlgMenu->EnableMenuItem(ID_OPERATION_SELECTFIRMWARE, MF_BYCOMMAND | MF_ENABLED);
		}

		printf("timeout \r\n");
		UpdateData(FALSE);
		break;

	default:
		break;
	}
}


void CBatchItDlg::OnOperationSelectfirmware()
{
	// TODO: Add your command handler code here
	CFileDialog  dlg(true, _T(".bin"), NULL, OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, _T("firmware(*.bin)|*.bin|All Files(*.*)|*.*||"));
	if (dlg.DoModal() == IDOK)
	{
		m_gpsFirmware = dlg.GetPathName();
		UpdateData(FALSE);
	}
}


void CBatchItDlg::OnControlStop()
{
	// TODO: Add your command handler code here
	POSITION pos = m_devList.GetFirstSelectedItemPosition(); //pos选中的首行位置
	while (pos) //如果选择多行
	{
		int nIdx = -1;

		nIdx = m_devList.GetNextSelectedItem(pos);

		if (nIdx >= 0 && nIdx < m_devList.GetItemCount())
		{
			int ipaddr = (int)m_devList.GetItemData(nIdx);

			for (int i = 0; i < sizeof(g_devlist) / sizeof(IPList_S); i++)
			{
				if (ipaddr == g_devlist[i].ipaddr)
				{
					g_devlist[i].state = E_STATE_FAIL;
					if (NULL != g_devlist[i].hProcess)
					{
						TerminateProcess(g_devlist[i].hProcess, 0);
						g_devlist[i].hProcess = NULL;
					}

					g_devlist[i].running = false;
					m_devList.SetItemText(i, E_ITEM_STATUS, _T("user cancelled"));
					m_devList.SetItemColor(i, RGB(0, 0, 0), RGB(0xff, 0, 0));

					break;
				}
			}
		}
		else
		{
			break;
		}
	}
}


void CBatchItDlg::OnControlStart()
{
	// TODO: Add your command handler code here
	POSITION pos = m_devList.GetFirstSelectedItemPosition(); //pos选中的首行位置
	while (pos) //如果选择多行
	{
		int nIdx = -1;

		nIdx = m_devList.GetNextSelectedItem(pos);

		if (nIdx >= 0 && nIdx < m_devList.GetItemCount())
		{
			int ipaddr = (int)m_devList.GetItemData(nIdx);

			for (int i = 0; i < sizeof(g_devlist) / sizeof(IPList_S); i++)
			{
				if (ipaddr == g_devlist[i].ipaddr)
				{
					if (g_devlist[i].running)
					{
						CString str;
						str.Format(_T("device %d.%d.%d.%d is now running!"), (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, ipaddr & 0xFF);
						MessageBox(str);
						break;
					}

					m_devList.SetItemColor(i, RGB(0, 0, 0), RGB(0xFF, 0xFF, 0xFF));

					STARTUPINFO si;
					PROCESS_INFORMATION pi;

					ZeroMemory(&pi, sizeof(pi));
					ZeroMemory(&si, sizeof(si));
					si.cb = sizeof(si);
					si.wShowWindow = SW_HIDE;

					TCHAR tcmdline[4096] = { 0 };
					TCHAR ipstr[32] = { 0 };
					TCHAR username[256] = { 0 };
					TCHAR passwd[256] = { 0 };

					int ipaddr = g_devlist[i].ipaddr;
					wsprintf(ipstr, _T("%d.%d.%d.%d"), (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, ipaddr & 0xFF);

					utils_Char2Tchar(g_devlist[i].szuser, username, sizeof(username));
					utils_Char2Tchar(g_devlist[i].szpasswd, passwd, sizeof(passwd));
					wsprintf(tcmdline, _T("gpsup.exe %s %s %s %s"), ipstr, username, passwd, m_gpsFirmware.GetString());

					if (CreateProcess(NULL, tcmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
					{
						g_devlist[i].running = true;
						g_devlist[i].run_seconds = 0;
						g_devlist[i].hProcess = pi.hProcess;

						m_devList.SetItemText(i, E_ITEM_STATUS, _T("login..."));
					}
					else
					{
						g_devlist[i].state = E_STATE_FAIL;
						g_devlist[i].running = false;
						g_devlist[i].run_seconds = 0;
						g_devlist[i].hProcess = NULL;

						m_devList.SetItemText(i, E_ITEM_STATUS, _T("connect device fail"));
						m_devList.SetItemColor(i, RGB(0, 0, 0), RGB(0xff, 0, 0));
					}

					break;
				}
			}
		}
		else
		{
			break;
		}
	}
}


void CBatchItDlg::OnRclickListDev(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;

	if (m_devList.GetSelectedCount() <= 0)
	{
		return;
	}

	CMenu menu, *pPopup;
	menu.LoadMenu(IDR_MENU2);
	pPopup = menu.GetSubMenu(0);
	CPoint myPoint;
	ClientToScreen(&myPoint);
	GetCursorPos(&myPoint); //鼠标位置  
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, myPoint.x, myPoint.y, this);
}
