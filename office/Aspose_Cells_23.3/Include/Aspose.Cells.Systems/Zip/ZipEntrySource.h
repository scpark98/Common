#ifndef _SYSTEM_ZIP_ZIPSRC_H_
#define _SYSTEM_ZIP_ZIPSRC_H_

#include <stdint.h>

namespace Aspose {
	namespace Cells {
		namespace Zip
		{
			enum ZipEntrySource
			{
				/// <summary>
				/// Default value.  Invalid on a bonafide ZipEntry.
				/// </summary>
				ZipEntrySource_None = 0,

				/// <summary>
				/// The entry was instantiated by calling AddFile() or another method that 
				/// added an entry from the filesystem.
				/// </summary>
				ZipEntrySource_FileSystem,

				/// <summary>
				/// The entry was instantiated via <see cref="Aspose.Cells.Zip.ZipFile.AddEntry(string,string)"/> or
				/// <see cref="Aspose.Cells.Zip.ZipFile.AddEntry(string,System.IO.Stream)"/> .
				/// </summary>
				ZipEntrySource_Stream,

				/// <summary>
				/// The ZipEntry was instantiated by reading a zipfile.
				/// </summary>
				ZipEntrySource_ZipFile,

				/// <summary>
				/// The content for the ZipEntry will be or was provided by the WriteDelegate.
				/// </summary>
				ZipEntrySource_WriteDelegate,

				/// <summary>
				/// The content for the ZipEntry will be obtained from the stream dispensed by the <c>OpenDelegate</c>.
				/// The entry was instantiated via <see cref="Aspose.Cells.Zip.ZipFile.AddEntry(string,OpenDelegate,CloseDelegate)"/>.
				/// </summary>
				ZipEntrySource_JitStream,

				/// <summary>
				/// The content for the ZipEntry will be or was obtained from a <c>ZipOutputStream</c>.
				/// </summary>
				ZipEntrySource_ZipOutputStream,
			};

 
 

		}
	}
}
#endif//_SYSTEM_ZIP_ZIPOPTIONS_H_
