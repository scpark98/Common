#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "SaveFormat.h"
#include "Aspose.Cells.Systems/Drawing/Printing/PrintPageEventHandler.h"
#include "Aspose.Cells.Systems/Drawing/Printing/QueryPageSettingsEventHandler.h"
#include "Aspose.Cells.Systems/Int32.h"
#include "TiffCompression.h"
#include "ColorDepth.h"
#include "PrintingPageType.h"
#include "ImageType.h"
#include "Aspose.Cells.Systems/Drawing/Imaging/ImageFormat.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Drawing/Text/TextRenderingHint.h"
#include "Aspose.Cells.Systems/Drawing/Drawing2D/SmoothingMode.h"
#include "Aspose.Cells.Systems/Drawing/Imaging/PixelFormat.h"
#include "GridlineType.h"
#include "TextCrossType.h"
#include "Aspose.Cells.Systems/Drawing/Imaging/EmfType.h"
#include "DefaultEditLanguage.h"

namespace Aspose {
	namespace Cells {
		enum SaveFormat;
		enum PrintingPageType;
		enum GridlineType;
		enum TextCrossType;
		enum DefaultEditLanguage;
	}
}
namespace Aspose {
	namespace Cells {
		namespace Rendering {
			enum TiffCompression;
			enum ColorDepth;
			class IIPageSavingCallback;
		}
	}
}
namespace Aspose {
	namespace Cells {
		namespace Drawing {
			enum ImageType;
		}
	}
}
namespace Aspose{
namespace Cells{
namespace Rendering{
			/// <summary>
			/// Allows to specify options when rendering worksheet to images, printing worksheet or rendering chart to image.
			/// </summary>
	class ASPOSE_CELLS_API IImageOrPrintOptions : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Gets the output file format type
			/// Support Tiff/XPS
			/// </summary>
			 virtual Aspose::Cells::SaveFormat GetSaveFormat()=0;
			/// <summary>
			/// Sets the output file format type
			/// Support Tiff/XPS
			/// </summary>
			 virtual void SetSaveFormat(Aspose::Cells::SaveFormat value)=0;
			/// <summary>
			/// Client can special output to printer when print each page using this EventHandler
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Drawing::Printing::PrintPageEventHandler> GetCustomPrintPageEventHandler()=0;
			/// <summary>
			/// Client can special output to printer when print each page using this EventHandler
			/// </summary>
			 virtual void SetCustomPrintPageEventHandler(intrusive_ptr<Aspose::Cells::Systems::Drawing::Printing::PrintPageEventHandler> value)=0;
			/// <summary>
			/// Client can control page setting of printer when print each page using this EventHandler
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Drawing::Printing::QueryPageSettingsEventHandler> GetCustomQueryPageSettingsEventHandler()=0;
			/// <summary>
			/// Client can control page setting of printer when print each page using this EventHandler
			/// </summary>
			 virtual void SetCustomQueryPageSettingsEventHandler(intrusive_ptr<Aspose::Cells::Systems::Drawing::Printing::QueryPageSettingsEventHandler> value)=0;
			/// <summary>
			/// If PrintWithStatusDialog = true , there will be a dialog that shows current print status.
			/// else no such dialog will show.
			/// </summary>
			 virtual bool GetPrintWithStatusDialog()=0;
			/// <summary>
			/// If PrintWithStatusDialog = true , there will be a dialog that shows current print status.
			/// else no such dialog will show.
			/// </summary>
			 virtual void SetPrintWithStatusDialog(bool value)=0;
			/// <summary>
			/// Gets the horizontal resolution for generated images, in dots per inch.
			/// Applies generating image method except Emf format images.
			/// </summary>
			/// <remarks>
			/// The default value is 96.
			/// </remarks>
			 virtual Aspose::Cells::Systems::Int32 GetHorizontalResolution()=0;
			/// <summary>
			/// Sets the horizontal resolution for generated images, in dots per inch.
			/// Applies generating image method except Emf format images.
			/// </summary>
			/// <remarks>
			/// The default value is 96.
			/// </remarks>
			 virtual void SetHorizontalResolution(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets the vertical  resolution for generated images, in dots per inch.
			/// Applies generating image method except Emf format image.
			/// </summary>
			/// <remarks>
			/// The default value is 96.
			/// </remarks>
			 virtual Aspose::Cells::Systems::Int32 GetVerticalResolution()=0;
			/// <summary>
			/// Sets the vertical  resolution for generated images, in dots per inch.
			/// Applies generating image method except Emf format image.
			/// </summary>
			/// <remarks>
			/// The default value is 96.
			/// </remarks>
			 virtual void SetVerticalResolution(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets the type of compression to apply only when saving pages to the <c>Tiff</c>
			///  format. 
			/// </summary>
			/// <remarks>
			/// Has effect only when saving to TIFF.
			/// The default value is Lzw.
			/// </remarks>
			 virtual Aspose::Cells::Rendering::TiffCompression GetTiffCompression()=0;
			/// <summary>
			/// Sets the type of compression to apply only when saving pages to the <c>Tiff</c>
			///  format. 
			/// </summary>
			/// <remarks>
			/// Has effect only when saving to TIFF.
			/// The default value is Lzw.
			/// </remarks>
			 virtual void SetTiffCompression(Aspose::Cells::Rendering::TiffCompression value)=0;
			/// <summary>
			/// Gets bit depth to apply only when saving pages to the <c>Tiff</c>
			///  format. 
			/// </summary>
			/// <remarks>
			/// Has effect only when saving to TIFF.
			/// If TiffCompression is set to CCITT3, CCITT4, this will not take effect, the bit depth of the generated tiff image will be always 1.
			/// </remarks>
			 virtual Aspose::Cells::Rendering::ColorDepth GetTiffColorDepth()=0;
			/// <summary>
			/// Sets bit depth to apply only when saving pages to the <c>Tiff</c>
			///  format. 
			/// </summary>
			/// <remarks>
			/// Has effect only when saving to TIFF.
			/// If TiffCompression is set to CCITT3, CCITT4, this will not take effect, the bit depth of the generated tiff image will be always 1.
			/// </remarks>
			 virtual void SetTiffColorDepth(Aspose::Cells::Rendering::ColorDepth value)=0;
			/// <summary>
			/// Indicates which pages will not be printed.
			/// </summary>
			 virtual Aspose::Cells::PrintingPageType GetPrintingPage()=0;
			/// <summary>
			/// Indicates which pages will not be printed.
			/// </summary>
			 virtual void SetPrintingPage(Aspose::Cells::PrintingPageType value)=0;
			/// <summary>
			/// Gets a value determining the quality of the generated  images
			/// to apply only when saving pages to the <c>Jpeg</c>
			///  format. The default value is 100
			/// </summary>
			/// <remarks>
			/// Has effect only when saving to JPEG.
			/// The value must be between 0 and 100.
			/// The default value is 100.
			/// </remarks>
			 virtual Aspose::Cells::Systems::Int32 GetQuality()=0;
			/// <summary>
			/// Sets a value determining the quality of the generated  images
			/// to apply only when saving pages to the <c>Jpeg</c>
			///  format. The default value is 100
			/// </summary>
			/// <remarks>
			/// Has effect only when saving to JPEG.
			/// The value must be between 0 and 100.
			/// The default value is 100.
			/// </remarks>
			 virtual void SetQuality(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets or sets the format of the generated images.
			/// default value: PNG.
			/// </summary>
			 virtual Aspose::Cells::Drawing::ImageType GetImageType()=0;
			/// <summary>
			/// Gets or sets the format of the generated images.
			/// default value: PNG.
			/// </summary>
			 virtual void SetImageType(Aspose::Cells::Drawing::ImageType value)=0;
			/// <summary>
			/// Indicates whether the width and height of the cells is automatically fitted by cell value. 
			/// The default value is false.
			/// </summary>
			 virtual bool IsCellAutoFit()=0;
			/// <summary>
			/// Indicates whether the width and height of the cells is automatically fitted by cell value. 
			/// The default value is false.
			/// </summary>
			 virtual void SetCellAutoFit(bool value)=0;
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
			/// The width of paper size of pagesetup will be invalid, and the other settings of pagesetup 
			/// will still take effect.
			/// </summary>
			 virtual bool GetAllColumnsInOnePagePerSheet()=0;
			/// <summary>
			/// If AllColumnsInOnePagePerSheet is true , all column content of one sheet will output to only one page in result. 
			/// The width of paper size of pagesetup will be invalid, and the other settings of pagesetup 
			/// will still take effect.
			/// </summary>
			 virtual void SetAllColumnsInOnePagePerSheet(bool value)=0;
			/// <summary>
			/// Indicate the chart imagetype when converting.
			/// default value: PNG.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Drawing::Imaging::ImageFormat> GetChartImageType()=0;
			/// <summary>
			/// Indicate the chart imagetype when converting.
			/// default value: PNG.
			/// </summary>
			 virtual void SetChartImageType(intrusive_ptr<Aspose::Cells::Systems::Drawing::Imaging::ImageFormat> value)=0;
			/// <summary>
			/// Indicate the filename of embedded image in svg. 
			/// This should be full path with directory like "c:\\xpsEmbedded"
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetEmbededImageNameInSvg()=0;
			/// <summary>
			/// Indicate the filename of embedded image in svg. 
			/// This should be full path with directory like "c:\\xpsEmbedded"
			/// </summary>
			 virtual void SetEmbededImageNameInSvg(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// if this property is true, the generated svg will fit to view port.
			/// </summary>
			 virtual bool GetSVGFitToViewPort()=0;
			/// <summary>
			/// if this property is true, the generated svg will fit to view port.
			/// </summary>
			 virtual void SetSVGFitToViewPort(bool value)=0;
			/// <summary>
			/// If this property is true , one Area will be output, and no scale will take effect.
			/// </summary>
			 virtual bool GetOnlyArea()=0;
			/// <summary>
			/// If this property is true , one Area will be output, and no scale will take effect.
			/// </summary>
			 virtual void SetOnlyArea(bool value)=0;
			/// <summary>
			/// Specifies the quality of text rendering.
			/// The default value is TextRenderingHint.SystemDefault
			/// </summary>
			 virtual Aspose::Cells::Systems::Drawing::Text::TextRenderingHint GetTextRenderingHint()=0;
			/// <summary>
			/// Specifies the quality of text rendering.
			/// The default value is TextRenderingHint.SystemDefault
			/// </summary>
			 virtual void SetTextRenderingHint(Aspose::Cells::Systems::Drawing::Text::TextRenderingHint value)=0;
			/// <summary>
			/// Specifies whether smoothing (antialiasing) is applied to lines and curves and the edges of filled areas.
			/// The default value is SmoothingMode.None
			/// </summary>
			 virtual Aspose::Cells::Systems::Drawing::Drawing2D::SmoothingMode GetSmoothingMode()=0;
			/// <summary>
			/// Specifies whether smoothing (antialiasing) is applied to lines and curves and the edges of filled areas.
			/// The default value is SmoothingMode.None
			/// </summary>
			 virtual void SetSmoothingMode(Aspose::Cells::Systems::Drawing::Drawing2D::SmoothingMode value)=0;
			/// <summary>
			/// Indicates if the background of generated image should be transparent.
			/// </summary>
			/// <remarks>
			/// The default value is false. That means the background of the generated images is white.
			/// </remarks>
			 virtual bool GetTransparent()=0;
			/// <summary>
			/// Indicates if the background of generated image should be transparent.
			/// </summary>
			/// <remarks>
			/// The default value is false. That means the background of the generated images is white.
			/// </remarks>
			 virtual void SetTransparent(bool value)=0;
			/// <summary>
			/// Gets the pixel format for the generated images.
			/// </summary>
			/// <remarks>
			/// The default value is PixelFormat.Format32bppArgb.
			/// </remarks>
			 virtual Aspose::Cells::Systems::Drawing::Imaging::PixelFormat GetPixelFormat()=0;
			/// <summary>
			/// Sets the pixel format for the generated images.
			/// </summary>
			/// <remarks>
			/// The default value is PixelFormat.Format32bppArgb.
			/// </remarks>
			 virtual void SetPixelFormat(Aspose::Cells::Systems::Drawing::Imaging::PixelFormat value)=0;
			/// <summary>
			/// Control/Indicate progress of page saving process.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Rendering::IIPageSavingCallback> 		GetIPageSavingCallback()=0;
			/// <summary>
			/// Control/Indicate progress of page saving process.
			/// </summary>
			 virtual void 		SetPageSavingCallback(intrusive_ptr<Aspose::Cells::Rendering::IIPageSavingCallback> value)=0;
			/// <summary>
			/// Indicates whether only substitute the font of character when the cell font is not compatibility for it.
			/// </summary>
			/// <remarks>
			/// Default is false. We will try default font of Workbook and PdfSaveOption/system for cell font first.
			/// </remarks>
			 virtual bool IsFontSubstitutionCharGranularity()=0;
			/// <summary>
			/// Indicates whether only substitute the font of character when the cell font is not compatibility for it.
			/// </summary>
			/// <remarks>
			/// Default is false. We will try default font of Workbook and PdfSaveOption/system for cell font first.
			/// </remarks>
			 virtual void SetFontSubstitutionCharGranularity(bool value)=0;
			/// <summary>
			///  Gets the 0-based index of the first page to save.
			/// </summary>
			/// <remarks>
			/// Default is 0.
			/// </remarks>
			 virtual Aspose::Cells::Systems::Int32 GetPageIndex()=0;
			/// <summary>
			///  Sets the 0-based index of the first page to save.
			/// </summary>
			/// <remarks>
			/// Default is 0.
			/// </remarks>
			 virtual void SetPageIndex(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			///  Gets the number of pages to save.
			/// </summary>
			/// <remarks>
			///  Default is System.Int32.MaxValue which means all pages will be rendered..
			///  </remarks>
			 virtual Aspose::Cells::Systems::Int32 GetPageCount()=0;
			/// <summary>
			///  Sets the number of pages to save.
			/// </summary>
			/// <remarks>
			///  Default is System.Int32.MaxValue which means all pages will be rendered..
			///  </remarks>
			 virtual void SetPageCount(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Sets desired width and height of image.
			/// </summary>
			/// <param name="desiredWidth" >desired width in pixels</param>
			/// <param name="desiredHeight" >desired height in pixels</param>
			 virtual void SetDesiredSize(Aspose::Cells::Systems::Int32 desiredWidth , Aspose::Cells::Systems::Int32 desiredHeight)=0;
			/// <summary>
			/// Indicates whether optimize the output elements.
			/// </summary>
			/// <remarks>
			/// Default value is false. Currently only the border lines are?optimized when this property is set to true.
			/// </remarks>
			 virtual bool IsOptimized()=0;
			/// <summary>
			/// Indicates whether optimize the output elements.
			/// </summary>
			/// <remarks>
			/// Default value is false. Currently only the border lines are?optimized when this property is set to true.
			/// </remarks>
			 virtual void SetOptimized(bool value)=0;
			/// <summary>
			/// When characters in the Excel are unicode and not be set with correct font in cell style,
			/// They may appear as block in pdf,image.
			/// Set the DefaultFont such as MingLiu or MS Gothic to show these characters.
			/// If this property is not set, Aspose.Cells will use system default font to show these unicode characters.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetDefaultFont()=0;
			/// <summary>
			/// When characters in the Excel are unicode and not be set with correct font in cell style,
			/// They may appear as block in pdf,image.
			/// Set the DefaultFont such as MingLiu or MS Gothic to show these characters.
			/// If this property is not set, Aspose.Cells will use system default font to show these unicode characters.
			/// </summary>
			 virtual void SetDefaultFont(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// When characters in the Excel are unicode and not be set with correct font in cell style,
			/// They may appear as block in pdf,image.
			/// Set this to true to try to use workbook's default font to show these characters first.
			/// </summary>
			/// <remarks>
			/// Default is true.
			/// </remarks>
			 virtual bool GetCheckWorkbookDefaultFont()=0;
			/// <summary>
			/// When characters in the Excel are unicode and not be set with correct font in cell style,
			/// They may appear as block in pdf,image.
			/// Set this to true to try to use workbook's default font to show these characters first.
			/// </summary>
			/// <remarks>
			/// Default is true.
			/// </remarks>
			 virtual void SetCheckWorkbookDefaultFont(bool value)=0;
			/// <summary>
			/// Indicates whether to output a blank page when there is nothing to print.
			/// </summary>
			/// <remarks>
			/// Default is false.
			/// </remarks>
			 virtual bool GetOutputBlankPageWhenNothingToPrint()=0;
			/// <summary>
			/// Indicates whether to output a blank page when there is nothing to print.
			/// </summary>
			/// <remarks>
			/// Default is false.
			/// </remarks>
			 virtual void SetOutputBlankPageWhenNothingToPrint(bool value)=0;
			/// <summary>
			///  Gets gridline type.
			/// </summary>
			/// <remarks>
			///  Default is Dotted type.
			///  </remarks>
			 virtual Aspose::Cells::GridlineType GetGridlineType()=0;
			/// <summary>
			///  Sets gridline type.
			/// </summary>
			/// <remarks>
			///  Default is Dotted type.
			///  </remarks>
			 virtual void SetGridlineType(Aspose::Cells::GridlineType value)=0;
			/// <summary>
			/// Gets displaying text type when the text width is larger than cell width.
			/// </summary>
			 virtual Aspose::Cells::TextCrossType GetTextCrossType()=0;
			/// <summary>
			/// Sets displaying text type when the text width is larger than cell width.
			/// </summary>
			 virtual void SetTextCrossType(Aspose::Cells::TextCrossType value)=0;
			/// <summary>
			/// Gets an EmfType that specifies the format of the Metafile..
			/// <remarks>The default value is EmfPlusDual.</remarks>
			/// </summary>
			 virtual Aspose::Cells::Systems::Drawing::Imaging::EmfType GetEmfType()=0;
			/// <summary>
			/// Sets an EmfType that specifies the format of the Metafile..
			/// <remarks>The default value is EmfPlusDual.</remarks>
			/// </summary>
			 virtual void SetEmfType(Aspose::Cells::Systems::Drawing::Imaging::EmfType value)=0;
			/// <summary>
			/// Gets default edit language.
			/// </summary>
			/// <remarks>
			/// It may display/render different layouts for text paragraph when different edit languages is set.
			/// Default is <see cref="DefaultEditLanguage.Auto" />
			/// .
			/// </remarks>
			 virtual Aspose::Cells::DefaultEditLanguage GetDefaultEditLanguage()=0;
			/// <summary>
			/// Sets default edit language.
			/// </summary>
			/// <remarks>
			/// It may display/render different layouts for text paragraph when different edit languages is set.
			/// Default is <see cref="DefaultEditLanguage.Auto" />
			/// .
			/// </remarks>
			 virtual void SetDefaultEditLanguage(Aspose::Cells::DefaultEditLanguage value)=0;

	};
}
}
}
