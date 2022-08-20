//
// Copyright ?2001 Rui Godinho Lopes <ruiglopes@yahoo.com>
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
// "Portions Copyright ?2001 Rui Godinho Lopes"
//
// An email letting me know that you are using it would be nice as well.
// That's not much to ask considering the amount of work that went into
// this.
//
// THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED. USE IT AT YOUT OWN RISK. THE AUTHOR ACCEPTS NO
// LIABILITY FOR ANY DATA DAMAGE/LOSS THAT THIS PRODUCT MAY CAUSE.
//

#include "stdafx.h"
#include "PSPTypes.h"
#include "PSPLoader.h"
#include <fstream>
#include <string>

#ifdef _UNICODE
typedef std::wstring STD_TSTRING;
#else
typedef std::string STD_TSTRING;
#endif


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Paint Shop Pro(tm) (.PSP) internal file structures.
// All of this info was retrieved from Jasc(r) psp file spec.
// document (available at http://www.jasc.com)
//


#pragma pack(push, 1)

const DWORD PSPBLOCKHEADERID= 0x004b427e;

struct TPSPBBLOCKHEADER
{
	DWORD m_dwHeaderId;	///< Allways PSPBLOCKHEADERID
	WORD m_wBlockId;		///< @see PSPBlockID.
	DWORD m_dwTotalBlockLength;
};


const BYTE PSPFILESIGNATURE[32]= {
	0x50, 0x61, 0x69, 0x6E, 0x74, 0x20, 0x53, 0x68, 0x6F, 0x70, 0x20,
	0x50, 0x72, 0x6F, 0x20, 0x49, 0x6D, 0x61, 0x67, 0x65, 0x20, 0x46,
	0x69, 0x6C, 0x65, 0x0A, 0x1A, 0x00, 0x00, 0x00, 0x00
	};

struct TPSPFILEHEADER
{
	BYTE m_bFileSignature[32]; ///< allways PSPFILESIGNATURE
	WORD m_wMajorVersion;
	WORD m_wMinorVersion;
};


struct TGeneralImageAttributesChunk
{
	DWORD m_dwSize;
	LONG m_lImageWidth;
	LONG m_lImageHeight;
	double m_dblResolutionValue;
	BYTE m_bResolutionMetric;		///< one of PSP_METRIC
	WORD m_wCompressionType;		///< one of PSPCompression
	WORD m_wBitDepth;
	WORD m_wPlaneCount;					///< must be 1
	DWORD m_dwColorCount;
	BYTE m_bGrayScaleFlag;
	DWORD m_dwTotalImageSize;
	LONG m_lActiveLayer;
	WORD m_wLayerCount;
	DWORD m_dwGraphicContents;	///< Flag field (@see PSPGraphicContents)
	// Expansion field.
};

struct TCompositeImageBankInformationChunk
{
	DWORD m_dwSize;
	DWORD m_dwImageCount;
	// Expansion field.
};

struct TCompositeImageAttributesInformationChunk
{
	DWORD m_dwSize;
	LONG m_lWidth;
	LONG m_lHeight;
	WORD m_wBitDepth;
	WORD m_wCompressionType;		///< one of @see PSPCompression
	WORD m_wPlaneCount;
	DWORD m_dwColorCount;
	WORD m_wCompositeImageType; ///< one of @see PSPCompositeImageType
	// Expansion field.
};

struct TLayerInformationChunkHeader
{
	DWORD m_dwSize;
	// WORD m_wCharacterCount; ///< Length of following string (the string does NOT contain the end NULL char)
	// char m_LayerName[m_wCharacterCount];
};

