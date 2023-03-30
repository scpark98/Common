#ifndef _SYSTEM_ZIP_CPMMETH_H_
#define _SYSTEM_ZIP_CPMMETH_H_

//#include <stdint.h>

namespace Aspose {
	namespace Cells {
		namespace Zip{
			enum CompressionMethod
			{
				/// <summary>
				/// No compression at all. For COM environments, the value is 0 (zero).
				/// </summary>
				CompressionMethod_None = 0,

				/// <summary>
				///   DEFLATE compression, as described in <see
				///   href="http://www.ietf.org/rfc/rfc1951.txt">IETF RFC
				///   1951</see>.  This is the "normal" compression used in zip
				///   files. For COM environments, the value is 8.
				/// </summary>
				CompressionMethod_Deflate = 8,
			};
		}
	}
}
#endif//_SYSTEM_ZIP_ZIPOPTIONS_H_
