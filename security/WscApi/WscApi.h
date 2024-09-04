#pragma once

//https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/WebSecurityCenter/cpp/WscApiSample.cpp

#include <afxwin.h>
#include <Wscapi.h>
#include <iwscapi.h>
#include <deque>

#define WSC_PROVIDER_COUNT 3	//for WSC_SECURITY_PROVIDER_ANTIVIRUS, WSC_SECURITY_PROVIDER_ANTISPYWARE, WSC_SECURITY_PROVIDER_FIREWALL

class CWscProduct
{
public :
	CString name;
	WSC_SECURITY_PRODUCT_STATE state;		//WSC_SECURITY_PRODUCT_STATE_ON / WSC_SECURITY_PRODUCT_STATE_OFF / WSC_SECURITY_PRODUCT_STATE_SNOOZED / WSC_SECURITY_PRODUCT_STATE_EXPIRED
	WSC_SECURITY_SIGNATURE_STATUS status;	//WSC_SECURITY_PRODUCT_UP_TO_DATE or WSC_SECURITY_PRODUCT_OUT_OF_DATE
	CString path;
	CString state_timestamp;
};

class CWscApi
{
public :
	CWscApi();

	void	refresh();

	//사용자가 좀 더 쉽게 사용하기 위한 멤버함수 정의.
	bool	is_antivirus_state_on();
	bool	is_antivirus_latest_version();

	//provider is not a index, but defined value. ex)
	//provider = WSC_SECURITY_PROVIDER_ANTIVIRUS or
	//provider = WSC_SECURITY_PROVIDER_ANTISPYWARE or
	//provider = WSC_SECURITY_PROVIDER_FIREWALL
	size_t	get_product_count(WSC_SECURITY_PROVIDER provider);

	//product_index를 -1로 줄 경우 다음과 같이 처리된다.
	//product 리스트가 1개일 경우는 Windows Defender 항목의 값을 리턴하고
	//2개 이상일 경우는 Windows Defender가 아닌 항목 중 첫 번째 항목의 상태값을 리턴한다.
	//Windows Defender가 기본 설치되어 있는 윈도우에 V3 or Avast등을 설치하면
	//Windows Defender의 Antivirus state는 Off로 변경되고
	//V3 or Avast의 state가 실제 유효한 state가 되므로 이 항목의 state를 리턴해야 함.
	WSC_SECURITY_PRODUCT_STATE	get_product_state(WSC_SECURITY_PROVIDER provider, int product_index);
	WSC_SECURITY_SIGNATURE_STATUS	get_product_status(WSC_SECURITY_PROVIDER provider, int product_index);

protected :
	WSC_SECURITY_PROVIDER provider[WSC_PROVIDER_COUNT];		//for antiVirus, antiSpyware, firewall
	std::deque<CWscProduct> product[WSC_PROVIDER_COUNT];
	IWSCProductList* productList[WSC_PROVIDER_COUNT];
	LONG productCount[WSC_PROVIDER_COUNT];
	int get_provider_index(WSC_SECURITY_PROVIDER provider_def);
};
