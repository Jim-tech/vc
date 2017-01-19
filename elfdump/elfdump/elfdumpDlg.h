
// elfdumpDlg.h : header file
//
#include "ColorListCtrl.h"
#include "elf.h"

#pragma once


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

	CColorListCtrl m_hdrList;
	CString m_briefInfo;
	CString m_binInfo;
	CString m_filePath;
	CStatusBarCtrl *m_pStatusBar;

};
