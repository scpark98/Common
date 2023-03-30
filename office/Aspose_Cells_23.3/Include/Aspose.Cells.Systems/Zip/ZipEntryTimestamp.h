#ifndef _SYSTEM_ZIP_ZIPTIMESTAP_H_
#define _SYSTEM_ZIP_ZIPTIMESTAP_H_

#include <stdint.h>

namespace Aspose {
	namespace Cells {
		namespace Zip
		{
			enum ZipEntryTimestamp
			{
				/// <summary>
				/// Default value.
				/// </summary>
				ZipEntryTimestamp_None = 0,

				/// <summary>
				/// A DOS timestamp with 2-second precision.
				/// </summary>
				ZipEntryTimestamp_DOS = 1,

				/// <summary>
				/// A Windows timestamp with 100-ns precision.
				/// </summary>
				ZipEntryTimestamp_Windows = 2,

				/// <summary>
				/// A Unix timestamp with 1-second precision.
				/// </summary>
				ZipEntryTimestamp_Unix = 4,

				/// <summary>
				/// A Unix timestamp with 1-second precision, stored in InfoZip v1 format.  This
				/// format is outdated and is supported for reading archives only.
				/// </summary>
				ZipEntryTimestamp_InfoZip1 = 8,
			};
		}
	}
}
#endif//_SYSTEM_ZIP_ZIPOPTIONS_H_
