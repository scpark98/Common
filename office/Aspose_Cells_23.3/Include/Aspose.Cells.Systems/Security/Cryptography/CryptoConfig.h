#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_CRYPTOCONFIG_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_CRYPTOCONFIG_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Array1D.h"
//#include "Aspose.Cells.Systems/Security/Cryptography/HashAlgorithm.h"
#ifdef _DEBUG
#include "Aspose.Cells.Systems/Console.h"
#endif

//using namespace Aspose::Cells::Systems::Security::Cryptography;
namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API CryptoConfig : public Object //to do
					{
					public:
						CryptoConfig(){}
						static intrusive_ptr<Object> CreateFromName(StringPtr name);
						static intrusive_ptr<Array1D<Byte>> EncodeOID(StringPtr str);
						static StringPtr MapNameToOID(StringPtr name);

					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif	// _SYSTEM_SECURITY_CRYPTOGRAPHY_MD5_H_