struct TLayerInformationChunk
{
	BYTE m_bLayerType;	///< one of PSPLayerType
	RECT m_rectImage;
	RECT m_rectSavedImage;
	BYTE m_bLayerOpacity;
	BYTE m_bBlendingMode;
	BYTE m_bLayerFlags;	///< one of @see PSPLayerProperties
	BYTE m_bTransparencyProtectedFlag;
	BYTE m_bLinkGroupId;
	RECT m_rectMask;
	RECT m_rectSavedMask;
	BYTE m_bMaskLinkedFlag;
	BYTE m_bMaskDisabledFlag;
	BYTE m_bInvertMaskOnBlendFlag;
	WORD m_wBlendRangeCount;
	BYTE m_bSourceBlendRange1[4];
	BYTE m_bDestinationBlendRange1[4];
	BYTE m_bSourceBlendRange2[4];
	BYTE m_bDestinationBlendRange2[4];
	BYTE m_bSourceBlendRange3[4];
	BYTE m_bDestinationBlendRange3[4];
	BYTE m_bSourceBlendRange4[4];
	BYTE m_bDestinationBlendRange4[4];
	BYTE m_bSourceBlendRange5[4];
	BYTE m_bDestinationBlendRange5[4];
	// Expansion field.
};

struct TLayerBitmapChunk
{
	DWORD m_dwSize;
	WORD m_wBitmapCount;
	WORD m_wChannelCount;
	// Expansion field.
};

struct TChannelInformationChunk
{
	DWORD m_dwSize;
	DWORD m_dwCompressedLength;
	DWORD m_dwUncompressedLength;
	WORD m_wBitmapType;
	WORD m_wChannelType;
	// Expansion field.
};

#pragma pack(pop)


/**	Reads a PSP string from stream in.
	* @return 0 if no error.
	*/
LRESULT ReadPspString(std::ifstream &in, STD_TSTRING &strLayerName)
{
/*
#ifdef _UNICODE
#error This function does not yet support unicode..... ZZZZzzz
#endif
*/
	WORD wLength;
	in.read((char *)&wLength, sizeof(wLength));

	strLayerName.resize(wLength);
	in.read(&strLayerName[0], wLength);

	return 0;
}

#ifdef _DEBUG
//
// Some debug functions for dumping the contents of an PSP file
//

LRESULT DumpBlockType(DWORD dwBlockType)
{
	struct TBlockTypeInfo
	{
		LPCTSTR pTypeTag;
		LPCTSTR pTypeString;
	};
	static TBlockTypeInfo blockTypeInfo[]=
	{
		{ _T("PSP_IMAGE_BLOCK"), _T("General Image Attributes Block (main)") },
		{ _T("PSP_CREATOR_BLOCK"), _T("Creator Data Block (main)") },
		{ _T("PSP_COLOR_BLOCK"), _T("Color Palette Block (main and sub)") },
		{ _T("PSP_LAYER_START_BLOCK"), _T("Layer Bank Block (main)") },
		{ _T("PSP_LAYER_BLOCK"), _T("Layer Block (sub)") },
		{ _T("PSP_CHANNEL_BLOCK"), _T("Channel Block (sub)") },
		{ _T("PSP_SELECTION_BLOCK"), _T("Selection Block (main)") },
		{ _T("PSP_ALPHA_BANK_BLOCK"), _T("Alpha Bank Block (main)") },
		{ _T("PSP_ALPHA_CHANNEL_BLOCK"), _T("Alpha Channel Block (sub)") },
		{ _T("PSP_COMPOSITE_IMAGE_BLOCK"), _T("Composite Image Block (sub)") },
		{ _T("PSP_EXTENDED_DATA_BLOCK"), _T("Extended Data Block (main)") },
		{ _T("PSP_TUBE_BLOCK"), _T("Picture Tube Data Block (main)") },
		{ _T("PSP_ADJUSTMENT_EXTENSION_BLOCK"), _T("Adjustment Layer Block (sub)") },
		{ _T("PSP_VECTOR_EXTENSION_BLOCK"), _T("Vector Layer Block (sub)") },
		{ _T("PSP_SHAPE_BLOCK"), _T("Vector Shape Block (sub)") },
		{ _T("PSP_PAINTSTYLE_BLOCK"), _T("Paint Style Block (sub)") },
		{ _T("PSP_COMPOSITE_IMAGE_BANK_BLOCK"), _T("Composite Image Bank (main)") },
		{ _T("PSP_COMPOSITE_ATTRIBUTES_BLOCK"), _T("Composite Image Attr. (sub)") },
		{ _T("PSP_JPEG_BLOCK"), _T("JPEG Image Block (sub)") },
		{ _T("PSP_LINESTYLE_BLOCK"), _T("Line Style Block (sub)") },
		{ _T("PSP_TABLE_BANK_BLOCK"), _T("Table Bank Block (main)") },
		{ _T("PSP_TABLE_BLOCK"), _T("Table Block (sub)") },
		{ _T("PSP_PAPER_BLOCK"), _T("Vector Table Paper Block (sub)") },
		{ _T("PSP_PATTERN_BLOCK"), _T("Vector Table Pattern Block (sub)") },
	};

	if (dwBlockType>=	sizeof(blockTypeInfo)/sizeof(blockTypeInfo[0]))
	{
		TRACE("BlockType: %d (Unkown)\n", dwBlockType);
		return -1;
	}

	TRACE("BlockType: %s (%s)\n", blockTypeInfo[dwBlockType].pTypeTag, blockTypeInfo[dwBlockType].pTypeString);

	return 0;
}

