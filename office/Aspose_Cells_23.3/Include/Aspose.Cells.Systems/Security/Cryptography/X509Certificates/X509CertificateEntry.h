#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_X509CERTIFICATES_ENTRY_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_X509CERTIFICATES_ENTRY_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Security/Cryptography/Pkcs12Entry.h"
 
using namespace Aspose::Cells::Systems;
using namespace Aspose::Cells::Systems::Security;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {
					namespace X509Certificates {

						class ASPOSE_CELLS_API X509CertificateEntry : public Pkcs12Entry //to do
						{
						public:
							intrusive_ptr < Aspose::Cells::Systems::Security::Cryptography::X509Certificates::X509Certificate> Certificate;
						};

					} //namespace X509Certificates
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif