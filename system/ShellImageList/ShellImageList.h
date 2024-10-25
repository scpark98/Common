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
	drive_list_drives = 0,		//내 PC
	drive_list_desktop,			//바탕 화면
	drive_list_mydocuments,		//내 문서
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

	//내 PC, 문서, 바탕 화면 등의 정해진 문자열을 저장(다국어 지원시에 특히 필요)
	//Shlobj.h에 정의된 CSIDL_로 시작되는 값을 동일하게 이용
	void set_shell_known_string(int csidl, CString str);

	//OS언어에 따라, 윈도우 버전에 따라 "내 컴퓨터"는 "내 PC", "My Computer" 등 다양하므로 csidl로 구분한다.
	int get_csidl_by_shell_known_string(CString str);
	CString get_shell_known_string_by_csidl(int csidl);
	std::map<int, CString>* get_csidl_map();

	//drive_list가 local, remote가 다르므로 이 또한 멤버로 추가하여 기억하도록 구현했었으나
	//CShellImageList		m_shell_imagelist_local;
	//CShellImageList		m_shell_imagelist_remote;
	//위와 같이 두 인스턴스를 실행하면 CImageList의 Attach()에서 ASSERT FAIL이 발생한다.
	//한 프로세스에서는 시스템 이미지 리스트 핸들을 한번씩만 Attach()해야하는 규칙이 있다.
	//local

	//remote인 경우는 drive정보를 얻어와서 직접 넣어줘야 한다.
	//내 PC의 label, 바탕화면 fullpath, 문서 fullpath 또한 계속 사용되므로 리스트의 0, 1, 2번에 넣어준다.
	std::deque<CString> *get_drive_list(int index) { return &m_drive_lists[index]; }
	void	set_drive_list(int index, std::deque<CString> *drive_list);
	void	add_drive_list(std::deque<CString> *drive_list);

	//path는 반드시 "C:\Windows"와 같이 fullpath 또는 드라이브 문자 + ':'를 줘야 한다.
	CString	get_drive_volume(int index, CString path);

private:
	CStringArray m_ExtArray;
	CUIntArray   m_IDArray;

	//OS언어에 따라, 윈도우 버전에 따라 "내 컴퓨터"는 "내 PC", "My Computer" 등등 다양하므로 csidl로 구분한다.
	std::map<int, CString> m_csidl_map;

	//0:내 PC의 label, 1:바탕화면 경로, 2:문서 경로, 3:C드라이브 volume, 4:D드라이브 volume...
	//CShellImageList는 하나의 인스턴스만 사용해야 하는데 local, remote의 drive_list는 다르고
	//local, remote가 아닌 여러 remote 장치가 있을 수 있으므로 이중 백터로 처리한다.
	std::deque<std::deque<CString>> m_drive_lists;

	int m_osType;
};

#endif // !defined(AFX_SHELLLIST_H__E0B73BFC_2F7C_44D4_BC38_1FA6A74CBD31__INCLUDED_)
