/********************************************************************************************************************************************
	������		dogseller
---------------------------------------------------------------------------------------------------------------------------------------------
	�����Ͻ�		2001-10-01 ~
---------------------------------------------------------------------------------------------------------------------------------------------
	���ϼ���		DB�� �����ϰ� SELECT, UPDATE, INSERT, DELETE�� �����Ѵ�.
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
�Լ���		DBConnect(CString strConnectionString)
---------------------------------------------------------------------------------------------------------------------------------------------
�Լ�����	DB�� �����Ѵ�.
---------------------------------------------------------------------------------------------------------------------------------------------
�������	CString strConnectionString	=>	ADO�� �����ϱ� ���� Connect String
---------------------------------------------------------------------------------------------------------------------------------------------
���ϰ�		BOOL						=>	TURE : ����
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
		TRACE("*** DOG CHECK [ADO DB] => <DBConnect ����> �޽���[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <DBConnect ����> ����[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <DBConnect ����> �ҽ�[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	return TRUE;
}

/********************************************************************************************************************************************
�Լ���		DBDisConnect()
---------------------------------------------------------------------------------------------------------------------------------------------
�Լ�����	���ӵ� DB�� ������ ���´�.
---------------------------------------------------------------------------------------------------------------------------------------------
�������	����
---------------------------------------------------------------------------------------------------------------------------------------------
���ϰ�		BOOL			=>	TURE : ����
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
		TRACE("*** DOG CHECK [ADO DB] => <DBDisConnect ����> �޽���[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <DBDisConnect ����> ����[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <DBDisConnect ����> �ҽ�[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	::CoUninitialize();
	return TRUE;
}

/********************************************************************************************************************************************
�Լ���		ExecuteQuery(CString strSQL)
---------------------------------------------------------------------------------------------------------------------------------------------
�Լ�����	INSERT, UPDATE, DELETE �Ѵ�.
---------------------------------------------------------------------------------------------------------------------------------------------
�������	CString strSQL	=>	QUERY ����
---------------------------------------------------------------------------------------------------------------------------------------------
���ϰ�		BOOL			=>	TURE : ����
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
		TRACE("*** DOG CHECK [ADO DB] => <ExecuteQuery ����> �޽���[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <ExecuteQuery ����> ����[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <ExecuteQuery ����> �ҽ�[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	return TRUE;
}

/********************************************************************************************************************************************
�Լ���		OpenQuery(CString strSQL)
---------------------------------------------------------------------------------------------------------------------------------------------
�Լ�����	SELECT �Ѵ�.
---------------------------------------------------------------------------------------------------------------------------------------------
�������	CString strSQL	=>	QUERY ����
---------------------------------------------------------------------------------------------------------------------------------------------
���ϰ�		BOOL			=>	TURE : ����
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
		TRACE("*** DOG CHECK [ADO DB] => <OpenQuery ����> �޽���[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <OpenQuery ����> ����[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <OpenQuery ����> �ҽ�[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	return TRUE;
}

/********************************************************************************************************************************************
�Լ���		GetRecordCount(LONG * lRecordCount)
---------------------------------------------------------------------------------------------------------------------------------------------
�Լ�����	SELECT�� ReocordSet�� Record Count�� ����.
---------------------------------------------------------------------------------------------------------------------------------------------
�������	LONG * lRecordCount	=>	�� RecordSet�� Record Count
---------------------------------------------------------------------------------------------------------------------------------------------
���ϰ�		BOOL				=>	TURE : ��� ����
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
		TRACE("*** DOG CHECK [ADO DB] => <GetRecordCount ����> �޽���[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <GetRecordCount ����> ����[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <GetRecordCount ����> �ҽ�[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	return TRUE;
}


/********************************************************************************************************************************************
�Լ���		GetFieldValue(LONG lRecordNO, CString strName, CString *strValue)
---------------------------------------------------------------------------------------------------------------------------------------------
�Լ�����	SELECT�� �� Field�� ����Ÿ�� ���´�.
---------------------------------------------------------------------------------------------------------------------------------------------
�������	LONG lRecordNO		=>	���� SELECT�� RecordSet�� Record ��ȣ
			CString strName		=>	���� SELECT�� RecordSet�� Filed �̸�
			CString *strValue	=>	���� SELECT�� RecordSet�� Filed ����Ÿ
---------------------------------------------------------------------------------------------------------------------------------------------
���ϰ�		BOOL				=>	TURE : ��� ����
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
			TRACE("*** DOG CHECK [ADO DB] => <Field���� �������> !! ***\n");
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
			TRACE("Field���� ������ �� �����ϴ�.\n");
			return FALSE;
		}
	}
	catch(_com_error &e)
	{
		TRACE("*** DOG CHECK [ADO DB] => <GetFieldValue ����> �޽���[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <GetFieldValue ����> ����[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <GetFieldValue ����> �ҽ�[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	return TRUE;
}

/********************************************************************************************************************************************
�Լ���		SetFieldList()
---------------------------------------------------------------------------------------------------------------------------------------------
�Լ�����	SELECT�� �� Field�� �̸��� List�� �����Ѵ�.
---------------------------------------------------------------------------------------------------------------------------------------------
�������	����
---------------------------------------------------------------------------------------------------------------------------------------------
���ϰ�		BOOL			=>	TURE : ���� ����
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
		TRACE("*** DOG CHECK [ADO DB] => <SetFieldList ����> �޽���[%s] !! ***\n", e.ErrorMessage());
		TRACE("*** DOG CHECK [ADO DB] => <SetFieldList ����> ����[%s] !! ***\n", (LPCSTR)e.Description());
		TRACE("*** DOG CHECK [ADO DB] => <SetFieldList ����> �ҽ�[%s] !! ***\n", (LPCSTR)e.Source());
		return FALSE;
	}	
	return TRUE;

}

/********************************************************************************************************************************************
�Լ���		ViewFieldNameList()
---------------------------------------------------------------------------------------------------------------------------------------------
�Լ�����	SELECT�� �� Field�� �̸��� Ȯ���Ҷ� ���ȴ�. 
---------------------------------------------------------------------------------------------------------------------------------------------
�������	����
---------------------------------------------------------------------------------------------------------------------------------------------
���ϰ�		����
********************************************************************************************************************************************/
void CDogADODB::ViewFieldNameList()
{
	int nIndex;
	CString strField;

	for(nIndex = 0; nIndex < m_FieldList.GetSize(); nIndex++)
	{
		TRACE("*** DOG CHECK [ADO DB] => <Field�� ����> Field[%s] !! ***\n", m_FieldList.GetAt(nIndex));
	}
}

