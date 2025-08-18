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

	//std::deque<CDiskDriveInfo> drive_list;
	//::get_drive_list(&drive_list);
	set_drive_list();
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
		if (_tcsstr(m_drives[i].path, drive_letter) != NULL)
			return m_drives[i].label;
	}

	return _T("");
}

void CShellVolumeList::get_drive_space(CString path, ULARGE_INTEGER* total_space, ULARGE_INTEGER* free_space)
{
	for (auto drive : m_drives)
	{
		if (CString(drive.path).Find(path) >= 0)
		{
			total_space->QuadPart = drive.total_space.QuadPart;
			free_space->QuadPart = drive.free_space.QuadPart;
		}
	}
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

void CShellVolumeList::set_drive_list(std::deque<CDiskDriveInfo>* drive_list)
{
	m_drives.clear();

	//remote�� ���� drive_list�� �޾Ƽ� ä����� �ϰ�
	if (drive_list)
	{
		m_drives.assign(drive_list->begin(), drive_list->end());
	}
	//local�� ���� ���� ���ͼ� ä����� �Ѵ�.
	else
	{
		DWORD dwError = 0;
		TCHAR tzDriveString[MAX_PATH] = { 0, };
		CString strDrive;

		DWORD logicalDrives = GetLogicalDrives();
		unsigned int i = 64;

		do
		{
			i++;
			if ((logicalDrives & 1) != 0)
			{
				strDrive.Format(_T("%c:\\"), i);
				UINT driveType = GetDriveType(strDrive);

				// CD-ROM�̳� floppy disk�� ���� ������ ǥ������ �ʴ´�.
				if ((driveType == DRIVE_REMOVABLE || driveType == DRIVE_CDROM))
					continue;

				ULARGE_INTEGER total_space, free_space;
				total_space.QuadPart = get_disk_total_size(strDrive);
				free_space.QuadPart = get_disk_free_size(strDrive);
				m_drives.push_back(CDiskDriveInfo(driveType, ::get_drive_volume(strDrive[0]), strDrive, total_space, free_space));
			}
		} while ((logicalDrives >>= 1) != 0);
	}
}


CShellImageList::CShellImageList()
{
	Initialize();
}

CShellImageList::~CShellImageList()
{
	m_imagelist_small.Detach();
	m_imagelist_large.Detach();

	CoUninitialize();
}

int CShellImageList::GetSystemImageListIcon(int index, int csidl, BOOL bDrive)
{
	return GetSystemImageListIcon(index, m_volume[index].get_label(csidl), bDrive);
}

int CShellImageList::GetSystemImageListIcon(int index, CString szFile, BOOL bDrive)
{   
	SHFILEINFO shFileInfo;

	if(szFile.IsEmpty() || szFile == m_volume[index].get_label(CSIDL_DRIVES))
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
				//���� remote�� ������ ������ ���� �� ���� ��� �⺻ ���� �̹����� ǥ���Ѵ�.
				if (!PathFileExists(szFile))
					szFile = _T("C:\\Windows");
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

UINT CShellImageList::get_drive_type(int index, CString path)
{
	if (index >= (int)m_volume.size() || path.IsEmpty())
		return DRIVE_UNKNOWN;

	if (path.GetLength() == 1)
		path += _T(":\\");
	else if (path.GetLength() == 2 && path.Right(1) == ';')
		path += _T("\\");
	else
		path = path.Left(3);

	std::deque<CDiskDriveInfo>* drives = m_volume[index].get_drive_list();
	for (int i = 0; i < drives->size(); i++)
	{
		if (CString(drives->at(i).path).CompareNoCase(path) == 0)
			return drives->at(i).type;
	}

	return DRIVE_UNKNOWN;
}

int CShellImageList::get_drive_index(int index, CString path)
{
	path = normalize_drive_path(path);

	if (index >= (int)m_volume.size() || path.IsEmpty())
		return 0;

	std::deque<CDiskDriveInfo>* drives = m_volume[index].get_drive_list();
	for (int i = 0; i < drives->size(); i++)
	{
		if (CString(drives->at(i).path).CompareNoCase(path) == 0)
			return i;
	}

	return 0;
}

//shell32.dll�� �ִ� �������� �ε��� ����
int CShellImageList::get_drive_icon(UINT drive_type)
{
	switch (drive_type)
	{
	case DRIVE_FIXED :
		return 7;
	case DRIVE_REMOTE :
		return 9;
	case DRIVE_CDROM :
		return 11;
	case DRIVE_NO_ROOT_DIR :
		return 12;
	case DRIVE_REMOVABLE :
		return 26;
	case DRIVE_RAMDISK :
		return 12;
	}

	return 7;
}

int CShellImageList::get_drive_icon(int index, CString path)
{
	path = normalize_drive_path(path);
	if (path.IsEmpty())
		return 0;

	//C drive�� local�̵� remote�̵� �����ϴ�.
	if (path.Left(3) == _T("C:\\"))
		return GetSystemImageListIcon(index, _T("C:\\"), TRUE);

	return get_drive_icon(get_drive_type(index, path));
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
	int cache = GetSystemImageListIcon(0, "c:\\nFTDTemp_qrehadfkjn\\");
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
	cache = GetSystemImageListIcon(0, szPath);
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
	
	cache = GetSystemImageListIcon(0, temp, FALSE);
	InsertCache(szExt,cache);
	
	return cache;
}

void CShellImageList::set_system_label(int index, std::map<int, CString>* map)
{
	if (index >= (int)m_volume.size())
		m_volume.resize(index + 1);

	m_volume[index].set_system_label(map);
}

void CShellImageList::set_system_path(int index, std::map<int, CString>* map)
{
	if (index >= (int)m_volume.size())
		m_volume.resize(index + 1);

	m_volume[index].set_system_path(map);
}

void CShellImageList::set_drive_list(int index, std::deque<CDiskDriveInfo>* drive_list)
{
	if (index >= (int)m_volume.size())
		m_volume.resize(index + 1);

	m_volume[index].set_drive_list(drive_list);
}

CString CShellImageList::get_system_label(int index, int csidl)
{
	if (index >= (int)m_volume.size())
		return _T("");

	return m_volume[index].get_label(csidl);
}

CString CShellImageList::get_system_path(int index, int csidl)
{
	if (index >= (int)m_volume.size())
		return _T("");

	return m_volume[index].get_path(csidl);
}

//"�� PC\\���� ��ũ (C:)" -> "C:\\"
//"���� ��ũ (C:)" -> "C:\\"
//"����" -> "C:\\Documents"
//"����\\AnySupport" -> "C:\\Documents\\AnySupport"
//"Seagate(\\192.168.0.52) (X:)" -> "X:"	(��Ʈ��ũ ����̺�)
//���� ���� ���� ������ ������� ��ȯ.
CString CShellImageList::convert_special_folder_to_real_path(int index, CString special_folder)
{
	if (index >= (int)m_volume.size())
		return _T("");

	//���� �����Ѵٰ� �Ǻ��Ǹ� �̴� real_path�̹Ƿ� �״�� ����.
	if (special_folder.IsEmpty() || PathFileExists(special_folder))
	{
		//"c:\\program files"�� �Ѿ�͵� ���� �������� "C:\\Program Files"�� ��ȯ�Ѵ�.
		if (PathFileExists(special_folder))
			special_folder = get_original_path(special_folder);
		return special_folder;
	}

	CString myPC_label;

	myPC_label = m_volume[index].get_label(CSIDL_DRIVES);

	if (special_folder == myPC_label)
		return myPC_label;

	//"�� PC" ��� ������ �����ϸ� �����Ѵ�.
	//"�� PC\\�����ҹ���(\\\\192.168.1.103) (Y:)" -> "\\�����ҹ���(\\\\192.168.1.103) (Y:)"
	if (special_folder.Find(myPC_label) >= 0)
		special_folder.Replace(myPC_label, _T(""));

	//"\\�����ҹ���(\\\\192.168.1.103) (Y:)"�� ���� �� �տ� '\\'�� �پ��ִٸ� ����.
	if (special_folder.GetLength() > 1 && special_folder.Left(1) == '\\')
		special_folder = special_folder.Mid(1);

	CString real_path = special_folder;
	CString drive_prefix;

	//"���� ȭ��\\", "����", "����\\abc"�� ���� �Ѿ���Ƿ� �� ó�� �׸��� ���� ��θ� ���ؾ� �Ѵ�.
	//"Seagate(\\192.168.0.52) (X:)"�� ���� ��Ʈ��ũ ����̺굵 �����ϹǷ� '\\'�θ� �Ǻ��ؼ��� �ȵȴ�.
	bool is_network_drive = false;

	int pos1 = special_folder.Find(_T(":)"));
	int pos2 = special_folder.Find(_T("\\\\"));
	if (pos1 > 0 && pos2 > 0 && pos1 > pos2)
	{
		is_network_drive = true;
		drive_prefix = special_folder.Left(special_folder.Find(_T(":)")) + 2);
	}
	else
	{
		std::deque<CString> token;
		get_token_string(special_folder, token, _T("\\"), false);
		drive_prefix = token[0];
	}

	CString rest_path = special_folder;
	rest_path.Replace(drive_prefix, _T(""));

	if (drive_prefix == m_volume[index].get_label(CSIDL_DRIVES))
		real_path.Format(_T("%s%s"), m_volume[index].get_label(CSIDL_DRIVES), rest_path);
	else if (drive_prefix == ::get_system_label(CSIDL_DESKTOP))
		real_path.Format(_T("%s%s"), m_volume[index].get_path(CSIDL_DESKTOP), rest_path);
	else if (drive_prefix == ::get_system_label(CSIDL_MYDOCUMENTS))
		real_path.Format(_T("%s%s"), m_volume[index].get_path(CSIDL_MYDOCUMENTS), rest_path);
	else
	{
		auto drive_list = m_volume[index].get_drive_list();

		for (int i = 0; i < drive_list->size(); i++)
		{
			//"���� ��ũ (C:)"
			if (_tcsicmp(drive_list->at(i).label, drive_prefix) == 0)
			{
				int pos = real_path.Find(_T(":)"));
				if (pos < 0)
					return real_path;

				CString rest = real_path.Mid(pos + 2);
				CString drive_letter = real_path.Mid(pos - 1, 1);

				if (rest.Left(1) == _T("\\"))
					rest = rest.Mid(1);

				real_path.Format(_T("%s:\\%s"), drive_letter, rest);
				break;
			}
		}
	}

	return real_path;
}

//"c:\\abc\\def"				=> "���� ��ũ (C:)\\abc\\def"
//"C:\Users\scpark\Desktop"		=> "���� ȭ��"
//"C:\Users\scpark\Documents"	=> "����"
CString	CShellImageList::convert_real_path_to_special_folder(int index, CString real_path)
{
	if (index >= (int)m_volume.size())
		return _T("");

	//real_path�� local������(index = 0�� ���) ������ �����ؾ� �ϴ� ����̹Ƿ�
	//"c:\\program files"��� �Ѿ�͵� "C:\\Program Files"�� ��ȯ�ؼ� ������� �Ѵ�.
	if (index == 0 && PathFileExists(real_path))
	{
		//������ �ƴ� ���� ���翩�θ� �˻��� �� �����Ƿ� ��ȯ���� �ʴ´�.
		real_path = get_original_path(real_path);
	}

	if (real_path == m_volume[index].get_path(CSIDL_DESKTOP))
		return m_volume[index].get_label(CSIDL_DESKTOP);
	else if (real_path == m_volume[index].get_path(CSIDL_MYDOCUMENTS))
		return m_volume[index].get_label(CSIDL_MYDOCUMENTS);

	CString volume_path = real_path;

	if (real_path.Mid(1, 2) != _T(":\\"))
		return real_path;

	CString volume = m_volume[index].get_drive_volume(real_path);
	volume_path.Replace(CString(real_path[0]) + _T(":"), volume);

	return volume_path;
}

//C:\\, C:\\Program Files, C:\\Windows ��� ���� �ֿ� ������ rename, delete���� �׼��� ������� �ʾƾ� �Ѵ�.
//�� PC, �ٿ�ε�, ���� ȭ��, ���� ���� ������ ������� �ʾƾ� �Ѵ�.
bool CShellImageList::is_protected(int index, CString folder)
{
	folder = convert_special_folder_to_real_path(index, folder);
	//folder.MakeLower();

	//���� ȭ��, ���� �� Ư�� ������ ��ȣ. "�� PC"�� �н��� ""���� ���õǾ� �����Ƿ� �� ����.
	//std::map<int, CString> path_map = *plist->m_volume[index].get_path_map();
	//if (folder.Find(path_map[CSIDL_DESKTOP]) >= 0)
	//	return true;
	//if (folder.Find(path_map[CSIDL_MYDOCUMENTS]) >= 0)
	//	return true;

	//����̺� ��Ʈ�� ��� ��ȣ.
	for (auto drive_list : *m_volume[index].get_drive_list())
	{
		//CString drive_root = convert_special_folder_to_real_path(drive_list.path, plist, index);
		if (folder.CompareNoCase(drive_list.path) == 0)
			return true;
	}

	//windows ���� �� ���� ������ ��� ��ȣ.
	if (folder.Find(_T("C:\\Windows")) >= 0)
		return true;


	std::deque<CString> protected_folder;
	protected_folder.push_back(_T("C:\\Documents and Settings"));
	protected_folder.push_back(_T("C:\\Program Files"));
	protected_folder.push_back(_T("C:\\Program Files (x86)"));
	protected_folder.push_back(_T("C:\\ProgramData"));
	protected_folder.push_back(_T("C:\\Recovery"));
	protected_folder.push_back(_T("C:\\System Volume Information"));
	protected_folder.push_back(_T("C:\\Windows"));
	protected_folder.push_back(_T("C:\\Users"));

	int found = find_dqstring(protected_folder, folder, true, false);
	if (found >= 0)
		return true;

	return false;
}
