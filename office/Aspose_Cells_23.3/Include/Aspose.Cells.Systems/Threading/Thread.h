#ifndef _SYSTEM_THREADING_THREAD_H_
#define _SYSTEM_THREADING_THREAD_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
#include "Aspose.Cells.Systems/Console.h"
namespace Aspose {
	namespace Cells {
		namespace Systems
		{
			namespace Threading
			{
				class ASPOSE_CELLS_API Thread : public Object
				{
				private:

				protected:

				public:
					static void Sleep(int millisecondsTimeout); 
					//static void Sleep(); 
					static void BeginCriticalRegion() {};
					static void EndCriticalRegion() {}; 
					static void Start(){};
					bool IsAlive()  { return true; };
				};
			}
		}
	}
}
#endif//_SYSTEM_THREADING_THREAD_H_