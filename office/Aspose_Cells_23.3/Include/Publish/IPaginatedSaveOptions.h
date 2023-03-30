#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Int32.h"
#include "PrintingPageType.h"
#include "GridlineType.h"
#include "TextCrossType.h"
#include "DefaultEditLanguage.h"
#include "ISaveOptions.h"

namespace Aspose {
	namespace Cells {
		enum PrintingPageType;
		enum GridlineType;
		enum TextCrossType;
		enum DefaultEditLanguage;
	}
}
namespace Aspose {
	namespace Cells {
		namespace Rendering {
			class IIPageSavingCallback;
		}
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Represents the options for pagination.
			/// </summary>
	class ASPOSE_CELLS_API IPaginatedSaveOptions : virtual public Aspose::Cells::ISaveOptions
	{
		public:
			/// <summary>
			/// When characters in the Excel are Unicode and not be set with correct font in cell style,
			/// They may appear as block in pdf,image.
			/// Set the DefaultFont such as MingLiu or MS Gothic to show these characters.
			/// If this property is not set, Aspose.Cells will use system default font to show these unicode characters.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetDefaultFont()=0;
			/// <summary>
			/// When characters in the Excel are Unicode and not be set with correct font in cell style,
			/// They may appear as block in pdf,image.
			/// Set the DefaultFont such as MingLiu or MS Gothic to show these characters.
			/// If this property is not set, Aspose.Cells will use system default font to show these unicode characters.
			/// </summary>
			 virtual void SetDefaultFont(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// When characters in the Excel are Unicode and not be set with correct font in cell style,
			/// They may appear as block in pdf,image.
			/// Set this to true to try to use workbook's default font to show these characters first.
			/// </summary>
			/// <remarks>
			/// Default is true.
			/// </remarks>
			 virtual bool GetCheckWorkbookDefaultFont()=0;
			/// <summary>
			/// When characters in the Excel are Unicode and not be set with correct font in cell style,
			/// They may appear as block in pdf,image.
			/// Set this to true to try to use workbook's default font to show these characters first.
			/// </summary>
			/// <remarks>
			/// Default is true.
			/// </remarks>
			 virtual void SetCheckWorkbookDefaultFont(bool value)=0;
			/// <summary>
			/// Indicates whether to check font compatibility for every character in text.
			/// </summary>
			/// <remarks>
			/// The default value is true.
			/// Disable this property may give better performance.
			/// But when the default or specified font of text/character cannot be used to render it,
			/// unreadable characters(such as block) maybe occur in the generated pdf.
			/// For such situation user should keep this property as true so that
			/// alternative font can be searched and used to render the text instead;
			/// </remarks>
			 virtual bool GetCheckFontCompatibility()=0;
			/// <summary>
			/// Indicates whether to check font compatibility for every character in text.
			/// </summary>
			/// <remarks>
			/// The default value is true.
			/// Disable this property may give better performance.
			/// But when the default or specified font of text/character cannot be used to render it,
			/// unreadable characters(such as block) maybe occur in the generated pdf.
			/// For such situation user should keep this property as true so that
			/// alternative font can be searched and used to render the text instead;
			/// </remarks>
			 virtual void SetCheckFontCompatibility(bool value)=0;
			/// <summary>
			/// Indicates whether to only substitute the font of character when the cell font is not compatibility for it.
			/// </summary>
			/// <remarks>
			/// Default is false. We will try default font of Workbook and PdfSaveOption/system for cell font first.
			/// </remarks>
			 virtual bool IsFontSubstitutionCharGranularity()=0;
			/// <summary>
			/// Indicates whether to only substitute the font of character when the cell font is not compatibility for it.
			/// </summary>
			/// <remarks>
			/// Default is false. We will try default font of Workbook and PdfSaveOption/system for cell font first.
			/// </remarks>
			 virtual void SetFontSubstitutionCharGranularity(bool value)=0;
			/// <summary>
			/// If OnePagePerSheet is true , all content of one sheet will output to only one page in result. 
			/// The paper size of pagesetup will be invalid, and the other settings of pagesetup 
			/// will still take effect.
			/// </summary>
			 virtual bool GetOnePagePerSheet()=0;
			/// <summary>
			/// If OnePagePerSheet is true , all content of one sheet will output to only one page in result. 
			/// The paper size of pagesetup will be invalid, and the other settings of pagesetup 
			/// will still take effect.
			/// </summary>
			 virtual void SetOnePagePerSheet(bool value)=0;
			/// <summary>
			/// If AllColumnsInOnePagePerSheet is true , all column content of one sheet will output to only one page in result. 
			/// The width of paper size of pagesetup will be ignored, and the other settings of pagesetup 
			/// will still take effect.
			/// </summary>
			 virtual bool GetAllColumnsInOnePagePerSheet()=0;
			/// <summary>
			/// If AllColumnsInOnePagePerSheet is true , all column content of one sheet will output to only one page in result. 
			/// The width of paper size of pagesetup will be ignored, and the other settings of pagesetup 
			/// will still take effect.
			/// </summary>
			 virtual void SetAllColumnsInOnePagePerSheet(bool value)=0;
			/// <summary>
			/// Indicates if you need to hide the error while rendering.
			/// The error can be error in shape, image, chart rendering, etc.
			/// </summary>
			 virtual bool GetIgnoreError()=0;
			/// <summary>
			/// Indicates if you need to hide the error while rendering.
			/// The error can be error in shape, image, chart rendering, etc.
			/// </summary>
			 virtual void SetIgnoreError(bool value)=0;
			/// <summary>
			/// Indicates whether to output a blank page when there is nothing to print.
			/// </summary>
			/// <remarks>
			/// Default is true.
			/// </remarks>
			 virtual bool GetOutputBlankPageWhenNothingToPrint()=0;
			/// <summary>
			/// Indicates whether to output a blank page when there is nothing to print.
			/// </summary>
			/// <remarks>
			/// Default is true.
			/// </remarks>
			 virtual void SetOutputBlankPageWhenNothingToPrint(bool value)=0;
			/// <summary>
			///  Gets or sets the 0-based index of the first page to save.
			/// </summary>
			/// <remarks>
			/// Default is 0.
			/// </remarks>
			/// <example>
			/// The following example shows how to render a range of pages (3 and 4) in a Microsoft Excel file to PDF.
			/// <code>
			///  //Open an Excel file
			///  Workbook wb = new Workbook("Book1.xlsx");
			///  
			///  PdfSaveOptions options = new PdfSaveOptions();
			///  
			///  //Print only Page 3 and Page 4 in the output PDF
			///  //Starting page index (0-based index)
			///  options.PageIndex = 3;
			///  //Number of pages to be printed
			///  options.PageCount = 2;
			///  
			///  //Save the PDF file
			///  wb.Save("output.pdf", options);
			/// </code>
			/// </example>
			 virtual Aspose::Cells::Systems::Int32 GetPageIndex()=0;
			/// <summary>
			///  Gets or sets the 0-based index of the first page to save.
			/// </summary>
			/// <remarks>
			/// Default is 0.
			/// </remarks>
			/// <example>
			/// The following example shows how to render a range of pages (3 and 4) in a Microsoft Excel file to PDF.
			/// <code>
			///  //Open an Excel file
			///  Workbook wb = new Workbook("Book1.xlsx");
			///  
			///  PdfSaveOptions options = new PdfSaveOptions();
			///  
			///  //Print only Page 3 and Page 4 in the output PDF
			///  //Starting page index (0-based index)
			///  options.PageIndex = 3;
			///  //Number of pages to be printed
			///  options.PageCount = 2;
			///  
			///  //Save the PDF file
			///  wb.Save("output.pdf", options);
			/// </code>
			/// </example>
			 virtual void SetPageIndex(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			///  Gets or sets the number of pages to save.
			/// </summary>
			/// <remarks>
			///  Default is System.Int32.MaxValue which means all pages will be rendered..
			///  </remarks>
			/// <example>
			/// The following example shows how to render a range of pages (3 and 4) in a Microsoft Excel file to PDF.
			/// <code>
			///  //Open an Excel file
			///  Workbook wb = new Workbook("Book1.xlsx");
			///  
			///  PdfSaveOptions options = new PdfSaveOptions();
			///  
			///  //Print only Page 3 and Page 4 in the output PDF
			///  //Starting page index (0-based index)
			///  options.PageIndex = 3;
			///  //Number of pages to be printed
			///  options.PageCount = 2;
			///  
			///  //Save the PDF file
			///  wb.Save("output.pdf", options);
			/// </code>
			/// </example>
			 virtual Aspose::Cells::Systems::Int32 GetPageCount()=0;
			/// <summary>
			///  Gets or sets the number of pages to save.
			/// </summary>
			/// <remarks>
			///  Default is System.Int32.MaxValue which means all pages will be rendered..
			///  </remarks>
			/// <example>
			/// The following example shows how to render a range of pages (3 and 4) in a Microsoft Excel file to PDF.
			/// <code>
			///  //Open an Excel file
			///  Workbook wb = new Workbook("Book1.xlsx");
			///  
			///  PdfSaveOptions options = new PdfSaveOptions();
			///  
			///  //Print only Page 3 and Page 4 in the output PDF
			///  //Starting page index (0-based index)
			///  options.PageIndex = 3;
			///  //Number of pages to be printed
			///  options.PageCount = 2;
			///  
			///  //Save the PDF file
			///  wb.Save("output.pdf", options);
			/// </code>
			/// </example>
			 virtual void SetPageCount(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Indicates which pages will not be printed.
			/// </summary>
			/// <remarks>
			/// If content in the sheet is sparse, there will be some pages are totally blank in the output pdf file.
			/// If you don't want these blank pages, you can use this option to omit them.
			/// </remarks>
			/// <example>
			/// The following code omits blank pages or pages which only contains some style content like cell background, borders.
			/// <code>
			/// Workbook wb = new Workbook("Book1.xlsx");
			/// 
			/// PdfSaveOptions pdfSaveOptions = new PdfSaveOptions();
			/// 
			/// //ignore blank pages
			/// pdfSaveOptions.PrintingPageType = PrintingPageType.IgnoreBlank;
			/// 
			/// wb.Save("output_ignore_blank_page.pdf", pdfSaveOptions);
			/// 
			/// 
			/// //ignore blank pages and pages which only contains some style content like cell background
			/// pdfSaveOptions.PrintingPageType = PrintingPageType.IgnoreStyle;
			/// 
			/// wb.Save("output_ignore_blank_and_style_page.pdf", pdfSaveOptions);
			/// </code>
			/// </example>
			 virtual Aspose::Cells::PrintingPageType GetPrintingPageType()=0;
			/// <summary>
			/// Indicates which pages will not be printed.
			/// </summary>
			/// <remarks>
			/// If content in the sheet is sparse, there will be some pages are totally blank in the output pdf file.
			/// If you don't want these blank pages, you can use this option to omit them.
			/// </remarks>
			/// <example>
			/// The following code omits blank pages or pages which only contains some style content like cell background, borders.
			/// <code>
			/// Workbook wb = new Workbook("Book1.xlsx");
			/// 
			/// PdfSaveOptions pdfSaveOptions = new PdfSaveOptions();
			/// 
			/// //ignore blank pages
			/// pdfSaveOptions.PrintingPageType = PrintingPageType.IgnoreBlank;
			/// 
			/// wb.Save("output_ignore_blank_page.pdf", pdfSaveOptions);
			/// 
			/// 
			/// //ignore blank pages and pages which only contains some style content like cell background
			/// pdfSaveOptions.PrintingPageType = PrintingPageType.IgnoreStyle;
			/// 
			/// wb.Save("output_ignore_blank_and_style_page.pdf", pdfSaveOptions);
			/// </code>
			/// </example>
			 virtual void SetPrintingPageType(Aspose::Cells::PrintingPageType value)=0;
			/// <summary>
			///  Gets or sets gridline type.
			/// </summary>
			/// <remarks>
			///  Default is Dotted type.
			///  </remarks>
			 virtual Aspose::Cells::GridlineType GetGridlineType()=0;
			/// <summary>
			///  Gets or sets gridline type.
			/// </summary>
			/// <remarks>
			///  Default is Dotted type.
			///  </remarks>
			 virtual void SetGridlineType(Aspose::Cells::GridlineType value)=0;
			/// <summary>
			/// Gets or sets displaying text type when the text width is larger than cell width.
			/// </summary>
			 virtual Aspose::Cells::TextCrossType GetTextCrossType()=0;
			/// <summary>
			/// Gets or sets displaying text type when the text width is larger than cell width.
			/// </summary>
			 virtual void SetTextCrossType(Aspose::Cells::TextCrossType value)=0;
			/// <summary>
			/// Gets or sets default edit language.
			/// </summary>
			/// <remarks>
			/// It may display/render different layouts for text paragraph when different edit languages is set.
			/// Default is <see cref="Aspose.Cells.DefaultEditLanguage.Auto" />
			/// .
			/// </remarks>
			 virtual Aspose::Cells::DefaultEditLanguage GetDefaultEditLanguage()=0;
			/// <summary>
			/// Gets or sets default edit language.
			/// </summary>
			/// <remarks>
			/// It may display/render different layouts for text paragraph when different edit languages is set.
			/// Default is <see cref="Aspose.Cells.DefaultEditLanguage.Auto" />
			/// .
			/// </remarks>
			 virtual void SetDefaultEditLanguage(Aspose::Cells::DefaultEditLanguage value)=0;
			/// <summary>
			/// Control/Indicate progress of page saving process.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Rendering::IIPageSavingCallback> 		GetIPageSavingCallback()=0;
			/// <summary>
			/// Control/Indicate progress of page saving process.
			/// </summary>
			 virtual void 		SetIPageSavingCallback(intrusive_ptr<Aspose::Cells::Rendering::IIPageSavingCallback> value)=0;

	};
}
}
