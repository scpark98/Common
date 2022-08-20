/********************************************************************************************************************************************
	만든사람		dogseller
---------------------------------------------------------------------------------------------------------------------------------------------
	만든일시		2001-10-01 ~
---------------------------------------------------------------------------------------------------------------------------------------------
	파일설명		DB에 접속하고 SELECT, UPDATE, INSERT, DELETE를 수행한다.
********************************************************************************************************************************************/
// DOGAdodb.cpp: implementation of the CDogADODB class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "siteprotect.h"
#include "DogADODB.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDogADODB::CDogADODB()
{
	m_pDBConnection = NULL;
	m_pRecordSet = NULL;
}

CDogADODB::~CDogADODB()
{
	DBDisConnect();
}

/********************************************************************************************************************************************
함수명		DBConnect(CString strConnectionString)
---------------------------------------------------------------------------------------------------------------------------------------------
함수설명	DB에 접속한다.
---------------------------------------------------------------------------------------------------------------------------------------------
멤버변수	CString strConnectionString	=>	ADO에 접속하기 위한 Connect String
---------------------------------------------------------------------------------------------------------------------------------------------
리턴값		BOOL						=>	TURE : 성공
********************************************************************************************************************************************/
BOOL CDogADODB::DBConnect(CString strConnectionString)
{
	::CoInitialize(NULL);
	if(m_pDBConnection != NULL) return FALSE;
	try
	{
		m_pDBConnection.CreateInstance(__uuidof(Connection));
		m_pDBConnection->Open(_bstr_t(strConnectionString), "", "", adConnectUnspecified);		
	}
	catch(_com_error &e)
	{
		TRACE("*** DOG CHECK [ADO DB] => <DBConnect 오류> 메시지[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <DBConnect 오류> 설명[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <DBConnect 오류> 소스[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	return TRUE;
}

/********************************************************************************************************************************************
함수명		DBDisConnect()
---------------------------------------------------------------------------------------------------------------------------------------------
함수설명	접속된 DB의 연결을 끊는다.
---------------------------------------------------------------------------------------------------------------------------------------------
멤버변수	없음
---------------------------------------------------------------------------------------------------------------------------------------------
리턴값		BOOL			=>	TURE : 성공
********************************************************************************************************************************************/
BOOL CDogADODB::DBDisConnect()
{
	if(m_pDBConnection == NULL) return FALSE;
	try
	{
		if(m_pRecordSet != NULL)
		{
			m_pRecordSet->Close();
			m_pRecordSet.Release();
		}
		m_pDBConnection->Close();
		m_pDBConnection.Release();
	}
	catch(_com_error &e)
	{
		TRACE("*** DOG CHECK [ADO DB] => <DBDisConnect 오류> 메시지[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <DBDisConnect 오류> 설명[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <DBDisConnect 오류> 소스[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	::CoUninitialize();
	return TRUE;
}

/********************************************************************************************************************************************
함수명		ExecuteQuery(CString strSQL)
---------------------------------------------------------------------------------------------------------------------------------------------
함수설명	INSERT, UPDATE, DELETE 한다.
---------------------------------------------------------------------------------------------------------------------------------------------
멤버변수	CString strSQL	=>	QUERY 문당
---------------------------------------------------------------------------------------------------------------------------------------------
리턴값		BOOL			=>	TURE : 성공
********************************************************************************************************************************************/
BOOL CDogADODB::ExecuteQuery(CString strSQL)
{
	if(m_pDBConnection == NULL) return FALSE;
    try
    {
		m_pDBConnection->BeginTrans();
		m_pDBConnection->Execute(_bstr_t(strSQL), NULL, adExecuteNoRecords);
		m_pDBConnection->CommitTrans(); 
	}
 	catch(_com_error &e)
	{
		m_pDBConnection->RollbackTrans(); 
		TRACE("*** DOG CHECK [ADO DB] => <ExecuteQuery 오류> 메시지[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <ExecuteQuery 오류> 설명[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <ExecuteQuery 오류> 소스[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	return TRUE;
}

/********************************************************************************************************************************************
함수명		OpenQuery(CString strSQL)
---------------------------------------------------------------------------------------------------------------------------------------------
함수설명	SELECT 한다.
---------------------------------------------------------------------------------------------------------------------------------------------
멤버변수	CString strSQL	=>	QUERY 문당
---------------------------------------------------------------------------------------------------------------------------------------------
리턴값		BOOL			=>	TURE : 성공
********************************************************************************************************************************************/
BOOL CDogADODB::OpenQuery(CString strSQL)
{
	if(m_pDBConnection == NULL) return FALSE;
	try
	{
		if(m_pRecordSet != NULL)
		{
			m_pRecordSet->Close();
			m_pRecordSet.Release();
			m_pRecordSet = NULL;
		}
		m_pRecordSet.CreateInstance(__uuidof(Recordset));
        m_pRecordSet->Open(_bstr_t(strSQL), _variant_t((IDispatch *) m_pDBConnection), adOpenStatic, adLockOptimistic, adCmdUnknown);
		if(SetFieldList() == FALSE) return FALSE;
	}
	catch(_com_error &e)
	{
		TRACE("*** DOG CHECK [ADO DB] => <OpenQuery 오류> 메시지[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <OpenQuery 오류> 설명[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <OpenQuery 오류> 소스[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	return TRUE;
}

/********************************************************************************************************************************************
함수명		GetRecordCount(LONG * lRecordCount)
---------------------------------------------------------------------------------------------------------------------------------------------
함수설명	SELECT된 ReocordSet의 Record Count를 얻어낸다.
---------------------------------------------------------------------------------------------------------------------------------------------
멤버변수	LONG * lRecordCount	=>	얻어낼 RecordSet의 Record Count
---------------------------------------------------------------------------------------------------------------------------------------------
리턴값		BOOL				=>	TURE : 얻기 성공
********************************************************************************************************************************************/
BOOL CDogADODB::GetRecordCount(LONG * lRecordCount)
{
	if(m_pDBConnection == NULL) return FALSE;
	try
	{
		if(m_pRecordSet == NULL) return FALSE;
		*lRecordCount = m_pRecordSet->GetRecordCount();
	}
	catch(_com_error &e)
	{
		TRACE("*** DOG CHECK [ADO DB] => <GetRecordCount 오류> 메시지[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <GetRecordCount 오류> 설명[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <GetRecordCount 오류> 소스[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	return TRUE;
}


/********************************************************************************************************************************************
함수명		GetFieldValue(LONG lRecordNO, CString strName, CString *strValue)
---------------------------------------------------------------------------------------------------------------------------------------------
함수설명	SELECT된 각 Field의 데이타를 얻어온다.
---------------------------------------------------------------------------------------------------------------------------------------------
멤버변수	LONG lRecordNO		=>	현재 SELECT된 RecordSet의 Record 번호
			CString strName		=>	현재 SELECT된 RecordSet의 Filed 이름
			CString *strValue	=>	현재 SELECT된 RecordSet의 Filed 데이타
---------------------------------------------------------------------------------------------------------------------------------------------
리턴값		BOOL				=>	TURE : 얻기 성공
********************************************************************************************************************************************/
BOOL CDogADODB::GetFieldValue(LONG lRecordNO, CString strName, CString *strValue)
{
	FieldsPtr pFields = NULL;
	FieldPtr pField = NULL;
	LONG lFieldNO = 0;
	_variant_t vtIndex;
	_variant_t vtValue;

	if(m_pDBConnection == NULL) return FALSE;
	try
	{
		if(m_pRecordSet == NULL) return FALSE;
		if(GetFieldNO(strName, &lFieldNO) == FALSE) 
		{
			TRACE("*** DOG CHECK [ADO DB] => <Field명이 존재안함> !! ***\n");
			return FALSE;
		}
		m_pRecordSet->MoveFirst();
		m_pRecordSet->Move(lRecordNO);
		pFields = m_pRecordSet->GetFields();
		vtIndex.vt = VT_I4;
		vtIndex.lVal = lFieldNO;
		pField = pFields->GetItem(vtIndex);
		vtValue = pField->GetValue();
		if(ConvertValue(vtValue, strValue) == FALSE) 
		{
			TRACE("Field값을 변경할 수 없습니다.\n");
			return FALSE;
		}
	}
	catch(_com_error &e)
	{
		TRACE("*** DOG CHECK [ADO DB] => <GetFieldValue 오류> 메시지[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <GetFieldValue 오류> 설명[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <GetFieldValue 오류> 소스[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	return TRUE;
}

/********************************************************************************************************************************************
함수명		SetFieldList()
---------------------------------------------------------------------------------------------------------------------------------------------
함수설명	SELECT된 각 Field의 이름을 List에 저장한다.
---------------------------------------------------------------------------------------------------------------------------------------------
멤버변수	없음
---------------------------------------------------------------------------------------------------------------------------------------------
리턴값		BOOL			=>	TURE : 저장 성공
********************************************************************************************************************************************/
BOOL CDogADODB::SetFieldList()
{
	FieldsPtr pFields = NULL;
	FieldPtr pField = NULL;
	LONG lFieldNO = 0;
	CString strField;
	_variant_t vtIndex;

	try
	{
		if(m_pRecordSet == NULL) return FALSE;
		m_FieldList.RemoveAll();
		pFields = m_pRecordSet->GetFields();
		for(lFieldNO = 0; lFieldNO < pFields->GetCount(); lFieldNO++)
		{
			vtIndex.vt = VT_I4;
			vtIndex.lVal =lFieldNO;
			pField = pFields->GetItem(vtIndex);
			strField.Format("%s", (char *)pField->GetName());
			m_FieldList.Add(strField);
		}
	}
	catch(_com_error &e)
	{
		TRACE("*** DOG CHECK [ADO DB] => <SetFieldList 오류> 메시지[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <SetFieldList 오류> 설명[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <SetFieldList 오류> 소스[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	return TRUE;

}

/********************************************************************************************************************************************
함수명		ViewFieldNameList()
---------------------------------------------------------------------------------------------------------------------------------------------
함수설명	SELECT된 각 Field의 이름을 확인할때 사용된다. 
---------------------------------------------------------------------------------------------------------------------------------------------
멤버변수	없음
---------------------------------------------------------------------------------------------------------------------------------------------
리턴값		없음
********************************************************************************************************************************************/
void CDogADODB::ViewFieldNameList()
{
	int nIndex;
	CString strField;

	for(nIndex = 0; nIndex < m_FieldList.GetSize(); nIndex++)
	{
		TRACE("*** DOG CHECK [ADO DB] => <Field명 보기> Field[%s] !! ***\n", m_FieldList.GetAt(nIndex));
	}
}

/********************************************************************************************************************************************
함수명		GetFieldNO(CString strName, LONG * lFieldNO)
---------------------------------------------------------------------------------------------------------------------------------------------
함수설명	ADO에서는 각 Field의 명으로 Filed의 데이타를 얻을수 없다. 번호로만 가능(?)
			그래서 각 Filed명으로 실제 번호를 얻어내기 위한 함수다.
---------------------------------------------------------------------------------------------------------------------------------------------
멤버변수	CString strName	=>	SELECT에서 얻어낼 Filed의 이름
			LONG * lFieldNO	=>	얻어낼 실제 Field의 번호
---------------------------------------------------------------------------------------------------------------------------------------------
리턴값		BOOL			=>	TURE : 얻기 성공
********************************************************************************************************************************************/
BOOL CDogADODB::GetFieldNO(CString strName, LONG * lFieldNO)
{
	int nIndex = 0;

	for(nIndex = 0; nIndex < m_FieldList.GetSize(); nIndex++)
	{
		if(strName == m_FieldList.GetAt(nIndex))
		{
			*lFieldNO = (LONG) nIndex;
			return TRUE;
		}
	}
	return FALSE;
}

/********************************************************************************************************************************************
함수명		ConvertValue(_variant_t vtValue, CString *strValue)
---------------------------------------------------------------------------------------------------------------------------------------------
함수설명	ADO에서 SELECT된 각 Filed의 데이타를 CString형으로 변경한다.
---------------------------------------------------------------------------------------------------------------------------------------------
멤버변수	_variant_t vtValue	=>	SELECT에서 얻어낸 Field의 데이타형 및 데이타 값
			CString *strValue	=>	변경된 데이타
---------------------------------------------------------------------------------------------------------------------------------------------
리턴값		BOOL				=>	TURE : 변경 성공
********************************************************************************************************************************************/
BOOL CDogADODB::ConvertValue(_variant_t vtValue, CString *strValue)
{
	if(vtValue.vt == VT_NULL)
	{
		(*strValue).Format("");
		return TRUE;	
	}
	else if(vtValue.vt == VT_BSTR)
	{
		CString bstrVal(vtValue.bstrVal);
		(*strValue).Format("%s", bstrVal);
		return TRUE;	
	}
	else if(vtValue.vt == VT_DECIMAL)
	{
		(*strValue).Format("%d", vtValue.decVal);
		return TRUE;	
	}
	else if(vtValue.vt == VT_DATE)
	{
		COleDateTime date(vtValue.date);
		(*strValue).Format("%04d-%02d-%02d", date.GetYear(), date.GetMonth(), date.GetDay());
		return TRUE;	
	}
	else if(vtValue.vt == VT_UI1)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_UI1 !! ***\n");
	}
	else if(vtValue.vt == VT_UI2)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_UI2 !! ***\n");
	}
	else if(vtValue.vt == VT_UI4)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_UI4 !! ***\n");
	}
	else if(vtValue.vt == VT_UINT)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_UINT !! ***\n");
	}
	else if(vtValue.vt == VT_INT)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_INT !! ***\n");
	}
	else if(vtValue.vt == VT_I1)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_I1 !! ***\n");
	}
	else if(vtValue.vt == VT_I2)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_I2 !! ***\n");
	}
	else if(vtValue.vt == VT_I4)
	{
		(*strValue).Format("%d", vtValue.lVal);
		return TRUE;
	}
	else if(vtValue.vt == VT_R4)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_R4 !! ***\n");
	}
	else if(vtValue.vt == VT_R8)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_R8 !! ***\n");
	}
	else if(vtValue.vt == VT_CY)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_CY !! ***\n");
	}
	else if(vtValue.vt == VT_ERROR)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_ERROR !! ***\n");
	}
	else if(vtValue.vt == VT_BOOL)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_BOOL !! ***\n");
	}
	else if(vtValue.vt == VT_DISPATCH)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_DISPATCH !! ***\n");
	}
	else if(vtValue.vt == VT_VARIANT)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_VARIANT !! ***\n");
	}
	else if(vtValue.vt == VT_UNKNOWN)
	{
		TRACE("*** DOG CHECK [ADO DB] => <데이타형> VT_UNKNOWN !! ***\n");
	}
	return FALSE;
}

/********************************************************************************************************************************************
함수명		ConvertDBField(CString strFieldData)
---------------------------------------------------------------------------------------------------------------------------------------------
함수설명	Query문에 부적합한 문자를 적합한 문자로 변경한다.
			1) ' 문자
---------------------------------------------------------------------------------------------------------------------------------------------
멤버변수	CString strFieldData	=>	Query문에 부적합한 문자를 변경해준다.
---------------------------------------------------------------------------------------------------------------------------------------------
리턴값		CString					=>	변경된 데이타를 리턴한다.
********************************************************************************************************************************************/
CString CDogADODB::ConvertDBField(CString strFieldData)
{
	int nPos = 0;

	while(TRUE)
	{
		nPos = strFieldData.Find("'", nPos);
		if(nPos == -1) break;
		strFieldData.Insert(nPos, "'");
		nPos = nPos + 2;
	}
	return strFieldData;
}
