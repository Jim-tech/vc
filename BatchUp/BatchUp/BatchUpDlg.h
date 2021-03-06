
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
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);    
	afx_msg void OnBnClickedButtonFile();
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnClickedTypeCheckBox();
	afx_msg void OnSelchangeComboNet();
	afx_msg void OnLvnColumnclickListUp(NMHDR *pNMHDR, LRESULT *pResult);    
	afx_msg void OnNMRClickListUp(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCopy2ClipBoard();    
    afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();

	POINT           old;
	CString         m_imgPath;
	CString         m_ver;
	CColorListCtrl  m_listCtrl;
    CComboBox       m_netcardCtrl;
	HANDLE          m_pCapThread;
	HANDLE          m_pArpThread;
	HANDLE          m_pProcessThread;
	BOOL            m_doublearea;
	CComboBox       m_platform_cmb;
    int             m_list_clickedCol;
    BOOL            m_sortorder_inc;
};
