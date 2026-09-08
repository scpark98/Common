#include "SCSelfPatch.h"

#include "../../Functions.h"
#include "../../zip/zip/unzip.h"

//배치파일은 exe 옆에 만들고 다음 실행 때 지운다.
//배치파일이 스스로를 지우게 하지 않는 이유는 ManualLauncher 에서 그렇게 했다가 동작이 이상해진 이력 때문이다.
static LPCTSTR	kBatchSelfUpdate = _T("self_updater.bat");

//20260907 by claude. 초기 구현이 %LOCALAPPDATA% 로 옮겨 심을 때 쓰던 배치파일 이름.
//이제 옮겨 심지 않지만, 그 시절에 옮겨진 사본이 남아 있을 수 있어 한동안 같이 지운다.
static LPCTSTR	kBatchRelocateLegacy = _T("auto_move_and_run.bat");

static LPCTSTR	kStartupRunKey = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");

//20260907 by claude. 쓰기 권한이 없어 갱신하지 못했다고 알린 exe 경로를 기록해 두는 곳.
//값 이름 = exe 전체 경로, 값 = 그때의 서버 버전. 앱의 SetRegistryKey 에 의존하지 않는 독립 경로다 —
//startup() 은 SetRegistryKey 보다 먼저 불리는 프로젝트가 있다.
static LPCTSTR	kNotifiedKey = _T("Software\\SCSelfPatch\\readonly_notified");

//배치파일이 앱을 다시 띄울 때 지금 받은 인자를 그대로 넘겨준다.
//안 그러면 파일을 더블클릭해 실행한 경우(Endorphin2 로 동영상 열기 등) 패치하는 그 한 번은
//그 파일을 열지 못한 채 빈 앱이 뜬다. 배치파일 안에서 '%' 는 변수 참조라 두 개로 늘려야 글자 그대로 나간다.
static CString get_command_line_for_batch()
{
	CWinApp* app = AfxGetApp();

	if (app == NULL || app->m_lpCmdLine == NULL)
		return _T("");

	CString args = app->m_lpCmdLine;
	args.Trim();
	args.Replace(_T("%"), _T("%%"));

	return args;
}

//20260907 by claude. 그 폴더에 쓸 수 있는지 실제로 만들어 보고 판단한다.
//GetFileAttributes 의 읽기 전용 플래그만으로는 상속 ACL·네트워크 권한·읽기 전용 매체를 걸러내지 못한다.
//FILE_FLAG_DELETE_ON_CLOSE 라 핸들을 닫으면 파일은 저절로 사라진다.
static bool is_folder_writable(const CString& folder)
{
	CString probe;
	probe.Format(_T("%s\\~scselfpatch_%lu.tmp"), folder, GetCurrentProcessId());

	HANDLE h = CreateFile(probe, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);

	if (h == INVALID_HANDLE_VALUE)
		return false;

	CloseHandle(h);

	return true;
}

bool CSCSelfPatch::startup()
{
	remove_batch_files();

	if (check_new_version())
	{
		m_exit_for_self_update = true;
		return true;
	}

	if (register_startup)
		update_startup_registration();

	return false;
}

void CSCSelfPatch::shutdown()
{
	//배치파일이 잠금 때문에 교체하지 못하고 <exe>_ 만 남기고 끝난 경우가 있다. 종료할 때 한 번 더 시도한다.
	//방금 startup() 이 띄운 배치파일이 도는 중이면 건드리지 않는다.
	if (!m_exit_for_self_update)
		self_update();
}

//여기까지 실행됐다는 것으로 배치파일은 제 할 일을 마쳤다. exe 옆에 남겨두지 않는다.
void CSCSelfPatch::remove_batch_files()
{
	const CString folder = get_exe_directory();

	DeleteFile(folder + _T("\\") + kBatchSelfUpdate);
	DeleteFile(folder + _T("\\") + kBatchRelocateLegacy);
}

