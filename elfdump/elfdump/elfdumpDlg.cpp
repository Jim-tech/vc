
// elfdumpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "elfdump.h"
#include "elfdumpDlg.h"
#include "afxdialogex.h"

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


// CelfdumpDlg dialog



CelfdumpDlg::CelfdumpDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CelfdumpDlg::IDD, pParent)
	, m_briefInfo(_T(""))
	, m_binInfo(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CelfdumpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_HDRS, m_hdrList);
	DDX_Text(pDX, IDC_EDIT_BRIEF, m_briefInfo);
	DDX_Text(pDX, IDC_EDIT_BIN, m_binInfo);
}

BEGIN_MESSAGE_MAP(CelfdumpDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(ID_HELP_ABOUT, &CelfdumpDlg::OnHelpAbout)
	ON_COMMAND(ID_FILE_QUIT, &CelfdumpDlg::OnFileQuit)
	ON_COMMAND(ID_FILE_OPEN, &CelfdumpDlg::OnFileOpen)
END_MESSAGE_MAP()


// CelfdumpDlg message handlers

BOOL CelfdumpDlg::OnInitDialog()
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

	/*默认状态  */

	LONG lStyle;
	lStyle = GetWindowLong(m_hdrList.m_hWnd, GWL_STYLE);	//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK;							    //清除显示方式位
	lStyle |= LVS_REPORT;									//设置style
	SetWindowLong(m_hdrList.m_hWnd, GWL_STYLE, lStyle);	//设置style

	DWORD dwStyle = m_hdrList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;						//选中某行使整行高亮（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_GRIDLINES;							//网格线（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_CHECKBOXES;							//item前生成checkbox控件
	m_hdrList.SetExtendedStyle(dwStyle);					//设置扩展风格

	m_hdrList.InsertColumn(0, _T("No."), LVCFMT_LEFT, 60);//插入列
	m_hdrList.InsertColumn(1, _T("OFFSET"), LVCFMT_LEFT, 80);
	m_hdrList.InsertColumn(2, _T("VA"), LVCFMT_LEFT, 80);
	m_hdrList.InsertColumn(3, _T("SIZE"), LVCFMT_LEFT, 80);

	//状态条
	m_pStatusBar = new CStatusBarCtrl;
	RECT Rect;
	GetClientRect(&Rect); //获取对话框的矩形区域
	Rect.top = Rect.bottom - 20; //设置状态栏的矩形区域
	m_pStatusBar->Create(WS_VISIBLE | CBRS_BOTTOM, Rect, this, 99999);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CelfdumpDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CelfdumpDlg::OnPaint()
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
HCURSOR CelfdumpDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CelfdumpDlg::OnHelpAbout()
{
	// TODO: Add your command handler code here
	CAboutDlg dlg;
	dlg.DoModal();
}


void CelfdumpDlg::OnFileQuit()
{
	// TODO: Add your command handler code here
	CDialogEx::OnCancel();
}


void CelfdumpDlg::OnFileOpen()
{
	// TODO: Add your command handler code here
	CFileDialog  dlg(true, _T("elf file"), NULL, OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, _T("All Files(*.*)|*.*||"));
	if (dlg.DoModal() == IDOK)
	{
		m_filePath = dlg.GetPathName();
		m_pStatusBar->SetText(m_filePath, 0, 0);
		UpdateData(FALSE);
	}



}
