/********************************************************************************************************************************************
	만든사람		dogseller
---------------------------------------------------------------------------------------------------------------------------------------------
	만든일시		2001-10-01 ~
---------------------------------------------------------------------------------------------------------------------------------------------
	파일설명		DB에 접속하고 SELECT, UPDATE, INSERT, DELETE를 수행한다.
********************************************************************************************************************************************/
// dogADODB.h: interface for the CDogADODB class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DOGADODB_H__B8FAE9F9_C49D_4306_B70B_CFE547491C64__INCLUDED_)
#define AFX_DOGADODB_H__B8FAE9F9_C49D_4306_B70B_CFE547491C64__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#import "msado15.dll" no_namespace rename("EOF", "EndOfFile")
//#import "msado21.tlb" no_namespace rename("EOF", "EndOfFile")
//#include <afxcoll.h>

class CDogADODB  
{
public:
	CString ConvertDBField(CString strFieldData);
	BOOL ExecuteQuery(CString strSQL);
	void ViewFieldNameList();
	BOOL GetFieldValue(LONG lRecordNO, CString strName, CString *strValue);
	BOOL DBConnect(CString strConnectionString);
	BOOL GetRecordCount(LONG * lRecordCount);
	BOOL OpenQuery(CString strSQL);
	BOOL DBDisConnect();
	CDogADODB();
	virtual ~CDogADODB();

private:
	BOOL ConvertValue(_variant_t vtValue, CString * strValue);
	BOOL GetFieldNO(CString strName, LONG * lFieldNO);
	BOOL SetFieldList();
	CStringArray m_FieldList;
	_ConnectionPtr m_pDBConnection;
	_RecordsetPtr m_pRecordSet;
};

#endif // !defined(AFX_DOGADODB_H__B8FAE9F9_C49D_4306_B70B_CFE547491C64__INCLUDED_)
