
// elfdumpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "elfdump.h"
#include "elfdumpDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

TCHAR *g_astRegName[] =
{
    _T("r0"),_T("r1"),_T("r2"),_T("r3"),_T("r4"),_T("r5"),_T("r6"),_T("r7"),
    _T("r8"),_T("r9"),_T("r10"),_T("r11"),_T("r12"),_T("sp"),_T("lr"),_T("pc"),
    _T("fps"), _T("cpsr"),
};

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
	, m_desc(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CelfdumpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_HDRS, m_hdrList);
	DDX_Text(pDX, IDC_EDIT_DESC, m_desc);
}

BEGIN_MESSAGE_MAP(CelfdumpDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(ID_HELP_ABOUT, &CelfdumpDlg::OnHelpAbout)
	ON_COMMAND(ID_FILE_QUIT, &CelfdumpDlg::OnFileQuit)
	ON_COMMAND(ID_FILE_OPEN, &CelfdumpDlg::OnFileOpen)
	ON_WM_DESTROY()
	ON_COMMAND(ID_OPR_SEL_ALL, &CelfdumpDlg::OnOprSelAll)
	ON_COMMAND(ID_OPR_DESEL_ALL, &CelfdumpDlg::OnOprDeselAll)
	ON_COMMAND(ID_OPR_DUMP_ELF, &CelfdumpDlg::OnOprDumpElf)
	ON_COMMAND(ID_OPR_DUMP_BIN, &CelfdumpDlg::OnOprDumpBin)
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

	m_hdrList.InsertColumn(COL_ID, _T("No."), LVCFMT_LEFT, 60);//插入列
	m_hdrList.InsertColumn(COL_OFFSET, _T("OFFSET"), LVCFMT_LEFT, 80);
	m_hdrList.InsertColumn(COL_VA, _T("VA"), LVCFMT_LEFT, 80);
	m_hdrList.InsertColumn(COL_SIZE, _T("SIZE"), LVCFMT_LEFT, 80);
	m_hdrList.InsertColumn(COL_FLAG, _T("FLAG"), LVCFMT_LEFT, 40);

	//状态条
	m_pStatusBar = new CStatusBarCtrl;
	RECT Rect;
	GetClientRect(&Rect); //获取对话框的矩形区域
	Rect.top = Rect.bottom - 20; //设置状态栏的矩形区域
	m_pStatusBar->Create(WS_VISIBLE | CBRS_BOTTOM, Rect, this, 99999);

	CWnd*   pEdit;   
	pEdit = GetDlgItem(IDC_EDIT_DESC);   
	HFONT   hFont = (HFONT)::GetStockObject(SYSTEM_FIXED_FONT);   
	CFont*  pFont = CFont::FromHandle(hFont);   
	pEdit->SetFont(pFont); 
	
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
	if (NULL != m_elfhdr)
	{
		free(m_elfhdr);
		m_elfhdr = NULL;
	}
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

	CFile f_in;
	if (0 == f_in.Open(m_filePath, CFile::modeRead))
	{
		MessageBox(_T("打开文件失败!"));
		return;
	}

	CString strlog;

	corehdr_s elfhdr;
	if (sizeof(corehdr_s) != f_in.Read(&elfhdr, sizeof(elfhdr)))
	{
		f_in.Close();
		strlog.Format(_T("读取文件%s失败!"), m_filePath);
		MessageBox(strlog);
		return;
	}

	if (NULL != m_elfhdr)
	{
		free(m_elfhdr);
		m_hdrList.DeleteAllItems();
	}

	m_desc = _T("");

	int hdr_len = sizeof(corehdr_s) + elfhdr.st_elfhdr.e_phnum * sizeof(Elf32_Phdr);
	m_elfhdr = (corehdr_s *)malloc(hdr_len);
	if (NULL == m_elfhdr)
	{
		f_in.Close();
		strlog.Format(_T("读取文件时申请内存%d失败"), hdr_len);
		MessageBox(strlog);
		return;
	}

	f_in.SeekToBegin();
	if (hdr_len != f_in.Read(m_elfhdr, hdr_len))
	{
		f_in.Close();
		strlog.Format(_T("读取文件%s失败!"), m_filePath);
		MessageBox(strlog);
		return;
	}

	if (0 != m_elfhdr->st_elfhdr.e_shnum)
	{
		f_in.Close();
		strlog.Format(_T("文件%s非core文件!"), m_filePath);
		MessageBox(strlog);
		return;
	}

	//文件完整性检查
	unsigned long long filelen = f_in.GetLength();
	unsigned int file_end = 0;
	for (int i = 0; i < m_elfhdr->st_elfhdr.e_phnum; i++)
	{
		if (file_end < m_elfhdr->ast_phdr[i].p_offset + m_elfhdr->ast_phdr[i].p_filesz)
		{
			file_end = m_elfhdr->ast_phdr[i].p_offset + m_elfhdr->ast_phdr[i].p_filesz;
		}
	}

	if (filelen != file_end)
	{
		f_in.Close();
		strlog.Format(_T("文件%s非法!"), m_filePath);
		MessageBox(strlog);
		return;
	}

	for (int i = 0; i < m_elfhdr->st_elfhdr.e_phnum; i++)
	{
		CString strtext;
		strtext.Format(_T("%04d"), i);
		m_hdrList.InsertItem(i, strtext);

		strtext.Format(_T("0x%08X"), m_elfhdr->ast_phdr[i].p_offset);
		m_hdrList.SetItemText(i, COL_OFFSET, strtext);
		strtext.Format(_T("0x%08X"), m_elfhdr->ast_phdr[i].p_vaddr);
		m_hdrList.SetItemText(i, COL_VA, strtext);
		strtext.Format(_T("0x%08X"), m_elfhdr->ast_phdr[i].p_memsz);
		m_hdrList.SetItemText(i, COL_SIZE, strtext);

		strtext = _T("");
		strtext += (m_elfhdr->ast_phdr[i].p_flags & PF_R) ? _T("R") : _T("-");
		strtext += (m_elfhdr->ast_phdr[i].p_flags & PF_W) ? _T("W") : _T("-");
		strtext += (m_elfhdr->ast_phdr[i].p_flags & PF_X) ? _T("X") : _T("-");
		m_hdrList.SetItemText(i, COL_FLAG, strtext);
	}

	CString str_prpsinfo = _T("");
	CString str_prstatus = _T("");
	for (int i = 0; i < m_elfhdr->st_elfhdr.e_phnum; i++)
	{
		if (PT_NOTE != m_elfhdr->ast_phdr[i].p_type)
		{
			continue;
		}

		unsigned char *buffer = (unsigned char *)malloc(m_elfhdr->ast_phdr[i].p_filesz);
		if (NULL == buffer)
		{
			f_in.Close();
			strlog.Format(_T("处理note信息时发现内存不足!"));
			MessageBox(strlog);
			return;
		}

		f_in.Seek(m_elfhdr->ast_phdr[i].p_offset, CFile::begin);
		f_in.Read(buffer, m_elfhdr->ast_phdr[i].p_filesz);

		unsigned int offset = 0;
		while (offset < m_elfhdr->ast_phdr[i].p_filesz)
		{
			Elf32_Nhdr *pstNhdr = (Elf32_Nhdr *)(buffer + offset);
			unsigned int note_off = sizeof(Elf32_Nhdr) + pstNhdr->n_namesz;
			if (0 != (note_off & 3))
			{
				note_off = (note_off & ~(0x3)) + 4;
			}
			
			CString stritem;
			TCHAR  tstr[512];
			char   astr[512];
					
			switch (pstNhdr->n_type)
			{
			case NT_PRSTATUS:
				{
					struct elf_prstatus *pdata = (struct elf_prstatus *)((char *)pstNhdr + note_off);

					stritem.Format(_T("%-16s:%d\r\n"), _T("pid"), pdata->pr_pid);
					str_prstatus += stritem;
					stritem.Format(_T("%-16s:%d\r\n"), _T("parent pid"), pdata->pr_ppid);
					str_prstatus += stritem;
					stritem.Format(_T("%-16s:%d\r\n"), _T("process group"), pdata->pr_pgrp);
					str_prstatus += stritem;
					stritem.Format(_T("%-16s:%d\r\n"), _T("session"), pdata->pr_sid);
					str_prstatus += stritem;
					stritem.Format(_T("%-16s:%d\r\n"), _T("signal"), pdata->pr_info.si_signo);
					str_prstatus += stritem;

					for (int j = 0; j < (ELF_NGREG + 1) / 2; j++)
					{
						stritem.Format(_T("%-16s:0x%08x  %-16s:0x%08x\r\n"), g_astRegName[2*j], pdata->pr_reg[2*j], g_astRegName[2*j+1], pdata->pr_reg[2*j+1]);
						str_prstatus += stritem;
					}
					
					str_prstatus += _T("\r\n");
				}
				break;


			case NT_PRFPREG:
				//printf("size=%d \n", sizeof(elf_fpregset_t));
				break;

			case NT_PRPSINFO:
				{
					struct elf_prpsinfo *pdata = (struct elf_prpsinfo *)((char *)pstNhdr + note_off);

					sprintf_s(astr, sizeof(astr), "%s", pdata->pr_psargs);
					utils_Char2Tchar(astr, tstr, sizeof(tstr));
					stritem.Format(_T("%-16s:%s\r\n"), _T("command"), tstr);
					str_prpsinfo += stritem;
					stritem.Format(_T("%-16s:%d\r\n"), _T("pid"), pdata->pr_pid);
					str_prpsinfo += stritem;
					stritem.Format(_T("%-16s:%d\r\n"), _T("parent pid"), pdata->pr_ppid);
					str_prpsinfo += stritem;
					stritem.Format(_T("%-16s:%d\r\n"), _T("process group"), pdata->pr_pgrp);
					str_prpsinfo += stritem;
					stritem.Format(_T("%-16s:%d\r\n"), _T("session"), pdata->pr_sid);
					str_prpsinfo += stritem;
				}
				break;

			case NT_AUXV:
				printf("size=%d \n", sizeof(struct elf_prstatus));            
				break;

			default:
				break;
			}


			unsigned int unitlen = pstNhdr->n_namesz + pstNhdr->n_descsz + sizeof(Elf32_Nhdr);
			if ((unitlen & 0x3) != 0)
			{
				unitlen = (unitlen & (~0x3)) + 4;
			}

			offset += unitlen;
		}

		m_desc += str_prpsinfo;
		m_desc += _T("\r\n\r\n\r\n");
		m_desc += str_prstatus;
		m_desc += _T("\r\n\r\n\r\n");
		
		free(buffer);
	}

	f_in.Close();
	UpdateData(FALSE);
}


void CelfdumpDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
	if (NULL != m_elfhdr)
	{
		free(m_elfhdr);
		m_elfhdr = NULL;
	}
}


void CelfdumpDlg::OnOprSelAll()
{
	// TODO: Add your command handler code here
	for (int i = 0; i < m_hdrList.GetItemCount(); i++)
	{
		m_hdrList.SetCheck(i, TRUE);
	}
}


void CelfdumpDlg::OnOprDeselAll()
{
	// TODO: Add your command handler code here
	for (int i = 0; i < m_hdrList.GetItemCount(); i++)
	{
		m_hdrList.SetCheck(i, FALSE);
	}
}


void CelfdumpDlg::OnOprDumpElf()
{
	// TODO: Add your command handler code here
	int count = 0;
	for (int i = 0; i < m_hdrList.GetItemCount(); i++)
	{
		if (m_hdrList.GetCheck(i))
		{
			count++;
		}
	}
	
	if (count == 0)
	{
		MessageBox(_T("请选择要导出的段"));
		return;
	}

	CFileDialog  dlg(false, _T("elf file"), NULL, OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, _T("All Files(*.*)|*.*||"));
	if (dlg.DoModal() != IDOK)
	{
		return;
	}

	CString savepath = dlg.GetPathName();

	CFile f_out;
	if (0 == f_out.Open(savepath, CFile::modeCreate | CFile::modeWrite))
	{
		MessageBox(_T("创建文件失败!"));
		return;
	}
	
	int offset = 0;

	elf32_hdr  st_elfhdr;
	memcpy(&st_elfhdr, &(m_elfhdr->st_elfhdr), sizeof(elf32_hdr));
	st_elfhdr.e_phnum = count;
	f_out.Write(&st_elfhdr, sizeof(corehdr_s));
	offset += sizeof(corehdr_s);

	offset += sizeof(Elf32_Phdr) * st_elfhdr.e_phnum;
	for (int i = 0; i < m_hdrList.GetItemCount(); i++)
	{
		if (m_hdrList.GetCheck(i))
		{
			Elf32_Phdr  st_phdr;
			memcpy(&st_phdr, &(m_elfhdr->ast_phdr[i]), sizeof(Elf32_Phdr));
			st_phdr.p_offset = offset;
			offset += m_elfhdr->ast_phdr[i].p_filesz;
			f_out.Write(&(st_phdr), sizeof(Elf32_Phdr));
		}
	}

	CFile f_in;
	if (0 == f_in.Open(m_filePath, CFile::modeRead))
	{
		MessageBox(_T("打开文件失败!"));
		f_out.Close();
		return;
	}

	for (int i = 0; i < m_hdrList.GetItemCount(); i++)
	{
		if (m_hdrList.GetCheck(i))
		{
			int align = 1;
			if (0 != m_elfhdr->ast_phdr[i].p_align)
			{
				align = m_elfhdr->ast_phdr[i].p_align;
			}
			else
			{
				align = 1;
			}
			unsigned char *buffer = NULL;

			if (align >= 0x1000)
			{
				buffer = (unsigned char *)malloc(align);
				if (NULL == buffer)
				{
					MessageBox(_T("申请buffer失败!"));
					f_in.Close();
					f_out.Close();
					return;
				}
			}
			else
			{
				unsigned char sz_buffer[0x1000];
				buffer = sz_buffer;
			}

			f_in.Seek(m_elfhdr->ast_phdr[i].p_offset, CFile::begin);

			unsigned int writenlen = 0;
			while (writenlen < m_elfhdr->ast_phdr[i].p_filesz)
			{
				f_in.Read(buffer, align);
				f_out.Write(buffer, align);

				writenlen += align;
			}

			if (align >= 0x1000)
			{
				free(buffer);
			}
		}
	}

	f_in.Close();
	f_out.Close();
	MessageBox(_T("导出完毕!"));
}


