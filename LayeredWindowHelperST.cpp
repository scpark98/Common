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

	m_hDll = NULL;
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

	return ::SetWindowLongPtr(hWnd, GWL_EXSTYLE, ::GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
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
		lpfnSetLayeredWindowAttributes pFn = NULL;
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
BOOL CLayeredWindowHelperST::SetTransparent(HWND hWnd, BYTE alpha_percentage)
{
	// Do not accept values greater than 100%
	if (alpha_percentage > 100)
		alpha_percentage = 100;

	m_alpha_percentage = alpha_percentage;

	//return SetLayeredWindowAttributes(hWnd, RGB( 26, 25, 17 ), 255, LWA_COLORKEY | LWA_ALPHA);
	return SetLayeredWindowAttributes(hWnd, 0, 255 - 255 * m_alpha_percentage/100, LWA_ALPHA);
	
} // End of SetTransparentPercentage
