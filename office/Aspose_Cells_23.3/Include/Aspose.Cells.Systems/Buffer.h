#ifndef _SYSTEM_BUFFER_H_
#define _SYSTEM_BUFFER_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Array1D.h"
#include "Aspose.Cells.Systems/ArgumentOutOfRangeException.h"

namespace Aspose {
	namespace Cells {
		namespace Systems {
			class ASPOSE_CELLS_API Buffer {
			public:
				template <class T>
				static void BlockCopy(intrusive_ptr<Array1D<T>> sourceArray, int sourceArrayIndex, intrusive_ptr<Array1D<T>> destinationArray, int destinationArrayIndex, int length)
				{
					/*for Talos report,xinya.zhu@aspose.com*/
					if (sourceArrayIndex < 0 || destinationArrayIndex < 0 || (sourceArrayIndex + length) > sourceArray->GetLength() || (destinationArrayIndex + length) > destinationArray->GetLength()
						|| length > sourceArray->GetLength() || length > destinationArray->GetLength()
						)
					{
						throw ArgumentOutOfRangeException("index", "ArgumentOutOfRange_Index");
					}
					memmove(destinationArray->ArrayPoint() + destinationArrayIndex, sourceArray->ArrayPoint() + sourceArrayIndex, sizeof(T) * length);
				}
			};
		}
	}
}
#endif//_SYSTEM_INT16HELPER_H_
