#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "WarningType.h"
#include "Aspose.Cells.Systems/String.h"

namespace Aspose {
	namespace Cells {
		enum WarningType;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Warning info
			/// </summary>
	class ASPOSE_CELLS_API IWarningInfo : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Get warning type.
			/// </summary>
			 virtual Aspose::Cells::WarningType GetWarningType()=0;
			/// <summary>
			/// Get description of warning info.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetDescription()=0;
			/// <summary>
			/// The error object.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Object> GetErrorObject()=0;
			/// <summary>
			/// Gets and sets the corrected object.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Object> GetCorrectedObject()=0;
			/// <summary>
			/// Gets and sets the corrected object.
			/// </summary>
			 virtual void SetCorrectedObject(intrusive_ptr<Aspose::Cells::Systems::Object> value)=0;

	};
}
}