LRESULT DumpPSPFile(LPCTSTR pFileName)
{
	std::ifstream in;
	in.open(pFileName, std::ios_base::binary);

	if (!in)
		return ERR_FILEOPEN;

	TPSPFILEHEADER header;
	in.read((char *)&header, sizeof(header));

	if (memcmp(header.m_bFileSignature, PSPFILESIGNATURE, sizeof(PSPFILESIGNATURE)))
		return ERR_INVALIDFILE;

	TRACE("File Version: %d.%d\n", header.m_wMajorVersion, header.m_wMinorVersion);


	TPSPBBLOCKHEADER blockHeader;

	//
	// Read the image attributes
	//
	in.read((char *)&blockHeader, sizeof(blockHeader));

	if (blockHeader.m_dwHeaderId != PSPBLOCKHEADERID)
		return ERR_CORRUPT;
	if (blockHeader.m_wBlockId != PSP_IMAGE_BLOCK)
		return ERR_CORRUPT;

	TGeneralImageAttributesChunk ImageAttributes;
	in.read((char *)&ImageAttributes, sizeof(ImageAttributes));
	in.seekg(blockHeader.m_dwTotalBlockLength-sizeof(ImageAttributes), std::ios_base::cur); // skip the expansion field

	//
	// Dump all blocks
	//
	while (1)
	{
		in.read((char *)&blockHeader, sizeof(blockHeader));
		if (in.eof())
			break;

		DWORD dwStartOfChunk= in.tellg();

		if (blockHeader.m_dwHeaderId != PSPBLOCKHEADERID)
			return ERR_CORRUPT;

		DumpBlockType(blockHeader.m_wBlockId);

		/*
		// Porra.... as composite images est? gravadas em JPEG e
		// n? h?maneira de alterar isso no PSP :(((( bem.... l?
		// vamos de ter que gravar a imagem no PSP como .PSP e merged!
		if (PSP_COMPOSITE_IMAGE_BANK_BLOCK == blockHeader.m_wBlockId)
		{
			TCompositeImageBankInformationChunk chunkHeader;
			in.read((char *)&chunkHeader, sizeof(chunkHeader));
			in.seekg(chunkHeader.m_dwSize - sizeof(chunkHeader), std::ios_base::cur); // skip extension block if any

			for (int i= 0; i<chunkHeader.m_dwImageCount; ++i)
			{
				TPSPBBLOCKHEADER subBlockHeader;
				in.read((char *)&subBlockHeader, sizeof(subBlockHeader));

				if (subBlockHeader.m_dwHeaderId != PSPBLOCKHEADERID)
					return ERR_CORRUPT;
				if (subBlockHeader.m_wBlockId != PSP_COMPOSITE_ATTRIBUTES_BLOCK)
					return ERR_CORRUPT;

				TCompositeImageAttributesInformationChunk imageInfoChunk;
				in.read((char *)&imageInfoChunk, sizeof(imageInfoChunk));
				in.seekg(imageInfoChunk.m_dwSize - sizeof(imageInfoChunk), std::ios_base::cur); // skip extension block if any
			}
		}
		*/

		// seek to next block
		in.seekg(dwStartOfChunk + blockHeader.m_dwTotalBlockLength);

		/*
		// skip block contents
		in.seekg(blockHeader.m_dwTotalBlockLength, std::ios_base::cur);
		*/
	}

	return 0;
}

