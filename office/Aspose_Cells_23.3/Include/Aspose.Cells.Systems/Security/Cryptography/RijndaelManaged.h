#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_RIJNDAELMANAGED_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_RIJNDAELMANAGED_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Array1D.h"
#include "Aspose.Cells.Systems/Security/Cryptography/Rijndael.h"
#include "Aspose.Cells.Systems/Security/Cryptography/ICryptoTransform.h"
#include "Aspose.Cells.Systems/Security/Cryptography/CipherMode.h"
#include "Aspose.Cells.Systems/Security/Cryptography/PaddingMode.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
using namespace Aspose::Cells::Systems;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API RijndaelManaged : public Rijndael  //to do
					{
					public:
						RijndaelManaged(){}

						virtual intrusive_ptr<ICryptoTransform> CreateDecryptor(){ return SymmetricAlgorithm::CreateDecryptor(); }
						virtual intrusive_ptr<ICryptoTransform> CreateDecryptor(intrusive_ptr<Array1D<Byte>> rgbKey, intrusive_ptr<Array1D<Byte>> rgbIV) { 
							throw NotSupportedException("RijndaelManaged::CreateDecryptor is not supported!");
						}
						virtual intrusive_ptr<ICryptoTransform> CreateEncryptor(){ return SymmetricAlgorithm::CreateEncryptor(); }
						virtual intrusive_ptr<ICryptoTransform> CreateEncryptor(intrusive_ptr<Array1D<Byte>> rgbKey, intrusive_ptr<Array1D<Byte>> rgbIV) { 
							throw NotSupportedException("RijndaelManaged::CreateEncryptor is not supported!");
						}
						void GenerateIV(){
							throw NotSupportedException("RijndaelManaged::GenerateIV is not supported!");
						}
						void GenerateKey(){
							throw NotSupportedException("RijndaelManaged::GenerateKey is not supported!");
						}

						int GetBlockSize(){
							throw NotSupportedException("RijndaelManaged::GetBlockSize is not supported!");
						}
						void SetBlockSize(int value){}
						int GetKeySize(){
							throw NotSupportedException("RijndaelManaged::GetKeySize is not supported!");
						}
						void SetKeySize(int value)
						{
							throw NotSupportedException("RijndaelManaged::SetKeySize is not supported!");
						}
						CipherMode GetMode(){ return CipherMode_CBC; }
						void SetMode(CipherMode value){
							throw NotSupportedException("RijndaelManaged::SetMode is not supported!");
						}
						PaddingMode GetPadding(){ return PaddingMode_None; }
						void SetPadding(PaddingMode value) {
							throw NotSupportedException("RijndaelManaged::GetPadding is not supported!");
						}
						intrusive_ptr<Array1D<Byte>> GetIV(){
							throw NotSupportedException("RijndaelManaged::GetIV is not supported!");
						}
						void SetIV(intrusive_ptr<Array1D<Byte>> value){}
						intrusive_ptr<Array1D<Byte>> GetKey(){
							throw NotSupportedException("RijndaelManaged::GetKey is not supported!");
						}
						void SetKey(intrusive_ptr<Array1D<Byte>> value){
							throw NotSupportedException("RijndaelManaged::SetKey is not supported!");
						}
					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif