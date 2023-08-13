// ShellList.h: interface for the CShellImageList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHELLLIST_H__E0B73BFC_2F7C_44D4_BC38_1FA6A74CBD31__INCLUDED_)
#define AFX_SHELLLIST_H__E0B73BFC_2F7C_44D4_BC38_1FA6A74CBD31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxwin.h>
#include <map>

class CShellImageList  
{
public:
	CImageList m_imagelist_small; // Imagelist for small icons
	CImageList m_imagelist_large; // Imagelist for large icons

	CShellImageList();
	virtual ~CShellImageList();

	void Initialize();
	int  FindCache(CString szFileExt);
	void InsertCache(CString szFileExt, int ID);
	
	int  GetImageListIcon(CString szPath, CString szFile);
	void GetSystemDisplayName(CString szFile, LPSTR szDisplayName);
	int  GetVirtualImageListIcon(CString szExt);

	int  GetSystemImageListIcon(CString szFile, BOOL bDrive = TRUE);
	int  GetSystemImageListIcon(int csidl, BOOL bDrive = TRUE);

	//내 PC, 문서, 바탕 화면 등의 정해진 문자열을 저장(다국어 지원시에 특히 필요)
	//Shlobj.h에 정의된 CSIDL_로 시작되는 값을 동일하게 이용
	void set_shell_known_string(int csidl, CString str);

	//OS언어에 따라, 윈도우 버전에 따라 "내 컴퓨터"는 "내 PC", "내 컴퓨터", "My Computer" 등등 다양하므로 csidl로 구분한다.
	int get_csidl_by_shell_known_string(CString str);
	CString get_shell_known_string_by_csidl(int csidl);
	std::map<int, CString>* get_csidl_map();
	std::map<TCHAR, CString>* get_drive_map() { return &m_drive_map; }

private:
	CStringArray m_ExtArray;
	CUIntArray   m_IDArray;
	std::map<int, CString> m_csidl_map;
	std::map<TCHAR, CString> m_drive_map;
	int m_osType;
};

#endif // !defined(AFX_SHELLLIST_H__E0B73BFC_2F7C_44D4_BC38_1FA6A74CBD31__INCLUDED_)