//서버 filelist.lst 를 읽어 로컬 exe 와 비교하고, 서버가 더 새 버전이면 받아서 교체 준비까지 마친다.
bool CSCSelfPatch::check_new_version()
{
#ifdef _DEBUG
	return false;
#endif

	if (server_path.IsEmpty())
		return false;

	CRequestUrlParams param(server_ip, server_port);

	param.sub_url.Format(_T("%s/filelist.lst"), server_path);
	param.is_https = is_https;

	//두 번째 인자 = 도달성 사전 검사. 오프라인·사내망 밖에서 실행하는 경우가 흔한데,
	//이게 없으면 WinInet 의 연결 타임아웃(기본 5초)만큼 앱 시작이 멈춘다. TCP 2초 안에 못 붙으면 바로 포기한다.
	//zip 다운로드는 이 검사를 통과한 뒤라 다시 볼 필요가 없다.
	request_url(&param, true);

	if (param.status != HTTP_STATUS_OK || param.result.IsEmpty())
		return false;

	param.result.Replace(_T("\r\n"), _T("\n"));

	std::deque<CString> lines;
	get_token_str(param.result, lines, _T("\n"), false);

	CString		server_name;
	uint64_t	server_size = 0;
	CString		server_version;

	const CString exe_name = get_part(get_exe_filename(true), fn_name);

	for (auto line : lines)
	{
		line.Trim();

		//filelist_maker.exe 의 규약 — ';' 또는 '#' 로 시작하는 라인은 주석.
		if (line.IsEmpty() || line[0] == _T(';') || line[0] == _T('#'))
			continue;

		std::deque<CString> fileinfo;
		get_token_str(line, fileinfo);

		//"파일명|크기|버전" 세 항목 이상.
		if (fileinfo.size() < 3)
			continue;

		//목록에 부가 파일이 섞여 있어도 실행파일 한 줄만 본다. 지금은 exe 하나만 갱신 대상이다.
		if (fileinfo[0].CompareNoCase(exe_name) != 0)
			continue;

		server_name = fileinfo[0];
		server_size = _ttoi64(fileinfo[1]);
		server_version = fileinfo[2];
		break;
	}

	if (server_name.IsEmpty() || server_version.IsEmpty())
		return false;

	const CString	local_version = get_file_property();
	const uint64_t	local_size = get_file_size(get_exe_filename(true));

	//버전이 같아도 크기가 다르면 받는다 — 재빌드·재서명처럼 버전은 그대로인 채 내용만 바뀐 경우.
	const int version_compare = compare_str(local_version, server_version);

	if (version_compare > 0 || (version_compare == 0 && local_size == server_size))
		return false;

	//여기까지 왔다 = 받아야 한다. 그런데 폴더에 쓸 수 없으면 받아 봐야 실패하므로 미리 걸러 알린다.
	//쓰기 권한이 없어도 서버 버전이 같으면 아무 문제가 없다. 그래서 *실제로 갱신이 막힌* 이 자리에서만 알린다.
	if (!is_folder_writable(get_exe_directory()))
	{
		notify_not_writable(server_version);
		return false;
	}

	//받는 것도 푸는 것도 exe 가 있는 그 폴더에서 한다.
	CString zip_file;
	zip_file.Format(_T("%s\\%s.zip"), get_exe_directory(), server_name);

	//받은 파일을 둘 이름은 서버의 이름이 아니라 *지금 돌고 있는 exe* 의 이름에서 만든다.
	//self_update() 도 같은 식으로 구하므로 exe 를 다른 이름으로 배포했더라도 둘이 어긋나지 않는다.
	const CString new_exe = get_exe_filename(true) + _T("_");

	param.reset();
	param.sub_url.Format(_T("%s/%s.zip"), server_path, server_name);
	param.is_https = is_https;
	param.local_file_path = zip_file;

	request_url(&param);

	if (param.status != HTTP_STATUS_OK)
	{
		DeleteFile(zip_file);
		return false;
	}

	HZIP hz = OpenZip(zip_file, 0);

	if (hz == NULL)
	{
		DeleteFile(zip_file);
		return false;
	}

	ZRESULT zr = UnzipItem(hz, 0, new_exe);
	CloseZip(hz);
	DeleteFile(zip_file);

	if (zr != ZR_OK)
	{
		DeleteFile(new_exe);
		return false;
	}

	//푼 파일 크기가 filelist 의 값과 다르면 전송이 잘린 것이다. 그대로 교체하면 실행되지 않는 exe 만 남는다.
	if (get_file_size(new_exe) != server_size)
	{
		DeleteFile(new_exe);
		return false;
	}

	self_update(true);

	return true;
}

//새 버전이 있는데 폴더에 쓸 수 없어 갱신하지 못했다고 알린다. 이 exe 경로에 대해 한 번만.
//
//트레이 풍선이 아니라 메시지박스인 이유 — 트레이 아이콘이 없는 앱도 있고 놓치기 쉽다.
//이건 사용자가 조치해야 풀리는 상황(쓰기 가능한 폴더로 옮겨 실행)이라 놓치면 알림의 의미가 없다.
void CSCSelfPatch::notify_not_writable(const CString& server_version)
{
	const CString exe = get_exe_filename(true);

	CString notified;
	get_registry_str(HKEY_CURRENT_USER, kNotifiedKey, exe, &notified);

	if (!notified.IsEmpty())
		return;

	set_registry_str(HKEY_CURRENT_USER, kNotifiedKey, exe, server_version);

	CString message;
	message.Format(
		_T("새 버전(%s)이 있지만 이 폴더에 쓰기 권한이 없어 자동 업데이트를 할 수 없습니다.\n\n")
		_T("%s\n\n")
		_T("자동 업데이트를 받으려면 이 프로그램을 쓰기 가능한 폴더로 옮긴 후 실행하십시오.\n")
		_T("(이 안내는 이 위치에 대해 한 번만 표시됩니다.)"),
		server_version,
		get_part(exe, fn_folder));

	AfxMessageBox(message, MB_ICONINFORMATION);
}

//실행 중인 exe 는 자기 자신을 덮어쓸 수 없다. 교체를 배치파일에 맡기고 이 프로세스는 종료한다.
void CSCSelfPatch::self_update(bool rerun)
{
	const CString old_file = get_exe_filename(true);
	const CString new_file = old_file + _T("_");

	//PathFileExists 는 shlwapi.lib 이 필요해 GetFileAttributes 로 대신한다.
	if (GetFileAttributes(new_file) == INVALID_FILE_ATTRIBUTES)
		return;

	const CString bat_file = get_exe_directory() + _T("\\") + kBatchSelfUpdate;

	FILE* fp = NULL;
	_tfopen_s(&fp, bat_file, _T("wt")CHARSET);

	if (fp == NULL)
		return;

	//:Repeat — 이 프로세스가 아직 완전히 죽지 않아 del 이 막히면 지워질 때까지 되돌아온다.
	//지우려는 대상이 방금까지 우리가 실행하던 exe 라 반드시 지워지므로 한도를 두지 않는다.
	//삭제가 성공하는 시점이 곧 이 프로세스가 죽은 시점이라, 별도의 대기 로직 없이 교체 시점을 잡는다.
	_ftprintf_s(fp,
		_T(":Repeat \r\n")
		_T("if exist \"%s\" del /f /q \"%s\" \r\n")
		_T("if exist \"%s\" goto Repeat \r\n")
		_T("move \"%s\" \"%s\" \r\n"),
		old_file, old_file,
		old_file,
		new_file, old_file);

	if (rerun)
	{
		_ftprintf_s(fp, _T("timeout /t 1 \r\n"));
		_ftprintf_s(fp, _T("start \"\" \"%s\" %s\r\n"), old_file, get_command_line_for_batch());
	}

	fflush(fp);
	fclose(fp);

	ShellExecute(NULL, _T("open"), bat_file, NULL, NULL, SW_HIDE);
}

//부팅 자동 실행 등록을 *지금 실행 중인 exe 경로* 로 맞춘다.
//사용자가 exe 를 옮기고 거기서 한 번 실행하면 등록도 그 자리로 따라간다.
//
//시작프로그램 폴더의 바로가기는 건드리지 않는다 — 사용자가 직접 만들어 둔 것일 수 있고,
//인자나 시작 폴더가 들어 있을 수도 있다. Run 등록과 겹쳐 두 번 뜨더라도 중복 실행 검사가 걸러낸다.
void CSCSelfPatch::update_startup_registration()
{
	const CString exe_title = get_part(get_exe_filename(true), fn_title);

	//경로에 공백이 들어갈 수 있으므로 따옴표로 감싼다. 사용자명이 두 단어인 계정에서 실제로 문제가 된다.
	CString command;
	command.Format(_T("\"%s\""), get_exe_filename(true));

	CString registered;
	get_registry_str(HKEY_CURRENT_USER, kStartupRunKey, exe_title, &registered);

	if (registered != command)
		set_registry_str(HKEY_CURRENT_USER, kStartupRunKey, exe_title, command);
}
