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

CShellVolumeList::CShellVolumeList()
{
	//local일 경우는 직접 얻어오지만 remote일 경우는 받아와서 채워줘야 한다.
	m_label.insert(std::pair<int, CString>(CSIDL_DRIVES, get_system_label(CSIDL_DRIVES)));
	m_label.insert(std::pair<int, CString>(CSIDL_DESKTOP, get_system_label(CSIDL_DESKTOP)));
	m_label.insert(std::pair<int, CString>(CSIDL_MYDOCUMENTS, get_system_label(CSIDL_MYDOCUMENTS)));

	//"내 PC"는 가상 폴더이므로 실제 경로는 없다. 하지만 ""로 처리할 경우 실제 공백인 경로와 구분되지 않으므로 경로 또한 레이블과 동일하게 "내 PC"로 설정한다.
	m_path.insert(std::pair<int, CString>(CSIDL_DRIVES, get_system_label(CSIDL_DRIVES)));
	m_path.insert(std::pair<int, CString>(CSIDL_DESKTOP, get_known_folder(CSIDL_DESKTOP)));
	m_path.insert(std::pair<int, CString>(CSIDL_MYDOCUMENTS, get_known_folder(CSIDL_MYDOCUMENTS)));

	std::deque<CString> drive_list;
	::get_drive_list(&drive_list);
	set_drive_list(&drive_list);
}

int CShellVolumeList::get_csidl(CString label)
{
	std::map<int, CString>::iterator it;

	for (it = m_label.begin(); it != m_label.end(); it++)
	{
		if (it->second == label)
			return it->first;
	}

	return -1;
}

CString CShellVolumeList::get_label(int csidl)
{
	if (m_label.find(csidl) != m_label.end())
		return m_label[csidl];

	return _T("");
}

//csidl에 해당하는 실제 경로 리턴
CString CShellVolumeList::get_path(int csidl)
{
	if (m_path.find(csidl) != m_path.end())
		return m_path[csidl];

	return _T("");
}

//label에 해당하는 실제 경로 리턴
CString CShellVolumeList::get_path(CString label)
{
	int csidl = get_csidl(label);
	return get_path(csidl);
}

CString CShellVolumeList::get_drive_volume(CString path)
{
	int pos = path.Find(':');
	if (pos < 0)
		return _T("");

	CString drive_letter = path.Mid(pos - 1, 2);

	for (int i = 0; i < m_drives.size(); i++)
	{
		if (m_drives[i].Find(drive_letter) >= 0)
			return m_drives[i];
	}

	return _T("");
}

void CShellVolumeList::set_system_label(std::map<int, CString>* map)
{
	m_label.clear();
	m_label.insert(map->begin(), map->end());
}

void CShellVolumeList::set_system_path(std::map<int, CString>* map)
{
	m_path.clear();
	m_path.insert(map->begin(), map->end());
}

void CShellVolumeList::set_drive_list(std::deque<CString>* drive_list)
{
	m_drives.clear();
	m_drives.assign(drive_list->begin(), drive_list->end());
}


CShellImageList::CShellImageList()
{
}

CShellImageList::~CShellImageList()
{
	m_imagelist_small.Detach();
	m_imagelist_large.Detach();

	CoUninitialize();
}

int CShellImageList::GetSystemImageListIcon(int csidl, BOOL bDrive)
{
	return GetSystemImageListIcon(m_volume[0].get_label(csidl), bDrive);
}

