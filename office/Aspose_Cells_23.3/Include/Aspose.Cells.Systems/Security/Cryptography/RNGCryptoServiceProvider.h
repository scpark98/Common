#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_RNGCRYPTOSERVICEPROVIDER_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_RNGCRYPTOSERVICEPROVIDER_H_

#include "Aspose.Cells.Systems/Security/Cryptography/RandomNumberGenerator.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API RNGCryptoServiceProvider : public RandomNumberGenerator //to do
					{
					public:
						RNGCryptoServiceProvider(){}

						void GetBytes(intrusive_ptr<Array1D<Byte>> data) {
							throw NotSupportedException("RNGCryptoServiceProvider::GetBytes is not supported!");
						}
						void GetNonZeroBytes(intrusive_ptr<Array1D<Byte>> data) {
							throw NotSupportedException("RNGCryptoServiceProvider::GetNonZeroBytes is not supported!");
						}
					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif	// _SYSTEM_SECURITY_CRYPTOGRAPHY_MD5_H_