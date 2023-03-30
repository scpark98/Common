#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Text/Encoding.h"
#include "TxtLoadStyleStrategy.h"
#include "ILoadOptions.h"

namespace Aspose {
	namespace Cells {
		enum TxtLoadStyleStrategy;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Common options for loading text values
			/// </summary>
	class ASPOSE_CELLS_API IAbstractTextLoadOptions : virtual public Aspose::Cells::ILoadOptions
	{
		public:
			/// <summary>
			/// Gets and sets the default encoding. Only applies for csv file.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Text::Encoding> GetEncoding()=0;
			/// <summary>
			/// Gets and sets the default encoding. Only applies for csv file.
			/// </summary>
			 virtual void SetEncoding(intrusive_ptr<Aspose::Cells::Systems::Text::Encoding> value)=0;
			/// <summary>
			/// Indicates the strategy to apply style for parsed values when converting string value to number or datetime.
			/// </summary>
			 virtual Aspose::Cells::TxtLoadStyleStrategy GetLoadStyleStrategy()=0;
			/// <summary>
			/// Indicates the strategy to apply style for parsed values when converting string value to number or datetime.
			/// </summary>
			 virtual void SetLoadStyleStrategy(Aspose::Cells::TxtLoadStyleStrategy value)=0;
			/// <summary>
			/// Gets or sets a value that indicates whether the string in text file is converted to numeric data.
			/// </summary>
			 virtual bool GetConvertNumericData()=0;
			/// <summary>
			/// Gets or sets a value that indicates whether the string in text file is converted to numeric data.
			/// </summary>
			 virtual void SetConvertNumericData(bool value)=0;
			/// <summary>
			/// Gets or sets a value that indicates whether the string in text file is converted to date data.
			/// </summary>
			 virtual bool GetConvertDateTimeData()=0;
			/// <summary>
			/// Gets or sets a value that indicates whether the string in text file is converted to date data.
			/// </summary>
			 virtual void SetConvertDateTimeData(bool value)=0;
			/// <summary>
			/// Indicates whether not parsing a string value if the length is 15.
			/// </summary>
			 virtual bool GetKeepPrecision()=0;
			/// <summary>
			/// Indicates whether not parsing a string value if the length is 15.
			/// </summary>
			 virtual void SetKeepPrecision(bool value)=0;

	};
}
}
