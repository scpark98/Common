
// gfx_printjob.cpp

#include "stdafx.h"
//#include "resource.h"
//#include "afxres.h"
#include "gfx_printjob.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(GPrintJob, CObject)


GPrintJob::GPrintJob()
{
   m_pDC = NULL;
   m_pDialog = NULL;
   m_pInfo = NULL;
   m_pPD = NULL;
   m_dwFlags = 0;

   m_ptCursor = CPoint(0,0);
   m_rectClient = CRect(0,0,0,0);
   m_rectDevice = CRect(0,0,0,0);

   m_pActiveTree = NULL;
}



GPrintJob::~GPrintJob()
{
   if((m_dwFlags & PJF_ALLOCATEDPD) && m_pPD)
   {
      // never free this structs 'hDevNames' and 'hDevMode' members...
      // these handles are maintained by CWinApp
      if(m_pPD->hDC)
      {
         DeleteObject(m_pPD->hDC);
         m_pPD->hDC = NULL;
      }

      delete m_pPD;
      m_pPD = NULL;
   }

   if(!UsingDefaultPrintDialog())
   {
      delete m_pDialog;
      m_pDialog = NULL;
   }

   delete m_pDC;
   m_pDC = NULL;

   delete m_pInfo;
   m_pInfo = NULL;
}




int GPrintJob::Print()
{
   int nPrintCode = PRINTJOB_READY;

   m_pInfo = new GPrintInfo;
   ASSERT(m_pInfo->m_pPD);  // CPrintInfo should have created this

   m_pDialog = CreatePrintDialog();
   if(m_pDialog)
   {
      if(InitPrintDialog())
      {
         if(DoPrintDialog() != IDOK)
         {
            nPrintCode = PRINTJOB_DIALOGCANCEL;
         }
      }
      else
      {
         nPrintCode = PRINTJOB_DIALOGNOINIT;
      }
   }


   if(nPrintCode == PRINTJOB_READY)
   {
      if(DoPrintToFileDialog(m_strPrintToFileName))
      {
         m_pDC = CreatePrintDC();

         // if we're missing any of the required print info, throw exception
         if(!m_pDC || !m_pDC->m_hDC || !m_pPD || 
                    !m_pPD->hDevMode || !m_pPD->hDevNames)
         {
            return PRINTJOB_DIALOGNOINIT;
         }

         InitPrintDC();
         InitPrintInfo();

         DOCINFO docInfo;
         InitDocInfo(docInfo);

         BeginPrinting();
         if( !StartDocPrinting(docInfo) )
         {
            return PRINTJOB_DIALOGNOINIT;
         }

         OnPrint();
         EndDocPrinting();
         EndPrinting();
      }
      else
      {
         nPrintCode = PRINTJOB_NOPRINTTOFILE;
      }
   }

   return nPrintCode;
}



void GPrintJob::InitDocInfo(DOCINFO& docInfo)
{
   GMAKESTNUL(docInfo);
   docInfo.cbSize = sizeof(docInfo);

   if(!m_strPrintToFileName.IsEmpty())
   {
      docInfo.lpszOutput = m_strPrintToFileName;
   }
}




CDialog *GPrintJob::CreatePrintDialog()
{
   return m_pInfo->m_pPD;
}





BOOL GPrintJob::UsingDefaultPrintDialog()
{
   if(m_pDialog && m_pInfo && (m_pDialog == m_pInfo->m_pPD))
      return TRUE;

   return FALSE;
}




BOOL GPrintJob::InitPrintDialog()
{
   if(UsingDefaultPrintDialog())
   {
      CPrintDialog *pDialog = (CPrintDialog *)m_pDialog;

      if(pDialog->m_pd.nMinPage > pDialog->m_pd.nMaxPage)
      {
         pDialog->m_pd.nMaxPage = pDialog->m_pd.nMinPage;
      }

      pDialog->m_pd.nFromPage = (WORD)m_pInfo->GetMinPage();
      pDialog->m_pd.nToPage = (WORD)m_pInfo->GetMaxPage();

      m_pPD = &pDialog->m_pd;
   }

   return TRUE;
}


