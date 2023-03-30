 
#ifndef _SYSTEM_DRAWING_DRAWING2D_BLEND_H_
#define _SYSTEM_DRAWING_DRAWING2D_BLEND_H_

#include "Aspose.Cells.Systems/Drawing/Color.h"
#include "Aspose.Cells.Systems/Array1D.h"
namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Drawing {
				namespace Drawing2D {
					class  Blend : public Object
					{
					
					public: //private before
						intrusive_ptr<Aspose::Cells::Systems::Array1D<float>> Positions;
						intrusive_ptr<Aspose::Cells::Systems::Array1D<float>> Factors;
					public:
						Blend()
						{
							Positions = NEW Aspose::Cells::Systems::Array1D<float>(1);
							Factors =   NEW Aspose::Cells::Systems::Array1D<float>(1);
						}
						Blend(int count)
						{
							Positions = NEW Aspose::Cells::Systems::Array1D<float>(count);
							Factors   = NEW Aspose::Cells::Systems::Array1D<float>(count);
						}
						 
						intrusive_ptr<Aspose::Cells::Systems::Array1D<float>> GetFactors()
						{
							return Factors;
						}		 
						void SetFactors(intrusive_ptr<Aspose::Cells::Systems::Array1D<float>> value)
						{
							Factors = value;
						}
						intrusive_ptr<Aspose::Cells::Systems::Array1D<float>> GetPositions()
						{
							return Positions;
						}
						void SetPositions(intrusive_ptr<Aspose::Cells::Systems::Array1D<float>> value)
						{
							Positions = value;
						}
 
					};
				}
			}
		}
	}
}
#endif
