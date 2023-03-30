#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_SHA512_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_SHA512_H_

#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Security/Cryptography/HashAlgorithm.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
using namespace Aspose::Cells::Systems;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API SHA512 : public HashAlgorithm  //to do
					{
					public:
						static intrusive_ptr<SHA512> Create(){
							throw NotSupportedException("SHA512::Create is not supported!");
						}
						static intrusive_ptr<SHA512> Create(intrusive_ptr<String> hashName){ 
							throw NotSupportedException("SHA512::Create is not supported!");
						}
					protected:
						SHA512(){}


					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif