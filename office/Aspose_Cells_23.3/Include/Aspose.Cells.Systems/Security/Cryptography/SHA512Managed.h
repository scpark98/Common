#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_SHA512MANAGED_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_SHA512MANAGED_H_

#include "Aspose.Cells.Systems/Security/Cryptography/SHA512.h"
#include "Aspose.Cells.Systems/Array1D.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
using namespace Aspose::Cells::Systems;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API SHA512Managed : public SHA512  //to do
					{
					public:
						SHA512Managed();
						void Initialize();
						void InitializeState();
						UInt64 _count; // Number of bytes in the hashed message
						intrusive_ptr<Array1D<UInt64>> _stateSHA512;
						intrusive_ptr<Array1D<UInt64>> _W;
						intrusive_ptr<Array1D<Byte>> _buffer;
						static intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::UInt64>> _K;
						static intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::UInt64>> Static_Array_Init_K();
					protected:
						void HashCore(intrusive_ptr<Array1D<Byte>> rgb, int ibStart, int cbSize);
						void _HashData(intrusive_ptr<Array1D<Byte>>  partIn, int ibStart, int cbSize);
						intrusive_ptr<Array1D<Byte>> HashFinal();
						intrusive_ptr<Array1D<Byte>> _EndHash();
						void QuadWordToBigEndian(intrusive_ptr<Array1D<Byte>> block, intrusive_ptr<Array1D<UInt64>> x, int digits);
						void QuadWordFromBigEndian(intrusive_ptr<Array1D<UInt64>> x, int digits, intrusive_ptr<Array1D<Byte>> block);
						void SHATransform(intrusive_ptr<Array1D<UInt64>> expandedBuffer, intrusive_ptr<Array1D<UInt64>> state, intrusive_ptr<Array1D<Byte>> block);
						void SHA512Expand(intrusive_ptr<Array1D<UInt64>> x);
						UInt64 RotateRight(UInt64 x, int n);
						UInt64 Ch(UInt64 x, UInt64 y, UInt64 z);
						UInt64 Maj(UInt64 x, UInt64 y, UInt64 z);
						UInt64 sigma_0(UInt64 x);
						UInt64 sigma_1(UInt64 x);
						UInt64 Sigma_0(UInt64 x);
						UInt64 Sigma_1(UInt64 x);
					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif