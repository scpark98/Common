#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "PdfCompliance.h"
#include "Aspose.Cells.Systems/Drawing/Imaging/ImageFormat.h"
#include "PdfCompressionCore.h"
#include "Aspose.Cells.Systems/Int32.h"
#include "Aspose.Cells.Systems/DateTime.h"
#include "Aspose.Cells.Systems/String.h"
#include "PdfOptimizationType.h"
#include "PdfCustomPropertiesExport.h"
#include "EmfRenderSetting.h"
#include "PdfFontEncoding.h"
#include "IPaginatedSaveOptions.h"

namespace Aspose {
	namespace Cells {
		namespace Rendering {
			class IPdfBookmarkEntry;
			enum PdfCompliance;
			enum PdfCompressionCore;
			enum PdfOptimizationType;
			enum PdfCustomPropertiesExport;
			enum PdfFontEncoding;
		}
	}
}
namespace Aspose {
	namespace Cells {
		enum EmfRenderSetting;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Represents the options for saving pdf file.
			/// </summary>
	class ASPOSE_CELLS_API IPdfSaveOptions : virtual public Aspose::Cells::IPaginatedSaveOptions
	{
		public:
			/// <summary>
			/// True to embed true type fonts. 
			/// Affects only ASCII characters 32-127.
			/// Fonts for character codes greater than 127 are always embedded.
			/// Fonts are always embedded for PDF/A-1a, PDF/A-1b standard.
			/// Default is true.
			/// </summary>
			 virtual bool GetEmbedStandardWindowsFonts()=0;
			/// <summary>
			/// True to embed true type fonts. 
			/// Affects only ASCII characters 32-127.
			/// Fonts for character codes greater than 127 are always embedded.
			/// Fonts are always embedded for PDF/A-1a, PDF/A-1b standard.
			/// Default is true.
			/// </summary>
			 virtual void SetEmbedStandardWindowsFonts(bool value)=0;
			/// <summary>
			/// Gets the <see cref="IPdfBookmarkEntry" >PdfBookmarkEntry</see>
			///  object.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Rendering::IPdfBookmarkEntry> 		GetIPdfBookmarkEntry()=0;
			/// <summary>
			/// Sets the <see cref="IPdfBookmarkEntry" >PdfBookmarkEntry</see>
			///  object.
			/// </summary>
			 virtual void 		SetIPdfBookmarkEntry(intrusive_ptr<Aspose::Cells::Rendering::IPdfBookmarkEntry> value)=0;
			/// <summary>
			/// Workbook converts to pdf will according to PdfCompliance in this property.
			/// </summary>
			 virtual Aspose::Cells::Rendering::PdfCompliance GetCompliance()=0;
			/// <summary>
			/// Workbook converts to pdf will according to PdfCompliance in this property.
			/// </summary>
			 virtual void SetCompliance(Aspose::Cells::Rendering::PdfCompliance value)=0;
			/// <summary>
			/// Represents the image type when converting the chart and shape .
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Drawing::Imaging::ImageFormat> GetImageType()=0;
			/// <summary>
			/// Represents the image type when converting the chart and shape .
			/// </summary>
			 virtual void SetImageType(intrusive_ptr<Aspose::Cells::Systems::Drawing::Imaging::ImageFormat> value)=0;
			/// <summary>
			/// Indicates whether calculate formulas before saving pdf file.
			/// </summary>
			/// <remarks>
			/// The default value is false.
			/// </remarks>
			 virtual bool GetCalculateFormula()=0;
			/// <summary>
			/// Indicates whether calculate formulas before saving pdf file.
			/// </summary>
			/// <remarks>
			/// The default value is false.
			/// </remarks>
			 virtual void SetCalculateFormula(bool value)=0;
			/// <summary>
			/// Indicate the compression algorithm 
			/// </summary>
			 virtual Aspose::Cells::Rendering::PdfCompressionCore GetPdfCompression()=0;
			/// <summary>
			/// Indicate the compression algorithm 
			/// </summary>
			 virtual void SetPdfCompression(Aspose::Cells::Rendering::PdfCompressionCore value)=0;
			/// <summary>
			/// Sets desired PPI(pixels per inch) of resample images and jpeg quality.  
			/// All images will be converted to JPEG with the specified quality setting, 
			/// and images that are greater than the specified PPI (pixels per inch) will be resampled.
			/// </summary>

