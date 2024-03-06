#pragma once

#include <afxwin.h>
#include <lmaccess.h>

const DWORD cDwNetWLevel = 1;
const DWORD cDwNetUserLevel = 2;

class CGetAccountInfo
{
public:
	static CGetAccountInfo & Instance();
	int GetAccountType();
	CString GetAccountTypeString();

private:
	bool GetAccountDomainInfo(LPUSER_INFO_2& pUserInfo);

private:
	CGetAccountInfo(void);
	~CGetAccountInfo(void);
	CGetAccountInfo& operator=(const CGetAccountInfo&);
};
