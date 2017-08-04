
// eepromsetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "eepromset.h"
#include "eepromsetDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MODULE_TYPE_ADDR        0x6964
#define MODULE_TYPE_LEN         16
#define DL_FREQ_MIN             0x6984
#define DL_FREQ_MAX             0x6988
#define UL_FREQ_MIN             0x698C
#define UL_FREQ_MAX             0x6990
#define HWCAP_ADDR              0x6994


typedef struct
{
	char     		moduleType[MODULE_TYPE_LEN+1];
	unsigned int	dl_min;
	unsigned int	dl_max;
	unsigned int	ul_min;
	unsigned int	ul_max;
	unsigned int	hwcap;
}ModuleList_S;

#define  MAX_MODULE_TYPE  128
ModuleList_S g_moduleList[MAX_MODULE_TYPE] = { 0 };

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
	memset(g_moduleList, 0xFF, sizeof(g_moduleList));
	for (int i = 0; i < MAX_MODULE_TYPE; i++)
	{
		memset(g_moduleList[i].moduleType, 0, sizeof(g_moduleList[i].moduleType));
	}

	TCHAR strpwd[1024];

	::GetCurrentDirectory(sizeof(strpwd), strpwd);
	StrCatW(strpwd, _T("\\config.ini"));

	TCHAR  szSections[1024] = {0};
	int secsize = GetPrivateProfileSectionNames(szSections, sizeof(szSections), strpwd);

	int  offset = 0;
	int  cnt = 0;
	while (0 != lstrlenW(&szSections[offset]))
	{
		wprintf(_T("%s\n"), &szSections[offset]);

		m_comboType.AddString(&szSections[offset]);

		utils_TChar2Char(&szSections[offset], g_moduleList[cnt].moduleType, sizeof(g_moduleList[cnt].moduleType));
		g_moduleList[cnt].moduleType[MODULE_TYPE_LEN] = '\0';
		
		g_moduleList[cnt].dl_min = GetPrivateProfileInt(&szSections[offset], _T("dlmin"), -1, strpwd);
		g_moduleList[cnt].dl_max = GetPrivateProfileInt(&szSections[offset], _T("dlmax"), -1, strpwd);
		g_moduleList[cnt].ul_min = GetPrivateProfileInt(&szSections[offset], _T("ulmin"), -1, strpwd);
		g_moduleList[cnt].ul_max = GetPrivateProfileInt(&szSections[offset], _T("ulmax"), -1, strpwd);
		g_moduleList[cnt].hwcap = GetPrivateProfileInt(&szSections[offset], _T("hwcap"), -1, strpwd);

		cnt++;

		offset += lstrlenW(szSections) + 1;
	}

	m_comboType.SetCurSel(0);

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

	char szbuffer[0x10000] = { 0xFF };
	CFile hFile(m_strFilePath, CFile::modeReadWrite);
	hFile.Read(szbuffer, sizeof(szbuffer));

	sprintf_s(&(szbuffer[MODULE_TYPE_ADDR]), MODULE_TYPE_LEN, g_moduleList[type].moduleType);
	*(int *)&(szbuffer[DL_FREQ_MIN]) = g_moduleList[type].dl_min;
	*(int *)&(szbuffer[DL_FREQ_MAX]) = g_moduleList[type].dl_max;
	*(int *)&(szbuffer[UL_FREQ_MIN]) = g_moduleList[type].ul_min;
	*(int *)&(szbuffer[UL_FREQ_MAX]) = g_moduleList[type].ul_max;
	*(int *)&(szbuffer[HWCAP_ADDR]) = g_moduleList[type].hwcap;

	hFile.SeekToBegin();
	hFile.Write(szbuffer, sizeof(szbuffer));
	hFile.Flush();
	hFile.Close();
	MessageBox(_T("保存OK"));

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
