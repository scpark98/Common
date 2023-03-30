#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "SaveFormat.h"
#include "OoxmlCompressionType.h"
#include "ISaveOptions.h"

namespace Aspose {
	namespace Cells {
		enum SaveFormat;
		enum OoxmlCompressionType;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Represents the options of saving office open xml file.
			/// </summary>
	class ASPOSE_CELLS_API IOoxmlSaveOptions : virtual public Aspose::Cells::ISaveOptions
	{
		public:
			/// <summary>
			/// Indicates if export cell name to Excel2007 .xlsx (.xlsm, .xltx, .xltm) file. 
			/// If the output file may be accessed by SQL Server DTS, this value must be true.
			/// Setting the value to false will highly increase the performance and reduce the file size when creating large file.
			/// Default value is true. 
			/// </summary>
			 virtual bool GetExportCellName()=0;
			/// <summary>
			/// Indicates if export cell name to Excel2007 .xlsx (.xlsm, .xltx, .xltm) file. 
			/// If the output file may be accessed by SQL Server DTS, this value must be true.
			/// Setting the value to false will highly increase the performance and reduce the file size when creating large file.
			/// Default value is true. 
			/// </summary>
			 virtual void SetExportCellName(bool value)=0;
			/// <summary>
			/// Indicates whether update scaling factor before saving the file 
			/// if the PageSetup.FitToPagesWide and PageSetup.FitToPagesTall properties control how the worksheet is scaled.
			/// </summary>
			/// <remarks>
			/// The default value is false for performance.
			/// 
			/// </remarks>
			 virtual bool GetUpdateZoom()=0;
			/// <summary>
			/// Indicates whether update scaling factor before saving the file 
			/// if the PageSetup.FitToPagesWide and PageSetup.FitToPagesTall properties control how the worksheet is scaled.
			/// </summary>
			/// <remarks>
			/// The default value is false for performance.
			/// 
			/// </remarks>
			 virtual void SetUpdateZoom(bool value)=0;
			/// <summary>
			/// Always use ZIP64 extensions when writing zip archives, even when unnecessary.
			/// </summary>
			 virtual bool GetEnableZip64()=0;
			/// <summary>
			/// Always use ZIP64 extensions when writing zip archives, even when unnecessary.
			/// </summary>
			 virtual void SetEnableZip64(bool value)=0;
			/// <summary>
			/// Indicates whether embedding Ooxml files of OleObject as ole object.
			/// </summary>
			/// <remarks>
			/// Only for OleObject.
			/// </remarks>
			 virtual bool GetEmbedOoxmlAsOleObject()=0;
			/// <summary>
			/// Indicates whether embedding Ooxml files of OleObject as ole object.
			/// </summary>
			/// <remarks>
			/// Only for OleObject.
			/// </remarks>
			 virtual void SetEmbedOoxmlAsOleObject(bool value)=0;
			/// <summary>
			/// Gets and sets the compression type for ooxml file.
			/// </summary>
			/// <remarks>The default value is OoxmlCompressionType.Level2.</remarks>
			 virtual Aspose::Cells::OoxmlCompressionType GetCompressionType()=0;
			/// <summary>
			/// Gets and sets the compression type for ooxml file.
			/// </summary>
			/// <remarks>The default value is OoxmlCompressionType.Level2.</remarks>
			 virtual void SetCompressionType(Aspose::Cells::OoxmlCompressionType value)=0;

	};
}
}
