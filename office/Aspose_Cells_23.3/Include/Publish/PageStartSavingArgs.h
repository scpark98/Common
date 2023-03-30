#pragma once
#include "Aspose.Cells.Systems/Int32.h"
#include "Aspose.Cells.Systems/Object.h"
#include "PageSavingArgs.h"
#define STATIC_PAGESTARTSAVINGARGS() 


namespace Aspose {
namespace Cells {
namespace Rendering {
#ifdef WIN32

	class ASPOSE_CELLS_API PageStartSavingArgs : public Aspose::Cells::Rendering::PageSavingArgs
#else
	class PageStartSavingArgs : public Aspose::Cells::Rendering::PageSavingArgs
#endif


	{
	public:

			 PageStartSavingArgs(Aspose::Cells::Systems::Int32 pageIndex , Aspose::Cells::Systems::Int32 pageCount);
	private:

			void Init_Vars();
			bool mIsToOutput;
	public:

			bool IsToOutput();
			void SetToOutput(bool value);
			 PageStartSavingArgs();
		public:
			virtual ~PageStartSavingArgs();
	};

}

}

}
