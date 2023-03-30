#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_X509CERTIFICATES_X509CERTIFICATE_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_X509CERTIFICATES_X509CERTIFICATE_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Security/Cryptography/AsymmetricAlgorithm.h"
#include "Aspose.Cells.Systems/Security/Cryptography/AsymmetricKeyParameter.h"
#ifdef _DEBUG
#include "Aspose.Cells.Systems/Console.h"
#endif
#include "Aspose.Cells.Systems/NotSupportedException.h"

using namespace Aspose::Cells::Systems;
using namespace Aspose::Cells::Systems::Security;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {
					namespace X509Certificates {

						class ASPOSE_CELLS_API X509Certificate : public Object //to do
						{
						public:
							intrusive_ptr<Cryptography::AsymmetricAlgorithm> GetKey() {
								throw  Aspose::Cells::Systems::NotSupportedException(new String("X509Certificate GetKey NotSupportedException!"));
							}

							intrusive_ptr<Cryptography::AsymmetricKeyParameter> GetPublicKey()
							{
								throw  Aspose::Cells::Systems::NotSupportedException(new String("X509Certificate GetPublicKey NotSupportedException!") );
							}

							intrusive_ptr<Aspose::Cells::Systems::Array1D< Aspose::Cells::Systems::Byte>> GetEncoded()
							{
								throw  Aspose::Cells::Systems::NotSupportedException(new String("X509Certificate GetEncoded NotSupportedException!"));
							}

							//actually the folowing are not String...
							StringPtr IssuerDN;
							StringPtr SerialNumber;


							

						};

					} //namespace X509Certificates
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif