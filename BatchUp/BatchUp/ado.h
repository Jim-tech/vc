#ifndef ADO_H
#define ADO_H
#include<afxwin.h> 
#import "c:\Program Files\Common Files\System\ado\msado15.dll" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")


class ADOConn  
{
public:
    _ConnectionPtr m_pConnection;
	//���һ��ָ��Recordset�����ָ��:
	_RecordsetPtr m_pRecordset;
	CString conn_str;
public:
	ADOConn(){};
	virtual ~ADOConn(){};

	int OnInitADOConn()
	{
		int ret = 0;
	    // ��ʼ��OLE/COM�⻷��
	    ::CoInitialize(NULL);
       try
		{
		  m_pConnection.CreateInstance("ADODB.Connection");
		  // ���������ַ�����������BSTR�ͻ���_bstr_t����
		  //_bstr_t strConnect="Password=test;Persist Security Info=True;User ID=gsate;Initial Catalog=atedatabase;Data Source=172.16.1.231;pooling=false;";
		  _bstr_t strConnect=(_bstr_t)conn_str;
	      m_pConnection->Open(strConnect,"","",adModeUnknown);
		}
		catch(_com_error e)
		{
	    	//AfxMessageBox(e.Description());
			return ret = -1;
		}
		return ret;
	}

	// ִ�в�ѯselect
	_RecordsetPtr& GetRecordSet(_bstr_t bstrSQL)
	{
	   try
	   {
		if(m_pConnection==NULL)   OnInitADOConn();
		m_pRecordset.CreateInstance(__uuidof(Recordset));
		m_pRecordset->CursorLocation=adUseClient;
		m_pRecordset->Open(bstrSQL,m_pConnection.GetInterfacePtr(),adOpenDynamic,adLockOptimistic,adCmdText);
	   }
	   catch(_com_error e)
	   {	
		   AfxMessageBox(e.Description());
	   }
	  return m_pRecordset;
	}
	

	
	CString GetFieldValue(CString Field)
	{
        _variant_t theValue;
        CString temp;
        theValue=m_pRecordset->GetCollect((_bstr_t)Field);
        if(theValue.vt==VT_EMPTY ||theValue.vt==VT_NULL )
            temp="";
        else 
	    {
          temp=(LPCTSTR)(_bstr_t)theValue;
          temp.TrimRight();
	      temp.TrimLeft();
		}
       return temp;

	}
	// ִ��SQL��䣬Insert Update _variant_t
	BOOL ExecuteSQL(_bstr_t bstrSQL)
	{
		// _variant_t RecordsAffected;
		try
		{
			// �Ƿ��Ѿ��������ݿ�
			if(m_pConnection == NULL)
			   OnInitADOConn();
			// Connection�����Execute����:(_bstr_t CommandText,
			// VARIANT * RecordsAffected, long Options )
			// ����CommandText�������ִ���ͨ����SQL���
			// ����RecordsAffected�ǲ�����ɺ���Ӱ�������,
			// ����Options��ʾCommandText�����ͣ�adCmdText-�ı����adCmdTable-����
			// adCmdProc-�洢���̣�adCmdUnknown-δ֪
			m_pConnection->Execute(bstrSQL,NULL,adCmdText);
			return true;
		}
		catch(_com_error e)
		{
			AfxMessageBox(e.Description());
			return false;
		}

	}

	void ExitConnect()
	{
	
    	if (m_pRecordset != NULL)  	m_pRecordset->Close();
    	m_pConnection->Close();	
	   ::CoUninitialize();// �ͷŻ���
	}


};
#endif

