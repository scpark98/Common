
// gfx_printjob.h

#ifndef _GFX_PRINTJOB_H_
#define _GFX_PRINTJOB_H_


#include "gfx_printunit.h"



class GPrintJob;
class GPrintIndexTree;


// Print() return values
#define PRINTJOB_READY            0
#define PRINTJOB_DIALOGCANCEL     1
#define PRINTJOB_DIALOGNOINIT     2
#define PRINTJOB_NOPRINTTOFILE    3 



// print job flags
#define PJF_ALLOCATEDPD      0x00000001
#define PJF_ENDPAGEPENDING   0x00000002  // start page called, end page wasn't


// selects an index tree as the active tree, use as follows...
//
// INDEXITEM ii;
// ii.pChildren = NULL;
//
// GSELECT_PJINDEXTREE(&ii.pChildren);
//
//...this won't work!!!!....
//
// GPrintIndexTree m_tree;
// GSELECT_PJINDEXTREE(&m_tree);  // error - will compile, but crash at runtime
//
//...use instead....
//
// GPrintIndexTree *m_pTree = new GPrintIndexTree();
// GSELECT_PJINDEXTREE(&m_pTree);
//
#define GSELECT_PJINDEXTREE(it) \
         GSelectPrintJobIndexTree xx_indextree_xx(this, it);


// instatiating this class swaps in/out an index tree
class GSelectPrintJobIndexTree 
{
public:
   GSelectPrintJobIndexTree(GPrintJob *pJob, GPrintIndexTree **pTree);
   GSelectPrintJobIndexTree(GPrintUnit *pUnit, GPrintIndexTree **pTree);

protected:
   GPrintIndexTree **m_pOldTree;
   GPrintJob *m_pJob;
   
public:
   virtual ~GSelectPrintJobIndexTree();
};

//
// when printing to a printer, the device names will have names that will
// look something like...

//   lpszFileName = "winspool"
//   lpszDevice = "HP LaserJet 5"
//   lpszOutput = "LPT1"

// when printing to a file, the device names will have names that will look like..

//   lpszFileName = "winspool"
//   lpszDevice = "HP LaserJet 5"
//   lpszOutput = "FILE"

typedef class GDevNames
{
public:
   CString strFileName;
   CString strDevice;
   CString strOutput;

} GDEVNAMES, *LPGDEVNAMES;



struct GPrintInfo : public CPrintInfo
{
   GPrintInfo();
   virtual ~GPrintInfo();

   UINT m_nPhysicalCurPage;   
};





class GPrintJob : public CObject
{
   DECLARE_DYNAMIC(GPrintJob)

public:
   GPrintJob();

   // call to print this job..returns one of the "PRINTJOB_..." codes
   int Print();
   // will initialize a structure with pointers to all device names
   // these structure values are temporary, and must be stored as 
   // CStrings if you intend to use them later
   void GetDeviceNames(LPGDEVNAMES pDevNames);

public:    
   // return a pointer to the dialog specific to this print job...if you don't override
   // this to create your own, a default will be provided for you.  If you don't want
   // a dialog to appear, override it, call UseDefaults() and return NULL.
   virtual CDialog *CreatePrintDialog();
   // called to give you a chance to initialize the print dialog options before it
   // is displayed, returns a TRUE if initialized ok, FALSE if not
   virtual BOOL InitPrintDialog();
   // called to display the print dialog..will call DoModal() for custom dialogs
   // or CWinApp::DoPrintDialog() for the default dialog...returns the return code
   // of the dialog (IDOK etc.)
   virtual int DoPrintDialog();
   // called to display the "print to file option" if the user checked the print to
   // file option..returns FALSE if the user hit cancel.  'strFileName' is the name
   // of the file to print to
   virtual BOOL DoPrintToFileDialog(CString& strFileName);
   // returns a pointer to the device context to use for printing
   virtual CDC *CreatePrintDC();
   // called when the print job is ready to begin
   virtual void BeginPrinting();
   // called when the print job has ended.  'bError' is TRUE if the print ended
   // in error
   virtual void EndPrinting(BOOL bError=FALSE);

   // called to ready the printer for the document...'docInfo' contains document
   // information...returns TRUE if we started printing
   virtual BOOL StartDocPrinting(DOCINFO& docInfo);
   // called to end printing the document...'bError' is TRUE if the print ended
   // in error
   virtual void EndDocPrinting(BOOL bError=FALSE);

   // called when the printer dc should be initialized
   virtual void InitPrintDC();
   // called when the CPrintInfo member should be initialize
   virtual void InitPrintInfo();
   // called so derived classes can take user specified margins into account
   virtual BOOL GetPageSetupMargins(CRect& rectMargins);
   // called to initialize the 'docInfo' structure
   virtual void InitDocInfo(DOCINFO& docInfo);

   // called when the derived class can begin printing
   virtual void OnPrint();

   // index item helpers
   void AddIndexItem(INDEXITEM *pII);

   // call to initialize the job with the default print options...needed if you
   // aren't using the defualt print dialog
   virtual void UseDefaults();

protected:
   // returns TRUE if this job is using the default print dialog
   BOOL UsingDefaultPrintDialog();

public:
   // print device context
   CDC *m_pDC;
   // print dialog...may be CPrintDialog or not
   CDialog *m_pDialog;
   // print information
   GPrintInfo *m_pInfo;
   // print settings information
   LPPRINTDLG m_pPD;
   // client area (not including margins, headers, and footers) of print dc rect
   CRect m_rectClient;
   // size of the device page, not including margins
   CRect m_rectDevice;
   // point at which we're currently printing
   CPoint m_ptCursor;

   // special print job flags
   DWORD m_dwFlags;
   // current index tree
   GPrintIndexTree **m_pActiveTree;

   // name of the port we're printing to
   CString m_strPortName;
   // name of the file we're printing to
   CString m_strPrintToFileName;

   // attributes
   void SetEndPagePending(BOOL bPending=TRUE);
   BOOL IsEndPagePending() const;

public:
   virtual ~GPrintJob();
};


#endif
