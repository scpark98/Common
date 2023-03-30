#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_RIJNDAEL_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_RIJNDAEL_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Security/Cryptography/SymmetricAlgorithm.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
using namespace Aspose::Cells::Systems;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API Rijndael : public SymmetricAlgorithm  //to do
					{
					public:
						static intrusive_ptr<Rijndael> Create()
						{
							throw NotSupportedException("Rijndael::Create is not supported!");
						}
						static intrusive_ptr<Rijndael> Create(StringPtr algName)
						{
							throw NotSupportedException("Rijndael::Create is not supported!");
						}

					protected:
						Rijndael(){}

					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif