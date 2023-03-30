#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_PKCS_12_BUILD_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_PKCS_12_BUILD_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Security/Cryptography/Pkcs12Store.h"
 
using namespace Aspose::Cells::Systems;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {
					class ASPOSE_CELLS_API Pkcs12StoreBuilder : public Object //to do
					{
					public:

						intrusive_ptr<Aspose::Cells::Systems::Security::Cryptography::Pkcs12Store>  Build()
						{
							throw  Aspose::Cells::Systems::NotSupportedException(new String("Pkcs12StoreBuilder Build NotSupportedException!"));
						}

					};
				}
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif