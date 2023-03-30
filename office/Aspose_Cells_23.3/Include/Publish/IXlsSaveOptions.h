#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "SaveFormat.h"
#include "ISaveOptions.h"

namespace Aspose {
	namespace Cells {
		enum SaveFormat;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Represents the save options for the Excel 97-2003 file format: xls and xlt.
			/// </summary>
	class ASPOSE_CELLS_API IXlsSaveOptions : virtual public Aspose::Cells::ISaveOptions
	{
		public:
			/// <summary>
			/// Indicates whether saving a template file.
			/// </summary>
			 virtual bool IsTemplate()=0;
			/// <summary>
			/// Indicates whether saving a template file.
			/// </summary>
			 virtual void SetTemplate(bool value)=0;
			/// <summary>
			/// Indicates whether matching font color because there are 56 colors in the standard color palette.
			/// </summary>
			 virtual bool GetMatchColor()=0;
			/// <summary>
			/// Indicates whether matching font color because there are 56 colors in the standard color palette.
			/// </summary>
			 virtual void SetMatchColor(bool value)=0;

	};
}
}
