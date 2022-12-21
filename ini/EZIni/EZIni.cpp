//#include "stdafx.h"
#include "EZIni.h"
#include <algorithm>
#include <Shlwapi.h>

using std::deque;

// EZIni::Section class
EZIni::Section::Section(LPCTSTR lpFileName, LPCTSTR lpSectionName)
:	m_sFileName(lpFileName),
	m_sSectionName(lpSectionName)
{	
}

EZIni::Key EZIni::Section::operator[](LPCSTR lpKeyName)
{
	return Key(m_sFileName, m_sSectionName, CA2T(lpKeyName));
}

EZIni::Key EZIni::Section::operator[](LPCWSTR lpKeyName)
{
	return Key(m_sFileName, m_sSectionName, CW2T(lpKeyName));
}

BOOL EZIni::Section::Exists() const
{	
	std::deque<CString> listOfSections;

	EZIni dummy(m_sFileName);
	dummy.EnumSectionNames(listOfSections);

	return std::find(listOfSections.begin(), listOfSections.end(), m_sSectionName) != listOfSections.end();
}

BOOL EZIni::Section::Delete()
{
	return WritePrivateProfileString(m_sSectionName, NULL, _T(""), m_sFileName);
}
/*
BOOL EZIni::Section::Rename(CString sNew)
{

}
*/
BOOL EZIni::Section::EnumKeyNames(std::deque<CString>& listOfKeyNames) const
{
	if (!PathFileExists(m_sFileName))
		return FALSE;

	listOfKeyNames.clear();

	DWORD dwLen = READ_BUFFER_SIZE;
	LPTSTR pBuffer = new TCHAR[dwLen + 1];
	DWORD dwCopied = GetPrivateProfileSection(m_sSectionName, pBuffer, dwLen, m_sFileName);
	while (dwCopied + 2 >= dwLen)
	{
		dwLen += READ_BUFFER_SIZE;
		delete [] pBuffer;
		pBuffer = new TCHAR[dwLen + 1];
		dwCopied = GetPrivateProfileSection(m_sSectionName, pBuffer, dwLen, m_sFileName);
	}	
	
	CString sLine, sKeyName;
	LPCTSTR pLines = pBuffer;
	while (*pLines)
	{
		sLine = pLines;
		AfxExtractSubString(sKeyName, pLines, 0, _T('='));
		sKeyName.Trim();

		listOfKeyNames.push_back(sKeyName);
		pLines += sLine.GetLength() + 1;
	}

	delete [] pBuffer;
	return TRUE;
}

// EzIni::Key class
EZIni::Key::Key(LPCTSTR lpFileName, LPCTSTR lpSectionName, LPCTSTR lpKeyName)
:	m_sFileName(lpFileName),
	m_sSectionName(lpSectionName),
	m_sKeyName(lpKeyName),	
	m_sDefaultValue(_T(""))
{	
	ZeroMemory(&m_nDefaultValue, sizeof(RECT));
}

EZIni::Key& EZIni::Key::operator=(int nValue)
{
	_SetKeyValue(nValue, _T("%d"));
	return *this;
}

EZIni::Key& EZIni::Key::operator=(UINT uValue)
{
	_SetKeyValue(uValue, _T("%u"));
	return *this;
}

EZIni::Key& EZIni::Key::operator=(INT64 n64Value)
{
	_SetKeyValue(n64Value, _T("%I64d"));
	return *this;
}

EZIni::Key& EZIni::Key::operator=(UINT64 u64Value)
{
	_SetKeyValue(u64Value, _T("%I64u"));
	return *this;
}

EZIni::Key& EZIni::Key::operator=(bool bValue)
{
	_SetKeyValue(bValue, _T("%d"));
	return *this;
}

EZIni::Key& EZIni::Key::operator=(double dValue)
{
	_SetKeyValue(dValue, _T("%f"));
	return *this;
}

EZIni::Key& EZIni::Key::operator=(LPCTSTR lpValue)
{
	_SetKeyValue(lpValue, _T("%s"));
	return *this;
}