/*
DumpPSPFile(pFileName);
return NULL;
/**/
#endif // #ifdef _DEBUG



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
LRESULT LoadPSPFileToDIB(/*[in]*/LPCTSTR pFileName, /*[out]*/HBITMAP &hBmp, /*[out]*/VOID **ppvBits/*= NULL*/)
{
	hBmp= NULL;

	std::ifstream in;
	in.open(pFileName, std::ios_base::binary);

	if (!in)
		return ERR_FILEOPEN;

	TPSPFILEHEADER header;
	in.read((char *)&header, sizeof(header));

	if (memcmp(header.m_bFileSignature, PSPFILESIGNATURE, sizeof(PSPFILESIGNATURE)))
		return ERR_INVALIDFILE;

	// only accept file version 4.0+ (PSP v6.0+)
	if (header.m_wMajorVersion <= 3)
		return ERR_NOTSUPORTEDFILEVERSION;

	TPSPBBLOCKHEADER blockHeader;

	//
	// Read the image attributes
	//
	in.read((char *)&blockHeader, sizeof(blockHeader));

	if (blockHeader.m_dwHeaderId != PSPBLOCKHEADERID)
		return ERR_CORRUPT;
	if (blockHeader.m_wBlockId != PSP_IMAGE_BLOCK)
		return ERR_CORRUPT;

	TGeneralImageAttributesChunk ImageAttributes;
	in.read((char *)&ImageAttributes, sizeof(ImageAttributes));
	in.seekg(blockHeader.m_dwTotalBlockLength-sizeof(ImageAttributes), std::ios_base::cur); // skip the expansion field


	// only loads files with the following attributes:
	// - Bit Depth == 24 (16.7M colors)
	// - Compression Type == PSP_COMP_NONE
	// - Plane Count == 1
	// - Layer Count == 1 (the image must be merged)
	if (ImageAttributes.m_wBitDepth != 24 || ImageAttributes.m_wCompressionType != PSP_COMP_NONE
		|| ImageAttributes.m_wPlaneCount != 1 || ImageAttributes.m_wLayerCount != 1)
		return ERR_ATTRIBUTESNOTSUPPORTED;


	//
	// find the PSP_LAYER_START_BLOCK block
	//
	while (1)
	{
		in.read((char *)&blockHeader, sizeof(blockHeader));
		if (in.eof())
			return ERR_CORRUPT;

		if (blockHeader.m_dwHeaderId != PSPBLOCKHEADERID)
			return ERR_CORRUPT;

		if (PSP_LAYER_START_BLOCK == blockHeader.m_wBlockId)
			break; // we found it!

		// seek to next block
		in.seekg(blockHeader.m_dwTotalBlockLength, std::ios_base::cur);
	}


	//
	// if we get here we have in blockHeader the PSP_LAYER_START_BLOCK block
	//

	// read the Layer Sub-Block Header
	in.read((char *)&blockHeader, sizeof(blockHeader));

	if (blockHeader.m_dwHeaderId != PSPBLOCKHEADERID)
		return ERR_CORRUPT;
	if (blockHeader.m_wBlockId != PSP_LAYER_BLOCK)
		return ERR_CORRUPT;


	TLayerInformationChunkHeader layerInformationHeader;
	in.read((char *)&layerInformationHeader, sizeof(layerInformationHeader));

	// skip the layer name
	WORD wLayerNameLength;
	in.read((char *)&wLayerNameLength, sizeof(wLayerNameLength));
	in.seekg(wLayerNameLength, std::ios_base::cur);

	// read layer information
	TLayerInformationChunk layerInformation;
	in.read((char *)&layerInformation, sizeof(layerInformation));
	in.seekg(layerInformationHeader.m_dwSize-(sizeof(layerInformationHeader)+sizeof(layerInformation)+sizeof(WORD)+wLayerNameLength), std::ios_base::cur); // skip the expansion field

	// read layer bitmap chunk
	TLayerBitmapChunk layerBitmapChunk;
	in.read((char *)&layerBitmapChunk, sizeof(layerBitmapChunk));
	in.seekg(layerBitmapChunk.m_dwSize-sizeof(layerBitmapChunk), std::ios_base::cur); // skip the expansion field


	//
	// read the channel (rgba) data one at a time
	//

	if (layerBitmapChunk.m_wChannelCount != 4) // there must be 4 channels on file!
		return ERR_ATTRIBUTESNOTSUPPORTED;

	TPSPBBLOCKHEADER channelBlockHeader;

		// create an windows DIB (24bit RGB)
		BITMAPINFO bmpInfo= { { sizeof(BITMAPINFOHEADER), layerInformation.m_rectSavedImage.right-layerInformation.m_rectSavedImage.left, layerInformation.m_rectSavedImage.bottom-layerInformation.m_rectSavedImage.top, 1, 32 } };
		RGBQUAD *pBitmapData= NULL;
		hBmp= ::CreateDIBSection(NULL, &bmpInfo, DIB_RGB_COLORS, (void **)&pBitmapData, NULL, NULL);

		if (hBmp == NULL)
			return ERR_CORRUPT; // not the best result code....

	DWORD dwBitmapFileDataSize= (layerInformation.m_rectSavedImage.right-layerInformation.m_rectSavedImage.left)*(layerInformation.m_rectSavedImage.bottom-layerInformation.m_rectSavedImage.top);
	BYTE *pBitmapFileData= new BYTE[dwBitmapFileDataSize]; // buffer for one channel (r, g, b or a)

	// read one channel per iteration
	for (int n= 0; n<4; ++n)
	{
		in.read((char *)&channelBlockHeader, sizeof(channelBlockHeader));

		if (channelBlockHeader.m_dwHeaderId != PSPBLOCKHEADERID)
			return ERR_CORRUPT;
		if (channelBlockHeader.m_wBlockId != PSP_CHANNEL_BLOCK)
			return ERR_CORRUPT;

		TChannelInformationChunk channelInformationChunk;
		in.read((char *)&channelInformationChunk, sizeof(channelInformationChunk));
		in.seekg(channelInformationChunk.m_dwSize-sizeof(channelInformationChunk), std::ios_base::cur); // skip the expansion field

		// test to make sure the file is okay
		ASSERT( channelInformationChunk.m_dwCompressedLength == dwBitmapFileDataSize );

		in.read((char *)pBitmapFileData, dwBitmapFileDataSize);

		//
		// copies the bitmap bits from file buffer to the dib buffer
		//
		for (int nY= 0; nY<bmpInfo.bmiHeader.biHeight; ++nY)
		{
			// fetch the start of scan line
			BYTE *pFileBits= pBitmapFileData + (nY * bmpInfo.bmiHeader.biWidth);
			// NOTE: Since our DIB is bottom-up (i.e. the height is positive) and
			// the PSP file format is top-down we have to swap bottom<->up scan lines.
			RGBQUAD *pDIBBits= pBitmapData + ((bmpInfo.bmiHeader.biHeight-1-nY) * bmpInfo.bmiHeader.biWidth);

			// do the actual copy
			for (int nX= 0; nX<bmpInfo.bmiHeader.biWidth; ++nX)
				*((BYTE *)&pDIBBits[nX] + (3-channelInformationChunk.m_wChannelType))= pFileBits[nX];
		}
	}

	delete []pBitmapFileData;

	if (ppvBits)
		*ppvBits= pBitmapData;

	return ERR_NOERROR;
}
