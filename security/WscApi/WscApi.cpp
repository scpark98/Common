#include "WscApi.h"

CWscApi::CWscApi()
{
	CoInitializeEx(0, COINIT_APARTMENTTHREADED);

	int i;

	for (i = 0; i < WSC_PROVIDER_COUNT; i++)
	{
		productList[i] = NULL;
	}

	refresh();
}

CWscApi::~CWscApi()
{
	release();

	//release() 전에 호출하면 종료 시 release() 함수내에서 엑세스 위반이 발생한다.
	CoUninitialize();
}

void CWscApi::release()
{
	int i;

	for (i = 0; i < WSC_PROVIDER_COUNT; i++)
	{
		product[i].clear();

		if (productList[i])
		{
			productList[i]->Release();
			productList[i] = NULL;
		}
	}
}

void CWscApi::refresh()
{
	release();


	int i, j;
	HRESULT hr = S_OK;
	provider[0] = WSC_SECURITY_PROVIDER_ANTIVIRUS;
	provider[1] = WSC_SECURITY_PROVIDER_ANTISPYWARE;
	provider[2] = WSC_SECURITY_PROVIDER_FIREWALL;

	for (i = 0; i < WSC_PROVIDER_COUNT; i++)
	{
		TRACE(_T("\n\n"));

		switch (i)
		{
			case 0:
				TRACE(_T("Antivirus Products:\n\n"));
				break;
			case 1:
				TRACE(_T("AAntispyware Products:\n\n"));
				break;
			case 2:
				TRACE(_T("Firewall Products:\n\n"));
				break;
		}

		// Initialize can only be called once per instance, so you need to
		// CoCreateInstance for each security product type you want to query.
		if (productList[i] == NULL)
		{
			hr = CoCreateInstance(
				__uuidof(WSCProductList),
				NULL,
				CLSCTX_INPROC_SERVER,
				__uuidof(IWSCProductList),
				reinterpret_cast<LPVOID*> (&productList[i]));

			if (FAILED(hr))
			{
				TRACE(_T("CoCreateInstance returned error = 0x%d \n"), hr);
				return;
			}
		}

		// Initialize the product list with the type of security product you're 
		// interested in.
		hr = productList[i]->Initialize(provider[i]);
		if (FAILED(hr))
		{
			TRACE(L"Initialize failed with error: 0x%d\n", hr);
			return;
		}

		// Get the number of security products of that type.
		hr = productList[i]->get_Count(&productCount[i]);
		if (FAILED(hr))
		{
			TRACE(L"get_Count failed with error: 0x%d\n", hr);
			return;
		}

		IWscProduct* PtrProduct = NULL;
		BSTR PtrVal;
		CWscProduct wscProduct;

		for (j = 0; j < productCount[i]; j++)
		{
			//
			// Get the next security product
			//
			hr = productList[i]->get_Item(j, &PtrProduct);
			if (FAILED(hr))
			{
				TRACE(L"get_Item failed with error: 0x%d\n", hr);
				return;
			}

			//
			// Get the product name
			//
			hr = PtrProduct->get_ProductName(&PtrVal);
			if (FAILED(hr))
			{
				TRACE(L"get_ProductName failed with error: 0x%d\n", hr);
				return;
			}

			wscProduct.name = PtrVal;
			TRACE(L"Product name: %s\n", PtrVal);
			// Caller is responsible for freeing the string
			SysFreeString(PtrVal);
			PtrVal = nullptr;

			// Get the product state
			hr = PtrProduct->get_ProductState(&wscProduct.state);
			if (FAILED(hr))
			{
				TRACE(L"get_ProductState failed with error: 0x%d\n", hr);
				return;
			}
			TRACE(L"Product state: %d\n", wscProduct.state);

			// Get the signature status (not applicable to firewall products)
			if (provider[i] != WSC_SECURITY_PROVIDER_FIREWALL)
			{
				hr = PtrProduct->get_SignatureStatus(&wscProduct.status);
				if (FAILED(hr))
				{
					TRACE(L"get_SignatureStatus failed with error: 0x%d\n", hr);
					return;
				}
				//LPWSTR pszStatus = (wscProduct.status == WSC_SECURITY_PRODUCT_UP_TO_DATE) ?
				//	L"Up-to-date" : L"Out-of-date";
				TRACE(L"Product status: %s\n", wscProduct.status == WSC_SECURITY_PRODUCT_UP_TO_DATE ? L"Up-to-date" : L"Out-of-date");
			}

			product[i].push_back(wscProduct);
			PtrProduct->Release();
		}
	}

}

