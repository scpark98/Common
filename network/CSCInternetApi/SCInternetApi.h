#pragma once

/*
* internet 관련 API 모음.
* CRequestUrlParams을 채워주고
* CSCInternetApi의 request_url(params)를 호출한다.
* use_thread = true일 경우 thread 방식으로 동작되고 메시지로 그 결과를 받아 처리한다.
*/

#include <afxwin.h>
#include <vector>
#include <queue>
#include <deque>
#include <WinInet.h>
#include <afxinet.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "../../Functions.h"

/*
* 개발 목적 :
*	단순 request 또는 web file download등의 http request를 간단히 사용하기 위한 클래스.
* 
* 사용 방법 :
*	- CSCInternetApi	m_api; //인스턴스 선언 후
* 
*	[단순 API request인 경우]
*			CRequestUrlParams params(m_server_ip, m_server_port, _T(""), _T("GET"));
			params.sub_url.Format(_T("/lmm/api/v1.0/temp_envcheck/config-value-return?input_type=os_version&mgrid=%s"), m_login_id);
			m_api.request_url(&params);
			//params의 status 및 result 멤버로 결과 처리.

	[용량이 큰 파일을 API request로 다운받을 경우]
			m_api.set_parent(m_hWnd);	//처리 상태를 받을 윈도우 핸들 지정.

			//params를 반드시 동적으로 생성하고 필요 파라미터들을 채운 후 request_url()호출.
			CString file_path = _T("lmmviewer/lmmviewer_old(solution).zip");	//210 MB
			CRequestUrlParams* params = (CRequestUrlParams*)new CRequestUrlParams(m_server_ip, m_server_port, _T(""), _T("GET"));
			params->sub_url.Format(_T("/download/%s"), file_path);
			params->local_file_path.Format(_T("d:\\temp\\%s_%d"), file_path, count);
			params->use_thread = true;
			params->request_id = count;
			m_api.request_url(params);

			//MESSAGE_MAP에 등록된 아래 메시지 핸들러에서 다운로드 상황등의 정보를 처리한 후 params를 delete.
			ON_REGISTERED_MESSAGE(Message_CInternetApi, on_message_internet_api)

* 주의 사항 :
	- 용량이 수십 MB 정도의 파일은 연속해서 다운로드 시켜도 queue에 순차적으로 job이 저장되고 처리되어 모두 다운로드되지만
	  (인터넷 환경에 따라 결과는 다를 수 있음)
	  200 MB 넘는 파일들을 동시에 다운로드 시키면 최대 2개까지만 다운로드되고
	  (https://deguls.tistory.com/entry/WinInet%EC%9D%B4-%EC%84%9C%EB%B2%84%EB%8B%B9-%EC%97%B0%EA%B2%B0-%EC%88%98%EB%A5%BC-%EC%A0%9C%ED%95%9C%ED%95%9C%EB%8B%A4)
	  HttpSendRequest()가 실패하므로 구현 방식을 달리해야 한다.
	  (ThreadPool을 이용해서도 테스트 해봤으나 동일하게 실패함)
	  즉, 다운로드는 thread를 통해 다운로드 하지만 하나의 파일이 다운로드가 끝나면 다음 파일을 다운로드 하도록 구현해야 한다.
	  이를 main에서 하려면 번거로우므로 이 클래스에서 별도의 thread를 생성한 후 순차적으로 다운로드 하도록 구현함.
* 
*/

static const UINT Message_CSCInternetApi = ::RegisterWindowMessage(_T("MessageString_CSCInternetApi"));
/*
class CRequestUrlParams
{
public:
	CRequestUrlParams() {}
	CRequestUrlParams(CString _ip, int _port, CString _sub_url = _T(""), CString _method = _T("GET"), bool _is_https = true, std::deque<CString>*_headers = NULL, CString _body = _T(""), CString _local_file_path = _T(""));
	CRequestUrlParams(CString _full_url, CString _method = _T("GET"), bool _is_https = true, std::vector<CString>* _headers = NULL, CString _body = _T(""), CString _local_file_path = _T(""));

	//용량이 큰 파일 다운로드 등은 CRequestUrlParams *params를 동적으로 할당받고 thread로 실행한 후 Message_CInternetApi 핸들러로 다운로드 상태를 알 수 있다.
	//다운로드 완료 후 추가적인 처리가 다 끝나면 동적 할당받은 params은 delete해 줘야 한다.
	//request 결과를 바로 받아서 처리하는 단순 request인 경우는 use_thread=false로 하여
	bool		use_thread = false;

	//m_request_id로 해당 작업이 무엇인지 구분한다.
	int			request_id = -1;

	//200, 404...와 같은 HTTP_STATUS를 담지만 invalid address 등과 같은 에러코드도 담기 위해 int로 사용한다. 0보다 작을 경우는 result 문자열에 에러 내용이 담겨있다.
	int			status = -1;

	//포트로 http와 https를 구분하는 것은 위험하다. is_https=true 또는 ip에 "https://"가 포함되어 있으면 is_https가 자동 true로 설정된다.
	CString		ip = _T("");

	int			port = 443;
	CString		sub_url = _T("");;			//domain을 제외한 나머지 주소
	CString		method = _T("");;
	bool		is_https = true;
	CString		body = _T("");;				//post data(json format)
	std::deque<CString> headers;			//각 항목의 끝에는 반드시 "\r\n"을 붙여줘야 한다.
	CString		full_url = _T("");;			//[in][out] full_url을 주고 호출하면 이를 ip, port, sub_url로 나눠서 처리한다. ""로 호출하면 
	CString		result = _T("");;
	long		elapsed = 0;				//소요시간. ms단위.


	//파일 다운로드 관련
	CString		local_file_path = _T("");;	//url의 파일을 다운받을 경우 로컬 파일 full path 지정.
	uint64_t	file_size = 0;				//url 파일 크기
	uint64_t	downloaded_size = 0;		//현재까지 받은 크기
	int			download_index = -1;		//n개의 파일 다운로드시 현재 파일의 인덱스. request_id와는 다름.
};
*/
class CSCInternetApi
{
public:
	CSCInternetApi();
	~CSCInternetApi();

	static bool	parse_url(CString full_url, CString& ip, int& port, CString& sub_url, bool is_https = true);

	enum API_MSG
	{
		msg_api_completed,
		msg_api_download_status,
	};

	//thread로 호출될 경우 다운로드 상태를 알려주기 위해 parent의 핸들 저장.
	void		set_parent(HWND hWnd) { m_hParentWnd = hWnd; }

	//use_thread=true이면 queue에 넣어 순차적으로 job을 처리하는데
	//용량이 큰 파일을 다운받는 request인 경우는 HttpSendRequest()가 바로 실패하므로.
	void		request_url(CRequestUrlParams* params);

	//구현중.
	//remote의 파일들을 local 경로로 다운로드(remote, local은 상대 경로로 명시)
	//HttpSendRequest()는 동시에 2개까지만 허용되므로 병렬적인 다운로드는 불가능하다.
	//AutoPatcher와 같이 UI상에도 순차적인 표시가 필요할 수 있으므로 별도로 구현한다.
	//
	void		download_files(CRequestUrlParams* params, CString remote_root, CString local_root, std::deque<CString> remote_files, std::deque<CString> local_files);
	void		cancel_download_files();

protected:
	HWND		m_hParentWnd = NULL;

	//단일 또는 thread에 의해서만 호출되서 사용된다.
	void		request_url_api(CRequestUrlParams* params);

	std::mutex	m_mutex;
	std::deque<CRequestUrlParams*> m_queue;
	bool		m_running_thread_request_queue = false;
	bool		m_processing_thread_terminated = false;		//큐에 저장된 request들을 처리하는 쓰레드가 정상 종료되었는지.
	void		thread_process_queued_request();

	bool		m_running_thread_download_files = false;
	void		thread_download_files(CRequestUrlParams* params, CString remote_root, CString local_root, std::deque<CString> remote_files, std::deque<CString> local_files);
};
