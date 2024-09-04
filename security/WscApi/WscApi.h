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

	//����ڰ� �� �� ���� ����ϱ� ���� ����Լ� ����.
	bool	is_antivirus_state_on();
	bool	is_antivirus_latest_version();

	//provider is not a index, but defined value. ex)
	//provider = WSC_SECURITY_PROVIDER_ANTIVIRUS or
	//provider = WSC_SECURITY_PROVIDER_ANTISPYWARE or
	//provider = WSC_SECURITY_PROVIDER_FIREWALL
	size_t	get_product_count(WSC_SECURITY_PROVIDER provider);

	//product_index�� -1�� �� ��� ������ ���� ó���ȴ�.
	//product ����Ʈ�� 1���� ���� Windows Defender �׸��� ���� �����ϰ�
	//2�� �̻��� ���� Windows Defender�� �ƴ� �׸� �� ù ��° �׸��� ���°��� �����Ѵ�.
	//Windows Defender�� �⺻ ��ġ�Ǿ� �ִ� �����쿡 V3 or Avast���� ��ġ�ϸ�
	//Windows Defender�� Antivirus state�� Off�� ����ǰ�
	//V3 or Avast�� state�� ���� ��ȿ�� state�� �ǹǷ� �� �׸��� state�� �����ؾ� ��.
	WSC_SECURITY_PRODUCT_STATE	get_product_state(WSC_SECURITY_PROVIDER provider, int product_index);
	WSC_SECURITY_SIGNATURE_STATUS	get_product_status(WSC_SECURITY_PROVIDER provider, int product_index);

protected :
	WSC_SECURITY_PROVIDER provider[WSC_PROVIDER_COUNT];		//for antiVirus, antiSpyware, firewall
	std::deque<CWscProduct> product[WSC_PROVIDER_COUNT];
	IWSCProductList* productList[WSC_PROVIDER_COUNT];
	LONG productCount[WSC_PROVIDER_COUNT];
	int get_provider_index(WSC_SECURITY_PROVIDER provider_def);
};
