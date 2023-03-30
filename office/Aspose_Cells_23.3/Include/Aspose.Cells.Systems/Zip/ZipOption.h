#ifndef _SYSTEM_ZIP_ZIPOPTIONS_H_
#define _SYSTEM_ZIP_ZIPOPTIONS_H_

namespace Aspose {
	namespace Cells {
		namespace Zip
		{
			//////////////////// zipfile options //////////////////////////
			enum ZipOption
			{
				/// <summary>
				/// The default behavior. This is the same as "Never".
				/// (For COM clients, this is a 0 (zero).)
				/// </summary>
				ZipOption_Default = 0,
				/// <summary>
				/// Never use the associated option.
				/// (For COM clients, this is a 0 (zero).)
				/// </summary>
				ZipOption_Never = 0,
				/// <summary>
				/// Use the associated behavior "as necessary."
				/// (For COM clients, this is a 1.)
				/// </summary>
				ZipOption_AsNecessary = 1,
				/// <summary>
				/// Use the associated behavior Always, whether necessary or not.
				/// (For COM clients, this is a 2.)
				/// </summary>
				ZipOption_Always
			};
		}
	}
}
#endif//_SYSTEM_ZIP_ZIPOPTIONS_H_
