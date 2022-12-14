// WQL.h: interface for the CWQL class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WQL_H__6344BB32_181B_4281_AA54_583B29EB1F1F__INCLUDED_)
#define AFX_WQL_H__6344BB32_181B_4281_AA54_583B29EB1F1F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include <map>
#include <string>

#ifndef _WIN32_DCOM
#define _WIN32_DCOM
#endif

#define MAX_INSTANCE	32		//한번의 쿼리로 가져올 레코드 셋의 최대 크기
#define WMI_LOCALHOST	0
#define WMI_DEFAULTNAME	0
#define WMI_DEFAULTPW	0	

#pragma comment( lib, "wbemuuid.lib" )
#pragma comment( lib, "comsupp.lib" )

#include <comdef.h>
#include <wbemidl.h>

class CWQL
{
public:	
	typedef std::map< std::wstring, std::wstring >	Row;			// Row 쿼리를 실행한 결과 Row
	typedef std::vector< Row >						RowSet;			// 여러개의 Row를 결과로 내는 쿼리의 RowSet

private:
	IWbemServices*		m_pWmiServices;
	IWbemLocator*		m_pWmiLocator;
	bool				m_bIsInit;
public:
	CWQL();
	~CWQL();

	bool connect(const wchar_t *host, const wchar_t* pszName, const wchar_t* pszPwd);
	void close();
	bool runWql( const wchar_t* query, CWQL::RowSet& rs );
	bool getClassProperties( const wchar_t * wmiClass, CWQL::RowSet& rs );

};

#endif // !defined(AFX_WQL_H__6344BB32_181B_4281_AA54_583B29EB1F1F__INCLUDED_)
