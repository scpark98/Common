#include "WscApi.h"

CWscApi::CWscApi()
{
	refresh();
}

void CWscApi::refresh()
{
	product->clear();

	CoInitializeEx(0, COINIT_APARTMENTTHREADED);

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

		IWscProduct* PtrProduct;
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
		}
	}

	CoUninitialize();
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

//product_index를 -1로 줄 경우 다음과 같이 처리된다.
//product 리스트가 1개일 경우는 Windows Defender 항목의 값을 리턴하고
//2개 이상일 경우는 Windows Defender가 아닌 항목 중 첫 번째 항목의 상태값을 리턴한다.
//Windows Defender가 기본 설치되어 있는 윈도우에 V3 or Avast등을 설치하면
//Windows Defender의 Antivirus state는 Off로 변경되고
//V3 or Avast의 state가 실제 유효한 state가 되므로 이 항목의 state를 리턴해야 함.
WSC_SECURITY_PRODUCT_STATE CWscApi::get_product_state(WSC_SECURITY_PROVIDER provider, int product_index)
{
	int index = get_provider_index(provider);
	if (index < 0 || index >= product->size())
		return (WSC_SECURITY_PRODUCT_STATE)-1;

	if (product_index < 0)
	{
		if (product->size() == 1)
		{
			product_index = 0;
		}
		else
		{
			for (int i = 0; i < product->size(); i++)
			{
				if (product[index][i].name.Find(_T("Microsoft Defender ")) < 0)
				{
					product_index = i;
					break;
				}
			}
		}
	}

	return product[index][product_index].state;
}

//product_index를 -1로 주면 product 리스트 중 맨 마지막 항목의 값을 리턴
WSC_SECURITY_SIGNATURE_STATUS CWscApi::get_product_status(WSC_SECURITY_PROVIDER provider, int product_index)
{
	int index = get_provider_index(provider);
	if (index < 0 || index >= product->size())
		return (WSC_SECURITY_SIGNATURE_STATUS)-1;

	if (product_index < 0)
	{
		if (product->size() == 1)
		{
			product_index = 0;
		}
		else
		{
			for (int i = 0; i < product->size(); i++)
			{
				if (product[index][i].name.Find(_T("Microsoft Defender ")) < 0)
				{
					product_index = i;
					break;
				}
			}
		}
	}

	return product[index][product_index].status;
}

//사용자가 좀 더 쉽게 사용하기 위한 멤버함수 정의.
bool CWscApi::is_antivirus_state_on()
{
	WSC_SECURITY_PRODUCT_STATE state = get_product_state(WSC_SECURITY_PROVIDER_ANTIVIRUS, -1);
	return (state == WSC_SECURITY_PRODUCT_STATE_ON);
}

bool CWscApi::is_antivirus_latest_version()
{
	WSC_SECURITY_SIGNATURE_STATUS status = get_product_status(WSC_SECURITY_PROVIDER_ANTIVIRUS, -1);
	return (status == WSC_SECURITY_PRODUCT_UP_TO_DATE);
}