int CWscApi::get_provider_index(WSC_SECURITY_PROVIDER provider_def)
{
	for (int i = 0; i < WSC_PROVIDER_COUNT; i++)
	{
		if (provider[i] == provider_def)
			return i;
	}

	return -1;
}

size_t CWscApi::get_product_count(WSC_SECURITY_PROVIDER provider)
{
	int index = get_provider_index(provider);
	if (index < 0 || index >= product->size())
		return 0;

	return product[index].size();
}

WSC_SECURITY_PRODUCT_STATE CWscApi::get_product_state(WSC_SECURITY_PROVIDER provider, int product_index)
{
	int provider_index = get_provider_index(provider);
	if (provider_index < 0 || provider_index >= WSC_PROVIDER_COUNT)
		return (WSC_SECURITY_PRODUCT_STATE)-1;

	if (product_index < 0 || product_index >= product[provider_index].size())
		return (WSC_SECURITY_PRODUCT_STATE)-1;

	return product[provider_index][product_index].state;
}

WSC_SECURITY_SIGNATURE_STATUS CWscApi::get_product_status(WSC_SECURITY_PROVIDER provider, int product_index)
{
	int provider_index = get_provider_index(provider);
	if (provider_index < 0 || provider_index >= WSC_PROVIDER_COUNT)
		return (WSC_SECURITY_SIGNATURE_STATUS)-1;

	return product[provider_index][product_index].status;
}

//기본 Microsoft Defender만 있을 경우는 해당 state가 on인지 판별하고
//그 외에 다른 백신을 설치한 경우에는 하나라도 state_on이면 true를 리턴한다.
bool CWscApi::is_antivirus_state_on()
{
	int provider_index = get_provider_index(WSC_SECURITY_PROVIDER_ANTIVIRUS);
	if (provider_index < 0 || provider_index >= WSC_PROVIDER_COUNT)
		return false;

	//기본 Microsoft Defender만 있을 경우
	if (product[provider_index].size() == 1)
		return (product[provider_index][0].state == WSC_SECURITY_PRODUCT_STATE_ON);

	//기본 Microsoft Defender 외에 다른 백신을 설치한 경우에는
	//하나라도 state_on이면 true를 리턴한다.
	for (int i = 0; i < product->size(); i++)
	{
		if (product[provider_index][i].name.Find(_T("Microsoft Defender ")) < 0)
		{
			if (product[provider_index][i].state == WSC_SECURITY_PRODUCT_STATE_ON)
				return true;
		}
	}

	return false;
}

//기본 Microsoft Defender만 있을 경우는 해당 status가 up-to-date인지 판별하고
//그 외에 다른 백신을 설치한 경우에는 하나라도 up-to-date이면 true를 리턴한다.
bool CWscApi::is_antivirus_latest_version()
{
	int provider_index = get_provider_index(WSC_SECURITY_PROVIDER_ANTIVIRUS);
	if (provider_index < 0 || provider_index >= WSC_PROVIDER_COUNT)
		return false;

	//기본 Microsoft Defender만 있을 경우
	if (product[provider_index].size() == 1)
		return (product[provider_index][0].status == WSC_SECURITY_PRODUCT_UP_TO_DATE);

	//기본 Microsoft Defender 외에 다른 백신을 설치한 경우에는
	//하나라도 up_to_date이면 true를 리턴한다.
	for (int i = 0; i < product->size(); i++)
	{
		if (product[provider_index][i].name.Find(_T("Microsoft Defender ")) < 0)
		{
			if (product[provider_index][i].status == WSC_SECURITY_PRODUCT_UP_TO_DATE)
				return true;
		}
	}

	return false;
}
