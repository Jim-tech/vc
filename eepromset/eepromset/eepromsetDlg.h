
// eepromsetDlg.h : header file
//

#pragma once
#include "afxwin.h"


// CeepromsetDlg dialog
class CeepromsetDlg : public CDialogEx
{
// Construction
public:
	CeepromsetDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_EEPROMSET_DIALOG };

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
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedFile();
	CComboBox m_comboMode;
	CComboBox m_comboType;
	CString m_strFilePath;
};
