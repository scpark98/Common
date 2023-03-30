#pragma once
#include "Aspose.Cells.Systems/Object.h"

namespace Aspose {
	namespace Cells {
		class IWarningInfo;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Callback interface of warning.
			/// </summary>
	class ASPOSE_CELLS_API IIWarningCallback : public Aspose::Cells::Systems::Object
	{
		public:

			 virtual void 		Warning(intrusive_ptr<Aspose::Cells::IWarningInfo> warningInfo)=0;

	};
}
}
