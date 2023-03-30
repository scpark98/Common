#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "LoadFormat.h"
#include "Aspose.Cells.Systems/String.h"
#include "PaperSizeType.h"
#include "CountryCode.h"
#include "Aspose.Cells.Systems/Globalization/CultureInfo.h"
#include "Aspose.Cells.Systems/Double.h"
#include "MemorySetting.h"

namespace Aspose {
	namespace Cells {
		enum LoadFormat;
		enum PaperSizeType;
		enum CountryCode;
		class ILoadFilter;
		enum MemorySetting;
		class IAutoFitterOptions;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Represents the options of loading the file.
			/// </summary>
	class ASPOSE_CELLS_API ILoadOptions : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Gets the load format.
			/// </summary>
			 virtual Aspose::Cells::LoadFormat GetLoadFormat()=0;
			/// <summary>
			/// Gets the password of the workbook.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetPassword()=0;
			/// <summary>
			/// Ssets the password of the workbook.
			/// </summary>
			 virtual void SetPassword(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Indicates whether parsing the formula when reading the file.
			/// </summary>
			/// <remarks>
			/// Only applies for Excel Xlsx, Xltx, Xltm and Xlsm file
			/// because the formulas in the files are stored with a string formula.
			/// </remarks>
			 virtual bool GetParsingFormulaOnOpen()=0;
			/// <summary>
			/// Indicates whether parsing the formula when reading the file.
			/// </summary>
			/// <remarks>
			/// Only applies for Excel Xlsx, Xltx, Xltm and Xlsm file
			/// because the formulas in the files are stored with a string formula.
			/// </remarks>
			 virtual void SetParsingFormulaOnOpen(bool value)=0;
			/// <summary>
			/// Indicates whether parsing pivot cached records when loading the file.
			/// The default value is false.
			/// </summary>
			/// <remarks>
			/// Only applies for Excel Xlsx, Xltx, Xltm , Xlsm and xlsb file</remarks>
			 virtual bool GetParsingPivotCachedRecords()=0;
			/// <summary>
			/// Indicates whether parsing pivot cached records when loading the file.
			/// The default value is false.
			/// </summary>
			/// <remarks>
			/// Only applies for Excel Xlsx, Xltx, Xltm , Xlsm and xlsb file</remarks>
			 virtual void SetParsingPivotCachedRecords(bool value)=0;
			/// <summary>
			/// Sets the default print paper size from default printer's setting.
			/// </summary>
			/// <param name="type" >The default paper size.</param>
			/// <remarks>
			/// If there is no setting about paper size,MS Excel will use default printer's setting.
			/// </remarks>
			 virtual void SetPaperSize(Aspose::Cells::PaperSizeType type)=0;
			/// <summary>
			/// Gets the user interface language of the Workbook version based on CountryCode that has saved the file. 
			/// </summary>
			 virtual Aspose::Cells::CountryCode GetLanguageCode()=0;
			/// <summary>
			/// Sets the user interface language of the Workbook version based on CountryCode that has saved the file. 
			/// </summary>
			 virtual void SetLanguageCode(Aspose::Cells::CountryCode value)=0;
			/// <summary>
			/// Gets the system regional settings based on CountryCode at the time the file was loaded. 
			/// </summary>
			/// <remarks>If you do not want to use the region  saved in the file, 
			/// please reset it after reading the file.</remarks>
			 virtual Aspose::Cells::CountryCode GetRegion()=0;
			/// <summary>
			/// Sets the system regional settings based on CountryCode at the time the file was loaded. 
			/// </summary>
			/// <remarks>If you do not want to use the region  saved in the file, 
			/// please reset it after reading the file.</remarks>
			 virtual void SetRegion(Aspose::Cells::CountryCode value)=0;
			/// <summary>
			/// Gets the system culture info at the time the file was loaded.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Globalization::CultureInfo> GetCultureInfo()=0;
			/// <summary>
			/// Sets the system culture info at the time the file was loaded.
			/// </summary>
			 virtual void SetCultureInfo(intrusive_ptr<Aspose::Cells::Systems::Globalization::CultureInfo> value)=0;
			/// <summary>
			/// Gets the default standard font name
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetStandardFont()=0;
			/// <summary>
			/// Sets the default standard font name
			/// </summary>
			 virtual void SetStandardFont(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Sets the default standard font size.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetStandardFontSize()=0;
			/// <summary>
			/// Sets the default standard font size.
			/// </summary>
			 virtual void SetStandardFontSize(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Ignore the data which are not printed if directly printing the file
			/// </summary>
			/// <remarks>
			/// Only for xlsx file.
			/// </remarks>
			 virtual bool GetIgnoreNotPrinted()=0;
			/// <summary>
			/// Ignore the data which are not printed if directly printing the file
			/// </summary>
			/// <remarks>
			/// Only for xlsx file.
			/// </remarks>
			 virtual void SetIgnoreNotPrinted(bool value)=0;
			/// <summary>
			/// Check whether data is valid in the template file.
			/// </summary>
			 virtual bool GetCheckDataValid()=0;
			/// <summary>
			/// Check whether data is valid in the template file.
			/// </summary>
			 virtual void SetCheckDataValid(bool value)=0;
			/// <summary>
			/// Whether check restriction of excel file when user modify cells related objects.
			/// For example, excel does not allow inputting string value longer than 32K.
			/// When you input a value longer than 32K such as by Cell.PutValue(string), if this property is true, you will get an Exception.
			/// If this property is false, we will accept your input string value as the cell's value so that later
			/// you can output the complete string value for other file formats such as CSV.
			/// However, if you have set such kind of value that is invalid for excel file format,
			/// you should not save the workbook as excel file format later. Otherwise there may be unexpected error for the generated excel file.
			/// </summary>
			 virtual bool GetCheckExcelRestriction()=0;
			/// <summary>
			/// Whether check restriction of excel file when user modify cells related objects.
			/// For example, excel does not allow inputting string value longer than 32K.
			/// When you input a value longer than 32K such as by Cell.PutValue(string), if this property is true, you will get an Exception.
			/// If this property is false, we will accept your input string value as the cell's value so that later
			/// you can output the complete string value for other file formats such as CSV.
			/// However, if you have set such kind of value that is invalid for excel file format,
			/// you should not save the workbook as excel file format later. Otherwise there may be unexpected error for the generated excel file.
			/// </summary>
			 virtual void SetCheckExcelRestriction(bool value)=0;
			/// <summary>
			/// Whether keep the unparsed data in memory for the Workbook when it is loaded from template file. Default is true.
			/// </summary>
			/// <remarks>
			/// For scenarios that user only needs to read some contents from template file and does not need to save the workbook back,
			/// set this property as false may improve performance, especially when using it together with some kind of LoadFilter,
			/// </remarks>
			 virtual bool GetKeepUnparsedData()=0;
			/// <summary>
			/// Whether keep the unparsed data in memory for the Workbook when it is loaded from template file. Default is true.
			/// </summary>
			/// <remarks>
			/// For scenarios that user only needs to read some contents from template file and does not need to save the workbook back,
			/// set this property as false may improve performance, especially when using it together with some kind of LoadFilter,
			/// </remarks>
			 virtual void SetKeepUnparsedData(bool value)=0;
			/// <summary>
			/// The filter to denote how to load data.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::ILoadFilter> 		GetILoadFilter()=0;
			/// <summary>
			/// The filter to denote how to load data.
			/// </summary>
			 virtual void 		SetILoadFilter(intrusive_ptr<Aspose::Cells::ILoadFilter> value)=0;
			/// <summary>
			/// Gets the memory usage options.
			/// </summary>
			 virtual Aspose::Cells::MemorySetting GetMemorySetting()=0;
			/// <summary>
			/// Sets the memory usage options.
			/// </summary>
			 virtual void SetMemorySetting(Aspose::Cells::MemorySetting value)=0;
			/// <summary>
			/// Gets and sets the auto fitter options
			/// </summary>
			/// <remarks>
			/// Only for xlsx ,spreadsheetML file now.
			/// </remarks>
			 virtual intrusive_ptr<Aspose::Cells::IAutoFitterOptions> 		GetIAutoFitterOptions()=0;
			/// <summary>
			/// Sets the auto fitter options
			/// </summary>
			/// <remarks>
			/// Only for xlsx ,spreadsheetML file now.
			/// </remarks>
			 virtual void 		SetIAutoFitterOptions(intrusive_ptr<Aspose::Cells::IAutoFitterOptions> value)=0;
			/// <summary>
			/// Indicates whether auto filtering the data when loading the files.
			/// </summary>
			/// <remarks>
			/// Sometimes although autofilter is set, the corresponding rows is not hidden in the file.
			/// Now only works for SpreadSheetML file.
			/// 
			/// </remarks>
			 virtual bool GetAutoFilter()=0;
			/// <summary>
			/// Indicates whether auto filtering the data when loading the files.
			/// </summary>
			/// <remarks>
			/// Sometimes although autofilter is set, the corresponding rows is not hidden in the file.
			/// Now only works for SpreadSheetML file.
			/// 
			/// </remarks>
			 virtual void SetAutoFilter(bool value)=0;

	};
}
}
