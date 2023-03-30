#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_SYMMETRICALGORITHM_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_SYMMETRICALGORITHM_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/IDisposable.h"
#include "Aspose.Cells.Systems/Array1D.h"


#include "Aspose.Cells.Systems/NotSupportedException.h"
#include "Aspose.Cells.Systems/Security/Cryptography/ICryptoTransform.h"
#include "Aspose.Cells.Systems/Security/Cryptography/CipherMode.h"
#include "Aspose.Cells.Systems/Security/Cryptography/PaddingMode.h"

using namespace Aspose::Cells::Systems;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API SymmetricAlgorithm : public Object, public IDisposable  //to do
					{
					public:

						CipherMode  ModeValue;
						PaddingMode PaddingValue;
						int         BlockSizeValue;
						intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Byte>> IVValue;
						void Dispose(){}
					
						virtual intrusive_ptr<ICryptoTransform> CreateDecryptor(){ 
							throw NotSupportedException("SymmetricAlgorithm::CreateDecryptor is not supported!");
							return NULL; 
						}
						virtual intrusive_ptr<ICryptoTransform> CreateDecryptor(intrusive_ptr<Array1D<Byte>> rgbKey, intrusive_ptr<Array1D<Byte>> rgbIV) = 0;
						virtual intrusive_ptr<ICryptoTransform> CreateEncryptor(){ 
							throw NotSupportedException("SymmetricAlgorithm::CreateEncryptor is not supported!");
						}
						virtual intrusive_ptr<ICryptoTransform> CreateEncryptor(intrusive_ptr<Array1D<Byte>> rgbKey, intrusive_ptr<Array1D<Byte>> rgbIV) = 0;
						virtual void SetMode(Aspose::Cells::Systems::Security::Cryptography::CipherMode value)
						{
							if ((value < CipherMode_CBC) || (CipherMode_CFB < value))
								throw new ArgumentException("Cryptography_InvalidCipherMode");

							ModeValue = value;
						}

						virtual void SetPadding(Aspose::Cells::Systems::Security::Cryptography::PaddingMode value)
						{
							if ((value < PaddingMode_None) || (PaddingMode_ISO10126 < value))
								throw new ArgumentException("Cryptography_InvalidPaddingMode");

							PaddingValue = value;
						}
						virtual void SetIV(intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Byte>> value)
						{
							if (value == NULL) throw new ArgumentException("value");
							 
							if (value->Length() != BlockSizeValue / 8)
								throw new ArgumentException("Cryptography_InvalidIVSize");

							IVValue =   value->Clone();
			            }

						void SetKey(intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Byte>> value)
						{
							throw new NotSupportedException("SymmetricAlgorithm UnSupport!");
						}



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