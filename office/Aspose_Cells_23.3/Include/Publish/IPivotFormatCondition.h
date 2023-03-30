#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "PivotConditionFormatScopeType.h"
#include "PivotConditionFormatRuleType.h"

namespace Aspose {
	namespace Cells {
		namespace Pivot {
			class IPivotField;
			enum PivotConditionFormatScopeType;
			enum PivotConditionFormatRuleType;
		}
	}
}
namespace Aspose {
	namespace Cells {
		class IFormatConditionCollection;
	}
}
namespace Aspose{
namespace Cells{
namespace Pivot{
			/// <summary>
			/// Represents a PivotTable Format Condition in PivotFormatCondition Collection.
			/// </summary>
	class ASPOSE_CELLS_API IPivotFormatCondition : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Adds PivotTable conditional format limit in the data fields.
			/// </summary>
			/// <param name="fieldName" >The name of PivotField.</param>
			 virtual void AddDataAreaCondition(intrusive_ptr<Aspose::Cells::Systems::String> fieldName)=0;
			/// <summary>
			/// Adds PivotTable conditional format limit in the data fields.
			/// </summary>
			/// <param name="dataField" >The PivotField in the data fields.</param>
			 virtual void 		AddDataAreaCondition(intrusive_ptr<Aspose::Cells::Pivot::IPivotField> dataField)=0;
			/// <summary>
			/// Adds PivotTable conditional format limit in the row fields.
			/// </summary>
			/// <param name="fieldName" >The name of PivotField.</param>
			 virtual void AddRowAreaCondition(intrusive_ptr<Aspose::Cells::Systems::String> fieldName)=0;
			/// <summary>
			/// Adds PivotTable conditional format limit in the row fields.
			/// </summary>
			/// <param name="rowField" >The PivotField in the row fields.</param>
			 virtual void 		AddRowAreaCondition(intrusive_ptr<Aspose::Cells::Pivot::IPivotField> rowField)=0;
			/// <summary>
			/// Adds PivotTable conditional format limit in the column fields.
			/// </summary>
			/// <param name="fieldName" >The name of PivotField.</param>
			 virtual void AddColumnAreaCondition(intrusive_ptr<Aspose::Cells::Systems::String> fieldName)=0;
			/// <summary>
			/// Adds PivotTable conditional format limit in the column fields.
			/// </summary>
			/// <param name="columnField" >The PivotField in the column fields.</param>
			 virtual void 		AddColumnAreaCondition(intrusive_ptr<Aspose::Cells::Pivot::IPivotField> columnField)=0;
			/// <summary>
			/// Sets conditional areas of PivotFormatCondition object.
			/// </summary>
			 virtual void SetConditionalAreas()=0;
			/// <summary>
			/// Get and set scope type for the pivot table condition format .
			/// </summary>
			 virtual Aspose::Cells::Pivot::PivotConditionFormatScopeType GetScopeType()=0;
			/// <summary>
			/// Set scope type for the pivot talbe condition format .
			/// </summary>
			 virtual void SetScopeType(Aspose::Cells::Pivot::PivotConditionFormatScopeType value)=0;
			/// <summary>
			/// Get rule type for the pivot talbe condition format .
			/// </summary>
			 virtual Aspose::Cells::Pivot::PivotConditionFormatRuleType GetRuleType()=0;
			/// <summary>
			/// Set rule type for the pivot talbe condition format .
			/// </summary>
			 virtual void SetRuleType(Aspose::Cells::Pivot::PivotConditionFormatRuleType value)=0;
			/// <summary>
			/// Get formatconditions for the pivot talbe condition format .
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::IFormatConditionCollection> 		GetIFormatConditions()=0;

	};
}
}
}
