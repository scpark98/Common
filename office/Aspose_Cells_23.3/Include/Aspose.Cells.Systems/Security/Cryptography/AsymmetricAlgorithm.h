#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_ASYMMETRICALGORITHM_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_ASYMMETRICALGORITHM_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/IDisposable.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"

using namespace Aspose::Cells::Systems;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API AsymmetricAlgorithm : public Object, public IDisposable  //to do
					{
					public:
						virtual StringPtr ToXmlString(bool includePrivateParameters) { 
							throw NotSupportedException("AsymmetricAlgorithm::ToXmlString is not supported!");
						}
					protected:
						virtual void Dispose(bool disposing) {}
					public:
						virtual void add_ref() {
							++ref_count_;
						}
						virtual int release_ref() {
							return --ref_count_;
						}

					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif