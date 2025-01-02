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
	//local�� ���� ���� �������� remote�� ���� �޾ƿͼ� ä����� �Ѵ�.
	m_label.insert(std::pair<int, CString>(CSIDL_DRIVES, get_system_label(CSIDL_DRIVES)));
	m_label.insert(std::pair<int, CString>(CSIDL_DESKTOP, get_system_label(CSIDL_DESKTOP)));
	m_label.insert(std::pair<int, CString>(CSIDL_MYDOCUMENTS, get_system_label(CSIDL_MYDOCUMENTS)));

	//"�� PC"�� ���� �����̹Ƿ� ���� ��δ� ����. ������ ""�� ó���� ��� ���� ������ ��ο� ���е��� �����Ƿ� ��� ���� ���̺�� �����ϰ� "�� PC"�� �����Ѵ�.
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

//csidl�� �ش��ϴ� ���� ��� ����
CString CShellVolumeList::get_path(int csidl)
{
	if (m_path.find(csidl) != m_path.end())
		return m_path[csidl];

	return _T("");
}

//label�� �ش��ϴ� ���� ��� ����
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

	//"�� PC"�� ���� �Ʒ� �Լ��� ���� ""�� ��ȯ�ȴ�.
	//szFile = convert_special_folder_to_real_path(szFile);
	
	if(szFile.IsEmpty() || szFile == m_volume[0].get_label(CSIDL_DRIVES))
	{
		LPITEMIDLIST pidl_Computer = NULL;
		SHGetFolderLocation( NULL, CSIDL_DRIVES, NULL, 0, &pidl_Computer ); // ��ǻ��
		SHGetFileInfo((TCHAR*)pidl_Computer, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if(szFile == m_volume[0].get_label(CSIDL_MYDOCUMENTS))
	{
		LPITEMIDLIST pidl_Document = NULL;
		SHGetFolderLocation( NULL, CSIDL_MYDOCUMENTS, NULL, 0, &pidl_Document ); // ������
		SHGetFileInfo((TCHAR*)pidl_Document, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if(szFile == m_volume[0].get_label(CSIDL_DESKTOP))
	{
		LPITEMIDLIST pidl_Desktop = NULL;
		SHGetFolderLocation( NULL, CSIDL_DESKTOP, NULL, 0, &pidl_Desktop ); // ����ȭ��
		SHGetFileInfo((TCHAR*)pidl_Desktop, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else
	{
		//SHGetStockIconInfo()
		//bDrive�� ��� remote�� F:\ ����̺� �������� ���Ϸ��� ���� "F:\"�� �༭�� �ȵȴ�.
		//������ ������ local���� �����ϴµ� local�� F����̺갡 ���� �� �ִ�.
		//�׷��ٰ� D��� �����Ҽ��� ����. D�� local drive�ϼ���, ��Ʈ��ũ ����̺��ϼ���, ���� ����̺��ϼ��� �ִ�.
		//�켱 ����� C:\\��� �����ϰ� ó����.
		//Ȯ�� �ʿ�!
		//�켱 bDrive�� ���� ���ÿ� �ش� ����̺갡 ���ٸ� DRIVE_FIXED ����̺����� ������ ����̺��� �����ϰ� ǥ���Ѵ�.
		//�̷��� ������ ���� ������ nFTD������ ����Ʈ�� ��� ����̺갡 �ý��� ����̺�� ǥ�õȴ�.
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

				//���� local�� �ش� ����̺갡 �������� �ʴ´ٸ� C:\\��� �����ϰ� ǥ���Ѵ�.
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

	//SHGetFileInfo()�� ���� himagelist�� CImageList.Attach(himagelist)�� ���
	//CShellImageList�� �ٸ� �ν��Ͻ����� �ٽ� CImageList.Attach(himagelist)�ϰԵǸ� ASSERT FAIL�� �߻��Ѵ�.
	//�ϳ��� ���μ������� ���� himagelist�� �ݵ�� �� �ѹ��� Attach()�ؾ��Ѵ�.
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

//OS�� ����, ������ ������ ���� "�� ��ǻ��"�� "�� PC", "�� ��ǻ��", "My Computer" ��� �پ��ϹǷ� csidl�� �����Ѵ�.
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
