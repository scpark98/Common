// This is part of the Professional User Interface Suite library.
// Copyright (C) 2001-2003 FOSS Software, Inc.
// All rights reserved.
//
// http://www.prof-uis.com
// http://www.fossware.com
// mailto:foss@fossware.com
//
// Warranties and Disclaimers:
// THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND
// INCLUDING, BUT NOT LIMITED TO, WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
// IN NO EVENT WILL FOSS SOFTWARE INC. BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES,
// INCLUDING DAMAGES FOR LOSS OF PROFITS, LOSS OR INACCURACY OF DATA,
// INCURRED BY ANY PERSON FROM SUCH PERSON'S USAGE OF THIS SOFTWARE
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

// ExtWndShadow.h: interface for the CExtWndShadow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EXTWNDSHADOW_H__A13108FB_D51A_459E_8CF7_B40B1A910362__INCLUDED_)
#define AFX_EXTWNDSHADOW_H__A13108FB_D51A_459E_8CF7_B40B1A910362__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxwin.h>

//////////////////////////////////////////////////////////////////////////
// class CExtWndShadow  
//////////////////////////////////////////////////////////////////////////

class CExtWndShadow  
{
	//////////////////////////////////////////////////////////////////////////
	// CExtMemoryDC
	//////////////////////////////////////////////////////////////////////////
	
	class CExtMemoryDC : public CDC 
	{
	protected:	
		CBitmap m_bmp;			// offscreen bitmap
		CBitmap * m_pOldBmp;	// bitmap originally found in CExtMemoryDC
		CDC * m_pDC;			// saves CDC passed in constructor
		CRect m_rect;			// rectangle of drawing area
		BOOL m_bMemDC;			// TRUE if CDC really is a memory DC
		COLORREF * m_pHelperDibSurface; // DIB surface if created as DIB
		
	public:
		enum eOptFlags
		{
			MDCOPT_TO_MEMORY	= 0x001L,
				MDCOPT_FILL_SURFACE	= 0x002L,
				MDCOPT_FORCE_BLACK	= 0x004L,
				MDCOPT_FORCE_DIB	= 0x008L,
				MDCOPT_NO_COPY_OPT	= 0x010L,
				MDCOPT_FILL_BITS	= 0x020L,
				MDCOPT_DEFAULT = MDCOPT_TO_MEMORY|MDCOPT_FILL_SURFACE
		};
		
		void __InitMemoryDC(
			CDC * pDC,
			const CRect * pRect = NULL,
			DWORD dwOptFlags = MDCOPT_DEFAULT
			)
		{
			ASSERT( m_pDC == NULL ); 
			ASSERT( pDC != NULL ); 
			ASSERT( pDC->GetSafeHdc() != NULL ); 
			
			m_pDC = pDC;
			m_pOldBmp = NULL;
			if( dwOptFlags & MDCOPT_TO_MEMORY )
				m_bMemDC = !pDC->IsPrinting();
			else
				m_bMemDC = FALSE;
			
			if( pRect == NULL ) 
				pDC->GetClipBox( &m_rect );
			else 
				m_rect = *pRect;
			if( m_bMemDC ) 
			{
				CreateCompatibleDC( pDC );
				if( (dwOptFlags & MDCOPT_NO_COPY_OPT) == 0 )
					pDC->LPtoDP( &m_rect );
				if( dwOptFlags & MDCOPT_FORCE_BLACK )
					m_bmp.CreateBitmap(
					m_rect.Width(),
					m_rect.Height(),
					1,
					1,
					NULL
					);
				else if( dwOptFlags & MDCOPT_FORCE_DIB )
				{
					BITMAPINFOHEADER bih;
					bih.biSize = sizeof(BITMAPINFOHEADER);
					bih.biWidth = m_rect.Width();
					bih.biHeight = m_rect.Height();
					bih.biPlanes = 1;
					bih.biBitCount = 32;
					bih.biCompression = BI_RGB;
					bih.biSizeImage = bih.biWidth*bih.biHeight;
					bih.biXPelsPerMeter = 0;
					bih.biYPelsPerMeter = 0;
					bih.biClrUsed = 0;
					bih.biClrImportant = 0;
					ASSERT( m_pHelperDibSurface == NULL );
					HBITMAP hDIB =
						::CreateDIBSection(
						pDC->GetSafeHdc(),
						(LPBITMAPINFO)&bih,
						DIB_RGB_COLORS,
						(void **)&m_pHelperDibSurface,
						NULL,
						NULL
						);
					if( hDIB == NULL || m_pHelperDibSurface == NULL )
					{
						ASSERT( FALSE );
						return;
					}
					m_bmp.Attach( hDIB );
				}
				else
					m_bmp.CreateCompatibleBitmap(
					pDC,
					m_rect.Width(),
					m_rect.Height()
					);
				m_pOldBmp = SelectObject( &m_bmp );
				if( (dwOptFlags & MDCOPT_NO_COPY_OPT) == 0 )
				{
					SetMapMode( pDC->GetMapMode() );
					pDC->DPtoLP( &m_rect );
					SetWindowOrg( m_rect.left, m_rect.top );
				}
			} 
			else 
			{
				if( pDC->IsPrinting() )
					m_bPrinting = pDC->m_bPrinting;
				m_hDC       = pDC->m_hDC;
				m_hAttribDC = pDC->m_hAttribDC;
			}
			
			if( dwOptFlags & MDCOPT_FILL_BITS )
				BitBlt(
				0, 0,
				m_rect.Width(), m_rect.Height(),
				pDC,
				m_rect.left, m_rect.top,
				SRCCOPY
				);
			else if( dwOptFlags & MDCOPT_FILL_SURFACE )
				FillSolidRect( m_rect, pDC->GetBkColor() );
		};
		
		void __Flush( BOOL bMoveImage = TRUE )
		{
			if( m_pDC == NULL )
				return;
			
			if( m_bMemDC ) 
			{
				if( bMoveImage )
					m_pDC->BitBlt(
					m_rect.left, m_rect.top,
					m_rect.Width(), m_rect.Height(),
					this,
					m_rect.left, m_rect.top,
					SRCCOPY
					);			
				SelectObject( m_pOldBmp );
			} 
			else 
			{
				m_hDC = m_hAttribDC = NULL;
			}	
			m_pDC = NULL;
			m_pOldBmp = NULL;
			m_rect.SetRectEmpty();
			m_bMemDC = FALSE;
			m_pHelperDibSurface = NULL;
		};
		
		CExtMemoryDC(
			CDC * pDC = NULL,
			const CRect * pRect = NULL,
			DWORD dwOptFlags = MDCOPT_DEFAULT
			) : CDC()
		{
			m_pOldBmp = NULL;
			m_pDC = NULL;
			m_rect.SetRectEmpty();
			m_pHelperDibSurface = NULL;
			if( pDC != NULL )
				__InitMemoryDC(
				pDC,
				pRect,
				dwOptFlags
				);
		};
		~CExtMemoryDC()	
		{
			__Flush();
		};
		
		CRect & __GetDrawRect()
		{
			return m_rect;
		};
		const CRect & __GetDrawRect() const
		{
			return m_rect;
		};
		
		BOOL __IsMemDC() const
		{
			return m_bMemDC;
		}
		
		CExtMemoryDC* operator->() 
		{
			return this;
		};
		operator CExtMemoryDC*() 
		{
			return this;
		};
		
		COLORREF * GetDibSurface()
		{
			ASSERT( m_pHelperDibSurface != NULL );
			return m_pHelperDibSurface;
		};
		const COLORREF * GetDibSurface() const
		{
			ASSERT( m_pHelperDibSurface != NULL );
			return m_pHelperDibSurface;
		};
	}; // class CExtMemoryDC
	
	CBitmap m_bmp0, m_bmp1;			// photo of right & bottom shadow parts to save/restore
	COLORREF * m_pHelperDibSurface;
	CRect m_rcWndArea;				// window are not including shadow
	UINT m_nShadowSize;				// shadow size (in pixels)
	UINT m_nBr0,m_nBr1;				// min/max brightness percents [0..100]
	bool m_bEnablePhotos:1;			// use photo bitmaps
	void _FreeWinObjects();
	void _DoPixelOvershadow(
		int nMakeSpec,
		int nPosX,
		int nPosY,
		COLORREF clrShadowAdjust
		);
	bool _MakePhotos( CDC & dc, CDC & dcmm );
	bool _PaintLo( CDC & dc );
	bool _PaintHi( CDC & dc );
	static int stat_GetBPP();
	CPalette m_PaletteWide;
	bool Paint( CDC & dc );
	bool Restore( CDC & dc );
	void Destroy();
	
public:
	CExtWndShadow();
	~CExtWndShadow();
	
	enum eShadowOptions
	{
		DEF_BRIGHTNESS_MIN =  65UL, // default min brightness percent
			DEF_BRIGHTNESS_MAX = 100UL, // default max brightness percent
			DEF_SHADOW_SIZE    =   5UL, // default shadow size (in pixels)
	};
	
	bool Paint(
		CDC & dc,
		const CRect & rcWndArea,
		UINT nShadowSize = DEF_SHADOW_SIZE,
		UINT nBr0 = DEF_BRIGHTNESS_MIN,
		UINT nBr1 = DEF_BRIGHTNESS_MAX,
		bool bEnablePhotos = true
		);
}; // class CExtWndShadow

#endif // !defined(AFX_EXTWNDSHADOW_H__A13108FB_D51A_459E_8CF7_B40B1A910362__INCLUDED_)
