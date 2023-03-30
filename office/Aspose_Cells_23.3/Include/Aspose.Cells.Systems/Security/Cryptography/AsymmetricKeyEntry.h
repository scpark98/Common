#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_ASY_ENTRY_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_ASY_ENTRY_H_

#include "Aspose.Cells.Systems/Security/Cryptography/Pkcs12Entry.h"
#include "Aspose.Cells.Systems/Security/Cryptography/AsymmetricKeyParameter.h"
namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API AsymmetricKeyEntry : public  Pkcs12Entry //to do
					{
					public:
						intrusive_ptr<Aspose::Cells::Systems::Security::Cryptography::AsymmetricKeyParameter> Key;
					protected:

					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif	// _SYSTEM_SECURITY_CRYPTOGRAPHY_MD5_H_