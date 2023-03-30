#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_PKCS_12_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_PKCS_12_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/IO/MemoryStream.h"
#include "Aspose.Cells.Systems/Array1D.h"
#include "Aspose.Cells.Systems/Security/Cryptography/AsymmetricKeyEntry.h"
#include "Aspose.Cells.Systems/Security/Cryptography/X509Certificates/X509CertificateEntry.h"


using namespace Aspose::Cells::Systems;
using namespace Aspose::Cells::Systems::Security;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {
					class ASPOSE_CELLS_API Pkcs12Store : public Object //to do
					{
					public:

						void Load(intrusive_ptr <Aspose::Cells::Systems::IO::MemoryStream>  mem, intrusive_ptr<Array1D<Char>> data)
						{
							throw  Aspose::Cells::Systems::NotSupportedException(new String("Pkcs12Store Load NotSupportedException!"));
						}

						intrusive_ptr<Aspose::Cells::Systems::Collections::ICollection> Aliases;

						bool IsKeyEntry(StringPtr alias)
						{
							throw  Aspose::Cells::Systems::NotSupportedException(new String("Pkcs12Store IsKeyEntry NotSupportedException!"));
						}

						intrusive_ptr <Aspose::Cells::Systems::Security::Cryptography::AsymmetricKeyEntry> GetKey(StringPtr)
						{
							throw  Aspose::Cells::Systems::NotSupportedException(new String("Pkcs12Store GetKey NotSupportedException!"));
						}


						intrusive_ptr <Aspose::Cells::Systems::Security::Cryptography::X509Certificates::X509CertificateEntry> GetCertificate(StringPtr)
						{
							throw  Aspose::Cells::Systems::NotSupportedException(new String("Pkcs12Store GetCertificate NotSupportedException!"));
						}
						
					};
				}
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif