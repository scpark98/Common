#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Represents the type of auto fitting merged cells.
			/// </summary>
	enum AutoFitMergedCellsType 

	{

			/// <summary>
			/// Ignore merged cells. 
			/// </summary>
			/// <remarks>
			/// Default.
			/// </remarks>
			AutoFitMergedCellsType_None ,
			/// <summary>
			/// Only expands the height of the first row.
			/// </summary>
			AutoFitMergedCellsType_FirstLine ,
			/// <summary>
			/// Only expands the height of the last row.
			/// </summary>
			AutoFitMergedCellsType_LastLine ,
			/// <summary>
			/// Only expands the height of each row.
			/// </summary>
			AutoFitMergedCellsType_EachLine ,
	};


}

}
