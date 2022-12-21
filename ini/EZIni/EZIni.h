/**
* Copyright (C) 2013 by Kyung-jin Kim
* e-mail		: devmachine@naver.com
*
*
* Description	: Easy Ini Helper Class
* Created		: Sep-5,2013
* Last Updated	: Jul-15,2014
* Version		: EZIni v1.2
*/

/**
* Sample code

************ Write Operation ************
try
{
	EZIni ini(_T("D:\\ServerInfo.ini"));

	ini["Primary"]["IP"] = _T("192.168.0.1");
	ini["Primary"]["Port"] = 5678;
	ini["Primary"]["Value"] = 89.97;
	ini["Primary"]["HasSecondary"] = true;
	ini["Primary"]["LastAccess"] = COleDateTime::GetCurrentTime();		

	EZIni::Section section = ini["Secondary"];
	section["IP"] = _T("192.168.0.2");
	section["Port"] = 6789;

	EZIni::Key key = section["Value"];
	key = 78.56;
	key = 90.12;
}
catch (EZIni::Exception& e)
{
	TRACE(_T("Ini Error: %s\n"), e.GetErrorMessage());
}

************ Read Operation ************
EZIni ini(_T("D:\\ServerInfo.ini"));

CString sIP = ini["Primary"]["IP"];
int nPort = ini["Primary"]["Port"];
double dValue = ini["Primary"]["Value"];
bool bHasSecondary = ini["Primary"]["HasSecondary"];
COleDateTime dtLastAccess = ini["Primary"]["LastAccess"];

TRACE(_T("Primary Server Info\n"));
TRACE(_T("IP: %s, Port: %d, Value: %.2f\n"), sIP, nPort, dValue);
TRACE(_T("HasSecondary: %s, LastAccess: %s\n\n"), 
	  bHasSecondary ? _T("true") : _T("false"), dtLastAccess.Format(_T("%Y-%m-%d %H:%M:%S")));

// Use default value if it doesn't exist;
sIP = ini["Tertiary"]["IP"] << _T("192.168.0.3");
nPort = ini["Tertiary"]["Port"] << 7890;
dValue = ini["Tertiary"]["Value"] << 12.79;

TRACE(_T("Tertiary Server Info\n"));
TRACE(_T("IP: %s, Port: %d, Value: %.2f\n"), sIP, nPort, dValue);

<<<< 주의!!! >>>>
str.Format( "%s", ini["Tertiary"]["IP"] ); => invalid!
str.Format( "%s", CString(ini["Tertiary"]["IP"]) ); => valid.
=> 값을 변수로 받아올때는 잘되지만 형식지정자로 받아올때는 버그가 있는듯하다.


************ Arithmetic/Logical Operation ************
try
{
	EZIni ini(_T("D:\\ServerInfo.ini"));

	ini["Primary"]["IP"] += _T(":5678");

	++ini["Primary"]["Port"];
	--ini["Primary"]["Port"];

	ini["Primary"]["Value"] *= 10.0;
	ini["Primary"]["LastAccess"] += COleDateTimeSpan(1, 0, 0, 0);
}
catch (EZIni::Exception& e)
{
	TRACE(_T("Ini Error: %s\n"), e.GetErrorMessage());
}

************ Helper Functions ************
EZIni ini(_T("D:\\ServerInfo.ini"));

// Print all sections
std::deque<CString> listOfSectionNames;
ini.EnumSectionNames(listOfSectionNames);

TRACE(_T("List of Section names\n"));
for (auto it = listOfSectionNames.begin(); it != listOfSectionNames.end(); ++it)
	TRACE(_T("%s\n"), *it);

// Print all keys in Primary Section
std::deque<CString> listOfKeyNames;
ini["Primary"].EnumKeyNames(listOfKeyNames);

TRACE(_T("\nList of Key names(Primary Section)\n"));
for (auto it = listOfKeyNames.begin(); it != listOfKeyNames.end(); ++it)
	TRACE(_T("%s\n"), *it);

// Delete key if it exists
if (ini["Secondary"]["IP"].Exists())
	ini["Secondary"]["IP"].Delete();

// Delete section if it exists
if (ini["Secondary"].Exists())
	ini["Secondary"].Delete();

************ Unicode Support ************
EZIni ini(_T("D:\\ServerInfo.ini"), TRUE);
*/

#pragma once

#include <afxwin.h>
#include <deque>
#include <ATLComTime.h>

class EZIni
{
public:
	class Exception
	{
	public:
		Exception(DWORD dwErrorCode, LPCTSTR lpErrorMessage) 
			: m_dwErrorCode(dwErrorCode), m_sErrorMessage(lpErrorMessage) {}

