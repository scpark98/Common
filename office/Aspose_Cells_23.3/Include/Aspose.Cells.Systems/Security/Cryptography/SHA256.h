#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_SHA256_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_SHA256_H_

#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Security/Cryptography/HashAlgorithm.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
using namespace Aspose::Cells::Systems;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API SHA256 : public HashAlgorithm  //to do
					{
					public:
						static intrusive_ptr<SHA256> Create() {
							throw NotSupportedException("SHA256::Create is not supported!");
						}
						static intrusive_ptr<SHA256> Create(intrusive_ptr<String> hashName){ 
							throw NotSupportedException("SHA256::Create is not supported!");
						}
					protected:
						SHA256(){}


					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif