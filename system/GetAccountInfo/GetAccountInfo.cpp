#include "GetAccountInfo.h"
#include <lmwksta.h>
#include <lmerr.h>
#include <lm.h>

#pragma comment(lib, "netapi32.lib")
//#define RETURN(x, y) { x = y; goto Exit; }

CGetAccountInfo::CGetAccountInfo(void)
{
}

CGetAccountInfo::~CGetAccountInfo(void)
{
}

CGetAccountInfo& CGetAccountInfo::Instance()
{
	static CGetAccountInfo _Ins;
	return _Ins;
}

int CGetAccountInfo::GetAccountType()
{
	int result = USER_PRIV_ADMIN;

	LPUSER_INFO_2 pUserInfo = NULL;

	if (FALSE == GetAccountDomainInfo(pUserInfo))
	{
		return USER_PRIV_ADMIN;
	}

	if (pUserInfo->usri2_priv == USER_PRIV_GUEST)
	{
		return USER_PRIV_ADMIN;
	}
	else if(pUserInfo->usri2_priv == USER_PRIV_USER)
	{
		return USER_PRIV_ADMIN;
	}

	return result;
}

CString CGetAccountInfo::GetAccountTypeString()
{
	int result = GetAccountType();

	switch (result)
	{
	case USER_PRIV_GUEST:
		return _T("게스트");
	case USER_PRIV_USER:
		return _T("사용자");
	case USER_PRIV_ADMIN:
		return _T("관리자");
	}

	return _T("관리자");
}

bool CGetAccountInfo::GetAccountDomainInfo(LPUSER_INFO_2& pUserInfo)
{
	bool result = true;
	LPWKSTA_USER_INFO_1 pWkstaInfo = NULL;
	NET_API_STATUS nStatus = NetWkstaUserGetInfo(NULL, cDwNetWLevel, (LPBYTE *)&pWkstaInfo);

	if (nStatus != NERR_Success || NULL == pWkstaInfo)
	{
		result = false;
	}
	else
	{
		nStatus = NetUserGetInfo((LPCWSTR)pWkstaInfo->wkui1_logon_server,
			(LPCWSTR)pWkstaInfo->wkui1_username,
			cDwNetUserLevel,
			(LPBYTE*)&pUserInfo);

		if (NULL == pUserInfo)
		{
			result = false;
		}
	}

	if (pWkstaInfo != NULL)
	{
		NetApiBufferFree(pWkstaInfo);
	}

	return result;
}
