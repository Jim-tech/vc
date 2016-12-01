
// ComOverTCPIPDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ComOverTCPIP.h"
#include "ComOverTCPIPDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HINSTANCE g_hVspDriver = NULL;

typedef BOOL (WINAPI *PtrToCreatePair)(char *port1, char *port2); 
typedef BOOL (WINAPI *PtrToDeletePair)(char *port); 
typedef BOOL (WINAPI *PtrToDeleteAll)(void); 

PtrToCreatePair                  g_pfCreatePair = NULL;
PtrToDeletePair           		 g_pfDeletePair = NULL;
PtrToDeleteAll                   g_pfDeleteAll = NULL;

#define VSPD_CreatePair          g_pfCreatePair
#define VSPD_DeletePair          g_pfDeletePair
#define VSPD_DeleteAll           g_pfDeleteAll

static HANDLE g_comHandle = NULL;
static SOCKET g_sock = -1;
static HANDLE g_hThreadInit = NULL;
static HANDLE g_hThreadTx = NULL;
static HANDLE g_hThreadRx = NULL;
static unsigned char g_stopflag = 0;
static UINT g_args[4] = {0};

#define dbgprint(...) \
    do\
    {\
        printf("[%s][%d]", __FUNCTION__, __LINE__);\
        printf(__VA_ARGS__);\
        printf("\n");\
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
public:
	
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

// CComOverTCPIPDlg dialog
CComOverTCPIPDlg::CComOverTCPIPDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CComOverTCPIPDlg::IDD, pParent)
	, m_port(0)
	, m_ipaddr(0)
	, m_console(_T(""))
	, m_commask(0)	
{
#ifdef _DEBUG  
	AllocConsole();  
#endif  

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	g_hVspDriver = LoadLibrary(_T("vspdctl.dll"));
	if (NULL == g_hVspDriver)
	{
		AfxMessageBox(_T("初始化vspdctl.dll失败"), MB_OK|MB_ICONSTOP);
		return;
	}

	VSPD_CreatePair = (PtrToCreatePair)GetProcAddress(g_hVspDriver, "CreatePair");
	VSPD_DeletePair = (PtrToDeletePair)GetProcAddress(g_hVspDriver, "DeletePair");
	VSPD_DeleteAll = (PtrToDeleteAll)GetProcAddress(g_hVspDriver, "DeleteAll");	
	if (   (NULL == VSPD_CreatePair)
		|| (NULL == VSPD_DeletePair)
		|| (NULL == VSPD_DeleteAll)
		)	
	{
		AfxMessageBox(_T("初始化vspdctl.dll失败"), MB_OK|MB_ICONSTOP);
		FreeLibrary(g_hVspDriver);
		return;
	}
	
}

CComOverTCPIPDlg::~CComOverTCPIPDlg()
{
	if (g_sock > 0)
	{
		shutdown(g_sock, SD_BOTH);
		closesocket(g_sock);
		g_sock = -1;
	}
	if (NULL != g_comHandle)
	{
		CloseHandle(g_comHandle);
		g_comHandle = NULL;
	}
	
	if (NULL != g_hVspDriver)
	{
		VSPD_DeleteAll();
		FreeLibrary(g_hVspDriver);
	}

	WSACleanup();
	FreeConsole();  
}

void CComOverTCPIPDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_PORT, m_port);
	DDV_MinMaxUInt(pDX, m_port, 5000, 65535);
	DDX_IPAddress(pDX, IDC_IPADDRESS, m_ipaddr);
	DDX_Text(pDX, IDC_EDIT_COM, m_console);
}

BEGIN_MESSAGE_MAP(CComOverTCPIPDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_START, &CComOverTCPIPDlg::OnBnClickedStart)
	ON_BN_CLICKED(ID_STOP, &CComOverTCPIPDlg::OnBnClickedStop)
END_MESSAGE_MAP()


// CComOverTCPIPDlg message handlers

