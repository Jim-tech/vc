
// McastDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Mcast.h"
#include "McastDlg.h"
#include "afxdialogex.h"
#include "pcap.h"
#include "utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define FRAME_DATA_LEN    1400

typedef struct
{
	unsigned char  dmac[6];
	unsigned char  smac[6];
	unsigned short ethtype;
}ether_hdr_s;

typedef struct
{
	unsigned int   filelen;
	unsigned int   filecrc32;
	unsigned int   framecnt;
}image_info_s;

typedef struct
{
	ether_hdr_s    ethhdr;
	image_info_s   imginfo;
	unsigned int   framelen;	
	unsigned int   seqno;
	unsigned char  resv[44];
	unsigned char  data[FRAME_DATA_LEN];
}mcast_frame_s;

#define MCAST_DMAC           {0x01, 0x00, 0x5E, 0xC7, 0xC8, 0xC9}       /* x.199.200.201 */
#define MCAST_SMAC           {0x00, 0x00, 0x34, 0x56, 0x78, 0x9A}
#define MCAST_ETYPE          0xdefa

unsigned char g_buffer[0x200000] = {0};
image_info_s  g_stimageinfo = {0};

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


// CMcastDlg dialog



CMcastDlg::CMcastDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMcastDlg::IDD, pParent)
	, m_imgPath(_T(""))
	, m_strInfo(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMcastDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_DEV, m_devctrl);
	DDX_Control(pDX, IDC_BTN_START, m_btnStart);
	DDX_Control(pDX, IDC_BTN_STOP, m_btnStop);
	DDX_Text(pDX, IDC_SW_PATH, m_imgPath);
	DDX_Text(pDX, IDC_EDIT_INFO, m_strInfo);
}

BEGIN_MESSAGE_MAP(CMcastDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_START, &CMcastDlg::OnClickedBtnStart)
	ON_BN_CLICKED(IDC_BTN_STOP, &CMcastDlg::OnClickedBtnStop)
	ON_BN_CLICKED(IDC_BTN_SW, &CMcastDlg::OnClickedBtnSw)
END_MESSAGE_MAP()


// CMcastDlg message handlers

BOOL CMcastDlg::OnInitDialog()
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
		m_devctrl.InsertString(index, szDev);
		index++;
	}

	/* Free the device list */
	pcap_freealldevs(alldevs);

	m_devctrl.SetCurSel(0);

	m_btnStart.EnableWindow(TRUE);
	m_btnStop.EnableWindow(FALSE);

	//初始化状态条
	m_pStatusBar = new CStatusBarCtrl;
	RECT Rect; 
	GetClientRect(&Rect); //获取对话框的矩形区域
	Rect.top=Rect.bottom-20; //设置状态栏的矩形区域
	m_pStatusBar->Create(WS_VISIBLE|CBRS_BOTTOM,Rect,this,3);
	int width = (Rect.right - Rect.left) / 2;
	int nParts[2] = {width, -1};
	m_pStatusBar->SetParts(2, nParts); 

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMcastDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMcastDlg::OnPaint()
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
HCURSOR CMcastDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

UINT Multicast_Thread(LPVOID pParam)
{
	CMcastDlg *pdlg= (CMcastDlg *)pParam;

	int sel_index = pdlg->m_devctrl.GetCurSel();	
	int index = 0;

	pcap_if_t *alldevs;
	pcap_if_t *d;
	char errbuf[PCAP_ERRBUF_SIZE+1];
	
	/* Retrieve the device list */
	if(pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		pdlg->MessageBox(_T("没有可用的网卡"));
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

	if (0 == strlen(ifname))
	{
		pdlg->MessageBox(_T("没有找到对应的网卡"));
		return -1;
	}

    pcap_t *fp;
    if ( (fp = pcap_open_live(ifname,            		// name of the device
	                          sizeof(mcast_frame_s),    // portion of the packet to capture (only the first 100 bytes)
	                          1,						// promiscuous mode
	                          1000,               		// read timeout
	                          errbuf              		// error buffer
	                        ) ) == NULL)
    {
		pdlg->MessageBox(_T("打开网卡失败"));
		return -1;
    }	

	mcast_frame_s 	stframe;
	unsigned char   dmac[] = MCAST_DMAC;
	unsigned char   smac[] = MCAST_SMAC;

	memset(&stframe, 0, sizeof(stframe));
	memcpy(stframe.ethhdr.dmac, dmac, sizeof(dmac));
	memcpy(stframe.ethhdr.smac, smac, sizeof(smac));
	stframe.ethhdr.ethtype = htons(MCAST_ETYPE);

	unsigned int current_seqno = 0;
	while (current_seqno < g_stimageinfo.framecnt)
	{
		

	    /* Send down the packet */
	    if (pcap_sendpacket(fp, (const unsigned char *)&stframe, sizeof(stframe)) != 0)
	    {
			pdlg->MessageBox(_T("发包失败"));
			pcap_close(fp);
			return -1;
	    }

		if (0 == (current_seqno + 1) % 1024)
		{
			Sleep(500);
		}

		current_seqno++;
	}
	
	pcap_close(fp);
	return 0;
}

void CMcastDlg::OnClickedBtnStart()
{
	// TODO: Add your control notification handler code here
	if (m_imgPath.GetLength() == 0)
	{
		MessageBox(_T("请选择软件"));
		return;
	}
	
	m_btnStart.EnableWindow(FALSE);
	m_btnStop.EnableWindow(TRUE);

	UpdateData(TRUE);

	CFile 			f;
	CFileException  e;
	if(!f.Open(m_imgPath, CFile::modeRead,&e))
	{
		MessageBox(_T("读取软件出错"));
		return;
	}

	g_stimageinfo.filelen = (unsigned int)f.GetLength();
	g_stimageinfo.framecnt = (0 == (g_stimageinfo.filelen % FRAME_DATA_LEN)) ? (g_stimageinfo.filelen / FRAME_DATA_LEN) : (g_stimageinfo.filelen / FRAME_DATA_LEN + 1);

	f.Close();

	m_pSendThread = AfxBeginThread(Multicast_Thread, this);
	if (NULL == m_pSendThread)
	{
		MessageBox(_T("处理出错"));
		return ;
	}	
}


void CMcastDlg::OnClickedBtnStop()
{
	// TODO: Add your control notification handler code here
	m_btnStart.EnableWindow(TRUE);
	m_btnStop.EnableWindow(FALSE);
}


void CMcastDlg::OnClickedBtnSw()
{
	// TODO: Add your control notification handler code here
	CFileDialog  dlg(true, _T(".IMG"), NULL, OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT, _T("software image(*.IMG)|*.IMG|All Files(*.*)|*.*||"));
	if (dlg.DoModal() == IDOK)
	{
		m_imgPath = dlg.GetPathName();
		UpdateData(FALSE);
	}
}
