#ifndef _SYSTEM_DATETIMEROFFSET_H_
#define _SYSTEM_DATETIMEROFFSET_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/ParseFlags.h"
#include "Aspose.Cells.Systems/TimeSpan.h"
#include "Aspose.Cells.Systems/DateTime.h"
#include "Aspose.Cells.Systems/ParseFailureKind.h"
#include "Aspose.Cells.Systems/NotSupportedException.h"

using namespace Aspose::Cells::Systems::Globalization;

namespace Aspose {
	namespace Cells {
		namespace Systems {
			class DateTimeOffset : public Aspose::Cells::Systems::Object{
			public:
				DateTimeOffset() {}
				DateTimeOffset(int32_t offset, TimeSpan& ts) {}
				DateTimeOffset(DateTime& dt) {}
				DateTimeOffset(DateTime& dt, TimeSpan& ts) {}
				intrusive_ptr<DateTime> GetDateTime()
				{
					throw NEW NotSupportedException("DateTimeOffset::GetDateTime is not supported!");
				}
				void setOffset(int32_t offset) {}
				void setTimeSpan(TimeSpan& ts) {}
				int64_t getOffsetInTicks() { return 0; }
			};
		}
	}
}
#endif