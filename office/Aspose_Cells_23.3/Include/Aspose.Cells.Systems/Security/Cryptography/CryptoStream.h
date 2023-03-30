#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_CRYPTOSTREAM_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_CRYPTOSTREAM_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/IO/Stream.h"
#include "Aspose.Cells.Systems/Security/Cryptography/CryptoStreamMode.h"
#include "Aspose.Cells.Systems/Security/Cryptography/ICryptoTransform.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
#ifdef _DEBUG
#include "Aspose.Cells.Systems/Console.h"
#endif
using namespace Aspose::Cells::Systems;
using namespace Aspose::Cells::Systems::IO;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API CryptoStream : public Stream  //to do
					{
					public:
						CryptoStream(Aspose::Cells::Systems::IO::StreamPtr stream, intrusive_ptr<ICryptoTransform> transform, CryptoStreamMode mode)
						{}//to do ...
						bool CanRead()
						{
							throw NotSupportedException("CryptoStream::CanRead is not supported!");
						}
						bool CanWrite()
						{
							throw NotSupportedException("CryptoStream::CanWrite is not supported!");  
						}
						bool CanSeek()
						{
							throw NotSupportedException("CryptoStream::CanSeek is not supported!");  
						}
						int64_t GetLength()
						{
							throw NotSupportedException("CryptoStream::GetLength is not supported!"); 
						}
						void SetLength(int64_t length)
						{
							throw NotSupportedException("CryptoStream::SetLength is not supported!");  
						}
						int64_t GetPosition()
						{
							throw NotSupportedException("CryptoStream::GetPosition is not supported!");
						}
						void SetPosition(int64_t position)
						{
							throw NotSupportedException("CryptoStream::SetPosition is not supported!");  
						}
						int32_t Read(intrusive_ptr<BString> buffer, int32_t offset, int32_t count)
						{
							throw NotSupportedException("CryptoStream::Read is not supported!"); 
						}
						void Write(intrusive_ptr<BString> buffer, int32_t offset, int32_t count)
						{
							throw NotSupportedException("CryptoStream::Write is not supported!");  
						}
						int32_t ReadByte()
						{
							throw NotSupportedException("CryptoStream::ReadByte is not supported!"); 
						}
						void WriteByte(uint8_t value)
						{
							throw NotSupportedException("CryptoStream::WriteByte is not supported!");
						}
						int64_t Seek(int64_t offset, SeekOrigin origin)
						{
							throw NotSupportedException("CryptoStream::Seek is not supported!"); 
						}
						void Flush()
						{
							throw NotSupportedException("CryptoStream::Flush is not supported!");		
						}
					protected:
						void Dispose(bool disposing){}
					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif	// _SYSTEM_SECURITY_CRYPTOGRAPHY_MD5_H_