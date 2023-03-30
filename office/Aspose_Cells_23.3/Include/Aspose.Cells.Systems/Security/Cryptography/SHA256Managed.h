#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_SHA256MANAGED_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_SHA256MANAGED_H_

#include "Aspose.Cells.Systems/Security/Cryptography/SHA256.h"
#include "Aspose.Cells.Systems/Array1D.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
using namespace Aspose::Cells::Systems;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API SHA256Managed : public SHA256  //to do
					{
					public:
						SHA256Managed();
						void Initialize();
						void InitializeState();
						long _count; // Number of bytes in the hashed message
						intrusive_ptr<Array1D<UInt32>> _stateSHA256;
						intrusive_ptr<Array1D<UInt32>> _W;
						intrusive_ptr<Array1D<Byte>> _buffer;
						static intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::UInt32>> _K;
						static intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::UInt32>> Static_Array_Init_K();
					protected:
						void HashCore(intrusive_ptr<Array1D<Byte>> rgb, int ibStart, int cbSize);
						void _HashData(intrusive_ptr<Array1D<Byte>>  partIn, int ibStart, int cbSize);
						intrusive_ptr<Array1D<Byte>> HashFinal();
						intrusive_ptr<Array1D<Byte>> _EndHash();
						void DWORDToBigEndian(intrusive_ptr<Array1D<Byte>> block, intrusive_ptr<Array1D<UInt32>> x, int digits);
						void DWORDFromBigEndian(intrusive_ptr<Array1D<UInt32>> x, int digits, intrusive_ptr<Array1D<Byte>> block);
						void SHATransform(intrusive_ptr<Array1D<UInt32>> expandedBuffer, intrusive_ptr<Array1D<UInt32>> state, intrusive_ptr<Array1D<Byte>> block);
						void SHA256Expand(intrusive_ptr<Array1D<UInt32>> x);
				   	    UInt32 RotateRight(UInt32 x, int n);
						UInt32 Ch(UInt32 x, UInt32 y, UInt32 z);
						UInt32 Maj(UInt32 x, UInt32 y, UInt32 z);
						UInt32 sigma_0(UInt32 x);
						UInt32 sigma_1(UInt32 x);
						UInt32 Sigma_0(UInt32 x);
						UInt32 Sigma_1(UInt32 x);
					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif