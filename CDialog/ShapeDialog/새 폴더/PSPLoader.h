//
// Copyright © 2001 Rui Godinho Lopes <ruiglopes@yahoo.com>
// All rights reserved.
//
// This source file(s) may be redistributed unmodified by any means
// PROVIDING they are not sold for profit without the authors expressed
// written consent, and providing that this notice and the authors name
// and all copyright notices remain intact.
//
// Any use of the software in source or binary forms, with or without
// modification, must include, in the user documentation ("About" box and
// printed documentation) and internal comments to the code, notices to
// the end user as follows:
//
// "Portions Copyright © 2001 Rui Godinho Lopes"
//
// An email letting me know that you are using it would be nice as well.
// That's not much to ask considering the amount of work that went into
// this.
//
// THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED. USE IT AT YOUT OWN RISK. THE AUTHOR ACCEPTS NO
// LIABILITY FOR ANY DATA DAMAGE/LOSS THAT THIS PRODUCT MAY CAUSE.
//


#ifndef PSPLOADER_H_INC
#define PSPLOADER_H_INC

enum
{
	ERR_NOERROR									= 0,
	ERR_FILEOPEN								= -1000,	///< Error opening file
	ERR_ATTRIBUTESNOTSUPPORTED,						///< 
	ERR_INVALIDFILE,											///< Unknown file type
	ERR_NOTSUPORTEDFILEVERSION,
	ERR_CORRUPT,													///< The file has invalid id field values
};


/**	Load's a PSP file into a windows DIB object.
	*
	* @param [in]pFileName File name of the PSP file.
	* @param [out]hBmp Reference to a HBITMAP variable that on return will
	*        contain an handle to an windows GDI bitmap object.
	* @param [out,optional]ppvBits Pointer to a VOID pointer that on return
	*        will contain the address of the DIB bits.
	* @return ERR_NOERROR if no error.
	* @author Rui Godinho Lopes <ruiglopes@yahoo.com>
	*/
LRESULT LoadPSPFileToDIB(/*[in]*/LPCTSTR pFileName, /*[out]*/HBITMAP &hBitmap, /*[out]*/VOID **ppvBits= NULL);


#endif