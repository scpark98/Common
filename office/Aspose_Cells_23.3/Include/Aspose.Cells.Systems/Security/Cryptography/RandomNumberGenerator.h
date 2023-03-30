#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_RANDOMNUMBERGENERATOR_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_RANDOMNUMBERGENERATOR_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Array1D.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API RandomNumberGenerator : public Object  //to do
					{
					public:
						static intrusive_ptr<RandomNumberGenerator> Create()
						{
							throw NotSupportedException("RandomNumberGenerator::Create is not supported!");
						}
						static intrusive_ptr<RandomNumberGenerator> Create(StringPtr rngName)
						{ 
							throw NotSupportedException("RandomNumberGenerator::Create(String) is not supported!");
						}
						virtual void GetBytes(intrusive_ptr<Array1D<Byte>> data) = 0;
						virtual void GetNonZeroBytes(intrusive_ptr<Array1D<Byte>> data) = 0;
					protected:
						RandomNumberGenerator(){}

					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif	// _SYSTEM_SECURITY_CRYPTOGRAPHY_MD5_H_