void CelfdumpDlg::OnOprDumpBin()
{
	// TODO: Add your command handler code here
	int count = 0;
	for (int i = 0; i < m_hdrList.GetItemCount(); i++)
	{
		if (m_hdrList.GetCheck(i))
		{
			count++;
		}
	}
	
	if (count > 1)
	{
		//if (IDOK != MessageBox(_T("选择了多个段，请确认是否继续?"), NULL, MB_OKCANCEL))
		{
			MessageBox(_T("选择了多个段"));
			return;
		}
	}
	else if (count == 0)
	{
		MessageBox(_T("请选择要导出的段"));
		return;
	}
	
	CFileDialog  dlg(false, _T("elf file"), NULL, OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, _T("All Files(*.*)|*.*||"));
	if (dlg.DoModal() != IDOK)
	{
		return;
	}

	CString savepath = dlg.GetPathName();

	CFile f_out;
	if (0 == f_out.Open(savepath, CFile::modeCreate | CFile::modeWrite))
	{
		MessageBox(_T("创建文件失败!"));
		return;
	}

	CFile f_in;
	if (0 == f_in.Open(m_filePath, CFile::modeRead))
	{
		MessageBox(_T("打开文件失败!"));
		f_out.Close();
		return;
	}

	for (int i = 0; i < m_hdrList.GetItemCount(); i++)
	{
		if (m_hdrList.GetCheck(i))
		{
			int align = 1;
			if (0 != m_elfhdr->ast_phdr[i].p_align)
			{
				align = m_elfhdr->ast_phdr[i].p_align;
			}
			else
			{
				align = 1;
			}
			unsigned char *buffer = NULL;

			if (align >= 0x1000)
			{
				buffer = (unsigned char *)malloc(align);
				if (NULL == buffer)
				{
					MessageBox(_T("申请buffer失败!"));
					f_in.Close();
					f_out.Close();
					return;
				}
			}
			else
			{
				unsigned char sz_buffer[0x1000];
				buffer = sz_buffer;
			}

			f_in.Seek(m_elfhdr->ast_phdr[i].p_offset, CFile::begin);

			unsigned int writenlen = 0;
			while (writenlen < m_elfhdr->ast_phdr[i].p_filesz)
			{
				f_in.Read(buffer, align);
				f_out.Write(buffer, align);

				writenlen += align;
			}

			if (align >= 0x1000)
			{
				free(buffer);
			}
		}
	}

	f_in.Close();
	f_out.Close();

	MessageBox(_T("导出完毕!"));
}
