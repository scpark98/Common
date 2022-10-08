- include 경로에 libcurl_ssl/include 추가

- lib 경로에 libcurl_ssl/lib 추가
  (libcurl.lib은 release용, libcurld.lib는 debug용이며 다른 2개의 lib는 ssl에서 사용된다)

- lib 입력에 다음 lib 입력
	libcurl.lib
	libeay32.lib
	ssleay32.lib
	wldap32.lib
	ws2_32.lib
	crypt32.lib

- 프로젝트 exe와 동일한 폴더에
  libcurl_ssl/bin에 있는 2개의 dll 복사.
  
[in .h]
#include "curl/curl.h"
...
	CURL* m_curl;
	std::string m_curl_res;

 [in .cpp]
 - onInit : curl_global_init(CURL_GLOBAL_ALL);
 - onExit : curl_global_cleanup();
 
 //callback function
 size_t curl_writefunc(void* ptr, size_t size, size_t nmemb, std::string* s)
{
	s->append(static_cast<char*>(ptr), size * nmemb);
	return size * nmemb;
}

//usage
void CTestCurlDlg::OnBnClickedOk()
{
	m_curl_res = "";

	//get a curl handle
	m_curl = curl_easy_init();
	if (!m_curl)
		return;

	curl_easy_setopt(m_curl, CURLOPT_URL, "https://kakaopayinsu-portal.spectra.co.kr/mocha/sso");

	curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, false);

	struct curl_slist* headerlist = nullptr;
	headerlist = curl_slist_append(headerlist, "Content-Type: application/json");
	headerlist = curl_slist_append(headerlist, "X-Attic-Device: pc");
	headerlist = curl_slist_append(headerlist, "Accept: application/json");
	curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headerlist);

	// Now specify the POST data
	curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, "{\"userId\" : \"koino\", \"ssoToken\" : \"NpbEm0ip3Yj0n5dbJIB5btNl\", \"force\" : true}");

	//set callback function
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl_writefunc);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_curl_res);


	//Perform the request, res will get the return code
	CURLcode res;
	res = curl_easy_perform(m_curl);

	curl_slist_free_all(headerlist);

	if (res != CURLE_OK)
	{
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		AfxMessageBox(CString(curl_easy_strerror(res)));
	}
	else
	{
		AfxMessageBox(CString(m_curl_res.c_str()));
	}

	//always cleanup
	curl_easy_cleanup(m_curl);
}