EZIni::Key& EZIni::Key::operator=(POINT ptValue)
{
	_SetKeyValue(_Point2String(ptValue), _T("%s"));
	return *this;
}

EZIni::Key& EZIni::Key::operator=(LPCRECT lpRectValue)
{
	_SetKeyValue(_Rect2String(lpRectValue), _T("%s"));
	return *this;
}

EZIni::Key& EZIni::Key::operator=(const COleDateTime& dtValue)
{
	_SetKeyValue(_DateTime2String(dtValue), _T("%s"));
	return *this;
}

EZIni::Key::operator int()
{
	CString sValue = _GetKeyValue(m_nDefaultValue, _T("%d"));
	return _tcstol(sValue, NULL, 10);
}

EZIni::Key::operator UINT()
{
	CString sValue = _GetKeyValue(m_uDefaultValue, _T("%u"));
	return _tcstoul(sValue, NULL, 10);
}

EZIni::Key::operator INT64()
{
	CString sValue = _GetKeyValue(m_n64DefaultValue, _T("%I64d"));
	return _tcstoi64(sValue, NULL, 10);
}

EZIni::Key::operator UINT64()
{
	CString sValue = _GetKeyValue(m_u64DefaultValue, _T("%I64u"));
	return _tcstoui64(sValue, NULL, 10);
}

EZIni::Key::operator bool()
{
	CString sValue = _GetKeyValue(m_bDefaultValue, _T("%d"));
	return (_tcstol(sValue, NULL, 10)) == 0 ? false : true;
}

EZIni::Key::operator double()
{
	CString sValue = _GetKeyValue(m_dDefaultValue, _T("%f"));
	return _tstof(sValue);
}

EZIni::Key::operator CString()
{
	CString sValue = _GetKeyValue(m_sDefaultValue, _T("%s"));
	return sValue;
}

EZIni::Key::operator CPoint()
{
	CString sValue = _GetKeyValue(_Point2String(m_ptDefaultValue), _T("%s"));
	return _String2Point(sValue);
}

EZIni::Key::operator CRect()
{
	CString sValue = _GetKeyValue(_Rect2String(&m_rcDefaultValue), _T("%s"));
	return _String2Rect(sValue);
}

EZIni::Key::operator COleDateTime()
{
	CString sValue = _GetKeyValue(_DateTime2String(m_dtDefaultValue), _T("%s"));
	return _String2DateTime(sValue);
}

