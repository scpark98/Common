#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_RSAPKCS1SIGNATUREFORMATTER_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_RSAPKCS1SIGNATUREFORMATTER_H_

#include "Aspose.Cells.Systems/Security/Cryptography/AsymmetricSignatureFormatter.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API RSAPKCS1SignatureFormatter : public AsymmetricSignatureFormatter  //to do
					{
					public:
						RSAPKCS1SignatureFormatter(){}
						RSAPKCS1SignatureFormatter(intrusive_ptr<AsymmetricAlgorithm> key){}

						intrusive_ptr<Array1D<Byte>> CreateSignature(intrusive_ptr<Array1D<Byte>> rgbHash)
						{ 
							throw NotSupportedException("RSAPKCS1SignatureFormatter::CreateSignature is not supported!");
						}
						void SetHashAlgorithm(StringPtr strName)
						{
							throw NotSupportedException("RSAPKCS1SignatureFormatter::SetHashAlgorithm is not supported!");
						}
						void SetKey(intrusive_ptr<AsymmetricAlgorithm> key)
						{
							throw NotSupportedException("RSAPKCS1SignatureFormatter::SetKey is not supported!");
						}
					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif	// _SYSTEM_SECURITY_CRYPTOGRAPHY_MD5_H_