int GPrintJob::DoPrintDialog()
{
   int nRC = IDCANCEL;

   if(UsingDefaultPrintDialog())
   {

      nRC = AfxGetApp()->DoPrintDialog((CPrintDialog *)m_pDialog);
      if(nRC == IDOK)
      {
         m_pInfo->m_nNumPreviewPages = AfxGetApp()->m_nNumPreviewPages;
         m_pInfo->m_strPageDesc.LoadString(AFX_IDS_PREVIEWPAGEDESC);
      }
   }
   else
   if(m_pDialog)
   {
      nRC = m_pDialog->DoModal();
   }

   return nRC;
}


// bogus print dialog - never gets displayed
class GBogusPrintDialog : public CPrintDialog
{
public:
   GBogusPrintDialog() : CPrintDialog(FALSE) {;}

   virtual int DoModal()
   {
      CreatePrinterDC();
   
      return IDOK;
   }

   virtual ~GBogusPrintDialog() {;}
};



void GPrintJob::UseDefaults()
{
   if(m_pPD)
   {
      if(m_dwFlags & PJF_ALLOCATEDPD)
      {
         delete m_pPD;
         m_pPD = NULL;
         m_dwFlags &= ~PJF_ALLOCATEDPD;
      }
      else
      {
         ASSERT(FALSE);
      }
   }

   m_dwFlags |= PJF_ALLOCATEDPD;

   m_pPD = new PRINTDLG;
   GMAKESTNUL(*m_pPD);

   // MFC's CPrintDialog does a lot for you, almost too much.  There are two members
   // of CWinApp, m_hDevMode and m_nDevNames, which contain global print information.
   // In order for you to print with the most recent print settings, you must have access
   // to these members and make them part of your PRINTDLG struct.  However, these
   // members are protected - we must get them by brute force.  We declare a bogus dialog
   // which infiltrates the CWinApp class and comes out with a properly initialized
   // PRINTDLG struct
   CWinApp *pApp = AfxGetApp();
   if(pApp)
   {
      GBogusPrintDialog bpd;
      pApp->DoPrintDialog(&bpd);
      // make a copy of the dialog's internal PRINTDLG struct
      *m_pPD = bpd.m_pd;
   }
}



BOOL GPrintJob::DoPrintToFileDialog(CString& strFileName)
{
   BOOL bRC = TRUE;

   if(m_pPD && m_pPD->Flags & PD_PRINTTOFILE)
   {
      bRC = FALSE;
      CString strDef(MAKEINTRESOURCE(AFX_IDS_PRINTDEFAULTEXT));
      CString strPrintDef(MAKEINTRESOURCE(AFX_IDS_PRINTDEFAULT));
      CString strFilter(MAKEINTRESOURCE(AFX_IDS_PRINTFILTER));
      CString strCaption(MAKEINTRESOURCE(AFX_IDS_PRINTCAPTION));

      CFileDialog dlg(FALSE, strDef, strPrintDef, 
                      OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST,
                      strFilter);
      dlg.m_ofn.lpstrTitle = strCaption;
      if(dlg.DoModal() == IDOK)
      {
         bRC = TRUE;
         strFileName = dlg.GetPathName();
      }
   }

   return bRC;
}



CDC *GPrintJob::CreatePrintDC()
{
   if(m_pPD)
   {
      CDC *pDC = new CDC();
      pDC->Attach(m_pPD->hDC);
      return pDC;
   }

   return NULL;
}



void GPrintJob::BeginPrinting()
{
}



void GPrintJob::EndPrinting(BOOL bError)
{
   if(!bError)
   {
      // return the number of copies back to 1
      if(m_pPD && m_pPD->hDevMode)
      {
         LPDEVMODE lpDevMode = (LPDEVMODE)::GlobalLock(m_pPD->hDevMode);
         if(lpDevMode)
         {
            lpDevMode->dmCopies = 1;
            ::GlobalUnlock(m_pPD->hDevMode);
         }
      }
   }
}



BOOL GPrintJob::StartDocPrinting(DOCINFO& docInfo)
{
   if(!m_pDC || m_pDC->StartDoc(&docInfo) == SP_ERROR)
   {
      BOOL bError = TRUE;

      EndPrinting(bError);
      EndDocPrinting(bError);
      return FALSE;
   }

   return TRUE;
}



