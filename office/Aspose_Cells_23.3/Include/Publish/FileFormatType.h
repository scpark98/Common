#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Represents the file format types.
			/// </summary>
	enum FileFormatType 

	{

			/// <summary>
			/// Comma-Separated Values(CSV) text file.
			/// </summary>
			FileFormatType_Csv = 1 ,
			/// <summary>
			/// Comma-Separated Values(CSV) text file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Csv" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_CSV = 1 ,
			/// <summary>
			/// Office Open XML SpreadsheetML file (macro-free).
			/// </summary>
			FileFormatType_Xlsx = 6 ,
			/// <summary>
			/// Office Open XML SpreadsheetML Macro-Enabled file.
			/// </summary>
			FileFormatType_Xlsm = 7 ,
			/// <summary>
			/// Office Open XML SpreadsheetML Template (macro-free).
			/// </summary>
			FileFormatType_Xltx = 8 ,
			/// <summary>
			/// Office Open XML SpreadsheetML Macro-Enabled Template.
			/// </summary>
			FileFormatType_Xltm = 9 ,
			/// <summary>
			/// Office Open XML SpreadsheetML addinMacro-Enabled file.
			/// </summary>
			FileFormatType_Xlam = 10 ,
			/// <summary>
			/// Tab-Separated Values(TSV) text file.
			/// </summary>
			FileFormatType_Tsv = 11 ,
			/// <summary>
			/// Tab-Separated Values(TSV) text file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Tsv" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_TSV = 11 ,
			/// <summary>
			/// Tab-Separated Values(TSV) text file, same with <see cref="Tsv" />
			/// .
			///  </summary>
			FileFormatType_TabDelimited = Aspose::Cells::FileFormatType_Tsv ,
			/// <summary>
			/// HTML format.
			/// </summary>
			FileFormatType_Html = 12 ,
			/// <summary>
			/// MHTML (Web archive) format.
			/// </summary>
			FileFormatType_MHtml = 17 ,
			/// <summary>
			/// Open Document Sheet(ODS) file.
			/// </summary>
			FileFormatType_Ods = 14 ,
			/// <summary>
			///  Open Document Sheet(ODS) file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Ods" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_ODS = 14 ,
			/// <summary>
			/// Excel97-2003 spreadsheet file.
			/// </summary>
			FileFormatType_Excel97To2003 = 5 ,
			/// <summary>
			/// Excel 2003 SpreadsheetML file.
			/// </summary>
			FileFormatType_SpreadsheetML = 15 ,
			/// <summary>
			/// Excel 2003 SpreadsheetML file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="SpreadsheetML" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_Excel2003XML = 15 ,
			/// <summary>
			/// The Excel Binary File Format (.xlsb)
			/// </summary>
			FileFormatType_Xlsb = 16 ,
			/// <summary>
			/// Represents unrecognized format, cannot be loaded. 
			/// </summary>
			FileFormatType_Unknown = 255 ,
			/// <summary>
			/// PDF (Adobe Portable Document) format.
			/// </summary>
			FileFormatType_Pdf = 13 ,
			/// <summary>
			/// XPS (XML Paper Specification) format.
			/// </summary>
			FileFormatType_Xps = 20 ,
			/// <summary>
			///  XPS (XML Paper Specification) format.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Xps" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_XPS = 20 ,
			/// <summary>
			/// Represents a TIFF file.
			/// </summary>
			FileFormatType_Tiff = 21 ,
			/// <summary>
			/// Represents a TIFF file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Tiff" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_TIFF = 21 ,
			/// <summary>
			/// SVG file.
			/// </summary>
			FileFormatType_Svg = 28 ,
			/// <summary>
			/// SVG file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Svg" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_SVG = 28 ,
			/// <summary>
			/// Represents an Excel95 xls file.
			/// </summary>
			FileFormatType_Excel95 = 22 ,
			/// <summary>
			/// Represents an Excel4.0 xls file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// </remarks>
			FileFormatType_Excel4 = 23 ,
			/// <summary>
			/// Represents an Excel3.0 xls file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// </remarks>
			FileFormatType_Excel3 = 24 ,
			/// <summary>
			/// Represents an Excel2.1 xls file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// </remarks>
			FileFormatType_Excel2 = 25 ,
			/// <summary>
			/// Represents a pptx file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Pptx = 26 ,
			/// <summary>
			/// Represents a docx file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Docx = 27 ,
			/// <summary>
			/// Data Interchange Format.
			/// </summary>
			FileFormatType_Dif = 30 ,
			/// <summary>
			/// Represents a doc file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Doc = 31 ,
			/// <summary>
			/// Represents a ppt file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Ppt = 32 ,
			/// <summary>
			/// Represents a email file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_MapiMessage = 33 ,
			/// <summary>
			/// Represents the MS Equation 3.0 object.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_MsEquation = 34 ,
			/// <summary>
			/// Represents the MS Equation 3.0 object.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="MsEquation" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_MSEquation = 34 ,
			/// <summary>
			/// Represents the embedded native object.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Ole10Native = 35 ,
			/// <summary>
			/// Represents MS Visio VSD binary format.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Vsd = 36 ,
			/// <summary>
			/// Represents MS Visio VSD binary format.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Vsd" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_VSD = 36 ,
			/// <summary>
			/// Represents MS Visio 2013 VSDX file format.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Vsdx = 37 ,
			/// <summary>
			/// Represents MS Visio 2013 VSDX file format.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Vsdx" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_VSDX = 37 ,
			/// <summary>
			/// Represents a docm file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Docm = 38 ,
			/// <summary>
			/// Represents a dotx file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Dotx = 39 ,
			/// <summary>
			/// Represents a dotm file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Dotm = 40 ,
			/// <summary>
			/// Represents a pptm file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Pptm = 41 ,
			/// <summary>
			/// Represents a Potx file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Potx = 42 ,
			/// <summary>
			/// Represents a Potm file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Potm = 43 ,
			/// <summary>
			/// Represents a ppsx file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Ppsx = 44 ,
			/// <summary>
			/// Represents a ppsm file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Ppsm = 45 ,
			/// <summary>
			/// Represents office open xml file(such as xlsx, docx,pptx, etc).
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// If the office open xml file is encrypted, it could not be detected as xlsx ,docx, pptx,etc.
			/// </remarks>
			FileFormatType_Ooxml = 46 ,
			/// <summary>
			/// Represents an ODT file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Odt = 47 ,
			/// <summary>
			/// Represents an ODT file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Odt" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_ODT = 47 ,
			/// <summary>
			/// Represents a ODP file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Odp = 48 ,
			/// <summary>
			/// Represents an ODP file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Odp" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_ODP = 48 ,
			/// <summary>
			/// Represents an ODF file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Odf = 49 ,
			/// <summary>
			/// Represents an ODF file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Odf" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_ODF = 49 ,
			/// <summary>
			/// Represents an ODG file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Odg = 50 ,
			/// <summary>
			/// Represents an ODG file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Odg" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_ODG = 50 ,
			/// <summary>
			/// Represents a simple xml file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Xml = 51 ,
			/// <summary>
			/// Represents a simple xml file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Xml" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_XML = 51 ,
			/// <summary>
			/// Excel97-2003 spreadsheet template.
			/// </summary>
			FileFormatType_Xlt = 52 ,
			/// <summary>
			/// Represents an OTT file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Ott = 53 ,
			/// <summary>
			/// Represents an OTT file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Ott" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_OTT = 53 ,
			/// <summary>
			/// Represents a BMP file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Bmp = 54 ,
			/// <summary>
			/// Represents an BMP file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Bmp" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_BMP = 54 ,
			/// <summary>
			/// Represents an ots file.
			/// </summary>
			FileFormatType_Ots = 55 ,
			/// <summary>
			/// Represents an OTS file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Ots" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_OTS = 55 ,
			/// <summary>
			/// Represents Numbers 9.0 file format by Apple Inc.
			/// </summary>
			FileFormatType_Numbers = 56 ,
			/// <summary>
			/// Represents Numbers 9.0 file format by Apple Inc.
			/// </summary>
			FileFormatType_Numbers09 = 56 ,
			/// <summary>
			/// Represents markdown document.
			/// </summary>
			FileFormatType_Markdown = 57 ,
			/// <summary>
			/// Represents embedded graph chart.
			/// </summary>
			FileFormatType_GraphChart = 58 ,
			/// <summary>
			/// Represents OpenDocument Flat XML Spreadsheet (.fods) file format.
			/// </summary>
			FileFormatType_Fods = 59 ,
			/// <summary>
			/// Represents OpenDocument Flat XML Spreadsheet (.fods) file format.
			///  NOTE: This member is now obsolete. Instead, 
			///  please use <see cref="Fods" />
			///  property.
			///  This property will be removed 6 months later since April 2021. 
			///  Aspose apologizes for any inconvenience you may have experienced.
			///  </summary>
			FileFormatType_FODS = 59 ,
			/// <summary>
			/// Represents StarOffice Calc Spreadsheet (.sxc) file format.
			/// </summary>
			FileFormatType_Sxc = 60 ,
			/// <summary>
			/// Represents StarOffice Calc Spreadsheet (.sxc) file format.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Sxc" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_SXC = 60 ,
			/// <summary>
			/// Represents a OTP file.
			/// </summary>
			/// <remarks>
			/// The file format is not supported.
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Otp = 61 ,
			/// <summary>
			/// Represents a OTP file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Otp" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			FileFormatType_OTP = 61 ,
			/// <summary>
			///  Represents Numbers 3.5 file format since 2014 by Apple Inc
			/// </summary>
			/// <remarks>
			/// The file format is not supported.
			/// Only for detecting file type. 
			/// </remarks>
			FileFormatType_Numbers35 = 62 ,
			/// <summary>
			/// Windows Enhanced Metafile.
			/// </summary>
			FileFormatType_Emf = 0x102 ,
			/// <summary>
			/// Windows Metafile. 
			/// </summary>
			FileFormatType_Wmf = 0x103 ,
			/// <summary>
			/// JPEG JFIF.
			/// </summary>
			FileFormatType_Jpg = 0x105 ,
			/// <summary>
			/// Portable Network Graphics.
			/// </summary>
			FileFormatType_Png = 0x106 ,
			/// <summary>
			/// Gif
			/// </summary>
			FileFormatType_Gif = 0x142 ,
			/// <summary>
			/// Json
			/// </summary>
			FileFormatType_Json = 0x201 ,
			/// <summary>
			/// Sql
			/// </summary>
			FileFormatType_SqlScript = 0x202 ,
	};


}

}
