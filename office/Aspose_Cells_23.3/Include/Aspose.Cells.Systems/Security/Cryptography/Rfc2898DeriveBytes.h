#pragma once
#pragma once
#ifndef _SYSTEM_SECURITY_CRYPTOGRAPHY_RFC2898_H_
#define _SYSTEM_SECURITY_CRYPTOGRAPHY_RFC2898_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"
namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace Security {
				namespace Cryptography {

					class ASPOSE_CELLS_API Rfc2898DeriveBytes : public Object
					{
					public:
						Rfc2898DeriveBytes(intrusive_ptr<Aspose::Cells::Systems::Array1D< Aspose::Cells::Systems::Byte>> password, intrusive_ptr<Aspose::Cells::Systems::Array1D< Aspose::Cells::Systems::Byte>> salt, Aspose::Cells::Systems::Int32 iterationCount) 
						{}
						intrusive_ptr<Aspose::Cells::Systems::Array1D< Aspose::Cells::Systems::Byte>>  GetBytes(Aspose::Cells::Systems::Int32 dkLen)
						{
							throw NEW NotSupportedException("Rfc2898DeriveBytes::GetBytes is not supported!");
						}
					protected:
						Rfc2898DeriveBytes() {}
					};
				} // namespace Cryptography
			}	// namespace Security
		}	// namespace Systems
	}
}
#endif