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
			/// Represents the options for saving xlsb file.
			/// </summary>
	class ASPOSE_CELLS_API IXlsbSaveOptions : virtual public Aspose::Cells::ISaveOptions
	{
		public:
			/// <summary>
			/// Gets the compression type for ooxml file.
			/// </summary>
			/// <remarks>The default value is OoxmlCompressionType.Level6.</remarks>
			 virtual Aspose::Cells::OoxmlCompressionType GetCompressionType()=0;
			/// <summary>
			/// Sets the compression type for ooxml file.
			/// </summary>
			/// <remarks>The default value is OoxmlCompressionType.Level6.</remarks>
			 virtual void SetCompressionType(Aspose::Cells::OoxmlCompressionType value)=0;
			/// <summary>
			/// Indicates whether exporting all column indexes for cells.
			/// </summary>
			/// <remarks>
			/// The default value is true.
			/// </remarks>
			 virtual bool GetExportAllColumnIndexes()=0;
			/// <summary>
			/// Indicates whether exporting all column indexes for cells.
			/// </summary>
			/// <remarks>
			/// The default value is true.
			/// </remarks>
			 virtual void SetExportAllColumnIndexes(bool value)=0;

	};
}
}