BOOL CComOverTCPIPDlg::OnInitDialog()
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
	VSPD_DeleteAll();
	
	#if  1
	m_port = 9090;
	m_ipaddr = 0xAC100133;
	#else
	m_port = 8023;
	m_ipaddr = 0x7f000001;
	#endif /* #if 0 */

	//找空闲的COM口
	m_commask = 0;
	HKEY hKey=NULL; 
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"),0,KEY_READ,&hKey) == ERROR_SUCCESS)  
    {  
        int i=0;
		CString valuename,databuffer;  
		DWORD   valuenamebufferlength=200,valuetype,databuddersize=200;  
		while(RegEnumValue(hKey,
			               i,
			               valuename.GetBuffer(200),
			               &valuenamebufferlength,
			               NULL,
			               &valuetype,
			               (BYTE*)databuffer.GetBuffer(200),
			               &databuddersize) != ERROR_NO_MORE_ITEMS) 
		{
			//AfxMessageBox(databuffer, MB_OK|MB_ICONSTOP);
			int comnum;
			swscanf_s(databuffer.GetString(), _T("COM%d"), &comnum);
			m_commask |= (1 << comnum);
			
			databuddersize=200;  
			valuenamebufferlength=200;
			i++;
		}

		RegCloseKey(hKey);  
    }	

	//找两个空闲的COM
	int src = 0;
	int dst = 0;
	for (src = 1; src < 32; src++)
	{
		if (0 == (m_commask & (1 << src)))
		{
			m_commask |= (1 << src);
			break;
		}
	}

	for (dst = 1; dst < 32; dst++)
	{
		if (0 == (m_commask & (1 << dst)))
		{
			m_commask |= (1 << dst);
			break;
		}
	}

	if (src >= 32 || dst >= 32)
	{
		CString strlog;
		strlog.Format(_T("没有找到可用的COM口"));
		AfxMessageBox(strlog, MB_OK|MB_ICONSTOP);
		return TRUE;
	}

	char szport1[32] = {0};
	char szport2[32] = {0};
	sprintf_s(szport1, sizeof(szport1), "COM%d", src);
	sprintf_s(szport2, sizeof(szport2), "COM%d", dst);
	if (TRUE != VSPD_CreatePair(szport1, szport2))
	{
		AfxMessageBox(_T("创建虚拟串口失败"), MB_OK|MB_ICONSTOP);
		return TRUE;
	}

	Sleep(2000);
	
	m_comport1 = src;
	m_comport2 = dst;
	m_console.Format(_T("COM%d"), m_comport1);

	UpdateData(FALSE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CComOverTCPIPDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CComOverTCPIPDlg::OnPaint()
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
HCURSOR CComOverTCPIPDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static HANDLE com_init(int comport)
{
	HANDLE Handle;
	
	//Create the named pipe file handle
	TCHAR szComName[64];
	wsprintf(szComName, _T("COM%d"), comport);
	if ((Handle = CreateFile(szComName,
								  GENERIC_READ | GENERIC_WRITE, 
								  0,
							      (LPSECURITY_ATTRIBUTES)NULL, 
							      OPEN_EXISTING,
								  FILE_ATTRIBUTE_NORMAL,
								  (HANDLE)NULL)) == INVALID_HANDLE_VALUE)
	{
		dbgprint("open console COM%d fail %d", comport, WSAGetLastError());
		return NULL;
	}

	DCB dcb;
	GetCommState(Handle, &dcb);
	dcb.BaudRate=9600;								//波特率
	dcb.Parity=false;								//无奇偶校验位
	dcb.StopBits=0;									//1位停止位
	dcb.ByteSize=8;									//数据宽度为8
	SetCommState(Handle, &dcb);
	SetupComm(Handle,1024,1024);
	PurgeComm(Handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	
	return Handle;
}

static int ip_connect(DWORD ip, unsigned short port)
{
	SOCKADDR_IN          ServerAddr;

	int                  Ret;
	WSADATA              wsaData;
	if ((Ret = WSAStartup(MAKEWORD(2,2), &wsaData)) != 0)
	{
		dbgprint("init winsock fail %d", WSAGetLastError());
	}
	
	if ((g_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		dbgprint("create socket fail %d", WSAGetLastError());
		return -1;
	}

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(port);    
	ServerAddr.sin_addr.s_addr = htonl(ip);
	if (connect(g_sock, (SOCKADDR *) &ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
	{
		dbgprint("connect to server fail %d", WSAGetLastError());
		closesocket(g_sock);
		g_sock = -1;
		return -1;
	} 

	dbgprint("connect to server ok");
	return 0;
}

DWORD WINAPI TxProc(LPVOID lpParameter)  
{
	unsigned char szbuffer[1024];
	unsigned long readlen = 0;

	DWORD dwErrorFlags;
	COMSTAT ComStat;
	while(1)
	{
		Sleep(100);

		if (g_stopflag)
		{
			break;
		}
		
		//获取错误信息以及了解缓冲区相应的状态
		ClearCommError(g_comHandle,&dwErrorFlags,&ComStat);
		//判断接受缓冲区是否为空
		if(ComStat.cbInQue!=0)
		{
			//如果接受缓冲区状态信息不为空，则把接受信息打印出来
			DWORD ReadFlag=ReadFile(g_comHandle,szbuffer,ComStat.cbInQue,&readlen,NULL);
			if(!ReadFlag)
			{
				dbgprint("read from COM failed with error %d", WSAGetLastError());
			}
			else
			{
				dbgprint("com-->ip %d bytes:", readlen);
				for (unsigned int i = 0; i < readlen; i++)
				{
					if (i % 16 == 0)
					{
						printf("%04x:", i);
					}

					printf("%02x ", szbuffer[i]);
					
					if ((i + 1) % 16 == 0)
					{
						printf("\n");
					}
				}
				printf("\n");
				
				send(g_sock, (char *)szbuffer, readlen, 0);
			}
			memset(szbuffer,0,ComStat.cbInQue);
		}
		
		//清空接受缓冲区
		PurgeComm(g_comHandle,PURGE_TXCLEAR);
	}

	return 0;
}

DWORD WINAPI RxProc(LPVOID lpParameter)  
{
	unsigned char szbuffer[1024] = {0};
	
	while (1)
	{
		Sleep(100);

		if (g_stopflag)
		{
			break;
		}
		
		unsigned int len = 0;
		#if 1
		if ((len = recv(g_sock, (char *)szbuffer, sizeof(szbuffer), 0)) == SOCKET_ERROR)
		{
			dbgprint("send failed with error %d", WSAGetLastError());
			continue;
		}
		
		if (len == 0)
		{
			continue;
		}
		#else
		sprintf_s((char *)szbuffer, sizeof(szbuffer), "fuck you\r\n");
		len = strlen((char *)szbuffer) + 1;		
		#endif /* #if 0 */

		dbgprint("ip-->com %d bytes:", len);
		for (unsigned int i = 0; i < len; i++)
		{
			if (i % 16 == 0)
			{
				printf("%04x:", i);
			}

			printf("%02x ", szbuffer[i]);
			
			if ((i + 1) % 16 == 0)
			{
				printf("\n");
			}
		}
		printf("\n");
		
		//写数据
		DWORD dwBytesWrite;
		DWORD WriteFlag=WriteFile(g_comHandle,(void *)szbuffer,len,&dwBytesWrite,NULL);
		if(!WriteFlag)
		{
			printf("write error %d \r\n", GetLastError());
		}
		//清空发送缓冲区
		PurgeComm(g_comHandle,PURGE_TXCLEAR);
	}	

	return 0;
}

DWORD WINAPI InitProc(LPVOID lpParameter)  
{
	UINT *pargs = (UINT *)lpParameter;
	UINT  comport = pargs[0];
	DWORD ip = pargs[1];
	UINT  port = pargs[2];

	if ((g_comHandle = com_init(comport)) == NULL)
	{
		dbgprint("open console fail");
		return -1;
	}

	if ((ip_connect(ip, port)) != 0)
	{
		dbgprint("connect to server fail");
		CloseHandle(g_comHandle);
		g_comHandle = NULL;
		return -1;
	}

	g_hThreadTx = CreateThread(NULL, 0, TxProc, NULL, 0, NULL); 
	g_hThreadRx = CreateThread(NULL, 0, RxProc, NULL, 0, NULL);

	WaitForSingleObject(g_hThreadTx, INFINITE);
	WaitForSingleObject(g_hThreadRx, INFINITE);	

	#if  0
	if (g_sock > 0)
	{
		closesocket(g_sock);
		g_sock = -1;
	}
	if (NULL != g_comHandle)
	{
		CloseHandle(g_comHandle);
		g_comHandle = NULL;
	}
	#endif /* #if 0 */
	
	return 0;
}
void CComOverTCPIPDlg::OnBnClickedStart()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);

	g_args[0] = m_comport2;
	g_args[1] = m_ipaddr;
	g_args[2] = m_port;
	g_hThreadInit = CreateThread(NULL, 0, InitProc, g_args, 0, NULL); 
	
	CloseHandle(g_hThreadInit);
	GetDlgItem(ID_START)->EnableWindow(FALSE);
	GetDlgItem(ID_STOP)->EnableWindow(TRUE);

	UpdateData(FALSE);
}

void CComOverTCPIPDlg::OnBnClickedStop()
{
	// TODO: Add your control notification handler code here
	//CDialogEx::OnCancel();

	g_stopflag = 1;
	if (g_hThreadTx)
	{
		TerminateThread(g_hThreadTx, 0);
		CloseHandle(g_hThreadTx);
		g_hThreadTx = NULL;	
	}
	if (g_hThreadRx)
	{
		TerminateThread(g_hThreadRx, 0);
		CloseHandle(g_hThreadRx);
		g_hThreadRx = NULL;	
	}
	
	g_stopflag = 0;

	Sleep(500);

	if (g_sock > 0)
	{
		shutdown(g_sock, SD_BOTH);
		closesocket(g_sock);
		g_sock = -1;
	}
	if (NULL != g_comHandle)
	{
		CloseHandle(g_comHandle);
		g_comHandle = NULL;
	}

	WSACleanup();

	GetDlgItem(ID_START)->EnableWindow(TRUE);
	GetDlgItem(ID_STOP)->EnableWindow(FALSE);
	
}
