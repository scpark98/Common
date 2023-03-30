#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Array1D.h"
#include "Aspose.Cells.Systems/Byte.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/DateTime.h"
#include "Aspose.Cells.Systems/Security/Cryptography/X509Certificates/X509Certificate2.h"
#include "Aspose.Cells.Systems/Guid.h"
#include "XAdESType.h"

namespace Aspose {
	namespace Cells {
		namespace DigitalSignatures {
			enum XAdESType;
		}
	}
}
namespace Aspose{
namespace Cells{
namespace DigitalSignatures{
			/// <summary>
			/// Signature in file.
			/// </summary>
	class ASPOSE_CELLS_API IDigitalSignature : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Certificate object that was used to sign the document.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Security::Cryptography::X509Certificates::X509Certificate2> 		GetICertificate()=0;
			/// <summary>
			/// Certificate object that was used to sign the document.
			/// </summary>
			 virtual void 		SetICertificate(intrusive_ptr<Aspose::Cells::Systems::Security::Cryptography::X509Certificates::X509Certificate2> value)=0;
			/// <summary>
			/// The purpose to signature.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetComments()=0;
			/// <summary>
			/// The purpose to signature.
			/// </summary>
			 virtual void SetComments(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// The time when the document was signed.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::DateTime> GetSignTime()=0;
			/// <summary>
			/// The time when the document was signed.
			/// </summary>
			 virtual void SetSignTime(intrusive_ptr<Aspose::Cells::Systems::DateTime> value)=0;
			/// <summary>
			/// Specifies a GUID which can be cross-referenced with the GUID of the signature line stored in the document content.
			/// Default value is Empty (all zeroes) Guid.
			/// </summary>

			 virtual intrusive_ptr<Aspose::Cells::Systems::Guid> GetId()=0;
			/// <summary>
			/// Specifies a GUID which can be cross-referenced with the GUID of the signature line stored in the document content.
			/// Default value is Empty (all zeroes) Guid.
			/// </summary>

			 virtual void SetId(intrusive_ptr<Aspose::Cells::Systems::Guid> value)=0;
			/// <summary>
			/// Specifies the text of actual signature in the digital signature.
			/// Default value ise Empty.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetText()=0;
			/// <summary>
			/// Specifies the text of actual signature in the digital signature.
			/// Default value ise Empty.
			/// </summary>
			 virtual void SetText(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Specifies an image for the digital signature.
			/// Default value is null.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Byte>> GetImage()=0;
			/// <summary>
			/// Specifies an image for the digital signature.
			/// Default value is null.
			/// </summary>
			 virtual void SetImage(intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Byte>> value)=0;
			/// <summary>
			///  Specifies the class ID of the signature provider.
			///  Default value is Empty (all zeroes) Guid.
			///  </summary>
			/// <remarks>
			///  The cryptographic service provider (CSP) is an independent software module that actually performs cryptography algorithms for authentication, encoding, and encryption.
			///  Microsoft Office reserves the value of {00000000-0000-0000-0000-000000000000} for its default signature provider,
			///  and {000CD6A4-0000-0000-C000-000000000046} for its East Asian signature provider.
			///  
			/// The GUID of the additionally installed provider should be obtained from the documentation shipped with the provider.
			///  </remarks>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Guid> GetProviderId()=0;
			/// <summary>
			///  Specifies the class ID of the signature provider.
			///  Default value is Empty (all zeroes) Guid.
			///  </summary>
			/// <remarks>
			///  The cryptographic service provider (CSP) is an independent software module that actually performs cryptography algorithms for authentication, encoding, and encryption.
			///  Microsoft Office reserves the value of {00000000-0000-0000-0000-000000000000} for its default signature provider,
			///  and {000CD6A4-0000-0000-C000-000000000046} for its East Asian signature provider.
			///  
			/// The GUID of the additionally installed provider should be obtained from the documentation shipped with the provider.
			///  </remarks>
			 virtual void SetProviderId(intrusive_ptr<Aspose::Cells::Systems::Guid> value)=0;
			/// <summary>
			///  If this digital signature is valid and the document has not been tampered with,
			///  this value will be true.
			/// </summary>
			 virtual bool IsValid()=0;
			/// <summary>
			/// XAdES type.
			/// Default value is None(XAdES is off).
			/// </summary>
			 virtual Aspose::Cells::DigitalSignatures::XAdESType GetXAdESType()=0;
			/// <summary>
			/// XAdES type.
			/// Default value is None(XAdES is off).
			/// </summary>
			 virtual void SetXAdESType(Aspose::Cells::DigitalSignatures::XAdESType value)=0;

	};
}
}
}