void GPrintJob::EndDocPrinting(BOOL bError)
{
   if(bError)
   {
      m_pDC->AbortDoc();
   }
   else
   {
      m_pDC->EndDoc();
   }

   if(m_pDC)
   {
      m_pDC->Detach();
   }
}



void GPrintJob::InitPrintDC()
{
   if(m_pDC)
   {
      m_pDC->m_bPrinting = TRUE;
      m_pDC->SetMapMode(MM_TEXT);
   }
}



void GPrintJob::InitPrintInfo()
{
   if(m_pInfo && m_pDC)
   {
      m_pInfo->m_rectDraw.SetRect(0, 0, m_pDC->GetDeviceCaps(HORZRES),
                                        m_pDC->GetDeviceCaps(VERTRES));

		m_pInfo->m_rectDraw.DeflateRect( 80, 0 );
		m_pInfo->m_rectDraw.OffsetRect( 0, 100 );
	  
      m_rectDevice = m_pInfo->m_rectDraw;

      CRect rectMargins;
      if(GetPageSetupMargins(rectMargins))
      {
         m_pInfo->m_rectDraw.left += rectMargins.left;
         m_pInfo->m_rectDraw.top += rectMargins.top;
         m_pInfo->m_rectDraw.right -= rectMargins.right;
         m_pInfo->m_rectDraw.bottom -= rectMargins.bottom;

         m_pInfo->m_rectDraw.NormalizeRect();
      }

      m_rectClient = m_pInfo->m_rectDraw;
      m_pDC->DPtoLP(&m_pInfo->m_rectDraw);
   }
}



BOOL GPrintJob::GetPageSetupMargins(CRect& rectMargins)
{
   return FALSE;
}


void GPrintJob::OnPrint()
{

}



void GPrintJob::GetDeviceNames(LPGDEVNAMES pDevNames)
{
   if(pDevNames)
   {
      if(m_pPD && m_pPD->hDevNames)
      {
         LPDEVNAMES lpDev = (LPDEVNAMES)::GlobalLock(m_pPD->hDevNames);
         pDevNames->strFileName = (LPCTSTR)lpDev + lpDev->wDriverOffset;
         pDevNames->strDevice = (LPCTSTR)lpDev + lpDev->wDeviceOffset;
         pDevNames->strOutput = (LPCTSTR)lpDev + lpDev->wOutputOffset;

         ::GlobalUnlock(m_pPD->hDevNames);
      }
   }
}



void GPrintJob::AddIndexItem(INDEXITEM *pII)
{
   if(m_pActiveTree)
   {
      if(!(*m_pActiveTree))
      {
         (*m_pActiveTree) = new GPrintIndexTree();
      }

      (*m_pActiveTree)->Add(*pII);   
   }
}



BOOL GPrintJob::IsEndPagePending() const
{
   return (m_dwFlags & PJF_ENDPAGEPENDING) ? TRUE : FALSE;
}


void GPrintJob::SetEndPagePending(BOOL bPending)
{
   GSET_BIT(m_dwFlags, PJF_ENDPAGEPENDING, bPending);
}


/////////////////////////////////////////////////////////////////////////////


 
GSelectPrintJobIndexTree::GSelectPrintJobIndexTree(GPrintJob *pJob, GPrintIndexTree **pTree)
{
   m_pJob = pJob;
   m_pOldTree = m_pJob->m_pActiveTree;
   m_pJob->m_pActiveTree = pTree;
}


GSelectPrintJobIndexTree::GSelectPrintJobIndexTree(GPrintUnit *pUnit, GPrintIndexTree **pTree) 
{
   m_pJob = pUnit->GetJob();
   m_pOldTree = m_pJob->m_pActiveTree;
   m_pJob->m_pActiveTree = pTree;
}


GSelectPrintJobIndexTree::~GSelectPrintJobIndexTree()
{
   // restore the old tree
   m_pJob->m_pActiveTree = m_pOldTree;
}





///////////////////////////////////////////////////////////////////////////////////

GPrintInfo::GPrintInfo()
{
   m_nPhysicalCurPage = 1;
}

GPrintInfo::~GPrintInfo()
{
}

