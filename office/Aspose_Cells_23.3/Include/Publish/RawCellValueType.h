#pragma once
namespace Aspose {
namespace Cells {
namespace CellsSs {
			/// <summary>
			/// Enumerates raw type of cell value in data model.
			/// </summary>
	enum RawCellValueType 

	{

			/// <summary>
			/// Blank cell
			/// </summary>
			RawCellValueType_Null = 0 ,
			/// <summary>
			/// Double
			/// </summary>
			RawCellValueType_Double = 1 ,
			/// <summary>
			/// Boolean, corresponding value in model depends on data model implementation.
			/// </summary>
			RawCellValueType_Bool = 2 ,
			/// <summary>
			/// Error, corresponding value is byte default in ErrorType
			/// </summary>
			RawCellValueType_Error = 3 ,
			/// <summary>
			/// String, corresponding value is RefString object or its index depending on data model implementation.
			/// </summary>
			RawCellValueType_String = 4 ,
			/// <summary>
			/// Formula, corresponding value is FormulaCell object or its index depending on data model implementation.
			/// </summary>
			RawCellValueType_Formula = 5 ,
			/// <summary>
			/// RK value for Double.
			/// </summary>
			RawCellValueType_RK = 6 ,
	};


}

}

}
