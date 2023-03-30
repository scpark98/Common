#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Represents the format in which the workbook is saved.
			/// </summary>
			/// 
			/// 
	enum SaveFormat 

	{

			/// <summary>
			/// Comma-Separated Values(CSV) text file.
			/// </summary>
			SaveFormat_Csv = 1 ,
			/// <summary>
			/// Comma-Separated Values(CSV) text file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Csv" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			SaveFormat_CSV = 1 ,
			/// <summary>
			/// Represents an xlsx file.
			/// </summary>
			/// 
			/// 
			SaveFormat_Xlsx = 6 ,
			/// <summary>
			/// Represents an xlsm file which enable macros.
			/// </summary>
			/// 
			/// 
			SaveFormat_Xlsm = 7 ,
			/// <summary>
			/// Represents an xltx file.
			/// </summary>
			/// 
			/// 
			SaveFormat_Xltx = 8 ,
			/// <summary>
			/// Represents an xltm file which enable macros.
			/// </summary>
			/// 
			/// 
			SaveFormat_Xltm = 9 ,
			/// <summary>
			/// Represents an xltm file which enable addin macros.
			/// </summary>
			/// 
			/// 
			SaveFormat_Xlam = 10 ,
			/// <summary>
			/// Tab-Separated Values(TSV) text file.
			/// </summary>
			SaveFormat_Tsv = 11 ,
			/// <summary>
			/// Tab-Separated Values(TSV) text file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Tsv" />
			///  property.
			/// </summary>
			SaveFormat_TSV = 11 ,
			/// <summary>
			/// Represents a tab delimited text file, same with <see cref="Tsv" />
			/// .
			/// </summary>
			SaveFormat_TabDelimited = Aspose::Cells::SaveFormat_Tsv ,
			/// <summary>
			/// Represents a html file.
			/// </summary>
			/// 
			/// 
			SaveFormat_Html = 12 ,
			/// <summary>
			/// Represents a mhtml file.
			/// </summary>
			/// 
			/// 
			SaveFormat_MHtml = 17 ,
			/// <summary>
			/// Open Document Sheet(ODS) file.
			/// </summary>
			SaveFormat_Ods = 14 ,
			/// <summary>
			///  Open Document Sheet(ODS) file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Ods" />
			///  property.
			/// </summary>
			SaveFormat_ODS = 14 ,
			/// <summary>
			/// Represents an Excel97-2003 xls file.
			/// </summary>
			/// 
			/// 
			SaveFormat_Excel97To2003 = 5 ,
			/// <summary>
			/// Represents an Excel 2003 xml file.
			/// </summary>
			/// 
			/// 
			SaveFormat_SpreadsheetML = 15 ,
			/// <summary>
			/// Represents an xlsb file.
			/// </summary>
			/// 
			/// 
			SaveFormat_Xlsb = 16 ,
			/// <summary>
			/// If saving the file to the disk,the file format accords to the extension of the file name.
			/// If saving the file to the stream, the file format is xlsx.
			/// </summary>
			/// 
			/// 
			SaveFormat_Auto = 0 ,
			/// <summary>
			/// Represents unrecognized format, cannot be saved. 
			/// </summary>
			/// 
			/// 
			SaveFormat_Unknown = 255 ,
			/// <summary>
			/// Represents a Pdf file.
			/// </summary>
			/// 
			/// 
			SaveFormat_Pdf = 13 ,
			/// <summary>
			/// XPS (XML Paper Specification) format.
			/// </summary>
			SaveFormat_Xps = 20 ,
			/// <summary>
			///  XPS (XML Paper Specification) format.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="SaveFormat_Xps" />
			///  property.
			/// </summary>
			SaveFormat_XPS = 20 ,
			/// <summary>
			/// Represents a TIFF file.
			/// </summary>
			SaveFormat_Tiff = 21 ,
			/// <summary>
			/// Represents a TIFF file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="SaveFormat_Tiff" />
			///  property.
			/// </summary>
			SaveFormat_TIFF = 21 ,
			/// <summary>
			/// SVG file.
			/// </summary>
			SaveFormat_Svg = 28 ,
			/// <summary>
			/// SVG file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="SaveFormat_Svg" />
			///  property.
			/// </summary>
			SaveFormat_SVG = 28 ,
			/// <summary>
			/// Data Interchange Format.
			/// </summary>
			SaveFormat_Dif = 30 ,
			/// <summary>
			/// Open Document Template Sheet(OTS) file.
			/// </summary>
			SaveFormat_Ots = 31 ,
			/// <summary>
			/// Excel 97-2003 template file.
			/// </summary>
			SaveFormat_Xlt = 32 ,
			/// <summary>
			/// Represents a simple xml file.
			/// </summary>
			SaveFormat_Xml = 51 ,
			/// <summary>
			/// Represents a numbers file.
			/// </summary>
			/// <remarks>
			/// Not supported.
			/// </remarks>
			SaveFormat_Numbers = 56 ,
			/// <summary>
			/// Represents markdown document.
			/// </summary>
			SaveFormat_Markdown = 57 ,
			/// <summary>
			/// Represents OpenDocument Flat XML Spreadsheet (.fods) file format.
			/// </summary>
			SaveFormat_Fods = 59 ,
			/// <summary>
			/// Represents OpenDocument Flat XML Spreadsheet (.fods) file format.
			///  NOTE: This member is now obsolete. Instead, 
			///  please use <see cref="Fods" />
			///  property.
			///  </summary>
			SaveFormat_FODS = 59 ,
			/// <summary>
			/// Represents StarOffice Calc Spreadsheet (.sxc) file format.
			/// </summary>
			SaveFormat_Sxc = 60 ,
			/// <summary>
			/// Represents StarOffice Calc Spreadsheet (.sxc) file format.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Sxc" />
			///  property.
			/// </summary>
			SaveFormat_SXC = 60 ,
			/// <summary>
			/// Represents .pptx file.
			/// </summary>
			SaveFormat_Pptx = 61 ,
			/// <summary>
			/// Represents .docx file.
			/// </summary>
			SaveFormat_Docx = 62 ,
			/// <summary>
			/// Windows Enhanced Metafile.
			/// </summary>
			SaveFormat_Emf = 0x102 ,
			/// <summary>
			/// JPEG JFIF.
			/// </summary>
			SaveFormat_Jpg = 0x105 ,
			/// <summary>
			/// Portable Network Graphics.
			/// </summary>
			SaveFormat_Png = 0x106 ,
			/// <summary>
			/// Windows Bitmap
			/// </summary>
			SaveFormat_Bmp = 0x107 ,
			/// <summary>
			/// Gif
			/// </summary>
			SaveFormat_Gif = 0x142 ,
			/// <summary>
			/// Json
			/// </summary>
			SaveFormat_Json = 0x201 ,
			/// <summary>
			/// Sql
			/// </summary>
			SaveFormat_SqlScript = 0x202 ,
	};


}

}
