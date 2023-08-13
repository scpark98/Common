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

	//�� PC, ����, ���� ȭ�� ���� ������ ���ڿ��� ����(�ٱ��� �����ÿ� Ư�� �ʿ�)
	//Shlobj.h�� ���ǵ� CSIDL_�� ���۵Ǵ� ���� �����ϰ� �̿�
	void set_shell_known_string(int csidl, CString str);

	//OS�� ����, ������ ������ ���� "�� ��ǻ��"�� "�� PC", "�� ��ǻ��", "My Computer" ��� �پ��ϹǷ� csidl�� �����Ѵ�.
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
