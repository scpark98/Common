#pragma once
#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_RSAMANAGED_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_RSAMANAGED_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/IDisposable.h"
#include "Aspose.Cells.Systems/Security/Cryptography/RSAs.h"
#include "Aspose.Cells.Systems/Security/Cryptography/RSAParameters.h"
#include "License/BigInteger.h"
using namespace Aspose::Cells::Systems;
using namespace Aspose::Crypto;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API RSAManaged : public RSAs
					{
					public:
						bool m_disposed = false;
						bool keypairGenerated = false;
						bool isCRTpossible = false;
						intrusive_ptr<BigInteger> d;
						intrusive_ptr<BigInteger> p;
						intrusive_ptr<BigInteger> q;
						intrusive_ptr<BigInteger> dp;
						intrusive_ptr<BigInteger> dq;
						intrusive_ptr<BigInteger> qInv;
						intrusive_ptr<BigInteger> n;		// modulus
						intrusive_ptr<BigInteger> e;
					public:
						virtual void Dispose(bool disposing) = 0;
						void ImportParameters(intrusive_ptr<RSAParameters> parameters);
					public:
 

					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif