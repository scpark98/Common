#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_SHA1CRYPTOSERVICEPROVIDER_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_SHA1CRYPTOSERVICEPROVIDER_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Array1D.h"
#include "Aspose.Cells.Systems/Security/Cryptography/SHA1.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
using namespace Aspose::Cells::Systems;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API SHA1CryptoServiceProvider : public SHA1  //to do
					{
					public:
						SHA1CryptoServiceProvider() {}
						void Initialize() {}
					protected:
						void Dispose(bool disposing) {}
						void HashCore(intrusive_ptr<Array1D<Byte>> rgb, int ibStart, int cbSize) {
							throw NotSupportedException("SHA1CryptoServiceProvider::HashCore is not supported!");
						}
						intrusive_ptr<Array1D<Byte>> HashFinal() { 
							throw NotSupportedException("SHA1CryptoServiceProvider::HashFinal is not supported!");
						}

					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif