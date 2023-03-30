#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "TotalsCalculation.h"

namespace Aspose {
	namespace Cells {
		namespace Tables {
			enum TotalsCalculation;
		}
	}
}
namespace Aspose {
	namespace Cells {
		class IRange;
	}
}
namespace Aspose{
namespace Cells{
namespace Tables{
			/// <summary>
			/// Represents a column in a list.
			/// </summary>
	class ASPOSE_CELLS_API IListColumn : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Gets the name of the column.
			/// </summary>
			/// <remarks>
			/// If sets the name of the column, the according cell' value will be changed too.
			/// </remarks>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetName()=0;
			/// <summary>
			/// Sets the name of the column.
			/// </summary>
			/// <remarks>
			/// If sets the name of the column, the according cell' value will be changed too.
			/// </remarks>
			 virtual void SetName(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Gets the type of calculation in the Totals row of the list column.
			/// </summary>
			 virtual Aspose::Cells::Tables::TotalsCalculation GetTotalsCalculation()=0;
			/// <summary>
			/// Sets the type of calculation in the Totals row of the list column.
			/// </summary>
			 virtual void SetTotalsCalculation(Aspose::Cells::Tables::TotalsCalculation value)=0;
			/// <summary>
			/// Gets the range of this list column.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::IRange> 		GetIRange()=0;
			/// <summary>
			/// Gets the formula of totals row of this list column.
			/// </summary>
			/// <param name="isR1C1" >Whether the formula needs to be formatted as R1C1.</param>
			/// <param name="isLocal" >Whether the formula needs to be formatted by locale.</param>
			/// <returns>The formula of this list column.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetCustomTotalsRowFormula(bool isR1C1 , bool isLocal)=0;
			/// <summary>
			/// Gets the formula of totals row of this list column.
			/// </summary>
			/// <param name="formula" >the formula for this list column.</param>
			/// <param name="isR1C1" >Whether the formula needs to be formatted as R1C1.</param>
			/// <param name="isLocal" >Whether the formula needs to be formatted by locale.</param>
			 virtual void SetCustomTotalsRowFormula(intrusive_ptr<Aspose::Cells::Systems::String> formula , bool isR1C1 , bool isLocal)=0;
			/// <summary>
			/// Gets the formula of the list column.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetFormula()=0;
			/// <summary>
			/// Sets the formula of the list column.
			/// </summary>
			 virtual void SetFormula(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Gets the formula of this list column.
			/// </summary>
			/// <param name="isR1C1" >Whether the formula needs to be formatted as R1C1.</param>
			/// <param name="isLocal" >Whether the formula needs to be formatted by locale.</param>
			/// <returns>The formula of this list column.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetCustomCalculatedFormula(bool isR1C1 , bool isLocal)=0;
			/// <summary>
			/// Sets the formula for this list column.
			/// </summary>
			/// <param name="formula" >the formula for this list column.</param>
			/// <param name="isR1C1" >Whether the formula needs to be formatted as R1C1.</param>
			/// <param name="isLocal" >Whether the formula needs to be formatted by locale.</param>
			 virtual void SetCustomCalculatedFormula(intrusive_ptr<Aspose::Cells::Systems::String> formula , bool isR1C1 , bool isLocal)=0;

	};
}
}
}
