#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_X509CERTIFICATES_PARSER_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_X509CERTIFICATES_PARSER_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Security/Cryptography/AsymmetricAlgorithm.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"

using namespace Aspose::Cells::Systems;
using namespace Aspose::Cells::Systems::Security;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {
					namespace X509Certificates {

						class ASPOSE_CELLS_API X509CertificateParser : public Object //to do
						{
						public:

							intrusive_ptr<Aspose::Cells::Systems::Security::Cryptography::X509Certificates::X509Certificate> 
								ReadCertificate(intrusive_ptr<Aspose::Cells::Systems::Array1D< Aspose::Cells::Systems::Byte>> data)
							{
								throw  Aspose::Cells::Systems::NotSupportedException(new String("X509CertificateParser ReadCertificate NotSupportedException!"));
							}
							

						};

					} //namespace X509Certificates
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif