#pragma once

#include <afxwin.h>

/*
[자체 패치 — 앱이 스스로 최신 버전으로 갈아끼운다]

	배포한 앱을 사용자가 손으로 다시 내려받지 않게 한다. 별도 패처 프로그램 없이 앱 자신이
	시작할 때 서버와 버전을 비교해, 새 버전이면 받아서 배치파일에 교체를 맡기고 스스로 종료한다.

	ManualLauncher(D:\1.Projects_C++\ManualLauncher) 의 check_new_version_itself() / self_update() 를
	여러 프로젝트가 쓸 수 있게 옮겨 온 것이다. 배치파일로 교체하는 원리와 filelist.lst 형식은 그쪽과 같다.

[실행한 그 자리에서 패치한다 — 정해진 설치 폴더가 없다]

	exe 를 어디에 두든 그 폴더에서 그대로 갱신된다. 다운로드 폴더든 D:\util 이든 사용자 마음이다.
	그래서 이미 만들어 둔 바로가기·시작프로그램 등록이 그대로 유효하다.

	(초기 구현은 %LOCALAPPDATA%\<앱> 으로 옮겨 심었다. Program Files 같은 권한 폴더에서
	 배치파일의 move 가 실패하는 것을 피하려던 것인데, 그 대가로 사용자가 둔 자리를 무시하고
	 바로가기를 깨뜨렸다. 권한 없는 폴더는 아래 [권한이 없는 폴더] 처럼 그냥 지나가면 될 일이었다.)

[서버 구성]

	앱마다 폴더를 하나씩 두고 그 안에 최신 한 벌만 올린다. 버전별 하위 폴더는 두지 않는다.

		/download/tools/KoinoTools/<앱 이름>/
			filelist.lst			//"파일명|크기|버전" 한 줄. ';' 또는 '#' 로 시작하면 주석.
			<파일명>.zip			//예: SCDeskTools.exe.zip

	zip 이름은 *확장자를 포함한 원본 파일명* 에 .zip 을 붙인다 (SCDeskTools.exe.zip).
	이름이 같고 확장자만 다른 파일들을 한 폴더에 둘 수 있어야 해서 filelist_maker.exe 가 그렇게 만든다.
	크기와 버전은 압축을 풀었을 때의 값이다.

[사용법]

	App 클래스에 CSCSelfPatch 를 멤버로 두고 InitInstance 앞부분에서 startup(), ExitInstance 에서 shutdown().

		//SCDeskTools.h
		CSCSelfPatch	m_self_patch;

		//InitInstance() — 다이얼로그를 만들기 전이면 된다. mutex 검사 앞뒤 어느 쪽이든 무방하다.
		m_self_patch.server_path = _T("/download/tools/KoinoTools/SCDeskTools");
		m_self_patch.register_startup = true;		//부팅 시 자동 실행이 필요한 앱만
		if (m_self_patch.startup())
			return FALSE;						//패치했다 — 배치파일이 이어서 새 exe 를 띄운다

		//ExitInstance()
		m_self_patch.shutdown();

	프로젝트에 Common/Functions.cpp 와 Common/zip/zip/unzip.cpp 가 함께 들어가야 한다.

[동작 순서]

	startup()
	  1. 지난번 배치파일 잔재를 지운다.
	  2. filelist.lst 를 읽어 로컬 exe 의 FileVersion 과 비교. 서버가 더 새 버전이면 zip 을 받아
	     <exe>_ 로 풀고 self_updater.bat 을 실행한 뒤 true 를 돌려준다.
	  3. register_startup 이면 부팅 자동 실행 등록을 *지금 실행 중인 exe 경로* 로 맞춘다.
	     2 에서 받았으면 여기까지 오지 않는다 — 등록 갱신은 교체 후 새로 뜬 다음 실행에서 이뤄진다.

	버전이 같아도 크기가 다르면 받는다 — 재빌드·재서명처럼 버전은 그대로인 채 내용만 바뀐 경우를 잡는다.

	배치파일은 실행 중인 exe 는 지워지지 않는다는 성질을 이용한다 — del 이 성공한 시점이 곧
	이 프로세스가 죽은 시점이라, 별도의 대기 로직 없이 교체 시점을 정확히 잡는다.

[건드리지 않는 경우 — 모두 조용히 지나간다]

	알림도 오류창도 없다. 앱은 평소대로 뜬다.

	  - 오프라인·사내망 밖 → filelist.lst 요청에 도달성 사전 검사(TCP 2초)를 걸어 두어 바로 포기한다.
	    이게 없으면 WinInet 연결 타임아웃(기본 5초)만큼 앱 시작이 멈춘다.
	  - 서버 폴더나 filelist.lst 가 없음(404) / 300·500번대 응답
	  - filelist.lst 는 받았는데 이 exe 이름과 일치하는 줄이 없음
	  - zip 이 없음(404) — request_url 은 400번대에서도 본문을 읽어 파일로 쓰므로
	    오류 페이지가 로컬 zip 으로 떨어진다. 상태를 보고 지운다.

	**그래서 서버 폴더를 지우거나 이름을 바꾸는 것이 그 앱의 자체 패치를 멈추는 스위치가 된다.**
	임시로 멈추려면 filelist.lst 만 치워도 되고, 영구히 멈추려면 폴더째 없애면 된다.
	클라이언트는 손댈 필요가 없다.

[권한이 없는 폴더 — 새 버전이 있을 때만 한 번 알린다]

	exe 가 Program Files 처럼 쓰기 권한이 없는 곳에 있으면 그 자리에서는 갱신할 수 없다.
	UAC 승격은 요구하지 않는다. 대신:

	  - **서버 버전이 로컬과 같으면 아무 말도 하지 않는다.** 갱신할 것이 없으므로 문제가 아니다.
	  - **새 버전이 실제로 있는데 못 받는 경우에만** 메시지박스로 알린다. 그 exe 경로에 대해 한 번만.
	    "새 버전이 있지만 이 폴더에 쓰기 권한이 없어 자동 업데이트를 할 수 없습니다.
	     쓰기 가능한 폴더로 옮긴 후 실행하십시오."

	권한 판정은 임시 파일을 실제로 만들어 본다 — GetFileAttributes 의 읽기 전용 플래그만으로는
	상속 ACL·네트워크 권한·읽기 전용 매체를 걸러내지 못한다.

	알린 기록은 HKCU\Software\SCSelfPatch\readonly_notified 에 (값 이름 = exe 전체 경로) 로 남는다.
	앱의 SetRegistryKey 에 의존하지 않는 독립 경로다 — startup() 은 SetRegistryKey 보다 먼저 불리는
	프로젝트가 있다. 다시 알리게 하려면 그 값을 지우면 된다.

[Debug 빌드는 패치하지 않는다]

	check_new_version() 이 _DEBUG 에서 즉시 반환한다. 안 그러면 F5 로 띄운 Debug exe 가
	서버의 Release 빌드로 교체되어 디버깅이 끊긴다.

	건너뛰는 것은 버전 검사뿐이다. 배치파일 잔재 정리와 부팅 자동 실행 등록은 Debug 에서도 그대로 돈다 —
	register_startup 인 앱을 Debug 로 실행하면 Run 등록이 Debug 빌드 경로를 가리키게 된다.

	Release 를 빌드 폴더에서 실행하는 것은 막지 않는다. 개발 중에는 로컬 버전이 서버보다 높으므로
	교체가 일어나지 않는다 — 배포 전에 .rc 의 FileVersion 을 올리는 흐름이 그 자체로 가드가 된다.
*/

class CSCSelfPatch
{
public:
	//서버. server_path 는 filelist.lst 와 zip 이 들어 있는 폴더의 경로다 (도메인 제외).
	CString		server_ip = _T("ss.linkmemine.com");
	int			server_port = 443;
	bool		is_https = true;
	CString		server_path;

	//부팅 시 자동 실행 등록 여부. 등록 이름은 exe 타이틀이고 HKCU 라 관리자 권한이 필요 없다.
	//끄면 등록도 하지 않고 이미 있던 등록을 지우지도 않는다 — 이 클래스가 만들지 않은 것을 건드리지 않기 위해서다.
	bool		register_startup = false;

	//InitInstance 에서 다이얼로그를 만들기 전에 호출한다. mutex 검사 앞뒤 어느 쪽이든 무방하다.
	//true = 패치를 배치파일에 넘겼다. 호출자는 즉시 return FALSE 해야 한다.
	bool		startup();

	//ExitInstance 에서 호출한다. 교체하지 못하고 남은 <exe>_ 가 있으면 여기서 한 번 더 시도한다.
	//이때의 배치파일은 교체만 하고 앱을 다시 띄우지 않는다.
	void		shutdown();

private:
	//startup() 이 배치파일을 띄우고 끝낸 경우. shutdown() 이 self_update() 를 또 부르지 않게 한다.
	bool		m_exit_for_self_update = false;

	bool		check_new_version();
	void		notify_not_writable(const CString& server_version);
	void		self_update(bool rerun = false);
	void		update_startup_registration();
	void		remove_batch_files();
};
