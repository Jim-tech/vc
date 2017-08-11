#ifndef ADO_H
#define ADO_H
#include<afxwin.h> 
#import "c:\Program Files\Common Files\System\ado\msado15.dll" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")


class ADOConn  
{
public:
    _ConnectionPtr m_pConnection;
	//添加一个指向Recordset对象的指针:
	_RecordsetPtr m_pRecordset;
	CString conn_str;
public:
	ADOConn(){};
	virtual ~ADOConn(){};

	int OnInitADOConn()
	{
		int ret = 0;
	    // 初始化OLE/COM库环境
	    ::CoInitialize(NULL);
       try
		{
		  m_pConnection.CreateInstance("ADODB.Connection");
		  // 设置连接字符串，必须是BSTR型或者_bstr_t类型
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

	// 执行查询select
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
	// 执行SQL语句，Insert Update _variant_t
	BOOL ExecuteSQL(_bstr_t bstrSQL)
	{
		// _variant_t RecordsAffected;
		try
		{
			// 是否已经连接数据库
			if(m_pConnection == NULL)
			   OnInitADOConn();
			// Connection对象的Execute方法:(_bstr_t CommandText,
			// VARIANT * RecordsAffected, long Options )
			// 其中CommandText是命令字串，通常是SQL命令。
			// 参数RecordsAffected是操作完成后所影响的行数,
			// 参数Options表示CommandText的类型：adCmdText-文本命令；adCmdTable-表名
			// adCmdProc-存储过程；adCmdUnknown-未知
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
	   ::CoUninitialize();// 释放环境
	}


};
#endif

