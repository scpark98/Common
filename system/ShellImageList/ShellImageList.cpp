 // ShellList.cpp: implementation of the CShellImageList class.
//
//////////////////////////////////////////////////////////////////////

#include "ShellImageList.h"
#include <ShlObj.h>
#include "../../Functions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellImageList::CShellImageList()
{
	CoInitialize(NULL);

	::get_drive_map(&m_drive_map);

	SHFILEINFO shInfo;
	m_imagelist_small.Attach((HIMAGELIST)SHGetFileInfo((LPCTSTR)_T("C:\\"), 0, &shInfo, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
	m_imagelist_large.Attach((HIMAGELIST)SHGetFileInfo((LPCTSTR)_T("C:\\"), 0, &shInfo, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_LARGEICON));

	m_csidl_map.insert(std::pair<int, CString>(CSIDL_DRIVES, get_system_label(CSIDL_DRIVES)));
	m_csidl_map.insert(std::pair<int, CString>(CSIDL_PERSONAL, get_system_label(CSIDL_PERSONAL)));
	m_csidl_map.insert(std::pair<int, CString>(CSIDL_DESKTOP, get_system_label(CSIDL_DESKTOP)));
}

CShellImageList::~CShellImageList()
{
	m_imagelist_small.Detach();
	m_imagelist_large.Detach();

	CoUninitialize();
}

int CShellImageList::GetSystemImageListIcon(int csidl, BOOL bDrive)
{
	return GetSystemImageListIcon(get_shell_known_string_by_csidl(csidl), bDrive);
}

int CShellImageList::GetSystemImageListIcon(CString szFile, BOOL bDrive)
{   
	SHFILEINFO shFileInfo;

	//"�� PC"�� ���� �Ʒ� �Լ��� ���� ""�� ��ȯ�ȴ�.
	//szFile = convert_special_folder_to_real_path(szFile);
	
	if(szFile.IsEmpty() || szFile == get_shell_known_string_by_csidl(CSIDL_DRIVES))
	{
		LPITEMIDLIST pidl_Computer = NULL;
		SHGetFolderLocation( NULL, CSIDL_DRIVES, NULL, 0, &pidl_Computer ); // ��ǻ��
		SHGetFileInfo((TCHAR*)pidl_Computer, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if(szFile == get_shell_known_string_by_csidl(CSIDL_PERSONAL))
	{
		LPITEMIDLIST pidl_Document = NULL;
		SHGetFolderLocation( NULL, CSIDL_PERSONAL, NULL, 0, &pidl_Document ); // ������
		SHGetFileInfo((TCHAR*)pidl_Document, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if(szFile == get_shell_known_string_by_csidl(CSIDL_DESKTOP))
	{
		LPITEMIDLIST pidl_Desktop = NULL;
		SHGetFolderLocation( NULL, CSIDL_DESKTOP, NULL, 0, &pidl_Desktop ); // ����ȭ��
		SHGetFileInfo((TCHAR*)pidl_Desktop, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else
	{
		if(bDrive) SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		else SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
	}
	
	return shFileInfo.iIcon;
}

void CShellImageList::GetSystemDisplayName(CString szFile, CString &szDisplayName)
{   
	SHFILEINFO shFileInfo;
	SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	//sprintf(szDisplayName, _T("%s"),shFileInfo.szDisplayName);
	szDisplayName = shFileInfo.szDisplayName;
}

void CShellImageList::Initialize()
{
	m_ExtArray.RemoveAll();
	m_IDArray.RemoveAll();

	CreateDirectory(_T("c:\\nFTDTemp_qrehadfkjn\\"),NULL);
	int cache = GetSystemImageListIcon("c:\\nFTDTemp_qrehadfkjn\\");
	RemoveDirectory(_T("c:\\nFTDTemp_qrehadfkjn\\"));
	InsertCache("\\", cache);
}

int CShellImageList::FindCache(CString szFileExt)
{
	int nExtSize = m_ExtArray.GetSize();
	BOOL bFound  = FALSE;
	szFileExt.MakeLower();
	int x = 0;
	for (; x<nExtSize; x++)
		if(m_ExtArray.GetAt(x) == szFileExt) break;

	if(x == nExtSize) return -1;
	return x;
}

void CShellImageList::InsertCache(CString szFileExt, int ID)
{
	szFileExt.MakeLower();
	m_ExtArray.Add(szFileExt);
	m_IDArray.Add(ID);
}

int CShellImageList::GetImageListIcon(CString szPath, CString szFile)
{
	int cache;
	CString szExt;

	// Ȯ���ڱ��ϱ�
	int pExt = szFile.ReverseFind('.');
	if(pExt == -1) szExt.Format(_T(".")); 
	else szExt = szFile.Mid(pExt + 1, szFile.GetLength()); 

	// ĳ���� �ִٸ�
	if((cache = FindCache(szExt)) != -1)
	{
		return m_IDArray.GetAt(cache);
	}
	
	// ĳ���� ���ٸ�
	cache = GetSystemImageListIcon(szPath);
	InsertCache(szExt, cache);
	return cache;
}

int CShellImageList::GetVirtualImageListIcon(CString szExt)
{
	int cache;

	// ĳ���� �ִٸ�
	if((cache = FindCache(szExt)) != -1)
	{
		return m_IDArray.GetAt(cache);
	}
	
	// ĳ���� ���ٸ�
	CString temp;
	temp.Format(_T("c:\\nFTDtemp.%s"),szExt);
	
	cache = GetSystemImageListIcon(temp, FALSE);
	InsertCache(szExt,cache);
	
	return cache;
}

CString CShellImageList::get_shell_known_string_by_csidl(int csidl)
{
	if (this == NULL)
		return _T("");

	//return m_csidl_map[id];
	CString str;
	//str.LoadString(id);

	std::map<int, CString>::iterator it = m_csidl_map.find(csidl);

	if (it == m_csidl_map.end())
		return _T("");

	return it->second;
}

std::map<int, CString>* CShellImageList::get_csidl_map()
{
	if (this == NULL)
		return NULL;

	if (m_csidl_map.size() == 0)
		return NULL;

	return &m_csidl_map;
}

//OS�� ����, ������ ������ ���� "�� ��ǻ��"�� "�� PC", "�� ��ǻ��", "My Computer" ��� �پ��ϹǷ� csidl�� �����Ѵ�.
int CShellImageList::get_csidl_by_shell_known_string(CString str)
{
	if (m_csidl_map.size() == 0)
		return -1;

	for (auto const& element : m_csidl_map)
	{
		if (element.second.Compare(str) == 0)
		{
			return element.first;
		}
	}

	return -1;
}
