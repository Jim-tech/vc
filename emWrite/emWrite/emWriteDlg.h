
// emWriteDlg.h : header file
//

#pragma once
#include "afxwin.h"

///////////////////////////////////////////////////////////////////////////////////////////
#define TFTP_RRQ        01 // ������
#define TFTP_WRQ        02 // д����
#define TFTP_DATA       03 // ���ݰ�
#define TFTP_ACK        04 // ȷ�ϰ�
#define TFTP_ERROR      05 // �������
#define TFTP_UDP_PORT   8899


#define MAX_MAC_LEN          12
#define MAX_SN_LEN           19
#define MAX_ITEM_LEN         10
#define MAX_CLEI_LEN         10
#define MAX_BOM_LEN          10
#define MAX_DESC_LEN         200
#define MAX_HWVER_LEN        20
#define MAX_BTYPE_LEN        8
#define MAX_ISSUE_LEN        2
#define MAX_VENDOR_LEN       10

#define MAX_CFG_HWVER_NUM    500
#define MAX_CFG_BTYPE_NUM    500
#define MAX_CFG_VENDOR_NUM   20
#define MAX_CFG_ISSUE_NUM    200

typedef struct
{
    TCHAR szItem[MAX_CFG_BTYPE_NUM][MAX_BTYPE_LEN + 1];
}HwInfo_List_S;

typedef struct
{
    char    ipaddr[32];
    char    localipaddr[32];
    char    user[256];
    char    passwd[256];
}SshCfg_S;

typedef struct
{
    HwInfo_List_S       stHwInfoCfg;
    SshCfg_S            stSshCfg;
}Cfg_S;

typedef struct
{
    int    err_code;
    TCHAR *pErrInfo;
}SSH_ERR_INFO_S;

typedef struct
{
    unsigned short  opcode;
    unsigned short  block;
}TftpData_S;


/*���½ṹ�漰����PC�Լ���������ͨ�ţ������Ҫ1�ֽڶ���  */
#pragma pack(1)

/*ruģ��IPCͨ��ʹ�õ�UDP�˿ں�  */
#define RU_IPC_UDP_PORT   8889

#define RU_IPC_MSG_MAXLEN 1400

#define WAGENT_WR_ALL 0x1000
#define WAGENT_RD_ALL 0x1001

#define RUIPC_PAYLOAD_LEN(type)  (sizeof(type)-sizeof(RUTlvHdr_S))

typedef struct
{
    unsigned short Type;
    unsigned short Length;
}RUTlvHdr_S;

/*�����������Ӧֻ���Ӧһ�������0��ʾ�ɹ���������ʾ��Ӧ�Ĵ���  */
typedef struct
{
    RUTlvHdr_S     tlvHdr;
    unsigned short errCode;
}RUSetAck_S;

/*��������  ����: */
typedef struct
{
    RUSetAck_S      stAck;
    unsigned char   szMac[6];
    unsigned char   szResv1[10];
    unsigned char   szSn[20];
    unsigned char   szResv2[12];
    int             TxGain0;
    int             TxGain1;
    int             freqOffset;
    int             unused[6];
    unsigned char   hwVer[20];
    int             gpsinfo;
    int             bootmode;
    int             upgradeinfo;
    int             RxGain0;
    int             RxGain1;    
}RUSaveAllReq_S;


#if  0
/*��������  ����: */
typedef struct
{
    RUSetAck_S      stAck;
    unsigned char   szMac[6];
    int             TxGain0;
    int             RxGain0;
    int             TxGain1;
    int             RxGain1;
    int             freqOffset;
    unsigned char   szBType[8];
    unsigned char   szBarcode[19];
    unsigned char   szItemBom[10];
    unsigned char   szDesc[200];
    unsigned char   szManufactured[10];
    unsigned char   szVendorName[10];
    unsigned char   szIssueNum[2];
    unsigned char   szCLEICode[10];
    unsigned char   szBOM[10];
}RUSaveAllReq_S;
#endif /* #if 0 */

#pragma pack()

///////////////////////////////////////////////////////////////////////////////////////////


// CemWriteDlg dialog
class CemWriteDlg : public CDialogEx
{
// Construction
public:
	CemWriteDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_EMWRITE_DIALOG };

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
	CComboBox m_hwInfoCtrl;
	CString m_freqOffset;
	CString m_Mac;
	CString m_elabel;
	CString m_GainRx0;
	CString m_GainRx1;
	CString m_SN;
	CString m_GainTx0;
	CString m_GainTx1;
    CString m_ReadInfo;
	afx_msg void OnBnClickedButtonWrite();
	afx_msg void OnBnClickedButtonRead();
	int CheckInput();
    int ReadConfig();
};