		CString GetErrorMessage() const { return m_sErrorMessage; }
		DWORD GetErrorCode() const { return m_dwErrorCode; }

	private:
		DWORD m_dwErrorCode;
		CString m_sErrorMessage;
	};

	class Key;
	class Section
	{
		friend class EZIni;

	public:
		// Get Key Object
		Key operator[](LPCSTR lpKeyName);
		Key operator[](LPCWSTR lpKeyName);

		// Ini Section Helper function
		BOOL Exists() const;
		BOOL Delete();
		//BOOL Rename(CString sNew);
		BOOL EnumKeyNames(std::deque<CString>& listOfKeyNames) const;

	private:
		Section(LPCTSTR lpFileName, LPCTSTR lpSectionName);

	private:
		CString m_sFileName;
		CString m_sSectionName;
	};

	class Key
	{
		friend class EZIni;
		friend class EZIni::Section;

	public:
		// Write Operation
		Key& operator=(int nValue);
		Key& operator=(UINT uValue);
		Key& operator=(INT64 n64Value);
		Key& operator=(UINT64 u64Value);
		Key& operator=(bool bValue);
		Key& operator=(double dValue);
		Key& operator=(LPCTSTR lpValue);
		Key& operator=(POINT ptValue);
		Key& operator=(LPCRECT lpRectValue);
		Key& operator=(const COleDateTime& dtValue);

		// Read Operation
		operator int();
		operator UINT();
		operator INT64();
		operator UINT64();
		operator bool();
		operator double();
		operator CString();
		operator CPoint();
		operator CRect();
		operator COleDateTime();
		
		// Arithmetic/Logical Operation
		Key& operator++();
		Key& operator--();
		Key& operator+=(int nValue);
		Key& operator-=(int nValue);
		Key& operator*=(int nValue);
		Key& operator/=(int nValue);
		Key& operator%=(int nValue);

		Key& operator+=(UINT uValue);
		Key& operator-=(UINT uValue);
		Key& operator*=(UINT uValue);
		Key& operator/=(UINT uValue);
		Key& operator%=(UINT uValue);
		Key& operator<<=(UINT uValue);
		Key& operator>>=(UINT uValue);
		Key& operator&=(UINT uValue);
		Key& operator|=(UINT uValue);
		Key& operator^=(UINT uValue);

		Key& operator+=(INT64 n64Value);
		Key& operator-=(INT64 n64Value);
		Key& operator*=(INT64 n64Value);
		Key& operator/=(INT64 n64Value);
		Key& operator%=(INT64 n64Value);

		Key& operator+=(UINT64 u64Value);
		Key& operator-=(UINT64 u64Value);
		Key& operator*=(UINT64 u64Value);
		Key& operator/=(UINT64 u64Value);
		Key& operator%=(UINT64 u64Value);
		Key& operator<<=(UINT64 u64Value);
		Key& operator>>=(UINT64 u64Value);
		Key& operator&=(UINT64 u64Value);
		Key& operator|=(UINT64 u64Value);
		Key& operator^=(UINT64 u64Value);

		Key& operator+=(double dValue);
		Key& operator-=(double dValue);
		Key& operator*=(double dValue);
		Key& operator/=(double dValue);

		Key& operator+=(POINT ptValue);
		Key& operator-=(POINT ptValue);
		Key& operator+=(SIZE sizeValue);
		Key& operator-=(SIZE sizeValue);
		Key& operator+=(LPCRECT lpRectValue);
		Key& operator-=(LPCRECT lpRectValue);
		Key& operator+=(COleDateTimeSpan dateSpan);
		Key& operator-=(COleDateTimeSpan dateSpan);

		// Append Operation
		Key& operator+=(LPCTSTR lpText);

		// Set Default Value
		Key& operator<<(int nDefaultValue);
		Key& operator<<(UINT uDefaultValue);
		Key& operator<<(INT64 n64DefaultValue);
		Key& operator<<(UINT64 u64DefaultValue);
		Key& operator<<(bool bDefaultValue);
		Key& operator<<(double dDefaultValue);
		Key& operator<<(LPCTSTR lpDefaultValue);
		Key& operator<<(POINT ptDefaultValue);
		Key& operator<<(LPCRECT lpDefaultValue);
		Key& operator<<(const COleDateTime& dtDefaultValue);

