#ifndef _SYSTEM_DRAWING_IMAGING_PROPERTYITEM_H_
#define _SYSTEM_DRAWING_IMAGING_PROPERTYITEM_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Array1D.h"

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Drawing {
				namespace Imaging {
					class ASPOSE_CELLS_API PropertyItem : public Object
					{

					private:
						Int32 id;
						Int32 len;
						short type;
						intrusive_ptr<Array1D<Byte>>  value;

						//constructor
						PropertyItem()
						{
							//Nothing to be done here
						}
						// properties
					public:
						int GetId() {
							return id;
						}
						void SetId(Int32 value) {
							id = value;
						}
						Int32 GetLen() {
							return len;
						}
						void SetLen(Int32 value) {
							len = value;
						}
						short GetType() {
							return type;
						}
						void SetType(short value) {
							type = value;
						}
						intrusive_ptr<Array1D<Byte>> GetValue() {
							return value;
						}
						void SetValue(intrusive_ptr<Array1D<Byte>> value) {
							value = value;
						}
					};
				}
			}
		}
	}
}
#endif
