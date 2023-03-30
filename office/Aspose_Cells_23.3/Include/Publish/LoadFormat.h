#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Represents the load file format.
			/// </summary>
	enum LoadFormat 

	{

			/// <summary>
			/// Represents recognizing the format automatically. 
			/// </summary>
			LoadFormat_Auto ,
			/// <summary>
			/// Comma-Separated Values(CSV) text file.
			/// </summary>
			LoadFormat_Csv = 1 ,
			/// <summary>
			/// Comma-Separated Values(CSV) text file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Csv" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			LoadFormat_CSV = 1 ,
			/// <summary>
			/// Represents Office Open XML spreadsheetML workbook or template, with or without macros. 
			/// </summary>
			LoadFormat_Xlsx = 6 ,
			/// <summary>
			/// Tab-Separated Values(TSV) text file.
			/// </summary>
			LoadFormat_Tsv = 11 ,
			/// <summary>
			/// Tab-Separated Values(TSV) text file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Tsv" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			LoadFormat_TSV = 11 ,
			/// <summary>
			/// Represents a tab delimited text file, same with <see cref="Tsv" />
			/// .
			/// </summary>
			LoadFormat_TabDelimited = Aspose::Cells::LoadFormat_Tsv ,
			/// <summary>
			/// Represents a html file.
			/// </summary>
			LoadFormat_Html = 12 ,
			/// <summary>
			/// Represents a mhtml file.
			/// </summary>
			LoadFormat_MHtml = 13 ,
			/// <summary>
			/// Open Document Sheet(ODS) file.
			/// </summary>
			LoadFormat_Ods = 14 ,
			/// <summary>
			///  Open Document Sheet(ODS) file.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Ods" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			LoadFormat_ODS = 14 ,
			/// <summary>
			/// Represents an Excel97-2003 xls file.
			/// </summary>
			LoadFormat_Excel97To2003 = 5 ,
			/// <summary>
			/// Represents an Excel 2003 xml file.
			/// </summary>
			LoadFormat_SpreadsheetML = 15 ,
			/// <summary>
			/// Represents an xlsb file.
			/// </summary>
			LoadFormat_Xlsb = 16 ,
			/// <summary>
			/// Open Document Template Sheet(OTS) file.
			/// </summary>
			LoadFormat_Ots = 31 ,
			/// <summary>
			/// Represents a numbers file.
			/// </summary>
			LoadFormat_Numbers = 56 ,
			/// <summary>
			/// Represents OpenDocument Flat XML Spreadsheet (.fods) file format.
			/// </summary>
			LoadFormat_Fods = 59 ,
			/// <summary>
			/// Represents OpenDocument Flat XML Spreadsheet (.fods) file format.
			///  NOTE: This member is now obsolete. Instead, 
			///  please use <see cref="Fods" />
			///  property.
			///  This property will be removed 6 months later since April 2021. 
			///  Aspose apologizes for any inconvenience you may have experienced.
			///  </summary>
			LoadFormat_FODS = 59 ,
			/// <summary>
			/// Represents StarOffice Calc Spreadsheet (.sxc) file format.
			/// </summary>
			LoadFormat_Sxc = 60 ,
			/// <summary>
			/// Represents StarOffice Calc Spreadsheet (.sxc) file format.
			/// NOTE: This member is now obsolete. Instead, 
			/// please use <see cref="Sxc" />
			///  property.
			/// This property will be removed 6 months later since April 2021. 
			/// Aspose apologizes for any inconvenience you may have experienced.
			/// </summary>
			LoadFormat_SXC = 60 ,
			/// <summary>
			/// Represents a simple xml file.
			/// </summary>
			LoadFormat_Xml = 51 ,
			/// <summary>
			/// Represents unrecognized format, cannot be loaded. 
			/// </summary>
			LoadFormat_Unknown = 255 ,
			/// <summary>
			/// Image
			/// </summary>
			LoadFormat_Image = 254 ,
			/// <summary>
			/// Json
			/// </summary>
			LoadFormat_Json = 0x201 ,
	};


}

}
