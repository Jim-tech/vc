
// BatchItDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "ColorListCtrl.h"

// CBatchItDlg dialog
class CBatchItDlg : public CDialogEx
{
// Construction
public:
	CBatchItDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_BATCHIT_DIALOG };

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
	afx_msg void OnMenuAbout();
	afx_msg void OnMenuExit();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMenuImpdev();
	afx_msg void OnMenuSelall();
	afx_msg void OnOperationDeselectall();
	afx_msg void OnOperationSelectfailed();
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnOperationSelectfirmware();

	CColorListCtrl  m_devList;
	POINT           old;
	CString         m_listFile;
	CString			m_gpsFirmware;
	HANDLE          m_pProcessThread;

};
