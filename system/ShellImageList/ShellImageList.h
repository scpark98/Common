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
#include <deque>

enum Enum_Shell_Drive_List
{
	drive_list_drives = 0,		//�� PC
	drive_list_desktop,			//���� ȭ��
	drive_list_mydocuments,		//�� ����
};

class CShellImageList  
{
public:
	CImageList m_imagelist_small; // Imagelist for small icons
	CImageList m_imagelist_large; // Imagelist for large icons

	CShellImageList();
	~CShellImageList();

	void Initialize();
	int  FindCache(CString szFileExt);
	void InsertCache(CString szFileExt, int ID);

	CImageList* get_imagelist(bool small_icon = true);
	
	int  GetImageListIcon(CString szPath, CString szFile);
	void GetSystemDisplayName(CString szFile, CString &szDisplayName);
	int  GetVirtualImageListIcon(CString szExt);

	int  GetSystemImageListIcon(CString szFile, BOOL bDrive = TRUE);
	int  GetSystemImageListIcon(int csidl, BOOL bDrive = TRUE);

	//�� PC, ����, ���� ȭ�� ���� ������ ���ڿ��� ����(�ٱ��� �����ÿ� Ư�� �ʿ�)
	//Shlobj.h�� ���ǵ� CSIDL_�� ���۵Ǵ� ���� �����ϰ� �̿�
	void set_shell_known_string(int csidl, CString str);

	//OS�� ����, ������ ������ ���� "�� ��ǻ��"�� "�� PC", "My Computer" �� �پ��ϹǷ� csidl�� �����Ѵ�.
	int get_csidl_by_shell_known_string(CString str);
	CString get_shell_known_string_by_csidl(int csidl);
	std::map<int, CString>* get_csidl_map();

	//drive_list�� local, remote�� �ٸ��Ƿ� �� ���� ����� �߰��Ͽ� ����ϵ��� �����߾�����
	//CShellImageList		m_shell_imagelist_local;
	//CShellImageList		m_shell_imagelist_remote;
	//���� ���� �� �ν��Ͻ��� �����ϸ� CImageList�� Attach()���� ASSERT FAIL�� �߻��Ѵ�.
	//�� ���μ��������� �ý��� �̹��� ����Ʈ �ڵ��� �ѹ����� Attach()�ؾ��ϴ� ��Ģ�� �ִ�.
	//local

	//remote�� ���� drive������ ���ͼ� ���� �־���� �Ѵ�.
	//�� PC�� label, ����ȭ�� fullpath, ���� fullpath ���� ��� ���ǹǷ� ����Ʈ�� 0, 1, 2���� �־��ش�.
	std::deque<CString> *get_drive_list(int index) { return &m_drive_lists[index]; }
	void	set_drive_list(int index, std::deque<CString> *drive_list);
	void	add_drive_list(std::deque<CString> *drive_list);

	//path�� �ݵ�� "C:\Windows"�� ���� fullpath �Ǵ� ����̺� ���� + ':'�� ��� �Ѵ�.
	CString	get_drive_volume(int index, CString path);

private:
	CStringArray m_ExtArray;
	CUIntArray   m_IDArray;

	//OS�� ����, ������ ������ ���� "�� ��ǻ��"�� "�� PC", "My Computer" ��� �پ��ϹǷ� csidl�� �����Ѵ�.
	std::map<int, CString> m_csidl_map;

	//0:�� PC�� label, 1:����ȭ�� ���, 2:���� ���, 3:C����̺� volume, 4:D����̺� volume...
	//CShellImageList�� �ϳ��� �ν��Ͻ��� ����ؾ� �ϴµ� local, remote�� drive_list�� �ٸ���
	//local, remote�� �ƴ� ���� remote ��ġ�� ���� �� �����Ƿ� ���� ���ͷ� ó���Ѵ�.
	std::deque<std::deque<CString>> m_drive_lists;

	int m_osType;
};

#endif // !defined(AFX_SHELLLIST_H__E0B73BFC_2F7C_44D4_BC38_1FA6A74CBD31__INCLUDED_)
