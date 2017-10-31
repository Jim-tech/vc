
// ComOverTCPIPDlg.h : header file
//

#pragma once
#include "afxwin.h"

#define UPDATE_TIMER  1

// CComOverTCPIPDlg dialog
class CComOverTCPIPDlg : public CDialogEx
{
// Construction
public:
	CComOverTCPIPDlg(CWnd* pParent = NULL);	// standard constructor
    virtual ~CComOverTCPIPDlg();
    
// Dialog Data
	enum { IDD = IDD_COMOVERTCPIP_DIALOG };

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
	UINT m_port;
	UINT m_comport1;
	UINT m_comport2;
	DWORD m_ipaddr;
	CString m_console;
    UINT m_commask;
	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedStop();
	CString m_logPath;
	afx_msg void OnBnClickedButtonLog();
};
