
// McastDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CMcastDlg dialog
class CMcastDlg : public CDialogEx
{
// Construction
public:
	CMcastDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CMcastDlg();

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
	afx_msg void OnClickedBtnStart();
	afx_msg void OnClickedBtnStop();
	afx_msg void OnClickedBtnSw();
	CComboBox m_devctrl;
	CButton m_btnStart;
	CButton m_btnStop;
	CButton m_btnselect;    
	CString m_imgPath;
	CString m_strInfo;
    CStatusBarCtrl *m_pStatusBar;
    CWinThread     *m_pSendThread;
	CSliderCtrl m_rateCtrl;
	CString m_rateInfo;
};
