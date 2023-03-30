#pragma once
#include "Aspose.Cells.Systems/Object.h"

namespace Aspose{
namespace Cells{
			/// <summary>
			/// Represents an outline on a worksheet.
			/// </summary>
	class ASPOSE_CELLS_API IOutline : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Indicates if the summary row will be positioned below the detail rows in the outline.
			/// </summary>
			 virtual bool GetSummaryRowBelow()=0;
			/// <summary>
			/// Indicates if the summary row will be positioned below the detail rows in the outline.
			/// </summary>
			 virtual void SetSummaryRowBelow(bool value)=0;
			/// <summary>
			///  Indicates if the summary column will be positioned to the right of the detail columns in the outline.
			/// </summary>
			 virtual bool GetSummaryColumnRight()=0;
			/// <summary>
			///  Indicates if the summary column will be positioned to the right of the detail columns in the outline.
			/// </summary>
			 virtual void SetSummaryColumnRight(bool value)=0;

	};
}
}
