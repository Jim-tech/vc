
// BatchUpDlg.h : header file
//

#pragma once
#include "afxwin.h"


// CBatchUpDlg dialog
class CBatchUpDlg : public CDialogEx
{
// Construction
public:
	CBatchUpDlg(CWnd* pParent = NULL);	// standard constructor
    ~CBatchUpDlg();

// Dialog Data
	enum { IDD = IDD_BATCHUP_DIALOG };

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
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedButtonFile();
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT nIDEvent);
    
	POINT       old;
	CString     m_imgPath;
	CString     m_ver;

	CColorListCtrl  m_listCtrl;
    CComboBox       m_netcardCtrl;
	CWinThread     *m_pScanThread;

};
