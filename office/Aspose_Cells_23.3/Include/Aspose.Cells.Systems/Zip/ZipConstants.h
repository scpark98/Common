#ifndef _SYSTEM_ZIP_ZIPCONS_H_
#define _SYSTEM_ZIP_ZIPCONS_H_

#include <stdint.h>

namespace Aspose {
	namespace Cells {
		namespace Zip
		{
			class ZipConstants
			{
			public:
				static const  uint32_t PackedToRemovableMedia = 0x30304b50;
				static const  uint32_t Zip64EndOfCentralDirectoryRecordSignature = 0x06064b50;
				static const  uint32_t Zip64EndOfCentralDirectoryLocatorSignature = 0x07064b50;
				static const  uint32_t EndOfCentralDirectorySignature = 0x06054b50;
				static const  int32_t ZipEntrySignature = 0x04034b50;
				static const  int32_t ZipEntryDataDescriptorSignature = 0x08074b50;
				static const  int32_t SplitArchiveSignature = 0x08074b50;
				static const  int32_t ZipDirEntrySignature = 0x02014b50;

				// These are dictated by the Zip Spec.See APPNOTE.txt
				static const  int32_t AesKeySize = 192;  // 128, 192, 256
				static const  int32_t AesBlockSize = 128;  // ???

				static const  uint16_t AesAlgId128 = 0x660E;
				static const  uint16_t AesAlgId192 = 0x660F;
				static const  uint16_t AesAlgId256 = 0x6610;
			};
		}
	}
}
#endif// 

