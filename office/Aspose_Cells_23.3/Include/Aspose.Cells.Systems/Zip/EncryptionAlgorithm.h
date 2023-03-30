#ifndef _SYSTEM_ZIP_ZIPENCALG_H_
#define _SYSTEM_ZIP_ZIPENCALG_H_

#include <stdint.h>

namespace Aspose {
	namespace Cells {
		namespace Zip
		{
			enum EncryptionAlgorithm
			{
				/// <summary>
				/// No encryption at all.
				/// </summary>
				EncryptionAlgorithm_None = 0,

				/// <summary>
				/// Traditional or Classic pkzip encryption.
				/// </summary>
				EncryptionAlgorithm_PkzipWeak,

				/// <summary>
				/// An encryption algorithm that is not supported by DotNetZip.
				/// </summary>
				EncryptionAlgorithm_Unsupported = 4,
				// others... not implemented (yet?)
			};

		}
	}
}
#endif//_SYSTEM_ZIP_ZIPOPTIONS_H_
