#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_RSACRYPTOSERVICEPROVIDER_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_RSACRYPTOSERVICEPROVIDER_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Array1D.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Security/Cryptography/RSAManaged.h"
#include "Aspose.Cells.Systems/Security/Cryptography/HashAlgorithm.h"
#include "Aspose.Cells.Systems/Security/Cryptography/CRSA.h"
#include "License/Pkcs1.h"
#include "License/Rsa.h"
namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API RSACryptoServiceProvider : public CRSA  //to do
					{
					public:
						//Add this 2 member to keep the same to .Net
						intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Byte>> mModulus;
						intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Byte>> mExponent;
						void Init_Vars();
						intrusive_ptr<Aspose::Crypto::Rsa> rsa;
						RSACryptoServiceProvider() {}
						RSACryptoServiceProvider(intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Byte>> keyModulusBytes, intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Byte>>  keyExponentBytes);
						void ImportParameters(intrusive_ptr<RSAParameters> parameters);
						bool VerifyData(intrusive_ptr<Array1D<Byte>> buffer, ObjectPtr halg, intrusive_ptr<Array1D<Byte>> signature);
						intrusive_ptr<HashAlgorithm> GetHash(ObjectPtr halg);
						intrusive_ptr<HashAlgorithm> GetHashFromString(StringPtr name);
						StringPtr GetHashNameFromOID(StringPtr oid);
						void Dispose() {}
					protected:


					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif	// _SYSTEM_SECURITY_CRYPTOGRAPHY_MD5_H_