			 virtual void SetImageResample(Aspose::Cells::Systems::Int32 desiredPPI , Aspose::Cells::Systems::Int32 jpegQuality)=0;
			/// <summary>
			///  Gets the time of generating the pdf document.
			/// </summary>
			/// <remarks>
			///  if it is not be set, it will be the time of generating the pdf.
			///  </remarks>
			 virtual intrusive_ptr<Aspose::Cells::Systems::DateTime> GetCreatedTime()=0;
			/// <summary>
			///  Sets the time of generating the pdf document.
			/// </summary>
			/// <remarks>
			///  if it is not be set, it will be the time of generating the pdf.
			///  </remarks>
			 virtual void SetCreatedTime(intrusive_ptr<Aspose::Cells::Systems::DateTime> value)=0;
			/// <summary>
			/// Gets producer of generated pdf document.
			/// </summary>
			/// <remarks>
			/// If the value is null, or a valid LICENSE is not set, string Aspose.Cells vVERSION will be used.
			/// </remarks>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetProducer()=0;
			/// <summary>
			/// Sets producer of generated pdf document.
			/// </summary>
			/// <remarks>
			/// If the value is null, or a valid LICENSE is not set, string Aspose.Cells vVERSION will be used.
			/// </remarks>
			 virtual void SetProducer(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Gets pdf optimization type.
			/// </summary>
			/// <remarks>
			/// Default value is <see cref="PdfOptimizationType.Standard" />
			/// </remarks>
			 virtual Aspose::Cells::Rendering::PdfOptimizationType GetOptimizationType()=0;
			/// <summary>
			/// Sets pdf optimization type.
			/// </summary>
			/// <remarks>
			/// Default value is <see cref="PdfOptimizationType.Standard" />
			/// </remarks>
			 virtual void SetOptimizationType(Aspose::Cells::Rendering::PdfOptimizationType value)=0;
			/// <summary>
			/// Gets a value determining the way <see cref="CustomDocumentPropertyCollection" />
			///  are exported to PDF file. Default value is None.
			/// </summary>
			 virtual Aspose::Cells::Rendering::PdfCustomPropertiesExport GetCustomPropertiesExport()=0;
			/// <summary>
			/// Sets a value determining the way <see cref="CustomDocumentPropertyCollection" />
			///  are exported to PDF file. Default value is None.
			/// </summary>
			 virtual void SetCustomPropertiesExport(Aspose::Cells::Rendering::PdfCustomPropertiesExport value)=0;
			/// <summary>
			/// Gets a value determining whether or not to export document structure.
			/// </summary>
			 virtual bool GetExportDocumentStructure()=0;
			/// <summary>
			/// Sets a value determining whether or not to export document structure.
			/// </summary>
			 virtual void SetExportDocumentStructure(bool value)=0;
			/// <summary>
			/// Setting for rendering Emf metafile.
			/// </summary>
			/// <remarks>
			/// EMF metafiles identified as "EMF+ Dual" can contain both EMF+ records and EMF records. 
			/// Either type of record can be used to render the image, only EMF+ records, or only EMF records.
			/// When <see cref="EmfRenderSetting.EmfPlusPrefer" />
			///  is set, then EMF+ records will be parsed while rendering to pdf, otherwise only EMF records will be parsed.
			/// Default value is <see cref="EmfRenderSetting.EmfOnly" />
			/// .
			/// </remarks>
			 virtual Aspose::Cells::EmfRenderSetting GetEmfRenderSetting()=0;
			/// <summary>
			/// Setting for rendering Emf metafile.
			/// </summary>
			/// <remarks>
			/// EMF metafiles identified as "EMF+ Dual" can contain both EMF+ records and EMF records. 
			/// Either type of record can be used to render the image, only EMF+ records, or only EMF records.
			/// When <see cref="EmfRenderSetting.EmfPlusPrefer" />
			///  is set, then EMF+ records will be parsed while rendering to pdf, otherwise only EMF records will be parsed.
			/// Default value is <see cref="EmfRenderSetting.EmfOnly" />
			/// .
			/// </remarks>
			 virtual void SetEmfRenderSetting(Aspose::Cells::EmfRenderSetting value)=0;
			/// <summary>
			/// Indicate whether the window's title bar should display the document title.
			/// </summary>
			/// <remarks>
			/// If false, the title bar should instead display the name of the PDF file.
			/// Default value is false.
			/// </remarks>
			 virtual bool GetDisplayDocTitle()=0;
			/// <summary>
			/// Indicate whether the window's title bar should display the document title.
			/// </summary>
			/// <remarks>
			/// If false, the title bar should instead display the name of the PDF file.
			/// Default value is false.
			/// </remarks>
			 virtual void SetDisplayDocTitle(bool value)=0;
			/// <summary>
			/// Gets embedded font encoding in pdf.
			/// </summary>
			/// <remarks>
			/// Default value is <see cref="PdfFontEncoding.Identity" />
			/// </remarks>
			 virtual Aspose::Cells::Rendering::PdfFontEncoding GetFontEncoding()=0;
			/// <summary>
			/// Sets embedded font encoding in pdf.
			/// </summary>
			/// <remarks>
			/// Default value is <see cref="PdfFontEncoding.Identity" />
			/// </remarks>
			 virtual void SetFontEncoding(Aspose::Cells::Rendering::PdfFontEncoding value)=0;

	};
}
}
