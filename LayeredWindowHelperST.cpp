//#include "stdafx.h"
#include "LayeredWindowHelperST.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CLayeredWindowHelperST::CLayeredWindowHelperST()
{
	// Load DLL.
	m_hDll = ::LoadLibrary(_T("USER32.dll"));
}

CLayeredWindowHelperST::~CLayeredWindowHelperST()
{
	// Unload DLL (if any)
	if (m_hDll)
		::FreeLibrary(m_hDll);

	m_hDll = nullptr;
}

// This function add the WS_EX_LAYERED style to the specified window.
//
// Parameters:
//		[IN]	Handle to the window and, indirectly, the class to which the window belongs. 
//				Windows 95/98/Me: The SetWindowLong function may fail if the window 
//				specified by the hWnd parameter does not belong to the same process 
//				as the calling thread. 
//
// Return value:
//		Non zero
//			Function executed successfully.
//		Zero
//			Function failed. To get extended error information, call ::GetLastError().
//
LONG CLayeredWindowHelperST::AddLayeredStyle(HWND hWnd)
{
	::SetLastError(0);

	LONG_PTR oldEx = ::GetWindowLongPtr(hWnd, GWL_EXSTYLE);
	LONG_PTR newEx = oldEx | WS_EX_LAYERED;

	::SetWindowLongPtr(hWnd, GWL_EXSTYLE, newEx);

	DWORD gle = ::GetLastError();
	if (gle != 0)
	{
		TRACE(_T("AddLayeredStyle failed. gle=%lu oldEx=0x%08llX newEx=0x%08llX\n"),
			gle, (unsigned long long)oldEx, (unsigned long long)newEx);
		return 0;
	}

	return (LONG)oldEx; // 성공 시 이전값 리턴
	/*
	LONG res = ::SetWindowLongPtr(hWnd, GWL_EXSTYLE, ::GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	if (!res)
		TRACE(_T("fail to CLayeredWindowHelperST::AddLayeredStyle. This window maybe has WS_CHILD property.\n"));
	return res;
	*/
} // End of AddLayeredStyle

// This function sets the opacity and transparency color key of a layered window.
//
// Parameters:
//		[IN]	hWnd
//				Handle to the layered window.
//		[IN]	crKey
//				A COLORREF value that specifies the transparency color key to be used when
//				composing the layered window. All pixels painted by the window in this color will be transparent.
//				To generate a COLORREF, use the RGB() macro.
//		[IN]	bAlpha
//				Alpha value used to describe the opacity of the layered window.
//				When bAlpha is 0, the window is completely transparent.
//				When bAlpha is 255, the window is opaque. 
//		[IN]	dwFlags 
//				Specifies an action to take. This parameter can be one or more of the following values:
//					LWA_COLORKEY	Use crKey as the transparency color.  
//					LWA_ALPHA		Use bAlpha to determine the opacity of the layered window.
//
// Return value:
//		TRUE
//			Function executed successfully.
//		FALSE
//			Function failed. To get extended error information, call ::GetLastError().
//
BOOL CLayeredWindowHelperST::SetLayeredWindowAttributes(HWND hWnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
	BOOL	bRetValue = TRUE;

	if (m_hDll)
	{
		lpfnSetLayeredWindowAttributes pFn = nullptr;
		pFn = (lpfnSetLayeredWindowAttributes)GetProcAddress(m_hDll, "SetLayeredWindowAttributes");
		if (pFn)
		{
			bRetValue = pFn(hWnd, crKey, bAlpha, dwFlags);
		} // if
		else bRetValue = FALSE;
	} // if

	return bRetValue;
} // End of SetLayeredWindowAttributes

// This function sets the percentage of opacity or transparency of a layered window.
//
// Parameters:
//		[IN]	hWnd
//				Handle to the layered window.
//		[IN]	byPercentage
//				Percentage (from 0 to 100)
//
// Return value:
//		TRUE
//			Function executed successfully.
//		FALSE
//			Function failed. To get extended error information, call ::GetLastError().
//
BOOL CLayeredWindowHelperST::SetTransparent(HWND hWnd, BYTE alpha)
{
	// Do not accept values greater than 255
	if (alpha < 0)
		alpha = 0;
	if (alpha > 255)
		alpha = 255;

	m_alpha = alpha;

	BOOL res = SetLayeredWindowAttributes(hWnd, 0, m_alpha, LWA_ALPHA);
	//TRACE(_T("GetLastError() = %d\n"), GetLastError());
	if (!res)
		TRACE(_T("fail to CLayeredWindowHelperST::SetTransparent. GetLastError() = %d\n"), GetLastError());

	COLORREF key = 0;
	//BYTE alpha = 0;
	DWORD flags = 0;

	BOOL ok = ::GetLayeredWindowAttributes(hWnd, &key, &alpha, &flags);
	DWORD gle = ::GetLastError();

	LONG_PTR ex = ::GetWindowLongPtr(hWnd, GWL_EXSTYLE);

	TRACE(_T("Layered: ok=%d gle=%lu ex=0x%08llX flags=0x%08X alpha=%u expected=%d\n"),
		ok, gle, (unsigned long long)ex, flags, (unsigned)alpha, m_alpha);

	return res;
} // End of SetTransparentPercentage
