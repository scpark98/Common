#pragma once
#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_RSA_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_RSA_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/IDisposable.h"
#include "Aspose.Cells.Systems/Security/Cryptography/AsymmetricAlgorithm.h"

using namespace Aspose::Cells::Systems;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API RSAs : public AsymmetricAlgorithm 
					{
					public:
						virtual StringPtr ToXmlString(bool includePrivateParameters) = 0;
					protected:
						virtual void Dispose(bool disposing) = 0;

					public:

					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif