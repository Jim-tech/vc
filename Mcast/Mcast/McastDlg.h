
// McastDlg.h : header file
//

#pragma once
#include "afxwin.h"


// CMcastDlg dialog
class CMcastDlg : public CDialogEx
{
// Construction
public:
	CMcastDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MCAST_DIALOG };

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
	CComboBox m_devctrl;
	afx_msg void OnClickedBtnStart();
	CButton m_btnStart;
	afx_msg void OnClickedBtnStop();
	CButton m_btnStop;
	afx_msg void OnClickedBtnSw();
	CString m_imgPath;
	CString m_strInfo;
    CStatusBarCtrl *m_pStatusBar;
    CWinThread     *m_pSendThread;
};