/********************************************************************************************************************************************
�Լ���		GetFieldNO(CString strName, LONG * lFieldNO)
---------------------------------------------------------------------------------------------------------------------------------------------
�Լ�����	ADO������ �� Field�� ������ Filed�� ����Ÿ�� ������ ����. ��ȣ�θ� ����(?)
			�׷��� �� Filed������ ���� ��ȣ�� ���� ���� �Լ���.
---------------------------------------------------------------------------------------------------------------------------------------------
�������	CString strName	=>	SELECT���� �� Filed�� �̸�
			LONG * lFieldNO	=>	�� ���� Field�� ��ȣ
---------------------------------------------------------------------------------------------------------------------------------------------
���ϰ�		BOOL			=>	TURE : ��� ����
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
�Լ���		ConvertValue(_variant_t vtValue, CString *strValue)
---------------------------------------------------------------------------------------------------------------------------------------------
�Լ�����	ADO���� SELECT�� �� Filed�� ����Ÿ�� CString������ �����Ѵ�.
---------------------------------------------------------------------------------------------------------------------------------------------
�������	_variant_t vtValue	=>	SELECT���� �� Field�� ����Ÿ�� �� ����Ÿ ��
			CString *strValue	=>	����� ����Ÿ
---------------------------------------------------------------------------------------------------------------------------------------------
���ϰ�		BOOL				=>	TURE : ���� ����
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
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_UI1 !! ***\n");
	}
	else if(vtValue.vt == VT_UI2)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_UI2 !! ***\n");
	}
	else if(vtValue.vt == VT_UI4)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_UI4 !! ***\n");
	}
	else if(vtValue.vt == VT_UINT)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_UINT !! ***\n");
	}
	else if(vtValue.vt == VT_INT)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_INT !! ***\n");
	}
	else if(vtValue.vt == VT_I1)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_I1 !! ***\n");
	}
	else if(vtValue.vt == VT_I2)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_I2 !! ***\n");
	}
	else if(vtValue.vt == VT_I4)
	{
		(*strValue).Format("%d", vtValue.lVal);
		return TRUE;
	}
	else if(vtValue.vt == VT_R4)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_R4 !! ***\n");
	}
	else if(vtValue.vt == VT_R8)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_R8 !! ***\n");
	}
	else if(vtValue.vt == VT_CY)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_CY !! ***\n");
	}
	else if(vtValue.vt == VT_ERROR)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_ERROR !! ***\n");
	}
	else if(vtValue.vt == VT_BOOL)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_BOOL !! ***\n");
	}
	else if(vtValue.vt == VT_DISPATCH)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_DISPATCH !! ***\n");
	}
	else if(vtValue.vt == VT_VARIANT)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_VARIANT !! ***\n");
	}
	else if(vtValue.vt == VT_UNKNOWN)
	{
		TRACE("*** DOG CHECK [ADO DB] => <����Ÿ��> VT_UNKNOWN !! ***\n");
	}
	return FALSE;
}

/********************************************************************************************************************************************
�Լ���		ConvertDBField(CString strFieldData)
---------------------------------------------------------------------------------------------------------------------------------------------
�Լ�����	Query���� �������� ���ڸ� ������ ���ڷ� �����Ѵ�.
			1) ' ����
---------------------------------------------------------------------------------------------------------------------------------------------
�������	CString strFieldData	=>	Query���� �������� ���ڸ� �������ش�.
---------------------------------------------------------------------------------------------------------------------------------------------
���ϰ�		CString					=>	����� ����Ÿ�� �����Ѵ�.
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
