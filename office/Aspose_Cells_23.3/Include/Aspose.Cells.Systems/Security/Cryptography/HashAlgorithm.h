#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_HASHALGORITHM_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_HASHALGORITHM_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/IO/Stream.h"
#include "Aspose.Cells.Systems/Security/Cryptography/ICryptoTransform.h"
#include "Aspose.Cells.Systems/Security/Cryptography/CryptoConfig.h"
#ifdef _DEBUG
#include "Aspose.Cells.Systems/Console.h"
#endif

using namespace Aspose::Cells::Systems::IO;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API HashAlgorithm : public Object, public ICryptoTransform
					{
					public:
						virtual ~HashAlgorithm();

						intrusive_ptr<BString> ComputeHash(Aspose::Cells::Systems::IO::StreamPtr inputStream);
						intrusive_ptr<BString> ComputeHash(intrusive_ptr<BString> buffer);
						intrusive_ptr<BString> ComputeHash(intrusive_ptr<BString> buffer, Int32 offset, Int32 count);

						virtual void Initialize() = 0;

						virtual Int32 GetInputBlockSize() {
							throw NEW NotSupportedException(NEW String("HashAlgorithm->GetInputBlockSize() is Not Supported!!!"));
						}
						virtual Int32 GetOutputBlockSize() {
							throw NEW NotSupportedException(NEW String("HashAlgorithm->GetOutputBlockSize() is Not Supported!!!"));
						}
						virtual bool GetCanTransformMultipleBlocks() {
							throw NEW NotSupportedException(NEW String("HashAlgorithm->GetCanTransformMultipleBlocks() is Not Supported!!!"));
						}
						virtual bool GetCanReuseTransform() {
							throw NEW NotSupportedException(NEW String("HashAlgorithm->GetCanReuseTransform() is Not Supported!!!"));
						}
						virtual bool Reset() {
							throw NEW NotSupportedException(NEW String("HashAlgorithm->Reset() is Not Supported!!!"));
						}

						void BlockUpdate(intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Byte>> input, Aspose::Cells::Systems::Int32 offset, Aspose::Cells::Systems::Int32 len)
						{
							throw NEW NotSupportedException(NEW String("HashAlgorithm->BlockUpdate() is Not Supported!!!"));
						}
						virtual Int32 TransformBlock(intrusive_ptr<BString> inputBuffer, Int32 inputOffset, Int32 inputCount, intrusive_ptr<BString> outputBuffer, Int32 outputOffset);
						virtual intrusive_ptr<BString> TransformFinalBlock(intrusive_ptr<BString> inputBuffer, Int32 inputOffset, Int32 inputCount);

						virtual Int32 GetHashSize();
						virtual intrusive_ptr<BString> GetHash();

						void Dispose();
						void Clear();

						static intrusive_ptr<Aspose::Cells::Systems::Security::Cryptography::HashAlgorithm> Create(StringPtr hashName);


					protected:
						HashAlgorithm();
						virtual void Dispose(bool disposing);

						virtual void HashCore(intrusive_ptr<BString> array, Int32 ibStart, Int32 cbSize) = 0;
						virtual intrusive_ptr<BString> HashFinal() = 0;

						Int32		_hashSizeValue;
						intrusive_ptr<BString>	_hashValue;
						Int32		_state;

					private:
						bool		m_bDisposed;


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
#endif	// _SYSTEM_SECURITY_CRYPTOGRAPHY_HASHALGORITHM_H_