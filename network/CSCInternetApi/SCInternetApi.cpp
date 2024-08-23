#include "SCInternetApi.h"

#pragma warning(disable : 4996)		// disable bogus deprecation warning
/*
CRequestUrlParams::CRequestUrlParams(CString _ip, int _port, CString _sub_url, CString _method, bool _is_https, std::vector<CString>* _headers, CString _body, CString _local_file_path)
{
	ip = _ip;
	port = _port;
	sub_url = _sub_url;

	method = _method;
	is_https = _is_https;
	body = _body;
	local_file_path = _local_file_path;

	if (_headers)
	{
		for (int i = 0; i < _headers->size(); i++)
		{
			if (_headers->at(i).Right(2) != _T("\r\n"))
				_headers->at(i) += _T("\r\n");
			headers.push_back(_headers->at(i));
		}
	}
}

//이 생성자는 임시 보류
CRequestUrlParams::CRequestUrlParams(CString _full_url, CString _method, bool _is_https, std::vector<CString>* _headers, CString _body, CString _local_file_path)
{
	full_url = _full_url;
	CSCInternetApi::parse_url(full_url, ip, port, sub_url);
	CRequestUrlParams(ip, port, sub_url, _method, _is_https, _headers, _body, _local_file_path);

	//method = _method;
	//is_https = _is_https;
	//body = _body;
	//local_file_path = _local_file_path;

	//if (_headers)
	//{
	//	for (int i = 0; i < _headers->size(); i++)
	//	{
	//		if (_headers->at(i).Right(2) != _T("\r\n"))
	//			_headers->at(i) += _T("\r\n");
	//		headers.push_back(_headers->at(i));
	//	}
	//}
}
*/
CSCInternetApi::CSCInternetApi()
{
	std::thread t(&CSCInternetApi::thread_process_queued_request, this);
	t.detach();
}

CSCInternetApi::~CSCInternetApi()
{
	m_running_thread_request_queue = false;

	while (!m_processing_thread_terminated)
		Sleep(100);
}

bool CSCInternetApi::parse_url(CString full_url, CString& ip, int& port, CString& sub_url, bool is_https)
{
	DWORD dwServiceType;
	INTERNET_PORT nPort;

	//AfxParseURL()을 사용하기 위해서는 url에 반드시 http:// 또는 https:// 등과 같은 서비스 종류가 표시되어야 한다.
	if (full_url.Left(7) != _T("http://") &&
		full_url.Left(8) != _T("https://"))
		full_url = (is_https ? _T("https://") : _T("http://")) + full_url;

	bool ret = AfxParseURL(full_url, dwServiceType, ip, sub_url, nPort);
	port = (int)nPort;
	return ret;
}

