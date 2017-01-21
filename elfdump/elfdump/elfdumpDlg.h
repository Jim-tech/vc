
// elfdumpDlg.h : header file
//
#include "ColorListCtrl.h"
#include "elf.h"

#pragma once

typedef struct
{
    elf32_hdr     st_elfhdr;
    #pragma warning(disable : 4200) 
    elf32_phdr    ast_phdr[0];
    #pragma warning(disable : 4200) 
}corehdr_s;

enum ListColAttr
{
    COL_ID,
    COL_OFFSET,
    COL_VA,
    COL_SIZE,
    COL_FLAG
};

// CelfdumpDlg dialog
class CelfdumpDlg : public CDialogEx
{
// Construction
public:
	CelfdumpDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_ELFDUMP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnHelpAbout();
	afx_msg void OnFileQuit();
	afx_msg void OnFileOpen();
	afx_msg void OnDestroy();
	afx_msg void OnOprSelAll();
	afx_msg void OnOprDeselAll();
	afx_msg void OnOprDumpElf();
	afx_msg void OnOprDumpBin();
    
	CColorListCtrl m_hdrList;
	CString m_filePath;
	CStatusBarCtrl *m_pStatusBar;
    corehdr_s      *m_elfhdr;
	CString m_desc;
};
