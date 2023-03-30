#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "LoadFormat.h"
#include "Aspose.Cells.Systems/Char.h"
#include "Aspose.Cells.Systems/String.h"
#include "IAbstractTextLoadOptions.h"

namespace Aspose {
	namespace Cells {
		enum LoadFormat;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Represents the options for loading text file.
			/// </summary>
	class ASPOSE_CELLS_API ITxtLoadOptions : virtual public Aspose::Cells::IAbstractTextLoadOptions
	{
		public:
			/// <summary>
			/// Gets character separator of text file.
			/// </summary>
			 virtual Aspose::Cells::Systems::Char GetSeparator()=0;
			/// <summary>
			/// Sets character separator of text file.
			/// </summary>
			 virtual void SetSeparator(Aspose::Cells::Systems::Char value)=0;
			/// <summary>
			/// Gets the a string value as separator.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetSeparatorString()=0;
			/// <summary>
			/// Sets the a string value as separator.
			/// </summary>
			 virtual void SetSeparatorString(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// True means that the file contains several encoding.
			/// </summary>
			 virtual bool IsMultiEncoded()=0;
			/// <summary>
			/// True means that the file contains several encoding.
			/// </summary>
			 virtual void SetMultiEncoded(bool value)=0;
			/// <summary>
			/// Indicates whether the text is formula if it starts with "=". 
			/// </summary>
			 virtual bool HasFormula()=0;
			/// <summary>
			/// Indicates whether the text is formula if it starts with "=". 
			/// </summary>
			 virtual void SetHasFormula(bool value)=0;
			/// <summary>
			/// Whether there is text qualifier for cell value. Default is true.
			/// </summary>
			 virtual bool HasTextQualifier()=0;
			/// <summary>
			/// Whether there is text qualifier for cell value. Default is true.
			/// </summary>
			 virtual void SetHasTextQualifier(bool value)=0;
			/// <summary>
			/// Specifies the text qualifier for cell values. Default qualifier is '"'.
			/// </summary>

			 virtual Aspose::Cells::Systems::Char GetTextQualifier()=0;
			/// <summary>
			/// Specifies the text qualifier for cell values. Default qualifier is '"'.
			/// </summary>

			 virtual void SetTextQualifier(Aspose::Cells::Systems::Char value)=0;
			/// <summary>
			/// Whether consecutive delimiters should be treated as one.
			/// </summary>
			 virtual bool GetTreatConsecutiveDelimitersAsOne()=0;
			/// <summary>
			/// Whether consecutive delimiters should be treated as one.
			/// </summary>
			 virtual void SetTreatConsecutiveDelimitersAsOne(bool value)=0;
			/// <summary>
			/// Indicates whether the leading single quote sign should be taken as part of the value of one cell.
			/// Default is true. If it is false, the leading single quote will be removed from corresponding cell's value
			/// and <see cref="Style.QuotePrefix" />
			///  will be set as true for the cell. 
			/// </summary>
			 virtual bool GetTreatQuotePrefixAsValue()=0;
			/// <summary>
			/// Indicates whether the leading single quote sign should be taken as part of the value of one cell.
			/// Default is true. If it is false, the leading single quote will be removed from corresponding cell's value
			/// and <see cref="Style.QuotePrefix" />
			///  will be set as true for the cell. 
			/// </summary>
			 virtual void SetTreatQuotePrefixAsValue(bool value)=0;
			/// <summary>
			/// Whether extends data to next sheet when the rows or columns of data exceed limit.
			/// If this property is true, extra data will be extended to next sheet behind current one(if current sheet is the last one,
			/// new sheet will be appended to current workbook).
			/// If this property is false, the data exceeds limit will be ignored.
			/// Default is false;
			/// </summary>
			 virtual bool GetExtendToNextSheet()=0;
			/// <summary>
			/// Whether extends data to next sheet when the rows or columns of data exceed limit.
			/// If this property is true, extra data will be extended to next sheet behind current one(if current sheet is the last one,
			/// new sheet will be appended to current workbook).
			/// If this property is false, the data exceeds limit will be ignored.
			/// Default is false;
			/// </summary>
			 virtual void SetExtendToNextSheet(bool value)=0;

	};
}
}
