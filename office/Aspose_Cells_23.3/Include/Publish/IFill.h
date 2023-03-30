#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Int32.h"

namespace Aspose{
namespace Cells{
namespace Drawing{
			/// <summary>
			/// Represents the fill format of the shape.
			/// </summary>
	class ASPOSE_CELLS_API IFill : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// /
			/// </summary>
			/// <param name="obj" />
			/// 
			/// <returns/>
			/// 
			  virtual bool Equals(intrusive_ptr<Aspose::Cells::Systems::Object> obj)=0;
			/// <summary>
			/// Gets the hash code.
			/// </summary>
			/// <returns/>
			/// 
			  virtual Aspose::Cells::Systems::Int32 GetHashCode()=0;

	};
}
}
}
