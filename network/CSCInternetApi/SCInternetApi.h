#pragma once

/*
* internet ���� API ����.
* CRequestUrlParams�� ä���ְ�
* CSCInternetApi�� request_url(params)�� ȣ���Ѵ�.
* use_thread = true�� ��� thread ������� ���۵ǰ� �޽����� �� ����� �޾� ó���Ѵ�.
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
* ���� ���� :
*	�ܼ� request �Ǵ� web file download���� http request�� ������ ����ϱ� ���� Ŭ����.
* 
* ��� ��� :
*	- CSCInternetApi	m_api; //�ν��Ͻ� ���� ��
* 
*	[�ܼ� API request�� ���]
*			CRequestUrlParams params(m_server_ip, m_server_port, _T(""), _T("GET"));
			params.sub_url.Format(_T("/lmm/api/v1.0/temp_envcheck/config-value-return?input_type=os_version&mgrid=%s"), m_login_id);
			m_api.request_url(&params);
			//params�� status �� result ����� ��� ó��.

	[�뷮�� ū ������ API request�� �ٿ���� ���]
			m_api.set_parent(m_hWnd);	//ó�� ���¸� ���� ������ �ڵ� ����.

			//params�� �ݵ�� �������� �����ϰ� �ʿ� �Ķ���͵��� ä�� �� request_url()ȣ��.
			CString file_path = _T("lmmviewer/lmmviewer_old(solution).zip");	//210 MB
			CRequestUrlParams* params = (CRequestUrlParams*)new CRequestUrlParams(m_server_ip, m_server_port, _T(""), _T("GET"));
			params->sub_url.Format(_T("/download/%s"), file_path);
			params->local_file_path.Format(_T("d:\\temp\\%s_%d"), file_path, count);
			params->use_thread = true;
			params->request_id = count;
			m_api.request_url(params);

			//MESSAGE_MAP�� ��ϵ� �Ʒ� �޽��� �ڵ鷯���� �ٿ�ε� ��Ȳ���� ������ ó���� �� params�� delete.
			ON_REGISTERED_MESSAGE(Message_CInternetApi, on_message_internet_api)

* ���� ���� :
	- �뷮�� ���� MB ������ ������ �����ؼ� �ٿ�ε� ���ѵ� queue�� ���������� job�� ����ǰ� ó���Ǿ� ��� �ٿ�ε������
	  (���ͳ� ȯ�濡 ���� ����� �ٸ� �� ����)
	  200 MB �Ѵ� ���ϵ��� ���ÿ� �ٿ�ε� ��Ű�� �ִ� 2�������� �ٿ�ε�ǰ�
	  (https://deguls.tistory.com/entry/WinInet%EC%9D%B4-%EC%84%9C%EB%B2%84%EB%8B%B9-%EC%97%B0%EA%B2%B0-%EC%88%98%EB%A5%BC-%EC%A0%9C%ED%95%9C%ED%95%9C%EB%8B%A4)
	  HttpSendRequest()�� �����ϹǷ� ���� ����� �޸��ؾ� �Ѵ�.
	  (ThreadPool�� �̿��ؼ��� �׽�Ʈ �غ����� �����ϰ� ������)
	  ��, �ٿ�ε�� thread�� ���� �ٿ�ε� ������ �ϳ��� ������ �ٿ�ε尡 ������ ���� ������ �ٿ�ε� �ϵ��� �����ؾ� �Ѵ�.
	  �̸� main���� �Ϸ��� ���ŷο�Ƿ� �� Ŭ�������� ������ thread�� ������ �� ���������� �ٿ�ε� �ϵ��� ������.
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

	//�뷮�� ū ���� �ٿ�ε� ���� CRequestUrlParams *params�� �������� �Ҵ�ް� thread�� ������ �� Message_CInternetApi �ڵ鷯�� �ٿ�ε� ���¸� �� �� �ִ�.
	//�ٿ�ε� �Ϸ� �� �߰����� ó���� �� ������ ���� �Ҵ���� params�� delete�� ��� �Ѵ�.
	//request ����� �ٷ� �޾Ƽ� ó���ϴ� �ܼ� request�� ���� use_thread=false�� �Ͽ�
	bool		use_thread = false;

	//m_request_id�� �ش� �۾��� �������� �����Ѵ�.
	int			request_id = -1;

	//200, 404...�� ���� HTTP_STATUS�� ������ invalid address ��� ���� �����ڵ嵵 ��� ���� int�� ����Ѵ�. 0���� ���� ���� result ���ڿ��� ���� ������ ����ִ�.
	int			status = -1;

	//��Ʈ�� http�� https�� �����ϴ� ���� �����ϴ�. is_https=true �Ǵ� ip�� "https://"�� ���ԵǾ� ������ is_https�� �ڵ� true�� �����ȴ�.
	CString		ip = _T("");

	int			port = 443;
	CString		sub_url = _T("");;			//domain�� ������ ������ �ּ�
	CString		method = _T("");;
	bool		is_https = true;
	CString		body = _T("");;				//post data(json format)
	std::deque<CString> headers;			//�� �׸��� ������ �ݵ�� "\r\n"�� �ٿ���� �Ѵ�.
	CString		full_url = _T("");;			//[in][out] full_url�� �ְ� ȣ���ϸ� �̸� ip, port, sub_url�� ������ ó���Ѵ�. ""�� ȣ���ϸ� 
	CString		result = _T("");;
	long		elapsed = 0;				//�ҿ�ð�. ms����.


	//���� �ٿ�ε� ����
	CString		local_file_path = _T("");;	//url�� ������ �ٿ���� ��� ���� ���� full path ����.
	uint64_t	file_size = 0;				//url ���� ũ��
	uint64_t	downloaded_size = 0;		//������� ���� ũ��
	int			download_index = -1;		//n���� ���� �ٿ�ε�� ���� ������ �ε���. request_id�ʹ� �ٸ�.
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

	//thread�� ȣ��� ��� �ٿ�ε� ���¸� �˷��ֱ� ���� parent�� �ڵ� ����.
	void		set_parent(HWND hWnd) { m_hParentWnd = hWnd; }

	//use_thread=true�̸� queue�� �־� ���������� job�� ó���ϴµ�
	//�뷮�� ū ������ �ٿ�޴� request�� ���� HttpSendRequest()�� �ٷ� �����ϹǷ�.
	void		request_url(CRequestUrlParams* params);

	//������.
	//remote�� ���ϵ��� local ��η� �ٿ�ε�(remote, local�� ��� ��η� ���)
	//HttpSendRequest()�� ���ÿ� 2�������� ���ǹǷ� �������� �ٿ�ε�� �Ұ����ϴ�.
	//AutoPatcher�� ���� UI�󿡵� �������� ǥ�ð� �ʿ��� �� �����Ƿ� ������ �����Ѵ�.
	//
	void		download_files(CRequestUrlParams* params, CString remote_root, CString local_root, std::deque<CString> remote_files, std::deque<CString> local_files);
	void		cancel_download_files();

protected:
	HWND		m_hParentWnd = NULL;

	//���� �Ǵ� thread�� ���ؼ��� ȣ��Ǽ� ���ȴ�.
	void		request_url_api(CRequestUrlParams* params);

	std::mutex	m_mutex;
	std::deque<CRequestUrlParams*> m_queue;
	bool		m_running_thread_request_queue = false;
	bool		m_processing_thread_terminated = false;		//ť�� ����� request���� ó���ϴ� �����尡 ���� ����Ǿ�����.
	void		thread_process_queued_request();

	bool		m_running_thread_download_files = false;
	void		thread_download_files(CRequestUrlParams* params, CString remote_root, CString local_root, std::deque<CString> remote_files, std::deque<CString> local_files);
};