		Key& operator>>(int nDefaultValue);
		Key& operator>>(UINT uDefaultValue);
		Key& operator>>(INT64 n64DefaultValue);
		Key& operator>>(UINT64 u64DefaultValue);
		Key& operator>>(bool bDefaultValue);
		Key& operator>>(double dDefaultValue);
		Key& operator>>(LPCTSTR lpDefaultValue);
		Key& operator>>(POINT ptDefaultValue);
		Key& operator>>(LPCRECT lpDefaultValue);
		Key& operator>>(const COleDateTime& dtDefaultValue);

		// Ini Key Helper function
		BOOL Exists() const;
		BOOL Delete();

	private:
		Key(LPCTSTR lpFileName, LPCTSTR lpSectionName, LPCTSTR lpKeyName);

		template <typename _T>
		void _SetKeyValue(_T value, LPCTSTR lpFormatSpec);
		template <typename _T>
		CString _GetKeyValue(const _T& defaultValue, LPCTSTR lpFormatSpec);

		CString _Point2String(const POINT& ptValue);
		CString _Rect2String(LPCRECT lpRectValue);
		CString _DateTime2String(const COleDateTime& dtValue);

		CPoint _String2Point(LPCTSTR lpValue);
		CRect _String2Rect(LPCTSTR lpValue);
		COleDateTime _String2DateTime(LPCTSTR lpValue);

	private:
		CString m_sFileName;
		CString m_sSectionName;
		CString m_sKeyName;
		
		CString m_sDefaultValue;
		union
		{
			int m_nDefaultValue;
			UINT m_uDefaultValue;
			INT64 m_n64DefaultValue;
			UINT64 m_u64DefaultValue;
			bool m_bDefaultValue;
			double m_dDefaultValue;
			POINT m_ptDefaultValue;
			RECT m_rcDefaultValue;
			DATE m_dtDefaultValue;
		};
	};	

	EZIni(void);
	EZIni(LPCTSTR lpFileName, BOOL bCreateAsUnicode = FALSE);
	~EZIni(void);

	// Get Section Object
	Section operator[](LPCSTR lpSectionName);
	Section operator[](LPCWSTR lpSectionName);
	Section getSection(LPCSTR lpSectionName);
	Section getSection(LPCWSTR lpSectionName);

	// Initialize Function
	void SetFileName(LPCTSTR lpFileName, BOOL bCreateAsUnicode = FALSE);
	CString	GetFileName() { return m_sFileName; }

	// Ini Helper Function
	BOOL EnumSectionNames(std::deque<CString>& listOfSectionNames) const;

	//all : true(section+key), false(section only)
	//method : true(print), false(TRACE)
	void printList(bool all = false, bool method = false);

private:
	enum { READ_BUFFER_SIZE = 512 };

	static CString _GetErrorMessage();

private:
	CString m_sFileName;
};

template <typename _T>
void EZIni::Key::_SetKeyValue(_T value, LPCTSTR lpFormatSpec)
{
	ASSERT(!m_sFileName.IsEmpty() && !m_sSectionName.IsEmpty() && !m_sKeyName.IsEmpty());

	EZIni dummy( m_sFileName );

	while ( dummy[m_sSectionName][m_sKeyName].Exists() )
		dummy[m_sSectionName][m_sKeyName].Delete();

	CString sValue;
	sValue.Format(lpFormatSpec, value);
	BOOL bResult = WritePrivateProfileString(m_sSectionName, m_sKeyName, sValue, m_sFileName);
	if (!bResult)
	{		
		CString sErrMessage, sFormat;
		sFormat.Format(_T("Failed to write value(%s)."), lpFormatSpec);
		sFormat += _T(" %s");
		sErrMessage.Format(sFormat, value, EZIni::_GetErrorMessage());
		throw EZIni::Exception(GetLastError(), sErrMessage);
	}
}

template <typename _T>
CString EZIni::Key::_GetKeyValue(const _T& defaultValue, LPCTSTR lpFormatSpec)
{
	CString sDefaultValue;
	sDefaultValue.Format(lpFormatSpec, defaultValue);

	DWORD dwLen = READ_BUFFER_SIZE;
	LPTSTR pBuffer = new TCHAR[dwLen + 1];

	DWORD dwCopied = GetPrivateProfileString(m_sSectionName, m_sKeyName, sDefaultValue, pBuffer, dwLen, m_sFileName);
	while (dwCopied + 1 >= dwLen)
	{		
		dwLen += READ_BUFFER_SIZE;
		delete [] pBuffer;
		pBuffer = new TCHAR[dwLen + 1];
		dwCopied = GetPrivateProfileString(m_sSectionName, m_sKeyName, sDefaultValue, pBuffer, dwLen, m_sFileName);
	}

	CString sValue(pBuffer);
	delete [] pBuffer;

	return sValue;
}