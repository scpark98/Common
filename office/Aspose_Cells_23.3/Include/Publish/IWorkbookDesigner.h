#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Int32.h"
#include "Aspose.Cells.Systems/Array1D.h"

namespace Aspose {
	namespace Cells {
		class IWorkbook;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Encapsulates the object that represents a designer spreadsheet.
			/// </summary>
	class ASPOSE_CELLS_API IWorkbookDesigner : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Gets and sets the <see cref="IWorkbook" />
			///  object.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::IWorkbook> 		GetIWorkbook()=0;
			/// <summary>
			/// Gets and sets the <see cref="IWorkbook" />
			///  object.
			/// </summary>
			 virtual void 		SetIWorkbook(intrusive_ptr<Aspose::Cells::IWorkbook> value)=0;
			/// <summary>
			/// Indicates whether repeating formulas with subtotal row.
			/// </summary>
			 virtual bool GetRepeatFormulasWithSubtotal()=0;
			/// <summary>
			/// Indicates whether repeating formulas with subtotal row.
			/// </summary>
			 virtual void SetRepeatFormulasWithSubtotal(bool value)=0;
			/// <summary>
			/// Clears all data sources.
			/// </summary>
			 virtual void ClearDataSource()=0;
			/// <summary>
			/// Sets data binding to a variable.
			/// </summary>
			/// <param name="variable" >Variable name created using smart marker.</param>
			/// <param name="data" >Source data.</param>
			 virtual void SetDataSource(intrusive_ptr<Aspose::Cells::Systems::String> variable , intrusive_ptr<Aspose::Cells::Systems::Object> data)=0;
			/// <summary>
			/// If TRUE, Null will be inserted if the value is "";
			/// </summary>
			 virtual bool GetUpdateEmptyStringAsNull()=0;
			/// <summary>
			/// If TRUE, Null will be inserted if the value is "";
			/// </summary>
			 virtual void SetUpdateEmptyStringAsNull(bool value)=0;
			/// <summary>
			/// Indicates if references in other worksheets will be updated.
			/// </summary>
			 virtual bool GetUpdateReference()=0;
			/// <summary>
			/// Indicates if references in other worksheets will be updated.
			/// </summary>
			 virtual void SetUpdateReference(bool value)=0;
			/// <summary>
			/// Indicates whether formulas should be calculated.
			/// </summary>
			 virtual bool GetCalculateFormula()=0;
			/// <summary>
			/// Indicates whether formulas should be calculated.
			/// </summary>
			 virtual void SetCalculateFormula(bool value)=0;
			/// <summary>
			/// Indicates wether processing the smart marker line by line.
			///  </summary>
			/// <remarks>
			///  The default value is false.
			///  If False, the template file must contain a range which is named as "_CellsSmartMarkers".
			///  </remarks>
			 virtual bool GetLineByLine()=0;
			/// <summary>
			/// Indicates wether processing the smart marker line by line.
			///  </summary>
			/// <remarks>
			///  The default value is false.
			///  If False, the template file must contain a range which is named as "_CellsSmartMarkers".
			///  </remarks>
			 virtual void SetLineByLine(bool value)=0;
			/// <summary>
			/// Processes the smart markers and populates the data source values.
			/// </summary>
			 virtual void Process()=0;
			/// <summary>
			/// Processes the smart markers and populates the data source values.
			/// </summary>
			/// <param name="isPreserved" >True if the unrecognized smart marker is preserved.</param>
			 virtual void Process(bool isPreserved)=0;
			/// <summary>
			/// Processes the smart markers and populates the data source values.
			/// </summary>
			/// <param name="sheetIndex" >Worksheet index.</param>
			/// <param name="isPreserved" >True if the unrecognized smart marker is preserved.</param>
			/// <remarks>This method works on worksheet level.</remarks>
			 virtual void Process(Aspose::Cells::Systems::Int32 sheetIndex , bool isPreserved)=0;
			/// <summary>
			/// Returns a collection of smart markers in a spreadsheet.
			/// </summary>
			/// <returns>A collection of smart markers</returns>
			/// <remarks>A string array is created on every call. The array is sorted and duplicated values are removed.</remarks>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::String*>> GetSmartMarkers()=0;

	};
}
}