//url을 호출하여 결과값을 리턴하거나 지정된 로컬 파일로 다운로드 한다.
//local_file_path가 ""이면 결과값을 문자열로 리턴받는다.
//local_file_path가 지정되어 있으면 파일로 다운받는다.
//리턴값이 200이 아닐 경우는 리턴된 에러코드와 result_str에 저장된 에러 메시지를 조합하여 에러 처리한다.
//HttpSendRequest()의 경우 동시 요청 제한은 2개로 제한된다.
void CSCInternetApi::request_url_api(CRequestUrlParams* params)
{
	TRACE(_T("request_url_api started. request_id = %d\n"), params->request_id);

	long t0 = clock();

	//ip에 http://인지 https://인지가 명시되어 있다면 이는 명확하므로
	//이를 판단하여 params->is_https값을 재설정한다.
	//포트번호로 https를 판별하는 것은 한계가 있으므로 ip에 명시하든, params->is_https에 정확히 명시하여 사용한다.
	if (params->ip.Left(7) == _T("http://") || params->port == 80)
	{
		params->is_https = false;
	}
	else if (params->ip.Left(8) == _T("https://") || params->port == 443)
	{
		params->is_https = true;
	}


	if (params->full_url.IsEmpty() == false)
	{
		parse_url(params->full_url, params->ip, params->port, params->sub_url, params->is_https);
	}

	//sub_url의 맨 앞에는 반드시 '/'가 붙어있어야 한다.
	if (params->sub_url[0] != '/')
		params->sub_url = _T("/") + params->sub_url;

	params->full_url.Format(_T("%s%s:%d%s"),
		(params->is_https ? _T("https://") : _T("http://")),
		params->ip, params->port, params->sub_url);

	bool ret;
	CString str;
	CString remoteURL;
	TCHAR szHead[] = _T("Accept: */*\r\n\r\n");

	if (params->ip.GetLength() < 7)
	{
		params->result = _T("Invalid IP address = ") + params->ip;
		params->status = -1;

		if (params->use_thread && m_hParentWnd)
			::SendMessage(m_hParentWnd, Message_CSCInternetApi, (WPARAM)msg_api_completed, (LPARAM)params);

		return;
	}

	//포트번호로 https를 판별하는 것은 한계가 있으므로 ip에 명시하든, params->is_https에 정확히 명시하여 사용한다.
	if (params->is_https != false && params->ip.Left(8) == _T("https://"))
	{
		params->is_https = true;
	}
	else if (params->is_https == true && params->ip.Left(7) == _T("http://"))
	{
		params->is_https = false;
	}

	//포트가 0보다 작으면 기본 포트를 사용한다.
	if (params->port <= 0)
	{
		if (params->is_https)
			params->port = 443;
		else
			params->port = 80;
	}

	params->method.MakeUpper();
	/*
	if (!is_one_of(params->method, _T("GET"), _T("PUT"), _T("POST"), _T("DELETE")))
	{
		params->status = HTTP_STATUS_BAD_METHOD;
		params->result = _T("Unknown HTTP Request method(\"") + params->method + _T("\")");
		return;
	}
	*/

	HINTERNET hInternetRoot = InternetOpen(_T("request_url"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternetRoot == NULL)
	{
		params->status = GetLastError();
		params->result = _T("InternetOpen() failed.");

		if (params->use_thread && m_hParentWnd)
			::SendMessage(m_hParentWnd, Message_CSCInternetApi, (WPARAM)msg_api_completed, (LPARAM)params);

		return;
	}

	//ip에 http:// 또는 https:// 가 붙어 있으면 InternetConnect()은 실패한다. 제거하고 호출해줘야 한다.
	CString ip = params->ip;
	if (ip.Left(7) == _T("http://"))
		ip = ip.Mid(7);
	if (ip.Left(8) == _T("https://"))
		ip = ip.Mid(8);


	HINTERNET hInternetConnect = InternetConnect(hInternetRoot,
		ip,
		params->port,
		_T(""),
		_T(""),
		INTERNET_SERVICE_HTTP,
		0,
		0);

	if (hInternetConnect == NULL)
	{
		params->result = _T("hInternetConnect() failed.");
		params->status = GetLastError();

		if (params->use_thread && m_hParentWnd)
			::SendMessage(m_hParentWnd, Message_CSCInternetApi, (WPARAM)msg_api_completed, (LPARAM)params);

		return;
	}


	//HINTERNET hURL = InternetOpenUrl(hInternetRoot, remoteURL, szHead, -1L, secureFlags, 0);
	//if (hURL == NULL) {
	//	InternetCloseHandle(hInternetRoot);
	//	return _T("error=InternetOpenUrl() failed.");
	//}

	int secureFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;// | INTERNET_FLAG_TRANSFER_BINARY; // http
	if (params->is_https)
	{
		secureFlags |= INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID; // https
	}

	HINTERNET hOpenRequest = HttpOpenRequest(hInternetConnect,
		params->method,
		params->sub_url,
		HTTP_VERSION,
		_T(""),
		NULL,
		secureFlags,
		0);

	DWORD dwTimeout = 10000;
	InternetSetOption(hOpenRequest, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTimeout, sizeof(DWORD));

	if (params->is_https)
	{
		DWORD dwFlags = 0;
		DWORD dwBuffLen = sizeof(dwFlags);

		dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
			SECURITY_FLAG_IGNORE_REVOCATION |
			SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTP |
			SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTPS |
			SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
			SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
		InternetSetOption(hOpenRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
	}

	//디폴트 헤더 세팅
	if (params->local_file_path.IsEmpty())
		params->headers.push_front(_T("Content-Type: application/json; charset=utf-8\r\n"));
	else
		params->headers.push_front(_T("Content-Type: application/x-www-form-urlencoded\r\n"));

	if (params->headers.size() >= 1)
	{
		for (int i = 0; i < params->headers.size(); i++)
		{
			if (params->headers[i].Right(2) != _T("\r\n"))
				params->headers[i] += _T("\r\n");
			HttpAddRequestHeaders(hOpenRequest, params->headers[i], -1, HTTP_ADDREQ_FLAG_ADD);
		}
	}


#ifdef _UNICODE
	int char_str_len = WideCharToMultiByte(CP_UTF8, 0, params->body, -1, NULL, NULL, NULL, NULL);
	char jsonData[1024] = { 0, };
	ZeroMemory(jsonData, char_str_len);
	WideCharToMultiByte(CP_UTF8, 0, params->body, -1, jsonData, char_str_len, 0, 0);

	ret = HttpSendRequest(hOpenRequest, NULL, 0, jsonData, strlen(jsonData));
#else
	//char* charMsg = jsonBody.GetBuffer(jsonBody.GetLength());
	int char_str_len = WideCharToMultiByte(CP_UTF8, 0, (CStringW)jsonBody, -1, NULL, 0, NULL, NULL);
	char jsonData[1024] = { 0, };
	WideCharToMultiByte(CP_UTF8, 0, (CStringW)jsonBody, -1, jsonData, char_str_len, 0, 0);

	ret = HttpSendRequest(hOpenRequest, NULL, 0, jsonData, strlen(jsonData));
#endif

	if (!ret)
	{
		DWORD dwError = GetLastError();

		params->result.Format(_T("HttpSendRequest failed. error code = %d"), dwError);
		params->status = dwError;

		if (params->use_thread && m_hParentWnd)
			::SendMessage(m_hParentWnd, Message_CSCInternetApi, (WPARAM)msg_api_completed, (LPARAM)params);

		InternetCloseHandle(hOpenRequest);
		InternetCloseHandle(hInternetConnect);
		InternetCloseHandle(hInternetRoot);


		return;
	}

	DWORD buffer_size = 1024 * 1024;
	DWORD dwSize, dwRead, dwWritten;
	char* buffer = new char[buffer_size];
	char* total_buffer = NULL;
	TCHAR query_buffer[32] = { 0, };
	DWORD query_buffer_size = sizeof(query_buffer);

	memset(buffer, 0, buffer_size);

	if (params->local_file_path.IsEmpty())
	{
		total_buffer = new char[buffer_size * 10];
		memset(total_buffer, 0, buffer_size * 10);
	}

	//size_buffer가 char면 파일크기를 못얻어온다.
	//파일이 존재하지 않아도 에러 내용이 포함된 html이 넘어오므로 항상 그 값이 0보다 크다.
	// 연결정보 확인

	ret = HttpQueryInfo(hOpenRequest, HTTP_QUERY_STATUS_CODE, (LPVOID)&query_buffer, &query_buffer_size, NULL);
	params->status = _ttol(query_buffer);

	if (!ret)
	{
		if (buffer)
			delete[] buffer;

		if (total_buffer)
			delete[] total_buffer;

		InternetCloseHandle(hOpenRequest);
		InternetCloseHandle(hInternetConnect);
		InternetCloseHandle(hInternetRoot);

		params->result = _T("HttpQueryInfo(HTTP_QUERY_STATUS_CODE) failed.");
		params->status = -1;

		if (params->use_thread && m_hParentWnd)
			::SendMessage(m_hParentWnd, Message_CSCInternetApi, (WPARAM)msg_api_completed, (LPARAM)params);

		return;
	}

	if (params->status != HTTP_STATUS_OK)
	{
		if (buffer)
			delete[] buffer;

		if (total_buffer)
			delete[] total_buffer;

		InternetCloseHandle(hOpenRequest);
		InternetCloseHandle(hInternetConnect);
		InternetCloseHandle(hInternetRoot);

		params->result = _T("request failed.");

		if (params->use_thread && m_hParentWnd)
			::SendMessage(m_hParentWnd, Message_CSCInternetApi, (WPARAM)msg_api_completed, (LPARAM)params);

		return;
	}

	HANDLE hFile = NULL;

	//다운로드 폴더가 없다면 생성.
	//Functions.h를 사용하지 않아야하므로 make_full_directory()는 잠시 주석처리 함.
	if (params->local_file_path.IsEmpty() == false)
	{
		CString folder = get_part(params->local_file_path, fn_folder);
		make_full_directory(folder);
	}

	params->result.Empty();

	//0바이트의 파일은 다운받지 않아도 될 듯 하지만
	//서버의 파일과 다운받은 로컬의 파일의 수가 같은지 등을 비교할 수도 있으므로 생성하자.
	//HTTP_QUERY_FLAG_NUMBER을 넣지 않으면 HttpQueryInfo()에서 오류가 발생한다.
	uint64_t total_read = 0;
	uint64_t total_size = 0;
	DWORD dwBufLen = sizeof(total_size);

	//(LPVOID)&params->file_size로 query하면 값을 얻어오지 못한다.
	ret = HttpQueryInfo(hOpenRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&total_size, &dwBufLen, NULL);
	params->file_size = total_size;

	params->downloaded_size = 0;

	do
	{
		//InternetQueryDataAvailable(hOpenRequest, &dwSize, 0, 0); //이 함수는 웹페이지의 크기를 리턴하는 듯하다.
		long t2 = clock();
		//1.48GB, buffer_size에 따른 시간 비교. 1K:133s, 4K:65s, 1M:63s, 4M:64s
		//버퍼 크기가 1K면 너무 빈번한 read가 발생하여 느리지만 4K이상이면 큰 차이는 발생하지 않는다.
		InternetReadFile(hOpenRequest, buffer, buffer_size, &dwRead);

		total_read += dwRead;

		if (dwRead == 0)
			break;

		if (params->local_file_path.IsEmpty())
		{
			strncat(total_buffer, buffer, dwRead);
		}
		else
		{
			//remote file이 존재하지 않을 경우 로컬에 파일을 만들지 않기 위해 여기서 체크.
			if (hFile == NULL)
			{
				if (PathFileExists(params->local_file_path) && !DeleteFile(params->local_file_path))
				{
					if (buffer)
						delete[] buffer;

					if (total_buffer)
						delete[] total_buffer;

					InternetCloseHandle(hOpenRequest);
					InternetCloseHandle(hInternetConnect);
					InternetCloseHandle(hInternetRoot);

					params->result = _T("error=") + params->local_file_path + _T("\n\nfail to DeleteFile().");
					params->status = -1;

					if (params->use_thread && m_hParentWnd)
						::PostMessage(m_hParentWnd, Message_CSCInternetApi, (WPARAM)msg_api_completed, (LPARAM)params);

					return;
				}

				hFile = CreateFile(params->local_file_path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile == INVALID_HANDLE_VALUE)
				{
					if (buffer)
						delete[] buffer;

					if (total_buffer)
						delete[] total_buffer;

					InternetCloseHandle(hOpenRequest);
					InternetCloseHandle(hInternetConnect);
					InternetCloseHandle(hInternetRoot);

					params->result = _T("fail to CreateFile() : ") + params->local_file_path;
					params->status = -1;

					if (params->use_thread && m_hParentWnd)
						::SendMessage(m_hParentWnd, Message_CSCInternetApi, (WPARAM)msg_api_completed, (LPARAM)params);

					return;
				}
			}

			WriteFile(hFile, buffer, dwRead, &dwWritten, NULL);
			params->downloaded_size += dwWritten;
			//TRACE(_T("clock = %ld, file_size = %I64u, write = %I64u\n"), clock() - t2, params->file_size, params->downloaded_size);

			if (params->use_thread && m_hParentWnd)
			{
				::SendMessage(m_hParentWnd, Message_CSCInternetApi, (WPARAM)msg_api_download_status, (LPARAM)params);
			}
		}
	} while (dwRead != 0);

	if (params->local_file_path.IsEmpty())
	{
		//params->result = UTF8toCString(total_buffer);
		params->result = CString(total_buffer);
	}
	else
	{
		CloseHandle(hFile);
		params->result = _T("");
	}

	if (buffer)
		delete[] buffer;

	if (total_buffer)
		delete[] total_buffer;

	InternetCloseHandle(hOpenRequest);
	InternetCloseHandle(hInternetConnect);
	InternetCloseHandle(hInternetRoot);

	params->elapsed = clock() - t0;
	TRACE(_T("elapsed = %ld\n"), clock() - t0);

	if (params->use_thread && m_hParentWnd)
		::SendMessage(m_hParentWnd, Message_CSCInternetApi, (WPARAM)msg_api_completed, (LPARAM)params);
}

void CSCInternetApi::request_url(CRequestUrlParams* params)
{
	if (params->use_thread)
	{
		m_mutex.lock();
		CRequestUrlParams* clone_param = new CRequestUrlParams(*params);
		//CRequestUrlParams* clone_param = new CRequestUrlParams;
		//memcpy(clone_param, params, sizeof(CRequestUrlParams));
		m_queue.push_back(clone_param);
		m_mutex.unlock();
		//std::thread t(&CSCInternetApi::request_url_api, this, params);
		//t.detach();
		//std::thread t(&CSCInternetApi::request_url_api, this, params);
		//t.detach();
	}
	else
	{
		request_url_api(params);
	}
}

void CSCInternetApi::thread_process_queued_request()
{
	m_running_thread_request_queue = true;
	m_processing_thread_terminated = false;

	while (m_running_thread_request_queue)
	{
		m_mutex.lock();

		if (m_queue.size())
		{
			TRACE(_T("thread queue size = %d\n"), m_queue.size());

			CRequestUrlParams* params = m_queue.front();
			std::thread t(&CSCInternetApi::request_url_api, this, params);
			t.detach();
			m_queue.pop_front();
		}

		m_mutex.unlock();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	//thread가 종료되면 queue에 남아있는 데이터를 모두 release하고 pop해줘야 한다.
	while (m_queue.size())
	{
		CRequestUrlParams* params = m_queue.front();
		delete params;
		m_queue.pop_front();
	}

	m_processing_thread_terminated = true;
}

void CSCInternetApi::download_files(CRequestUrlParams* params, CString remote_root, CString local_root, std::deque<CString> remote_files, std::deque<CString> local_files)
{
	params->use_thread = true;
	std::thread t(&CSCInternetApi::thread_download_files, this, params, remote_root, local_root, remote_files, local_files);
	t.detach();
}

void CSCInternetApi::cancel_download_files()
{
	m_running_thread_download_files = false;
}

void CSCInternetApi::thread_download_files(CRequestUrlParams* params, CString remote_root, CString local_root, std::deque<CString> remote_files, std::deque<CString> local_files)
{
	m_running_thread_download_files = true;

	int i;

	//다운로드가 완료되면 main으로 Message_CSCInternetApi 메시지를 보내서 필요한 처리를 한 후 delete하므로
	//다음 파일 전송시에는 params가 유효하지 않다.
	//이 구조를 유지하기 위해 각 request시에 새로 new로 할당하여 request를 보낸다.

	CRequestUrlParams temp(*params);
	delete params;

	for (i = 0; i < remote_files.size(); i++)
	{
		if (!m_running_thread_download_files)
			break;

		TRACE(_T("%d/%d download start...\n"), i + 1, remote_files.size());

		params = new CRequestUrlParams(temp);
		params->request_id = i;
		params->sub_url.Format(_T("%s/%s"), remote_root, remote_files[i]);
		params->local_file_path.Format(_T("%s\\%s"), local_root, local_files[i]);

		//로컬 파일이 저장될 경로는 반드시 존재해야 한다.
		//경우에 따라 local_files[i]에 파일명만 존재하는 것이 아니라 경로까지 포함할 수 있으므로 주의.
		//CreateDirectory();

		request_url_api(params);
		TRACE(_T("%d/%d download completed.\n"), i + 1, remote_files.size());
	}

	TRACE(_T("thread_download_files terminated.\n"));
}