EZIni::Key& EZIni::Key::operator++()
{
	int nValue = *this;	
	*this = ++nValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator--()
{
	int nValue = *this;
	*this = --nValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator+=(int nValue)
{
	int nRead = *this;
	*this = nRead + nValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator-=(int nValue)
{
	int nRead = *this;
	*this = nRead - nValue;
	return *this;
}

EZIni::Key& EZIni::Key::operator*=(int nValue)
{
	int nRead = *this;
	*this = nRead * nValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator/=(int nValue)
{
	ASSERT(0 != nValue);

	int nRead = *this;
	*this = nRead / nValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator%=(int nValue)
{
	ASSERT(0 != nValue);

	int nRead = *this;
	*this = nRead % nValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator+=(UINT uValue)
{
	UINT uRead = *this;
	*this = uRead + uValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator-=(UINT uValue)
{
	UINT uRead = *this;
	*this = uRead - uValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator*=(UINT uValue)
{
	UINT uRead = *this;
	*this = uRead * uValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator/=(UINT uValue)
{
	ASSERT(0 != uValue);

	UINT uRead = *this;
	*this = uRead / uValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator%=(UINT uValue)
{
	ASSERT(0 != uValue);

	UINT uRead = *this;
	*this = uRead + uValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator<<=(UINT uValue)
{
	UINT uRead = *this;
	*this = uRead << uValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator>>=(UINT uValue)
{
	UINT uRead = *this;
	*this = uRead >> uValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator&=(UINT uValue)
{
	UINT uRead = *this;
	*this = uRead & uValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator|=(UINT uValue)
{
	UINT uRead = *this;
	*this = uRead | uValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator^=(UINT uValue)
{
	UINT uRead = *this;
	*this = uRead ^ uValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator+=(INT64 n64Value)
{
	INT64 n64Read = *this;
	*this = n64Read + n64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator-=(INT64 n64Value)
{
	INT64 n64Read = *this;
	*this = n64Read - n64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator*=(INT64 n64Value)
{
	INT64 n64Read = *this;
	*this = n64Read * n64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator/=(INT64 n64Value)
{
	ASSERT(0 != n64Value);

	INT64 n64Read = *this;
	*this = n64Read / n64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator%=(INT64 n64Value)
{
	ASSERT(0 != n64Value);

	INT64 n64Read = *this;
	*this = n64Read % n64Value;

	return *this;
}


EZIni::Key& EZIni::Key::operator+=(UINT64 u64Value)
{
	UINT64 u64Read = *this;
	*this = u64Read + u64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator-=(UINT64 u64Value)
{
	UINT64 u64Read = *this;
	*this = u64Read - u64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator*=(UINT64 u64Value)
{
	UINT64 u64Read = *this;
	*this = u64Read * u64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator/=(UINT64 u64Value)
{
	ASSERT(0 != u64Value);

	UINT64 u64Read = *this;
	*this = u64Read / u64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator%=(UINT64 u64Value)
{
	ASSERT(0 != u64Value);

	UINT64 u64Read = *this;
	*this = u64Read % u64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator<<=(UINT64 u64Value)
{
	UINT64 u64Read = *this;
	*this = u64Read << u64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator>>=(UINT64 u64Value)
{
	UINT64 u64Read = *this;
	*this = u64Read >> u64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator&=(UINT64 u64Value)
{
	UINT64 u64Read = *this;
	*this = u64Read & u64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator|=(UINT64 u64Value)
{
	UINT64 u64Read = *this;
	*this = u64Read | u64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator^=(UINT64 u64Value)
{
	UINT64 u64Read = *this;
	*this = u64Read ^ u64Value;

	return *this;
}

EZIni::Key& EZIni::Key::operator+=(double dValue)
{
	double dRead = *this;
	*this = dRead + dValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator-=(double dValue)
{
	double dRead = *this;
	*this = dRead - dValue;
	return *this;
}

EZIni::Key& EZIni::Key::operator*=(double dValue)
{
	double dRead = *this;
	*this = dRead * dValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator/=(double dValue)
{
	ASSERT(0.0 != dValue);

	double dRead = *this;
	*this = dRead / dValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator+=(LPCTSTR lpText)
{
	CString sRead = *this;
	*this = sRead + lpText;

	return *this;
}

EZIni::Key& EZIni::Key::operator+=(POINT ptValue)
{
	CString sRead, sTemp;
	int nDelimiter;

	sTemp = sRead = *this;	
	nDelimiter = sTemp.Remove(_T(','));

	if (1 == nDelimiter)
		*this = _String2Point(sRead) + ptValue;
	
	else if (3 == nDelimiter)
		*this = _String2Rect(sRead) + ptValue;
	
	else
		ASSERT(FALSE);

	return *this;
}

EZIni::Key& EZIni::Key::operator-=(POINT ptValue)
{
	*this += CPoint(-ptValue.x, -ptValue.y);
	return *this;
}

EZIni::Key& EZIni::Key::operator+=(SIZE sizeValue)
{
	CString sRead, sTemp;
	int nDelimiter;

	sTemp = sRead = *this;	
	nDelimiter = sTemp.Remove(_T(','));

	if (1 == nDelimiter)
		*this = _String2Point(sRead) + sizeValue;

	else if (3 == nDelimiter)
		*this = _String2Rect(sRead) + sizeValue;

	else
		ASSERT(FALSE);

	return *this;
}

EZIni::Key& EZIni::Key::operator-=(SIZE sizeValue)
{
	*this += CSize(-sizeValue.cx, -sizeValue.cy);
	return *this;
}

EZIni::Key& EZIni::Key::operator+=(LPCRECT lpRectValue)
{
	CRect rcRead = *this;
	*this = rcRead + lpRectValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator-=(LPCRECT lpRectValue)
{
	CRect rcRead = *this;
	*this = rcRead - lpRectValue;

	return *this;
}

EZIni::Key& EZIni::Key::operator+=(COleDateTimeSpan dateSpan)
{
	COleDateTime dtRead = *this;
	*this = dtRead + dateSpan;

	return *this;
}

EZIni::Key& EZIni::Key::operator-=(COleDateTimeSpan dateSpan)
{
	COleDateTime dtRead = *this;
	*this = dtRead - dateSpan;

	return *this;
}


EZIni::Key& EZIni::Key::operator<<(int nDefaultValue)
{
	m_nDefaultValue = nDefaultValue;

	if ( !Exists() )
	{
		EZIni dummy( m_sFileName );
		dummy[m_sSectionName][m_sKeyName] = m_nDefaultValue;
	}
	return *this;
}

EZIni::Key& EZIni::Key::operator<<(UINT uDefaultValue)
{
	m_uDefaultValue = uDefaultValue;
	if ( !Exists() )
	{
		EZIni dummy( m_sFileName );
		dummy[m_sSectionName][m_sKeyName] = m_uDefaultValue;
	}
	return *this;
}

EZIni::Key& EZIni::Key::operator<<(INT64 n64DefaultValue)
{
	m_n64DefaultValue = n64DefaultValue;
	if ( !Exists() )
	{
		EZIni dummy( m_sFileName );
		dummy[m_sSectionName][m_sKeyName] = m_n64DefaultValue;
	}
	return *this;
}

EZIni::Key& EZIni::Key::operator<<(UINT64 u64DefaultValue)
{
	m_u64DefaultValue = u64DefaultValue;
	if ( !Exists() )
	{
		EZIni dummy( m_sFileName );
		dummy[m_sSectionName][m_sKeyName] = m_u64DefaultValue;
	}
	return *this;
}

EZIni::Key& EZIni::Key::operator<<(bool bDefaultValue)
{
	m_bDefaultValue = bDefaultValue;
	//scpark 2017 08 29 14
	//해당 키값이 없을 경우 기본값을 가져오는 << 연산자를 만나면
	//해당 키 항목을 ini에 생성하도록 수정.
	//다른 데이터 타입의 << 연산자도 동일.
	if ( !Exists() )
	{
		EZIni dummy( m_sFileName );
		dummy[m_sSectionName][m_sKeyName] = m_bDefaultValue;
	}
	return *this;
}

EZIni::Key& EZIni::Key::operator<<(double dDefaultValue)
{
	m_dDefaultValue = dDefaultValue;
	if ( !Exists() )
	{
		EZIni dummy( m_sFileName );
		dummy[m_sSectionName][m_sKeyName] = m_dDefaultValue;
	}
	return *this;
}

EZIni::Key& EZIni::Key::operator<<(LPCTSTR lpDefaultValue)
{
	m_sDefaultValue = lpDefaultValue;
	if ( !Exists() )
	{
		EZIni dummy( m_sFileName );
		dummy[m_sSectionName][m_sKeyName] = m_sDefaultValue;
	}
	return *this;
}

EZIni::Key& EZIni::Key::operator<<(POINT ptDefaultValue)
{
	m_ptDefaultValue = ptDefaultValue;
	if ( !Exists() )
	{
		EZIni dummy( m_sFileName );
		dummy[m_sSectionName][m_sKeyName] = _Point2String(m_ptDefaultValue);
	}
	return *this;
}

EZIni::Key& EZIni::Key::operator<<(LPCRECT lpDefaultValue)
{
	m_rcDefaultValue = *lpDefaultValue;
	if ( !Exists() )
	{
		EZIni dummy( m_sFileName );
		dummy[m_sSectionName][m_sKeyName] = _Rect2String(&m_rcDefaultValue);
	}
	return *this;
}

EZIni::Key& EZIni::Key::operator<<(const COleDateTime& dtDefaultValue)
{
	m_dtDefaultValue = dtDefaultValue;
	if ( !Exists() )
	{
		EZIni dummy( m_sFileName );
		dummy[m_sSectionName][m_sKeyName] = _DateTime2String(m_dtDefaultValue);
	}
	return *this;
}

EZIni::Key& EZIni::Key::operator>>(int nDefaultValue)
{
	m_nDefaultValue = nDefaultValue;
	return *this;
}

EZIni::Key& EZIni::Key::operator>>(UINT uDefaultValue)
{
	m_uDefaultValue = uDefaultValue;
	return *this;
}

EZIni::Key& EZIni::Key::operator>>(INT64 n64DefaultValue)
{
	m_n64DefaultValue = n64DefaultValue;
	return *this;
}

EZIni::Key& EZIni::Key::operator>>(UINT64 u64DefaultValue)
{
	m_u64DefaultValue = u64DefaultValue;
	return *this;
}

EZIni::Key& EZIni::Key::operator>>(bool bDefaultValue)
{
	m_bDefaultValue = bDefaultValue;
	return *this;
}

EZIni::Key& EZIni::Key::operator>>(double dDefaultValue)
{
	m_dDefaultValue = dDefaultValue;
	return *this;
}

EZIni::Key& EZIni::Key::operator>>(LPCTSTR lpDefaultValue)
{
	m_sDefaultValue = lpDefaultValue;
	return *this;
}

EZIni::Key& EZIni::Key::operator>>(POINT ptDefaultValue)
{
	m_ptDefaultValue = ptDefaultValue;
	return *this;
}

EZIni::Key& EZIni::Key::operator>>(LPCRECT lpDefaultValue)
{
	m_rcDefaultValue = *lpDefaultValue;
	return *this;
}

EZIni::Key& EZIni::Key::operator>>(const COleDateTime& dtDefaultValue)
{
	m_dtDefaultValue = dtDefaultValue;
	return *this;
}

BOOL EZIni::Key::Exists() const
{
	std::deque<CString> listOfKeyNames;
	deque <CString>::iterator Iter;

	EZIni dummy(m_sFileName);
	dummy[m_sSectionName].EnumKeyNames(listOfKeyNames);	

	if ( listOfKeyNames.size() == 0 )
		return false;
	
	bool	bFound = false;
	CString str;
	CString sKey = m_sKeyName;
	
	sKey.MakeLower();

	for ( Iter = listOfKeyNames.begin(); Iter != listOfKeyNames.end(); Iter++ )
	{
		str = *Iter;
		str.MakeLower();

		if ( str == sKey )
		{
			bFound = true;
			break;
		}
	}

	return bFound;

//	return std::find(listOfKeyNames.begin(), listOfKeyNames.end(), m_sKeyName) != listOfKeyNames.end();
}

BOOL EZIni::Key::Delete()
{
	return WritePrivateProfileString(m_sSectionName, m_sKeyName, NULL, m_sFileName);
}

CString EZIni::Key::_Point2String(const POINT& ptValue)
{
	CString sResult;
	sResult.Format(_T("%d,%d"), ptValue.x, ptValue.y);
	return sResult;
}

CString EZIni::Key::_Rect2String(LPCRECT lpRectValue)
{
	CString sResult;
	sResult.Format(_T("%d,%d,%d,%d"), lpRectValue->left, lpRectValue->top, lpRectValue->right, lpRectValue->bottom);
	return sResult;
}

CString EZIni::Key::_DateTime2String(const COleDateTime& dtValue)
{
	return dtValue.Format(_T("%Y-%m-%d %H:%M:%S"));
}

CPoint EZIni::Key::_String2Point(LPCTSTR lpValue)
{
	CString sPoint[2];
	for (int i = 0; i < 2; ++i)
		AfxExtractSubString(sPoint[i], lpValue, i, _T(','));

	return CPoint(_tstoi(sPoint[0]), _tstoi(sPoint[1]));
}

CRect EZIni::Key::_String2Rect(LPCTSTR lpValue)
{
	CString sRect[4];
	for (int i = 0; i < 4; ++i)
		AfxExtractSubString(sRect[i], lpValue, i, _T(','));

	return CRect(_tstoi(sRect[0]), _tstoi(sRect[1]), _tstoi(sRect[2]), _tstoi(sRect[3]));
}

COleDateTime EZIni::Key::_String2DateTime(LPCTSTR lpValue)
{
	COleDateTime dtResult;
	bool bParsed = dtResult.ParseDateTime(lpValue);
	ASSERT(bParsed);
	return dtResult;
}

// EZIni class
EZIni::EZIni(void)
{
}

EZIni::EZIni(LPCTSTR lpFileName, BOOL bCreateAsUnicode /*= FALSE*/)
{
	SetFileName(lpFileName, bCreateAsUnicode);
}

EZIni::~EZIni(void)
{
}

EZIni::Section EZIni::operator[](LPCSTR lpSectionName)
{
	return Section(m_sFileName, CA2T(lpSectionName));
}

EZIni::Section EZIni::operator[](LPCWSTR lpSectionName)
{
	return Section(m_sFileName, CW2T(lpSectionName));
}

EZIni::Section EZIni::getSection(LPCSTR lpSectionName)
{
	return Section(m_sFileName, CA2T(lpSectionName));
}

EZIni::Section EZIni::getSection(LPCWSTR lpSectionName)
{
	return Section(m_sFileName, CW2T(lpSectionName));
}

void EZIni::SetFileName(LPCTSTR lpFileName, BOOL bCreateAsUnicode /*= FALSE*/)
{
	m_sFileName = lpFileName;
	if (bCreateAsUnicode && !PathFileExists(lpFileName))
	{
		WORD BOM = 0xFEFF;
		HANDLE hFile = CreateFile(lpFileName, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE != hFile)
		{
			WriteFile(hFile, &BOM, sizeof(WORD), NULL, NULL);
			CloseHandle(hFile);
		}
	}
}

BOOL EZIni::EnumSectionNames(std::deque<CString>& listOfSectionNames) const
{
	if (!PathFileExists(m_sFileName))
		return FALSE;

	listOfSectionNames.clear();

	DWORD dwLen = READ_BUFFER_SIZE;
	LPTSTR pBuffer = new TCHAR[dwLen + 1];
	DWORD dwCopied = GetPrivateProfileSectionNames(pBuffer, dwLen, m_sFileName);
	while (dwCopied + 2 >= dwLen)
	{
		dwLen += READ_BUFFER_SIZE;
		delete [] pBuffer;
		pBuffer = new TCHAR[dwLen + 1];
		dwCopied = GetPrivateProfileSectionNames(pBuffer, dwLen, m_sFileName);
	}	
	
	LPCTSTR pSections = pBuffer;
	while (*pSections)
	{
		listOfSectionNames.push_back(pSections);
		pSections += listOfSectionNames.back().GetLength() + 1;
	}

	delete [] pBuffer;
	return TRUE;
}

CString EZIni::_GetErrorMessage()
{		
	LPVOID lpMessage;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |	FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(),	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMessage, 0, NULL);

	CString sResult((LPTSTR)lpMessage);
	LocalFree(lpMessage);
	sResult.TrimRight(_T(" \t\r\n"));

	return sResult;
}

//all : true(section+key), false(section only)
//method : true(print), false(TRACE)
void EZIni::printList(bool all, bool method)
{
	// Print all sections
	std::deque<CString> listOfSectionNames;
	std::deque<CString> listOfKeyNames;

	EnumSectionNames(listOfSectionNames);

	TRACE(_T("===List of Section names===\n"));
	for (auto itS = listOfSectionNames.begin(); itS != listOfSectionNames.end(); ++itS)
	{
		TRACE(_T("[%s]\n"), *itS);
		
		// Print all keys in Primary Section
		getSection(*itS).EnumKeyNames(listOfKeyNames);

		TRACE(_T("\nList of Key names(Primary Section)\n"));
		for (auto itK = listOfKeyNames.begin(); itK != listOfKeyNames.end(); ++itK)
			TRACE(_T("%s\n"), *itK);
	}
}

