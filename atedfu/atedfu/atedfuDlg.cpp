
// atedfuDlg.cpp : implementation file
//

#include "stdafx.h"
#include "atedfu.h"
#include "atedfuDlg.h"
#include "afxdialogex.h"
#include "libatecard.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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


// CatedfuDlg dialog



CatedfuDlg::CatedfuDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CatedfuDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CatedfuDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_MODE, m_dfuModeCtrl);
}

BEGIN_MESSAGE_MAP(CatedfuDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CatedfuDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CatedfuDlg message handlers

BOOL CatedfuDlg::OnInitDialog()
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
	m_dfuModeCtrl.InsertString(0, _T("APP Mode"));
	m_dfuModeCtrl.InsertString(1, _T("DFU Mode"));
	m_dfuModeCtrl.SetCurSel(0);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CatedfuDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CatedfuDlg::OnPaint()
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
HCURSOR CatedfuDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CatedfuDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	//CDialogEx::OnOK();
	if (1 != m_dfuModeCtrl.GetCurSel())
	{
		MessageBox(_T("暂不支持切换到APP模式"));
		return;
	}

	int  ret = 0;
	int  cnt = 0;

	ret = atecard_driver_init();
	if (0 != ret)
	{
		MessageBox(_T("初始化ATE驱动失败"));
		goto Quit;
	}

	ret = atecard_getdevcount(&cnt);
	if (0 != ret)
	{
		MessageBox(_T("获取ATE设备个数失败"));
		goto Quit;
	}

	if (0 == cnt)
	{
		MessageBox(_T("没有找到ATE设备"));
		goto Quit;
	}

	if (1 != cnt)
	{
		MessageBox(_T("一次只能操作一个ATE设备"));
		goto Quit;
	}

	ret = atecard_open();
	if (0 != ret)
	{
		MessageBox(_T("打开ATE设备失败"));
		goto Quit;
	}

	ret = atecard_dfu_enable();
	if (0 != ret)
	{
		MessageBox(_T("使能DFU模式失败"));
		goto Quit;
	}

	MessageBox(_T("使能DFU模式成功"));

Quit:
	atecard_close();
	atecard_driver_deinit();
}