int CShellImageList::GetSystemImageListIcon(CString szFile, BOOL bDrive)
{   
	SHFILEINFO shFileInfo;

	//"내 PC"인 경우는 아래 함수에 의해 ""로 변환된다.
	//szFile = convert_special_folder_to_real_path(szFile);
	
	if(szFile.IsEmpty() || szFile == m_volume[0].get_label(CSIDL_DRIVES))
	{
		LPITEMIDLIST pidl_Computer = NULL;
		SHGetFolderLocation( NULL, CSIDL_DRIVES, NULL, 0, &pidl_Computer ); // 컴퓨터
		SHGetFileInfo((TCHAR*)pidl_Computer, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if(szFile == m_volume[0].get_label(CSIDL_MYDOCUMENTS))
	{
		LPITEMIDLIST pidl_Document = NULL;
		SHGetFolderLocation( NULL, CSIDL_MYDOCUMENTS, NULL, 0, &pidl_Document ); // 내문서
		SHGetFileInfo((TCHAR*)pidl_Document, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if(szFile == m_volume[0].get_label(CSIDL_DESKTOP))
	{
		LPITEMIDLIST pidl_Desktop = NULL;
		SHGetFolderLocation( NULL, CSIDL_DESKTOP, NULL, 0, &pidl_Desktop ); // 바탕화면
		SHGetFileInfo((TCHAR*)pidl_Desktop, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else
	{
		//SHGetStockIconInfo()
		//bDrive일 경우 remote의 F:\ 드라이브 아이콘을 구하려고 실제 "F:\"를 줘서는 안된다.
		//아이콘 정보는 local에서 추출하는데 local에 F드라이브가 없을 수 있다.
		//그렇다고 D라고 가정할수도 없다. D는 local drive일수도, 네트워크 드라이브일수도, 구글 드라이브일수도 있다.
		//우선 현재는 C:\\라고 가정하고 처리함.
		//확인 필요!
		//우선 bDrive일 경우는 로컬에 해당 드라이브가 없다면 DRIVE_FIXED 드라이브중의 마지막 드라이브라고 가정하고 표시한다.
		//이러한 이유로 이전 버전의 nFTD에서도 리모트의 모든 드라이브가 시스템 드라이브로 표시된다.
		if (bDrive)
		{
			if (szFile.GetLength() == 3)
			{
				//LPITEMIDLIST pidl = NULL;
				//HRESULT hr = SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidl);
				//hr = SHGetFileInfo((LPCTSTR)pidl,
				//	-1,
				//	&shFileInfo,
				//	sizeof(shFileInfo),
				//	SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);

				//만약 local에 해당 드라이브가 존재하지 않는다면 C:\\라고 가정하고 표시한다.
				UINT drive_type = GetDriveType(szFile);
				if (GetDriveType(szFile) <= DRIVE_NO_ROOT_DIR)
					szFile = _T("C:\\");

				SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
			}
			else
			{
				SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
			}
		}
		else
		{
			SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		}
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
	CoInitialize(NULL);

	CShellVolumeList volume;
	m_volume.push_back(volume);

	SHFILEINFO shInfo;

	//SHGetFileInfo()로 얻은 himagelist를 CImageList.Attach(himagelist)할 경우
	//CShellImageList의 다른 인스턴스에서 다시 CImageList.Attach(himagelist)하게되면 ASSERT FAIL이 발생한다.
	//하나의 프로세스에서 얻어온 himagelist는 반드시 단 한번만 Attach()해야한다.
	m_imagelist_small.Attach((HIMAGELIST)SHGetFileInfo((LPCTSTR)_T("C:\\"), 0, &shInfo, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
	m_imagelist_large.Attach((HIMAGELIST)SHGetFileInfo((LPCTSTR)_T("C:\\"), 0, &shInfo, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_LARGEICON));

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

CImageList* CShellImageList::get_imagelist(bool small_icon)
{
	return (small_icon ? &m_imagelist_small : &m_imagelist_large);
}

int CShellImageList::GetImageListIcon(CString szPath, CString szFile)
{
	int cache;
	CString szExt;

	// 확장자구하기
	int pExt = szFile.ReverseFind('.');
	if(pExt == -1) szExt.Format(_T(".")); 
	else szExt = szFile.Mid(pExt + 1, szFile.GetLength()); 

	// 캐쉬에 있다면
	if((cache = FindCache(szExt)) != -1)
	{
		return m_IDArray.GetAt(cache);
	}
	
	// 캐쉬에 없다면
	cache = GetSystemImageListIcon(szPath);
	InsertCache(szExt, cache);
	return cache;
}

int CShellImageList::GetVirtualImageListIcon(CString szExt)
{
	int cache;

	// 캐쉬에 있다면
	if((cache = FindCache(szExt)) != -1)
	{
		return m_IDArray.GetAt(cache);
	}
	
	// 캐쉬에 없다면
	CString temp;
	temp.Format(_T("c:\\nFTDtemp.%s"),szExt);
	
	cache = GetSystemImageListIcon(temp, FALSE);
	InsertCache(szExt,cache);
	
	return cache;
}

void CShellImageList::set_system_label(int index, std::map<int, CString>* map)
{
	if (index >= m_volume.size())
		m_volume.resize(index + 1);

	m_volume[index].set_system_label(map);
}

void CShellImageList::set_system_path(int index, std::map<int, CString>* map)
{
	if (index >= m_volume.size())
		m_volume.resize(index + 1);

	m_volume[index].set_system_path(map);
}

void CShellImageList::set_drive_list(int index, std::deque<CString>* drive_list)
{
	if (index >= m_volume.size())
		m_volume.resize(index + 1);

	m_volume[index].set_drive_list(drive_list);
}

CString CShellImageList::get_system_label(int index, int csidl)
{
	if (index >= m_volume.size())
		return _T("");

	return m_volume[index].get_label(csidl);
}

CString CShellImageList::get_system_path(int index, int csidl)
{
	if (index >= m_volume.size())
		return _T("");

	return m_volume[index].get_path(csidl);
}

/*
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

//OS언어에 따라, 윈도우 버전에 따라 "내 컴퓨터"는 "내 PC", "내 컴퓨터", "My Computer" 등등 다양하므로 csidl로 구분한다.
int CShellImageList::get_csidl_by_shell_known_string(CString label)
{
	if (m_csidl_map.size() == 0)
		return -1;

	for (auto const& element : m_csidl_map)
	{
		if (element.second.Compare(label) == 0)
		{
			return element.first;
		}
	}

	return -1;
}

void CShellImageList::set_drive_map(std::map<TCHAR, CString>* drive_map)
{
	m_drive_map.clear();
	m_drive_map.insert(drive_map->begin(), drive_map->end());
}

void CShellImageList::set_drive_map(std::deque<CString>* drive_list)
{
	m_drive_map.clear();

	for (int i = 0; i < drive_list->size(); i++)
	{
		CString drive_volume = drive_list->at(i);
		int colon = drive_volume.Find(_T(":"));
		if (colon < 0)
			continue;
		m_drive_map.insert(std::pair<TCHAR, CString>(drive_volume[colon-1], drive_volume));
	}
}

void CShellImageList::set_drive_list(int index, std::deque<CString>* drive_list)
{
	if (index >= m_drive_lists.size())
		m_drive_lists.resize(index);

	m_drive_lists[index].clear();
	m_drive_lists[index].assign(drive_list->begin(), drive_list->end());
}

void CShellImageList::add_drive_list(std::deque<CString> *drive_list)
{
	m_drive_lists.push_back(*drive_list);
}

CString	CShellImageList::get_drive_volume(int index, CString path)
{
	int pos = path.Find(':');
	if (pos < 0)
		return _T("");

	if (index < 0 || index >= m_drive_lists.size())
		return _T("");

	CString drive_letter = path.Mid(pos - 1, 2);

	for (int i = 3; i < m_drive_lists[index].size(); i++)
	{
		if (m_drive_lists[index][i].Find(drive_letter) >= 0)
			return m_drive_lists[index][i];
	}

	return _T("");
}
*/
