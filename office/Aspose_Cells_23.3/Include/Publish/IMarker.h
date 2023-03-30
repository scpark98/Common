#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "ChartMarkerType.h"
#include "Aspose.Cells.Systems/Int32.h"
#include "Aspose.Cells.Systems/Drawing/Color.h"
#include "FormattingType.h"

namespace Aspose {
	namespace Cells {
		namespace Drawing {
			class ILine;
			class IArea;
		}
	}
}
namespace Aspose {
	namespace Cells {
		namespace Charts {
			enum ChartMarkerType;
			enum FormattingType;
		}
	}
}
namespace Aspose{
namespace Cells{
namespace Charts{
			/// <summary>
			/// Represents the marker in a line chart, scatter chart, or radar chart.
			/// </summary>
	class ASPOSE_CELLS_API IMarker : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Gets the <see cref="ILine" >border</see>
			/// .
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::ILine> 		GetBorderILine()=0;
			/// <summary>
			/// Gets the <see cref="IArea" >area</see>
			/// .
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::IArea> 		GetIArea()=0;
			/// <summary>
			/// Represents the marker style. Applies to line chart, scatter chart, or radar chart. 
			/// </summary>
			 virtual Aspose::Cells::Charts::ChartMarkerType GetMarkerStyle()=0;
			/// <summary>
			/// Represents the marker style. Applies to line chart, scatter chart, or radar chart. 
			/// </summary>
			 virtual void SetMarkerStyle(Aspose::Cells::Charts::ChartMarkerType value)=0;
			/// <summary>
			/// Represents the marker size in unit of points. Applies to line chart, scatter chart, or radar chart. 
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetMarkerSize()=0;
			/// <summary>
			/// Represents the marker size in unit of points. Applies to line chart, scatter chart, or radar chart. 
			/// </summary>
			 virtual void SetMarkerSize(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents the marker size in unit of pixels. Applies to line chart, scatter chart, or radar chart. 
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetMarkerSizePx()=0;
			/// <summary>
			/// Represents the marker size in unit of pixels. Applies to line chart, scatter chart, or radar chart. 
			/// </summary>
			 virtual void SetMarkerSizePx(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents the marker foreground color in a line chart, scatter chart, or radar chart. 
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Drawing::Color> GetForegroundColor()=0;
			/// <summary>
			/// Represents the marker foreground color in a line chart, scatter chart, or radar chart. 
			/// </summary>
			 virtual void SetForegroundColor(intrusive_ptr<Aspose::Cells::Systems::Drawing::Color> value)=0;
			/// <summary>
			/// Gets the marker foreground color set type.
			/// </summary>
			 virtual Aspose::Cells::Charts::FormattingType GetForegroundColorSetType()=0;
			/// <summary>
			/// Sets the marker foreground color set type.
			/// </summary>
			 virtual void SetForegroundColorSetType(Aspose::Cells::Charts::FormattingType value)=0;
			/// <summary>
			/// Represents the marker background color in a line chart, scatter chart, or radar chart. 
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Drawing::Color> GetBackgroundColor()=0;
			/// <summary>
			/// Represents the marker background color in a line chart, scatter chart, or radar chart. 
			/// </summary>
			 virtual void SetBackgroundColor(intrusive_ptr<Aspose::Cells::Systems::Drawing::Color> value)=0;
			/// <summary>
			/// Gets the marker background color set type.
			/// </summary>
			 virtual Aspose::Cells::Charts::FormattingType GetBackgroundColorSetType()=0;
			/// <summary>
			/// Sets the marker background color set type.
			/// </summary>
			 virtual void SetBackgroundColorSetType(Aspose::Cells::Charts::FormattingType value)=0;

	};
}
}
}
