#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Represents the options to filter data when loading workbook from template.
			/// </summary>
	enum LoadDataFilterOptions 

	{

			/// <summary>
			/// Load nothing for sheet data
			/// </summary>
			/// <remarks>
			/// NOTE: This member is now obsolete and please use Structure instead.
			/// This property will be removed 12 months later since December 2017. 
			/// Aspose apologizes for any inconvenience you may have experienced.</remarks>
			LoadDataFilterOptions_None = 0x00000000 ,
			/// <summary>
			/// Load all
			/// </summary>
			LoadDataFilterOptions_All = 0x7FFFFFFF ,
			/// <summary>
			/// Load cells whose value is blank
			/// </summary>
			LoadDataFilterOptions_CellBlank = 0x00000001 ,
			/// <summary>
			/// Load cells whose value is string
			/// </summary>
			LoadDataFilterOptions_CellString = 0x00000002 ,
			/// <summary>
			/// Load cells whose value is numeric(including datetime)
			/// </summary>
			LoadDataFilterOptions_CellNumeric = 0x00000004 ,
			/// <summary>
			/// Load cells whose value is error
			/// </summary>
			LoadDataFilterOptions_CellError = 0x00000008 ,
			/// <summary>
			/// Load cells whose value is bool
			/// </summary>
			LoadDataFilterOptions_CellBool = 0x00000010 ,
			/// <summary>
			/// Load cells value(all value types) only
			/// </summary>
			LoadDataFilterOptions_CellValue = 0x0000001F ,
			/// <summary>
			/// Load cell formulas.
			/// </summary>
			/// <remarks>
			/// Generally defined Name objects(DefinedNames) also need to be loaded when loading formulas because they may be referenced by formulas.
			/// So Formula or CellData option should work with DefinedNames option together(Formula|DefinedNames or CellData|DefinedNames) for most scenarios.
			/// </remarks>
			LoadDataFilterOptions_Formula = 0x00000020 ,
			/// <summary>
			/// Load cells data including values, formulas and formatting
			/// </summary>
			LoadDataFilterOptions_CellData = 0x0400003F ,
			/// <summary>
			/// Load charts
			/// </summary>
			LoadDataFilterOptions_Chart = 0x00000100 ,
			/// <summary>
			/// Load shapes
			/// </summary>
			/// <remarks>
			/// NOTE: This member is now obsolete and please use Drawing instead.
			/// This property will be removed 12 months later since November 2019. 
			/// Aspose apologizes for any inconvenience you may have experienced.</remarks>
			LoadDataFilterOptions_Shape = 0x18000200 ,
			/// <summary>
			/// Drawing objects(including Chart, Picture, OleObject and all other drawing objects)
			/// </summary>
			LoadDataFilterOptions_Drawing = 0x18000300 ,
			/// <summary>
			/// Load merged cells
			/// </summary>
			LoadDataFilterOptions_MergedArea = 0x00000400 ,
			/// <summary>
			/// Load conditional formatting
			/// </summary>
			LoadDataFilterOptions_ConditionalFormatting = 0x00000800 ,
			/// <summary>
			/// Load data validations
			/// </summary>
			LoadDataFilterOptions_DataValidation = 0x00001000 ,
			/// <summary>
			/// Load pivot tables
			/// </summary>
			LoadDataFilterOptions_PivotTable = 0x00002000 ,
			/// <summary>
			/// Load tables
			/// </summary>
			LoadDataFilterOptions_Table = 0x00004000 ,
			/// <summary>
			/// Load hyperlinks
			/// </summary>
			LoadDataFilterOptions_Hyperlinks = 0x00008000 ,
			/// <summary>
			/// Load settings for worksheet
			/// </summary>
			LoadDataFilterOptions_SheetSettings = 0x00010000 ,
			/// <summary>
			/// Load all data of worksheet, such as cells data, settings, objects, ...etc.
			/// </summary>
			LoadDataFilterOptions_SheetData = 0x180FFFFF ,
			/// <summary>
			/// Load settings for workbook
			/// </summary>
			LoadDataFilterOptions_BookSettings = 0x00100000 ,
			/// <summary>
			/// Load settings for workbook and worksheet
			/// </summary>
			LoadDataFilterOptions_Settings = 0x00110000 ,
			/// <summary>
			/// Load XmlMap
			/// </summary>
			LoadDataFilterOptions_XmlMap = 0x00200000 ,
			/// <summary>
			/// Load structure of the workbook
			/// </summary>
			LoadDataFilterOptions_Structure = 0x00400000 ,
			/// <summary>
			/// Load document properties
			/// </summary>
			LoadDataFilterOptions_DocumentProperties = 0x00800000 ,
			/// <summary>
			/// Load defined Name objects
			/// </summary>
			LoadDataFilterOptions_DefinedNames = 0x01000000 ,
			/// <summary>
			/// Load VBA projects
			/// </summary>
			LoadDataFilterOptions_VBA = 0x02000000 ,
			/// <summary>
			/// Load styles for cell formatting
			/// </summary>
			LoadDataFilterOptions_Style = 0x04000000 ,
			/// <summary>
			/// Load pictures
			/// </summary>
			LoadDataFilterOptions_Picture = 0x08000000 ,
			/// <summary>
			/// Load OleObjects
			/// </summary>
			LoadDataFilterOptions_OleObject = 0x10000000 ,
			/// <summary>
			/// Load revision logs
			/// </summary>
			LoadDataFilterOptions_Revision = 0x20000000 ,
	};


}

}
