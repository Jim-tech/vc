
// eepromsetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "eepromset.h"
#include "eepromsetDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

typedef enum
{
	TYPE_MBS1xxx   = 0,
	TYPE_BRU3xxx   = 1
}MODULE_TYPE_E;

typedef enum
{
	MODE_TDD   = 0,
	MODE_FDD   = 1
}DUPLEX_MODE_E;

#define MODULE_TYPE_ADDR        0x6964
#define MODULE_TYPE_LEN         16
#define HWCAP_ADDR              0x6994
#define HWCAP_LEN               4

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


// CeepromsetDlg dialog



CeepromsetDlg::CeepromsetDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CeepromsetDlg::IDD, pParent)
	, m_strFilePath(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CeepromsetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_MODE, m_comboMode);
	DDX_Control(pDX, IDC_COMBO_TYPE, m_comboType);
	DDX_Text(pDX, IDC_EDIT_PATH, m_strFilePath);
}

BEGIN_MESSAGE_MAP(CeepromsetDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CeepromsetDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CeepromsetDlg::OnBnClickedCancel)
	ON_BN_CLICKED(ID_FILE, &CeepromsetDlg::OnBnClickedFile)
END_MESSAGE_MAP()


// CeepromsetDlg message handlers

BOOL CeepromsetDlg::OnInitDialog()
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
	m_comboMode.AddString(_T("TDD"));
	m_comboMode.AddString(_T("FDD"));
	m_comboMode.SetCurSel(MODE_TDD);

	m_comboType.AddString(_T("mBS1"));
	m_comboType.AddString(_T("BRU3"));
	m_comboType.SetCurSel(TYPE_MBS1xxx);

	m_strFilePath = _T("");
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CeepromsetDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CeepromsetDlg::OnPaint()
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
HCURSOR CeepromsetDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CeepromsetDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	//CDialogEx::OnOK();

	UpdateData(true);

	if (0 == m_strFilePath.GetLength())
	{
		MessageBox(_T("请先选择文件"));
		return;
	}

	int type = m_comboType.GetCurSel();
	int mode = m_comboMode.GetCurSel();

	char szbuffer[0x10000] = { 0xFF };
	CFile hFile(m_strFilePath, CFile::modeReadWrite);
	hFile.Read(szbuffer, sizeof(szbuffer));

	switch (type)
	{
		case TYPE_MBS1xxx:
			sprintf_s(&(szbuffer[MODULE_TYPE_ADDR]), MODULE_TYPE_LEN, "mBS1xxx");
			break;
		case TYPE_BRU3xxx:
			sprintf_s(&(szbuffer[MODULE_TYPE_ADDR]), MODULE_TYPE_LEN, "BRU3xxx");
			break;
		default:
			goto OnFail;
	}

	switch (mode)
	{
		case MODE_TDD:
			*(int *)&(szbuffer[HWCAP_ADDR]) = 0xA0000001;
			break;
		case MODE_FDD:
			*(int *)&(szbuffer[HWCAP_ADDR]) = 0xA0000002;
			break;
		default:
			goto OnFail;
	}

	hFile.SeekToBegin();
	hFile.Write(szbuffer, sizeof(szbuffer));
	hFile.Flush();
	hFile.Close();
	MessageBox(_T("保存OK"));
	return;
	
OnFail:
	hFile.Close();
	MessageBox(_T("处理出错"));
	return;
}


void CeepromsetDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnCancel();
}


void CeepromsetDlg::OnBnClickedFile()
{
	// TODO: Add your control notification handler code here
	
	CFileDialog  dlg(true, _T("*"), NULL, OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, _T("All Files(*.*)|*.*||"));
	if (dlg.DoModal() == IDOK)
	{
		m_strFilePath = dlg.GetPathName();
		UpdateData(FALSE);
	}
	
}
